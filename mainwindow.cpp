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
	ui->sizeSpinBox->setValue(setting("Main/iLastDesiredFileSize").toInt());
	ui->sizeUnitComboBox->setCurrentText(setting("Main/sLastDesiredFileSizeUnit").toString());

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

		// get media audio bitrate
		ffprobe.startCommand("ffprobe -v error -select_streams a:0 -show_entries "
					   "stream=bit_rate -of default=noprint_wrappers=1:nokey=1 "
					   + selectedUrl.toLocalFile());
		ffprobe.waitForFinished();

		ok = false;
		double audioBitrateKbps = ffprobe.readAllStandardOutput().toDouble(&ok);

		if (!ok) {
			QMessageBox::critical(
				this,
				"Failed to probe medium",
				"An error occured while probing the media file bitrate: \n\n"
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

		// TODO: Calculate accurately? (statistics)
		double errorCorrection = (1.0 - setting("Main/dBitrateErrorMargin").toDouble());
		double bitrateKbps = ui->sizeSpinBox->value() * unitConversionFactor / lengthSeconds
					   * errorCorrection;
		double videoBitrateRatio = ui->qualityRatioSlider->value() / 100.0;

		QStringList fileName = selectedUrl.fileName().split(".");
		QProcess ffmpeg;
		QString command = QString("ffmpeg.exe -i %1 -b:v %2k -b:a %3k %4_%5.%6 -y")
						.arg(selectedUrl.toLocalFile(),
						     QString::number((1 - videoBitrateRatio) * bitrateKbps),
						     QString::number(videoBitrateRatio * bitrateKbps),
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

		QMessageBox::information(this,
						 "Compressed successfully",
						 QString("Requested size was %1 MB.\n\nCalculated bitrate "
							   "was %2 kbps.\n\nBased on quality ratio of "
							   "%3:\n\tVideo bitrate was set to %4 "
							   "kbps;\n\tAudio bitrate was set to %5 kbps.\n\nAn "
							   "error correction of %6 % was applied.")
							 .arg(QString::number(ui->sizeSpinBox->value()),
								QString::number(bitrateKbps),
								QString::number(videoBitrateRatio),
								QString::number((1 - videoBitrateRatio)
										    * bitrateKbps),
								QString::number(videoBitrateRatio * bitrateKbps),
								QString::number((1 - errorCorrection) * 100)));

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
					   "exist. Is this a typo?");
	}

	settings.setValue(key, value);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	// save current parameters for next program run
	setSetting("Main/iLastDesiredFileSize", ui->sizeSpinBox->value());
	setSetting("Main/sLastDesiredFileSizeUnit", ui->sizeUnitComboBox->currentText());

	event->accept();
}
