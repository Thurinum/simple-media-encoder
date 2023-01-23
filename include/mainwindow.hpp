#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "compressor.hpp"
#include "settings.hpp"

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
using Preset = Compressor::Preset;

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
	void SetControlsState(QAbstractButton* button);
	void ParseCodecs(QHash<QString, Codec>* codecs, const QString& type, QComboBox* comboBox);
	void ParseContainers(QHash<QString, Container>* containers, QComboBox* comboBox);
	void ParsePresets(QHash<QString, Preset>& presets,
				QHash<QString, Codec>& videoCodecs,
				QHash<QString, Codec>& audioCodecs,
				QHash<QString, Container>& containers,
				QComboBox* comboBox);
	void ParseMetadata(const QString& path);
	void CheckAspectRatioConflict();
	void CheckSpeedConflict();
	void		DownloadDependencies();
	QString getOutputPath(QString inputFilePath);
	inline bool isAutoValue(QAbstractSpinBox* spinBox);

	QHash<QString, Codec> videoCodecs;
	QHash<QString, Codec> audioCodecs;
	QHash<QString, Container> containers;
	QHash<QString, Preset> presets;

	const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";
	Ui::MainWindow* ui;
	Settings settings;
	Warnings* warnings;
	void InitSettings(const QString& fileName);
	Compressor* compressor = new Compressor(this);
	bool m_isNvidia = false;
	optional<Compressor::Metadata> m_metadata;
};

#endif // MAINWINDOW_HPP
