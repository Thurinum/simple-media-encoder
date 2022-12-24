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
	ui->slider->setValue(setting("Main/iSliderValue").toInt());
	ui->sliderValue->setText(QString::number(ui->slider->value()) + " %");

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

	// display slider value
	QObject::connect(ui->slider, &QSlider::valueChanged, [this]() {
		int value = ui->slider->value();

		ui->sliderValue->setText(QString::number(value) + " %");
		setSetting("Main/iSliderValue", value);
	});

	// start conversion
	QObject::connect(ui->startButton, &QPushButton::clicked, [this]() {
		if (!selectedUrl.isValid()) {
			QMessageBox::information(this,
							 "No file selected",
							 "Please select a file to compress.");
			return;
		}

		QString command = "explorer";
		ui->progressBar->setVisible(true);

		if (QProcess::execute(command, QStringList() << "") < 0) {
			QMessageBox::critical(this,
						    "Something went wrong",
						    "File compression failed. Please try again.");
		}

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
