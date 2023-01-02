#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QMovie>
#include <QPixmap>
#include <QProcess>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QWhatsThis>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

void MainWindow::parseCodecs(QList<Codec> *codecs, const QString &type, QComboBox *comboBox)
{
	settings.beginGroup(type);

	if (settings.childKeys().empty()) {
		ShowMessageBox(Severity::Critical,
				   tr("No %1").arg(type),
				   tr("Could not find %1 in the INI. Please check the config. Aborting.")
					   .arg(type));
		QApplication::exit(1);
	}

	for (const QString &codecLibrary : settings.childKeys()) {
		if (QRegularExpression("[^-_a-z0-9]").match(codecLibrary).hasMatch()) {
			ShowMessageBox(
				Severity::Critical,
				tr("Could not parse codec"),
				tr("Codec library %1 could not be parsed. Please validate the INI config.")
					.arg(codecLibrary));
			QApplication::exit(1);
		}

		QStringList values
			= setting(codecLibrary).toString().split(QRegularExpression("(\\s*),(\\s*)"));

		QString codecName = values.first();

		bool isConversionOk = false;
		double codecMinBitrateKbps = values.size() == 2
							     ? values.last().toDouble(&isConversionOk)
							     : 0;

		if (values.size() == 2 && !isConversionOk) {
			ShowMessageBox(Severity::Critical,
					   tr("Invalid codec bitrate"),
					   tr("Minimum codec bitrate %1 could not be parsed. Please validate "
						"the INI config.")
						   .arg(values.last()));
			QApplication::exit(1);
		}

		qDebug() << values.last();

		Codec codec{codecName, codecLibrary, codecMinBitrateKbps};
		codecs->append(codec);
		comboBox->addItem(codecName);
	}
	settings.endGroup();
}

void MainWindow::parseContainers(QList<Container> *containers, QComboBox *comboBox)
{
	QString type = "Containers";

	settings.beginGroup(type);

	if (settings.childKeys().empty()) {
		ShowMessageBox(Severity::Critical,
				   tr("No %1").arg(type),
				   tr("Could not find %1 in the INI. Please check the config. Aborting.")
					   .arg(type));
		QApplication::exit(1);
	}

	for (const QString &containerName : settings.childKeys()) {
		if (QRegularExpression("[^-_a-z0-9]").match(containerName).hasMatch()) {
			ShowMessageBox(
				Severity::Critical,
				tr("Could not parse container name"),
				tr("Container name %1 could not be parsed. Please validate the INI config.")
					.arg(containerName));
			QApplication::exit(1);
		}

		QStringList supportedCodecs
			= setting(containerName).toString().split(QRegularExpression("(\\s*),(\\s*)"));
		Container container{containerName, supportedCodecs};
		containers->append(container);
		comboBox->addItem(containerName.toUpper());
	}
	settings.endGroup();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	progressBarAnimation = new QPropertyAnimation(ui->progressBar, "value");
	progressBarAnimation->setDuration(100);
	ui->progressWidget->setVisible(false);

	// menu
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Help"), &QWhatsThis::enterWhatsThisMode);
	menu->addSeparator();
	menu->addAction(tr("About"));
	menu->addAction(tr("About Qt"), &QApplication::aboutQt);
	ui->toolButton->setMenu(menu);
	connect(ui->toolButton, &QToolButton::pressed, ui->toolButton, &QToolButton::showMenu);

	// parse codecs
	QList<Codec> videoCodecs;
	QList<Codec> audioCodecs;
	QList<Container> containers;

	parseCodecs(&videoCodecs, "VideoCodecs", ui->videoCodecComboBox);
	parseCodecs(&audioCodecs, "AudioCodecs", ui->audioCodecCombobox);
	parseContainers(&containers, ui->containerCombobox);

	// start compression button
	connect(ui->startButton, &QPushButton::clicked, [=, this]() {
		if (!selectedUrl.isValid()) {
			QMessageBox::information(this,
							 tr("No file selected"),
							 tr("Please select a file to compress."));
			return;
		}

		double sizeKbpsConversionFactor = -1;
		switch (ui->sizeUnitComboBox->currentIndex()) {
		case 0: //
			sizeKbpsConversionFactor = 8;
			break;
		case 1: // MB to kb
			sizeKbpsConversionFactor = 8000;
			break;
		case 2: //
			sizeKbpsConversionFactor = 8e+6;
			break;
		}

		showProgress();
		compressor->compress(selectedUrl,
					   ui->outputDirLineEdit->text().isEmpty()
						   ? QDir::currentPath()
						   : ui->outputDirLineEdit->text(),
					   ui->fileSuffix->text(),
					   videoCodecs.at(ui->videoCodecComboBox->currentIndex()),
					   audioCodecs.at(ui->audioCodecCombobox->currentIndex()),
					   containers.at(ui->containerCombobox->currentIndex()),
					   ui->sizeSpinBox->value() * sizeKbpsConversionFactor,
					   ui->qualityRatioSlider->value() / 100.0);
	});

	// handle displaying target bitrates during compression
	connect(compressor,
		  &Compressor::compressionStarted,
		  [this](double videoBitrateKbps, double audioBitrateKbps) {
			  ui->progressLabel->setText(
				  QString(tr("Video bitrate: %1 kbps | Audio bitrate: %2 kbps"))
					  .arg(QString::number(qRound(videoBitrateKbps)),
						 QString::number(qRound(audioBitrateKbps))));
		  });

	// handle successful compression
	connect(compressor,
		  &Compressor::compressionSucceeded,
		  [this](double requestedSizeKbps, double actualSizeKbps) {
			  setProgress(100);
			  ShowMessageBox(Severity::Information,
					     tr("Compressed successfully"),
					     QString(tr("Requested size was %1 kb.\nActual "
							    "compression achieved is %2 kb."))
						     .arg(QString::number(requestedSizeKbps),
							    QString::number(actualSizeKbps)));

			  setProgress(0);
			  hideProgress();
		  });

	// handle failed compression
	connect(compressor,
		  &Compressor::compressionFailed,
		  [this](QString shortError, QString longError) {
			  setProgress(0);
			  ShowMessageBox(Severity::Critical,
					     tr("Compression failed"),
					     shortError,
					     longError);
			  hideProgress();
		  });

	// handle compression progress updates
	connect(compressor, &Compressor::compressionProgressUpdate, this, &MainWindow::setProgress);

	// show name of file picked with file dialog
	connect(ui->fileButton, &QPushButton::clicked, [this]() {
		QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
									 tr("Select file to compress"),
									 QDir::currentPath(),
									 "*");

		ui->fileName->setText(fileUrl.toLocalFile());
		selectedUrl = fileUrl;
	});

	// show name of file picked with file dialog
	connect(ui->outputDirButton, &QPushButton::clicked, [this]() {
		QDir dir = QFileDialog::getExistingDirectory(this,
									   tr("Select output directory"),
									   QDir::currentPath());

		ui->outputDirLineEdit->setText(dir.absolutePath());
		selectedDir = dir;
	});

	// show value in kbps of audio quality slider
	connect(ui->qualityRatioSlider, &QSlider::valueChanged, [this]() {
		double currentValue = qMax(16.0,
						   ui->qualityRatioSlider->value() / 100.0
							   * setting("Main/dMaxBitrateAudioKbps").toDouble());
		ui->qualityRatioSliderLabel->setText(QString::number(qRound(currentValue)) + " kbps");
	});

	// restore UI state on run
	selectedUrl = QUrl(setting("Main/sLastFile").toString());
	selectedDir = QDir(setting("Main/sLastDir").toString());

	if (QFile::exists(selectedUrl.toLocalFile()))
		ui->fileName->setText(selectedUrl.toLocalFile());
	if (selectedDir.exists())
		ui->outputDirLineEdit->setText(selectedDir.absolutePath());

	ui->sizeSpinBox->setValue(setting("Main/dLastDesiredFileSize").toDouble());
	ui->fileSuffix->setText(setting("Main/sLastDesiredFileSuffix").toString());
	ui->sizeUnitComboBox->setCurrentText(setting("Main/sLastDesiredFileSizeUnit").toString());
	ui->qualityRatioSlider->setValue(setting("Main/iLastDesiredQualityRatio").toInt());
	ui->videoCodecComboBox->setCurrentText(setting("Main/sLastDesiredVideoCodec").toString());
	ui->audioCodecCombobox->setCurrentText(setting("Main/sLastDesiredAudioCodec").toString());
	ui->containerCombobox->setCurrentText(setting("Main/sLastDesiredContainer").toString());
	emit ui->qualityRatioSlider->valueChanged(ui->qualityRatioSlider->value());
}

