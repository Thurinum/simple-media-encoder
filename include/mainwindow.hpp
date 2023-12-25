#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "encoder.hpp"
#include "settings.hpp"
#include "warnings.hpp"

#include <QComboBox>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSpinBox>
#include <QUrl>

using Codec = MediaEncoder::Codec;
using Container = MediaEncoder::Container;
using Preset = MediaEncoder::Preset;

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

	void Notify(Severity severity, const QString& title, const QString& message, const QString& details = "");

protected:
	// initialization
	void SetupSettings();
	void CheckForBinaries();
	void SetupMenu();
	void DetectNvidia();
	void SetupUiInteractions();

	void LoadState();
	void SaveState();
	void closeEvent(QCloseEvent* event) override;

	// core logic
    void StartEncoding();
    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const MediaEncoder::Options &options,
                       const MediaEncoder::ComputedOptions &computed,
                       QFile &output);
    void HandleFailure(const QString &shortError, const QString &longError);

private slots:
	void SetAdvancedMode(bool enabled);
	void OpenInputFile();
	void ShowMetadata();
	void SelectPresetCodecs();
	void SetProgressShown(bool shown, int progressPercent = 0);
	void SetControlsState(QAbstractButton* button);
	void CheckAspectRatioConflict();
	void CheckSpeedConflict();

private:
	const bool IS_WINDOWS = QSysInfo::kernelType() == "winnt";

	void ParseCodecs(QHash<QString, Codec>* codecs, const QString& type, QComboBox* comboBox);
	void ParseContainers(QHash<QString, Container>* containers, QComboBox* comboBox);
	void ParsePresets(QHash<QString, Preset>& presets,
				QHash<QString, Codec>& videoCodecs,
				QHash<QString, Codec>& audioCodecs,
				QHash<QString, Container>& containers,
				QComboBox* comboBox);
	void ParseMetadata(const QString& path);
	QString getOutputPath(QString inputFilePath);
	inline bool isAutoValue(QAbstractSpinBox* spinBox);

	bool isNvidia = false;

	Ui::MainWindow* ui;
	Settings settings;
	Warnings* warnings;
    MediaEncoder *encoder = new MediaEncoder(this);

    QHash<QString, Codec> videoCodecs;
    QHash<QString, Codec> audioCodecs;
    QHash<QString, Container> containers;
    QHash<QString, Preset> presets;
    optional<Metadata>	  metadata;
};

#endif // MAINWINDOW_HPP
