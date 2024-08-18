#pragma once

#include "encoder/encoder.hpp"
#include "formats/format_support_loader.hpp"
#include "notifier/notifier.hpp"
#include "settings/serializer.hpp"
#include "settings/settings.hpp"
#include "utils/platform_info.hpp"
#include "utils/warnings.hpp"

#include <QMainWindow>
#include <QMessageBox>
#include <QPropertyAnimation>
#include <QSettings>
#include <QSpinBox>
#include <di.hpp>

QT_BEGIN_NAMESPACE
namespace Ui
{
class MainWindow;
}
QT_END_NAMESPACE

using namespace std::string_literals;

inline auto di_settings = [] {};
inline auto di_presets = [] {};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    BOOST_DI_INJECT(
        MainWindow,
        MediaEncoder& encoder,
        (named = di_settings) std::shared_ptr<Settings> settings,
        (named = di_presets) std::shared_ptr<Settings> presetsSettings,
        std::shared_ptr<Serializer> serializer,
        MetadataLoader& metadata,
        Notifier& notifier,
        PlatformInfo& platformInfo,
        FormatSupportLoader& formatSupportLoader
    );
    ~MainWindow() override;

protected:
    void CheckForBinaries();
    void SetupMenu();
    void SetupEventCallbacks();
    void QuerySupportedFormatsAsync();

    void LoadState();
    void LoadPresetNames() const;
    void SaveState() const;
    void closeEvent(QCloseEvent* event) override;

    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const EncoderOptions& options, const MediaEncoder::ComputedOptions& computed, QFile& output);
    void HandleFailure(const QString& shortError, const QString& longError);
    void ShowAbout();

private slots:
    void StartEncoding();
    void SetAdvancedMode(bool enabled);
    void OpenInputFile();
    void SelectOutputDirectory();
    void ShowMetadata();
    void LoadPreset(int index) const;
    void SetControlsState(QAbstractButton* button);
    void CheckAspectRatioConflict();
    void CheckSpeedConflict();
    void UpdateAudioQualityLabel(int value);
    void SetAllowPresetSelection(bool allowed);
    void HandleFormatsQueryResult(const std::variant<QSharedPointer<FormatSupport>, Message>& maybeFormats);
    void UpdateCodecsList(bool commonOnly) const;

private:
    struct ProgressState
    {
        optional<QString> status = optional<QString>();
        optional<int> progressPercent = optional<int>();
    };

    void QueryMediaMetadataAsync(const QString& path);
    void ReceiveMediaMetadata(MetadataResult result);
    QString getOutputPath(QString inputFilePath);
    inline bool isAutoValue(QAbstractSpinBox* spinBox);
    void SetProgressShown(ProgressState state);
    void LoadSelectedUrl();
    void ValidateSelectedDir();
    void SetupAdvancedModeAnimation();
    double getOutputSizeKbps();

    Ui::MainWindow* ui;
    Warnings* warnings;

    optional<Metadata> metadata;

    // todo: unique ptr
    const QList<QObject*>* preferenceWidgets;
    const QList<QObject*>* presetWidgets;

    QPropertyAnimation* sectionWidthAnim;
    QPropertyAnimation* sectionHeightAnim;
    QPropertyAnimation* windowSizeAnim;

    MediaEncoder& encoder;
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Settings> presetsSettings;
    std::shared_ptr<Serializer> serializer;
    MetadataLoader& metadataLoader;
    Notifier& notifier;
    PlatformInfo& platformInfo;
    FormatSupportLoader& formatSupport;

    QSharedPointer<FormatSupport> formatSupportCache;
};
