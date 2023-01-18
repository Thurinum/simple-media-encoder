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
#include <QSpinBox>
#include <QUrl>

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

	enum Severity {
		Info = QMessageBox::Information,
		Warning = QMessageBox::Warning,
		Error = QMessageBox::Critical,
		Critical // Will terminate program
	};

	const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";
	const QString CONFIG_FILE = "config.ini";

	QVariant setting(const QString& key);
	void setSetting(const QString& key, const QVariant& value);

	void Notify(Severity severity,
			const QString& title,
			const QString& message,
			const QString& details = "");

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	void LoadState();
	void SetProgressShown(bool shown, int progressPercent = 0);
	void SetAdvancedMode(bool enabled);

	void ParseCodecs(QList<Codec>* codecs, const QString& type, QComboBox* comboBox);
	void ParseContainers(QList<Container>* containers, QComboBox* comboBox);

	QString outputPath(QString inputFileName);

	inline bool isAutoValue(QAbstractSpinBox* spinBox);

	Ui::MainWindow* ui;
	Compressor* compressor = new Compressor(this);
	QSettings settings = QSettings("config.ini", QSettings::IniFormat);
	bool m_isNvidia = false;
};
#endif // MAINWINDOW_HPP
