#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "compressor.hpp"
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QMainWindow>
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

private:
	Ui::MainWindow *ui;
	QUrl selectedUrl;
	QSettings settings = QSettings("config.ini", QSettings::IniFormat);
	Compressor* compressor = new Compressor(this);
	QLabel* spinner = new QLabel(this);

	QVariant setting(const QString& key);
	void setSetting(const QString& key, const QVariant& value);

protected:
	void closeEvent(QCloseEvent* event) override;
};
#endif // MAINWINDOW_HPP
