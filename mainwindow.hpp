#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "compressor.hpp"
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QMainWindow>
#include <QPropertyAnimation>
#include <QSettings>
#include <QUrl>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	explicit MainWindow(QWidget* parent = nullptr);
	~MainWindow() override;

	QVariant setting(const QString& key);
	void setSetting(const QString& key, const QVariant& value);

private:
	QUrl selectedUrl;
	Compressor* compressor = new Compressor(this);

	Ui::MainWindow* ui;
	QLabel* spinner = new QLabel(this);
	QPropertyAnimation* progressBarAnimation;

	QSettings settings = QSettings("config.ini", QSettings::IniFormat);

protected:
	void closeEvent(QCloseEvent* event) override;
};
#endif // MAINWINDOW_HPP
