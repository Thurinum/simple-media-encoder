#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	ui->progressBar->setVisible(false);

	// restore UI state
	selectedUrl = QUrl(setting("Main/sLastFile").toString());

	ui->fileName->setText(selectedUrl.fileName());
	ui->sizeSpinBox->setValue(setting("Main/dLastDesiredFileSize").toDouble());
	ui->fileSuffix->setText(setting("Main/sLastDesiredFileSuffix").toString());
	ui->sizeUnitComboBox->setCurrentText(setting("Main/sLastDesiredFileSizeUnit").toString());
	ui->qualityRatioSlider->setValue(setting("Main/iLastDesiredQualityRatio").toInt());

	// pick file
	// TODO: Set file type filters
	QObject::connect(ui->fileButton, &QPushButton::clicked, [this]() {
		QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
									 "Select file to compress",
									 QDir::currentPath(),
									 "*");

		ui->fileName->setText(fileUrl.fileName());
		selectedUrl = fileUrl;
	});

	// start conversion
	QObject::connect(ui->startButton, &QPushButton::clicked, [this]() {
		if (!selectedUrl.isValid()) {
			QMessageBox::information(this,
							 "No file selected",
							 "Please select a file to compress.");
			return;
		}

		ui->progressBar->setVisible(true);

		// get media duration in seconds, which is required for calculating bitrate
		QProcess ffprobe;
		ffprobe.startCommand("ffprobe.exe -v error -show_entries format=duration -of "
					   "default=noprint_wrappers=1:nokey=1 "
					   + selectedUrl.toLocalFile());
		ffprobe.waitForFinished();

		bool ok = false;
		double lengthSeconds = ffprobe.readAllStandardOutput().toDouble(&ok);

		if (!ok) {
			QMessageBox::critical(this,
						    "Failed to probe medium",
						    "An error occured while probing the media file length: \n\n"
							    + ffprobe.readAllStandardOutput());
			ui->progressBar->setVisible(false);
			return;
		}

		double unitConversionFactor = -1;

		// TODO: Make more robust by populating combobox from C++
		switch (ui->sizeUnitComboBox->currentIndex()) {
		case 0: //
			unitConversionFactor = 8;
			break;
		case 1: // MB to kb
			unitConversionFactor = 8000;
			break;
		case 2: //
			unitConversionFactor = 8e+6;
			break;
		}

		double minBitrateVideo = setting("Main/dMinBitrateVideoKbps").toDouble();
		double minBitrateAudio = setting("Main/dMinBitrateAudioKbps").toDouble();
		double maxBitrateAudio = setting("Main/dMaxBitrateAudioKbps").toDouble();

		// TODO: Calculate accurately? (statistics)
		double errorCorrection = (1.0 - setting("Main/dBitrateErrorMargin").toDouble());
		double bitrateKbps = ui->sizeSpinBox->value() * unitConversionFactor / lengthSeconds
					   * errorCorrection;
		double audioBitrateRatio = ui->qualityRatioSlider->value() / 100.0;
		double audioBitrateKbps = qMax(minBitrateAudio, audioBitrateRatio * maxBitrateAudio);
		double videoBitrateKpbs = qMax(minBitrateVideo, bitrateKbps - audioBitrateKbps);

		QStringList fileName = selectedUrl.fileName().split(".");
		QProcess ffmpeg;
		QString command = QString("ffmpeg.exe -i %1 -b:v %2k -b:a %3k %4_%5.%6 -y")
						.arg(selectedUrl.toLocalFile(),
						     QString::number(videoBitrateKpbs),
						     QString::number(audioBitrateKbps),
						     fileName.first(),
						     ui->fileSuffix->text(),
						     fileName.last());
		ffmpeg.startCommand(command);

		if (!ffmpeg.waitForFinished())
			QMessageBox::critical(this,
						    "Failed to compress",
						    "An error occured while compressing the media file:\n\n"
							    + ffmpeg.readAllStandardOutput()
							    + "\n\nCommand: " + command);

		qDebug() << audioBitrateKbps;
		QMessageBox::information(
			this,
			"Compressed successfully",
			QString("Requested size was %1 MB.%7\n\nCalculated bitrate "
				  "was %2 kbps.\n\nBased on quality ratio of "
				  "%3:\n\tVideo bitrate was set to %4 "
				  "kbps;\n\tAudio bitrate was set to %5 kbps.\n\nAn "
				  "error correction of %6 % was applied.")
				.arg(QString::number(ui->sizeSpinBox->value()),
				     QString::number(bitrateKbps),
				     QString::number(audioBitrateRatio),
				     QString::number(videoBitrateKpbs),
				     QString::number(audioBitrateKbps),
				     QString::number((1 - errorCorrection) * 100),
				     videoBitrateKpbs + audioBitrateKbps > bitrateKbps
					     ? " It is too small and may not have been reached."
					     : ""));

		ui->progressBar->setVisible(false);
	});
}

MainWindow::~MainWindow()
{
	delete ui;
}

QVariant MainWindow::setting(QString key)
{
	if (!settings.contains(key)) {
		QMessageBox::critical(this,
					    "Missing configuration",
					    "The configuration file is missing a key. Please reinstall the "
					    "program.\n\nMissing key: "
						    + key + "\nWorking dir: " + QDir::currentPath());
		// TODO: Can't use QApplication:: to quit because window isn't yet opened.
		// Find a proper workaround lol
		return 0 / 0;
	}

	return settings.value(key);
}

void MainWindow::setSetting(QString key, QVariant value)
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

void MainWindow::closeEvent(QCloseEvent *event)
{
	// save current parameters for next program run
	setSetting("Main/sLastFile", selectedUrl);
	setSetting("Main/dLastDesiredFileSize", ui->sizeSpinBox->value());
	setSetting("Main/sLastDesiredFileSizeUnit", ui->sizeUnitComboBox->currentText());
	setSetting("Main/sLastDesiredFileSuffix", ui->fileSuffix->text());
	setSetting("Main/iLastDesiredQualityRatio", ui->qualityRatioSlider->value());

	event->accept();
}
