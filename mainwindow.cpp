#include "mainwindow.hpp"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);

	ui->slider->connect(ui->slider, &QSlider::sliderMoved, [=]() {
		ui->sliderValue->setText(QString::number(ui->slider->value()) + " %");
	});
}

MainWindow::~MainWindow()
{
	delete ui;
}

