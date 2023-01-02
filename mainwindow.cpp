#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMovie>
#include <QPixmap>
#include <QProcess>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QWhatsThis>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	progressBarAnimation = new QPropertyAnimation(ui->progressBar, "value");
	progressBarAnimation->setDuration(setting("Main/iProgressBarAnimDurationMs").toInt());
	progressBarAnimation->setEasingCurve(QEasingCurve::InQuad);
	ui->progressWidget->setVisible(false);

	// menu
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Help"), &QWhatsThis::enterWhatsThisMode);
	menu->addSeparator();
	menu->addAction(tr("About"));
	menu->addAction(tr("About Qt"), &QApplication::aboutQt);
	ui->toolButton->setMenu(menu);
	connect(ui->toolButton, &QToolButton::pressed, ui->toolButton, &QToolButton::showMenu);

	// detect nvidia
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);

	if (QSysInfo::kernelType() == "winnt") {
		process.startCommand("wmic path win32_VideoController get name");
		process.waitForFinished();

		if (process.readAllStandardOutput().toLower().contains("nvidia"))
			isNvidia = true;
	}

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
			Notify(Severity::Info,
				 tr("No file selected"),
				 tr("Please select a file to compress."));
			return;
		}

		double sizeKbpsConversionFactor = -1;
		switch (ui->sizeUnitComboBox->currentIndex()) {
		case 0: // KB to kb
			sizeKbpsConversionFactor = 8;
			break;
		case 1: // MB to kb
			sizeKbpsConversionFactor = 8000;
			break;
		case 2: // GB to kb
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
					   ui->qualityRatioSlider->value() / 100.0,
					   ui->widthSpinBox->value(),
					   ui->heightSpinBox->value(),
					   setting("Main/dMinBitrateVideoKbps").toDouble(),
					   setting("Main/dMinBitrateAudioKbps").toDouble(),
					   setting("Main/dMaxBitrateAudioKbps").toDouble());
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
		  [this](double requestedSizeKbps, double actualSizeKbps, QFile *outputFile) {
			  setProgress(100);
			  Notify(Severity::Info,
				   tr("Compressed successfully"),
				   QString(tr("Requested size was %1 kb.\nActual "
						  "compression achieved is %2 kb."))
					   .arg(QString::number(requestedSizeKbps),
						  QString::number(actualSizeKbps)));

			  setProgress(0);
			  hideProgress();

			  QFileInfo fileInfo(*outputFile);
			  if (ui->openInExplorerCheckbox->isChecked()) {
				  QProcess::execute(
					  QString(R"(explorer.exe "%1")").arg(fileInfo.dir().path()));
			  }

			  if (ui->playCheckbox->isChecked()) {
				  QProcess::execute(
					  QString(R"(explorer.exe "%1")").arg(fileInfo.absoluteFilePath()));
			  }

			  if (ui->closeToolCheckbox->isChecked()) {
				  QApplication::exit(0);
			  }

			  // Looks like windows clipboard doesn't support either videos or large clipboard items
			  //			  if (ui->copyToClipboardCheckbox->isChecked()) {
			  //				  QFileInfo fileInfo(*outputFile);
			  //				  QMimeType type = QMimeDatabase().mimeTypeForFile(fileInfo);
			  //				  QMimeData *mimeData = new QMimeData();
			  //				  QByteArray data = outputFile->readAll();
			  //				  QClipboard *clippy = QGuiApplication::clipboard();

			  //				  mimeData->setData("text/plain", data);
			  //				  clippy->setMimeData(mimeData);
			  //				  outputFile->close();
			  //			  }
		  });

	// handle failed compression
	connect(compressor,
		  &Compressor::compressionFailed,
		  [this](const QString &shortError, const QString &longError) {
			  setProgress(0);
			  Notify(Severity::Warning, tr("Compression failed"), shortError, longError);
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
		double currentValue = qMax(setting("Main/dMinBitrateAudioKbps").toDouble(),
						   ui->qualityRatioSlider->value() / 100.0
							   * setting("Main/dMaxBitrateAudioKbps").toDouble());
		ui->qualityRatioSliderLabel->setText(QString::number(qRound(currentValue)) + " kbps");
	});

	// restore UI state on run
	selectedUrl = QUrl(setting("LastDesired/sInputFile").toString());
	selectedDir = QDir(setting("LastDesired/sOutputDir").toString());

	if (QFile::exists(selectedUrl.toLocalFile()))
		ui->fileName->setText(selectedUrl.toLocalFile());
	if (selectedDir.exists())
		ui->outputDirLineEdit->setText(selectedDir.absolutePath());

	ui->sizeSpinBox->setValue(setting("LastDesired/dFileSize").toDouble());
	ui->fileSuffix->setText(setting("LastDesired/sFileSuffix").toString());
	ui->sizeUnitComboBox->setCurrentText(setting("LastDesired/sFileSizeUnit").toString());
	ui->qualityRatioSlider->setValue(setting("LastDesired/iQualityRatio").toInt());
	ui->videoCodecComboBox->setCurrentText(setting("LastDesired/sVideoCodec").toString());
	ui->audioCodecCombobox->setCurrentText(setting("LastDesired/sAudioCodec").toString());
	ui->containerCombobox->setCurrentText(setting("LastDesired/sContainer").toString());
	ui->openInExplorerCheckbox->setChecked(
		setting("LastDesired/bOpenInExplorerOnSuccess").toBool());
	ui->playCheckbox->setChecked(setting("LastDesired/bPlayResultOnSuccess").toBool());
	ui->closeToolCheckbox->setChecked(setting("LastDesired/bCloseOnSuccess").toBool());
	emit ui->qualityRatioSlider->valueChanged(ui->qualityRatioSlider->value());
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::parseCodecs(QList<Codec> *codecs, const QString &type, QComboBox *comboBox)
{
	settings.beginGroup(type);

	if (settings.childKeys().empty()) {
		Notify(Severity::Critical,
			 tr("Missing codecs definition").arg(type),
			 tr("Could not find codecs list of type '%1' in the configuration file. "
			    "Please validate the configuration in %2. Exiting.")
				 .arg(type));
	}

	QString availableCodecs = compressor->availableFormats();

	for (const QString &codecLibrary : settings.childKeys()) {
		if (!availableCodecs.contains(codecLibrary)) {
			Notify(Severity::Warning,
				 tr("Unsupported codec"),
				 tr("Codec library '%1' does not appear to be supported by our version of "
				    "FFMPEG. It has been skipped. Please validate the "
				    "configuration in %2.")
					 .arg(codecLibrary, CONFIG_FILE));
			continue;
		}

		if (codecLibrary.contains("nvenc") && !isNvidia)
			continue;

		if (QRegularExpression("[^-_a-z0-9]").match(codecLibrary).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Could not parse codec"),
				 tr("Codec library '%1' could not be parsed. Please validate the "
				    "configuration in %2. Exiting.")
					 .arg(codecLibrary, CONFIG_FILE));
		}

		QStringList values
			= setting(codecLibrary).toString().split(QRegularExpression("(\\s*),(\\s*)"));

		QString codecName = values.first();

		bool isConversionOk = false;
		double codecMinBitrateKbps = values.size() == 2
							     ? values.last().toDouble(&isConversionOk)
							     : 0;

		if (values.size() == 2 && !isConversionOk) {
			Notify(Severity::Critical,
				 tr("Invalid codec bitrate"),
				 tr("Minimum bitrate of codec '%1' could not be parsed. Please "
				    "validate the configuration in %2. Exiting.")
					 .arg(values.last(), CONFIG_FILE));
		}

		Codec codec{codecName, codecLibrary, codecMinBitrateKbps};
		codecs->append(codec);
		comboBox->addItem(codecName);
	}
	settings.endGroup();
}

