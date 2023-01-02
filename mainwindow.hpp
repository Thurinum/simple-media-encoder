#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "compressor.hpp"

#include <QComboBox>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QUrl>

using Severity = QMessageBox::Icon;
using Codec = Compressor::Codec;
using Container = Compressor::Container;

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
	QDir selectedDir;
	Compressor* compressor = new Compressor(this);
	QSettings settings = QSettings("config.ini", QSettings::IniFormat);

	Ui::MainWindow* ui;
	QPropertyAnimation* progressBarAnimation;

	void setProgress(int progressPercent);
	void showProgress();
	void hideProgress();

	void ShowMessageBox(Severity severity,
				  const QString& title,
				  const QString& message,
				  const QString& details = "");

	void parseCodecs(QList<Codec>* codecs, const QString& type, QComboBox* comboBox);
	void parseContainers(QList<Container>* containers, QComboBox* comboBox);

protected:
	void closeEvent(QCloseEvent* event) override;
};
#endif // MAINWINDOW_HPP
