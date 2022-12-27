#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMessageBox>
#include <QMovie>
#include <QPixmap>
#include <QProcess>
#include <QPropertyAnimation>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	//this->setFixedSize(this->width(), this->height());

	// setup spinner widget
	QMovie *spinnerGif = new QMovie("spinner.gif");
	spinner->setFixedSize(width(), height());
	spinner->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
	spinner->setStyleSheet("background-color: rgba(0, 0, 0, 69)");
	spinner->setMovie(spinnerGif);
	spinnerGif->setScaledSize(QSize(69, 69));
	spinnerGif->start();
	spinner->raise();
	spinner->close();

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

		spinner->show();
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

	// handle successful compression
	connect(compressor,
		  &Compressor::compressionSucceeded,
		  [this](double requestedSizeKbps, double actualSizeKbps) {
			  spinner->close();
			  QMessageBox::information(
				  this,
				  "Compressed successfully",
				  QString(
					  "Requested size was %1 kb.\nActual compression achieved is %2 kb.")
					  .arg(QString::number(requestedSizeKbps),
						 QString::number(actualSizeKbps)));
		  });

	// handle failed compression
	connect(compressor, &Compressor::compressionFailed, [this](QString errorMessage) {
		spinner->close();
		QMessageBox::critical(this,
					    "Failed to compress",
					    "Something went wrong when compressing the media file:\n\n\t"
						    + errorMessage,
					    QMessageBox::Ok);
	});

	// handle compression progress updates
	connect(compressor, &Compressor::compressionProgressUpdate, [this](int progressPercent) {
		ui->progressBar->setValue(progressPercent);
	});

	// show name of file picked with file dialog
	connect(ui->fileButton, &QPushButton::clicked, [this]() {
		QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
									 "Select file to compress",
									 QDir::currentPath(),
									 "*");

		ui->fileName->setText(fileUrl.fileName());
		selectedUrl = fileUrl;
	});

	// show value in kbps of audio quality slider
	connect(ui->qualityRatioSlider, &QSlider::valueChanged, [this]() {
		double currentValue = ui->qualityRatioSlider->value() / 100.0
					    * setting("Main/dMaxBitrateAudioKbps").toDouble();
		ui->qualityRatioSliderLabel->setText(QString::number(currentValue) + " kbps");
	});

	// restore UI state on run
	selectedUrl = QUrl(setting("Main/sLastFile").toString());
	ui->fileName->setText(selectedUrl.fileName());
	ui->sizeSpinBox->setValue(setting("Main/dLastDesiredFileSize").toDouble());
	ui->fileSuffix->setText(setting("Main/sLastDesiredFileSuffix").toString());
	ui->sizeUnitComboBox->setCurrentText(setting("Main/sLastDesiredFileSizeUnit").toString());
	ui->qualityRatioSlider->setValue(setting("Main/iLastDesiredQualityRatio").toInt());
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