void MainWindow::parseContainers(QList<Container> *containers, QComboBox *comboBox)
{
	settings.beginGroup("Containers");

	if (settings.childKeys().empty()) {
		Notify(Severity::Critical,
			 tr("Missing containers definition"),
			 tr("Could not find list of container in the configuration file. Please "
			    "validate the configuration in %1. Exiting.")
				 .arg(CONFIG_FILE));
	}

	for (const QString &containerName : settings.childKeys()) {
		if (QRegularExpression("[^-_a-z0-9]").match(containerName).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Invalid container name"),
				 tr("Name of container '%1' could not be parsed. Please "
				    "validate the configuration in %2. Exiting.")
					 .arg(containerName, CONFIG_FILE));
		}

		QStringList supportedCodecs
			= setting(containerName).toString().split(QRegularExpression("(\\s*),(\\s*)"));
		Container container{containerName, supportedCodecs};
		containers->append(container);
		comboBox->addItem(containerName.toUpper());
	}
	settings.endGroup();
}

QVariant MainWindow::setting(const QString &key)
{
	if (!settings.contains(key)) {
		Notify(Severity::Critical,
			 tr("Missing configuration"),
			 tr("Configuration file is missing key '%1'. Please validate the "
			    "configuration in %2. Exiting.")
				 .arg(key, CONFIG_FILE),
			 tr("Missing key: %1\nWorking directory: %2").arg(key, QDir::currentPath()));
	}

	return settings.value(key);
}

