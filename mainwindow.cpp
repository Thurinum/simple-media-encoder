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
		ui->sliderValue->setText(QString::number(ui->slider->value()) + " %");
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