MainWindow::~MainWindow()
{
	delete ui;
}

QVariant MainWindow::setting(const QString &key)
{
	if (!settings.contains(key)) {
		QMessageBox::critical(
			this,
			tr("Missing configuration"),
			QString(tr("The configuration file is missing a key. Please reinstall the "
				     "program.\n\nMissing key: %1\nWorking dir: %2"))
				.arg(key, QDir::currentPath()));
		return 0 / 0; // lol
	}

	return settings.value(key);
}

void MainWindow::setSetting(const QString &key, const QVariant &value)
{
	if (!settings.contains(key)) {
		QMessageBox::warning(this,
					   "Suspicious setting",
					   "Attempted to create setting that doesn't already "
					   "exist: "
						   + key + ". Is this a typo?");
	}

	settings.setValue(key, value);
}

void MainWindow::setProgress(int progressPercent)
{
	progressBarAnimation->setStartValue(ui->progressBar->value());
	progressBarAnimation->setEndValue(progressPercent);
	progressBarAnimation->start();
}

void MainWindow::showProgress()
{
	ui->owo->setEnabled(false);
	ui->progressWidget->setVisible(true);
	ui->startButton->setText("Compressing...");
}

void MainWindow::hideProgress()
{
	ui->owo->setEnabled(true);
	ui->progressWidget->setVisible(false);
	ui->startButton->setText("Start compression");
}

void MainWindow::ShowMessageBox(QMessageBox::Icon severity,
					  const QString &title,
					  const QString &message,
					  const QString &details)
{
	QFont font;
	font.setBold(true);

	QMessageBox dialog;
	dialog.setWindowTitle(ui->owo->windowTitle());
	dialog.setIcon(severity);
	dialog.setText("<font size=5><b>" + title + ".</b></font>");
	dialog.setInformativeText(message);
	dialog.setDetailedText(details);
	dialog.setStandardButtons(QMessageBox::Ok);
	dialog.exec();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// save current parameters for next program run
	setSetting("Main/sLastFile", selectedUrl);
	setSetting("Main/sLastDir", selectedDir.absolutePath());
	setSetting("Main/dLastDesiredFileSize", ui->sizeSpinBox->value());
	setSetting("Main/sLastDesiredFileSizeUnit", ui->sizeUnitComboBox->currentText());
	setSetting("Main/sLastDesiredFileSuffix", ui->fileSuffix->text());
	setSetting("Main/iLastDesiredQualityRatio", ui->qualityRatioSlider->value());

	setSetting("Main/sLastDesiredVideoCodec", ui->videoCodecComboBox->currentText());
	setSetting("Main/sLastDesiredAudioCodec", ui->audioCodecCombobox->currentText());
	setSetting("Main/sLastDesiredContainer", ui->containerCombobox->currentText());

	event->accept();
}