void MainWindow::setSetting(const QString &key, const QVariant &value)
{
	if (!settings.contains(key)) {
		Notify(Severity::Warning,
			 tr("New setting created"),
			 tr("Request to set configuration key '%1' which does not already "
			    "exist. Please report this warning to the developers.")
				 .arg(key));
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
	ui->startButton->setText(tr("Compressing..."));
}

void MainWindow::hideProgress()
{
	ui->owo->setEnabled(true);
	ui->progressWidget->setVisible(false);
	ui->startButton->setText(tr("Start compression"));
}

void MainWindow::Notify(Severity severity,
				const QString &title,
				const QString &message,
				const QString &details)
{
	QFont font;
	font.setBold(true);

	QMessageBox dialog;
	dialog.setWindowTitle(ui->owo->windowTitle());
	dialog.setIcon(severity == Severity::Critical ? QMessageBox::Critical
								    : (QMessageBox::Icon) severity);
	dialog.setText("<font size=5><b>" + title + ".</b></font>");
	dialog.setInformativeText(message);
	dialog.setDetailedText(details);
	dialog.setStandardButtons(QMessageBox::Ok);
	dialog.exec();

	// wait until event loop has begun before attempting to exit
	if (severity == Severity::Critical)
		QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}

void MainWindow::closeEvent(QCloseEvent *event)
{	
	// save current parameters for next program run
	setSetting("LastDesired/sInputFile", selectedUrl);
	setSetting("LastDesired/sOutputDir", selectedDir.absolutePath());
	setSetting("LastDesired/dFileSize", ui->sizeSpinBox->value());
	setSetting("LastDesired/sFileSizeUnit", ui->sizeUnitComboBox->currentText());
	setSetting("LastDesired/sFileSuffix", ui->fileSuffix->text());
	setSetting("LastDesired/iQualityRatio", ui->qualityRatioSlider->value());

	setSetting("LastDesired/sVideoCodec", ui->videoCodecComboBox->currentText());
	setSetting("LastDesired/sAudioCodec", ui->audioCodecCombobox->currentText());
	setSetting("LastDesired/sContainer", ui->containerCombobox->currentText());

	setSetting("LastDesired/bOpenInExplorerOnSuccess", ui->openInExplorerCheckbox->isChecked());
	setSetting("LastDesired/bPlayResultOnSuccess", ui->playCheckbox->isChecked());
	setSetting("LastDesired/bCloseOnSuccess", ui->closeToolCheckbox->isChecked());

	event->accept();
}
