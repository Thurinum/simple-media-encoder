#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "encoder.hpp"
#include "notifier.hpp"
#include "platform_info.hpp"
#include "serializer.hpp"
#include "settings.hpp"
#include "warnings.hpp"

#include <QComboBox>
#include <QGraphicsBlurEffect>
#include <QLabel>
#include <QMainWindow>
#include <QMessageBox>
#include <QPointer>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSpinBox>
#include <QUrl>

using Codec = MediaEncoder::Codec;
using Container = MediaEncoder::Container;
using Preset = MediaEncoder::Preset;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(
        MediaEncoder& encoder,
        QSharedPointer<Settings> settings,
        QSharedPointer<Serializer> serializer,
        const Notifier& notifier,
        const PlatformInfo& platformInfo,
        QWidget* parent = nullptr
    );
    ~MainWindow() override;

protected:
    void CheckForBinaries();
    void SetupMenu();
    void SetupEncodingCallbacks();

    void LoadState();
    void SaveState();
    void closeEvent(QCloseEvent* event) override;

    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const MediaEncoder::Options& options,
                       const MediaEncoder::ComputedOptions& computed,
                       QFile& output);
    void HandleFailure(const QString& shortError, const QString& longError);

private slots:
    void StartEncoding();
    void SetAdvancedMode(bool enabled);
    void OpenInputFile();
    void SelectOutputDirectory();
    void ShowMetadata();
    void SelectPresetCodecs();
    void SetControlsState(QAbstractButton* button);
    void CheckAspectRatioConflict();
    void CheckSpeedConflict();
    void UpdateAudioQualityLabel(int value);
    void SetAllowPresetSelection(bool allowed);

private:
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
    void SetProgressShown(bool shown, int progressPercent = 0);
    void ValidateSelectedUrl();
    void ValidateSelectedDir();

    Ui::MainWindow* ui;
    Warnings* warnings;

    QHash<QString, Codec> videoCodecs;
    QHash<QString, Codec> audioCodecs;
    QHash<QString, Container> containers;
    QHash<QString, Preset> presets;
    optional<Metadata> metadata;

    MediaEncoder& encoder;
    QSharedPointer<Settings> settings;
    QSharedPointer<Serializer> serializer;
    const Notifier& notifier;
    const PlatformInfo& platformInfo;
};

#endif // MAINWINDOW_HPP
