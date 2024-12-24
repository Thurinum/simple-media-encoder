#pragma once

#include "core/formats/metadata_loader.hpp"
#include "encoder/encoder.hpp"
#include "formats/format_support_loader.hpp"
#include "notifier/notifier.hpp"
#include "settings/serializer.hpp"
#include "settings/settings.hpp"
#include "ui/overlay_widget.hpp"
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

inline auto di_settings = [] {};
inline auto di_presets = [] {};

class MainWindow final : public QMainWindow
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

    enum StreamType
    {
        VideoAudio,
        VideoOnly,
        AudioOnly
    };

protected:
    void CheckForFFmpeg() const;
    void SetupMenu();
    void SetupEventCallbacks();
    void QuerySupportedFormatsAsync();

    void LoadState();
    void LoadPresetNames() const;
    void SaveState() const;

    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const EncoderOptions& options, const MediaEncoder::ComputedOptions& computed, QFile& output);
    void HandleFailure(const QString& shortError, const QString& longError);
    void ShowAbout() const;

    // NOTE: const parameters are NOT supported by Qt slots setup from the designer!
private slots:
    void StartEncoding();
    void SetAdvancedMode(bool enabled);
    void OpenInputFile();
    void SelectOutputDirectory();
    void ShowMetadata();
    void LoadPreset(int index) const;
    void SelectVideoCodec(int index) const;
    void SelectAudioCodec(int index) const;
    void UpdateControlsState() const;
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
    void SetProgressShown(const ProgressState& state) const;
    void LoadSelectedUrl();
    void LoadInputFile(const QUrl& url);
    void ValidateSelectedDir() const;
    void SetupAnimations();
    double getOutputSizeKbps() const;

    Ui::MainWindow* ui;
    QScopedPointer<OverlayWidget> overlay;
    QScopedPointer<Warnings> warnings;

    optional<Metadata> metadata;

    std::unique_ptr<const QList<QObject*>> preferenceWidgets;
    std::unique_ptr<const QList<QObject*>> presetWidgets;

    std::unique_ptr<const QList<QWidget*>> videoControls;
    std::unique_ptr<const QList<QWidget*>> audioControls;

    std::unique_ptr<QPropertyAnimation> sectionWidthAnim;
    std::unique_ptr<QPropertyAnimation> sectionHeightAnim;
    std::unique_ptr<QPropertyAnimation> windowSizeAnim;
    std::unique_ptr<QPropertyAnimation> progressBarValueAnim;
    std::unique_ptr<QPropertyAnimation> progressBarHeightAnim;

    QScopedPointer<QMenu> menu;

    MediaEncoder& encoder;
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Settings> presetsSettings;
    std::shared_ptr<Serializer> serializer;
    MetadataLoader& metadataLoader;
    Notifier& notifier;
    PlatformInfo& platformInfo;
    FormatSupportLoader& formatSupport;

    bool isDragging = false;
    bool isValidMimeForDrop = false;

    QSharedPointer<FormatSupport> formatSupportCache;
};
