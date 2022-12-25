#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), ui(new Ui::MainWindow)
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
						    "An error occured while probing the media file: \n\n"
							    + ffprobe.readAllStandardOutput());
			ui->progressBar->setVisible(false);
			return;
		}

		// get media audio bitrate
		//		ffprobe.waitForFinished();

		//		bool ok = false;
		//		double lengthSeconds = ffprobe.readAllStandardOutput().toDouble(&ok);

		//		if (!ok) {
		//			QMessageBox::critical(this,
		//						    "Failed to probe medium",
		//						    "An error occured while probing the media file: \n\n"
		//							    + ffprobe.readAllStandardOutput());
		//			ui->progressBar->setVisible(false);
		//			return;
		//		}

		double unitFactor = -1;

		// TODO: Make more robust by populating combobox from C++
		switch (ui->sizeUnitComboBox->currentIndex()) {
		case 0: //
			unitFactor = 8;
			break;
		case 1: // MB to kb
			unitFactor = 8000;
			break;
		case 2: //
			unitFactor = 8e+6;
			break;
		}

		double bitrate = ui->sizeSpinBox->value() * unitFactor / lengthSeconds
				     * (1.0 - setting("Main/dBitrateRelativeUncertainty").toDouble());
		QStringList fileName = selectedUrl.fileName().split(".");
		QProcess ffmpeg;
		QString command = QString("ffmpeg.exe -i %1 -b:v %2k -an %4_%5.%6 -y")
						.arg(selectedUrl.toLocalFile(),
						     QString::number(bitrate),
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

		QMessageBox::information(
			this,
			"Compressed successfully",
			QString("Requested size was %1 MB.\n\nCalculated bitrate "
				  "was %2 bits.\n\nCalculated length was %3 s.\n\nUncertainty was assumed "
				  "to be %4.\n\nCommand run:\n\n%5")
				.arg(QString::number(ui->sizeSpinBox->value()),
				     QString::number(bitrate),
				     QString::number(lengthSeconds),
				     QString::number(
					     1.0 - setting("Main/dBitrateRelativeUncertainty").toDouble()),
				     command));

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
		// TODO: Can't use QApplication:: to quit because window isn't yet opened. Find a proper workaround lol
		return 0 / 0;
	}

	return settings.value(key);
}

void MainWindow::setSetting(QString key, QVariant value)
{
	if (!settings.contains(key)) {
		QMessageBox::warning(
			this,
			"Suspicious setting",
			"Attempted to create setting that doesn't already exist. Is this a typo?");
	}

	settings.setValue(key, value);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
	// save current parameters for next program run
	setSetting("Main/iLastDesiredFileSize", ui->sizeSpinBox->value());
	setSetting("Main/sLastDesiredFileSizeUnit", ui->sizeUnitComboBox->currentText());

	event->accept();
}
