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

class Warnings : QHash<QString, QString>
{
public:
	explicit Warnings(QWidget* widget);
	void Add(QString name, QString description);
	void Remove(QString name);

private:
	QWidget* m_widget;
	void Update();
};

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
	void SaveState();
	void SetProgressShown(bool shown, int progressPercent = 0);
	void SetAdvancedMode(bool enabled);

	void ParseCodecs(QList<Codec>* codecs, const QString& type, QComboBox* comboBox);
	void ParseContainers(QList<Container>* containers, QComboBox* comboBox);

	QString getOutputPath(QString inputFilePath);

	inline bool isAutoValue(QAbstractSpinBox* spinBox);

	void CheckAspectRatioConflict();
	void CheckSpeedConflict();

	Ui::MainWindow* ui;
	Warnings* warnings;
	void InitSettings(const QString& fileName);
	Compressor* compressor = new Compressor(this);
	QSettings* settings;
	bool m_isNvidia = false;
};

#endif // MAINWINDOW_HPP
