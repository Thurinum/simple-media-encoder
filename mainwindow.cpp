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

		QString path = selectedUrl.toLocalFile();
		QProcess* ffmpeg = new QProcess(this);
		QStringList arguments;
		arguments << "-i" << path << "-fs"
			    << QString::number(ui->sizeSpinBox->value())
					 + ui->sizeUnitComboBox->currentText().first(1)
			    << path + ui->fileSuffix->text() << "-y";

		//		ffmpeg->setProgram("ffmpeg.exe");
		//		ffmpeg->setArguments(arguments);

		if (!QProcess::execute("ffmpeg.exe", arguments))
			QMessageBox::critical(this,
						    "Something went wrong",
						    "File compression failed:\n\n" + ffmpeg->errorString());

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
