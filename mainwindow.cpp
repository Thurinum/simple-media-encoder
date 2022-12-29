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

using Severity = QMessageBox::Icon;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	progressBarAnimation = new QPropertyAnimation(ui->progressBar, "value");
	progressBarAnimation->setDuration(100);
	ui->progressWidget->setVisible(false);

	// menu
	QMenu *menu = new QMenu(this);
	menu->addAction("Help", &QWhatsThis::enterWhatsThisMode);
	menu->addSeparator();
	menu->addAction("About");
	menu->addAction("About Qt", &QApplication::aboutQt);
	ui->toolButton->setMenu(menu);
	connect(ui->toolButton, &QToolButton::pressed, ui->toolButton, &QToolButton::showMenu);

	// populate format dropdowns
	for (const Compressor::Format &format : compressor->videoCodecs)
		ui->videoCodecComboBox->addItem(format.name, format.library);

	for (const Compressor::Format &format : compressor->audioCodecs)
		ui->audioCodecCombobox->addItem(format.name, format.library);

	for (const QString &container : compressor->containers)
		ui->containerCombobox->addItem(container);

	// start compression button
	connect(ui->startButton, &QPushButton::clicked, [this]() {
		if (!selectedUrl.isValid()) {
			QMessageBox::information(this,
							 "No file selected",
							 "Please select a file to compress.");
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
		compressor
			->compress(selectedUrl,
				     ui->fileSuffix->text(),
				     Compressor::Format(ui->videoCodecComboBox->currentText(),
								ui->videoCodecComboBox->currentData().toString()),
				     Compressor::Format(ui->audioCodecCombobox->currentText(),
								ui->audioCodecCombobox->currentData().toString()),
				     ui->containerCombobox->currentText(),
				     ui->sizeSpinBox->value() * sizeKbpsConversionFactor,
				     ui->qualityRatioSlider->value() / 100.0);
	});

	// handle displaying target bitrates during compression
	connect(compressor,
		  &Compressor::compressionStarted,
		  [this](double videoBitrateKbps, double audioBitrateKbps) {
			  ui->progressLabel->setText(
				  QString("Video bitrate: %1 kbps | Audio bitrate: %2 kbps")
					  .arg(QString::number(qRound(videoBitrateKbps)),
						 QString::number(qRound(audioBitrateKbps))));
		  });

	// handle successful compression
	connect(compressor,
		  &Compressor::compressionSucceeded,
		  [this](double requestedSizeKbps, double actualSizeKbps) {
			  setProgress(100);
			  QMessageBox::information(
				  this,
				  "Compressed successfully",
				  QString(
					  "Requested size was %1 kb.\nActual compression achieved is %2 kb.")
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
			  ShowMessageBox(Severity::Critical, "Compression failed", shortError, longError);
			  hideProgress();
		  });

	// handle compression progress updates
	connect(compressor, &Compressor::compressionProgressUpdate, this, &MainWindow::setProgress);

	// show name of file picked with file dialog
	connect(ui->fileButton, &QPushButton::clicked, [this]() {
		QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
									 "Select file to compress",
									 QDir::currentPath(),
									 "*");

		ui->fileName->setText(fileUrl.toLocalFile());
		selectedUrl = fileUrl;
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
	ui->fileName->setText(selectedUrl.toLocalFile());
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
		QMessageBox::critical(this,
					    "Missing configuration",
					    "The configuration file is missing a key. Please reinstall the "
					    "program.\n\nMissing key: "
						    + key + "\nWorking dir: " + QDir::currentPath());
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
	setSetting("Main/dLastDesiredFileSize", ui->sizeSpinBox->value());
	setSetting("Main/sLastDesiredFileSizeUnit", ui->sizeUnitComboBox->currentText());
	setSetting("Main/sLastDesiredFileSuffix", ui->fileSuffix->text());
	setSetting("Main/iLastDesiredQualityRatio", ui->qualityRatioSlider->value());

	setSetting("Main/sLastDesiredVideoCodec", ui->videoCodecComboBox->currentText());
	setSetting("Main/sLastDesiredAudioCodec", ui->audioCodecCombobox->currentText());
	setSetting("Main/sLastDesiredContainer", ui->containerCombobox->currentText());

	event->accept();
}
