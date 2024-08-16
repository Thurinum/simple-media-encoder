#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

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
        std::shared_ptr<Settings> settings,
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
    void SaveState();
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
    void SelectPresetCodecs();
    void SetControlsState(QAbstractButton* button);
    void CheckAspectRatioConflict();
    void CheckSpeedConflict();
    void UpdateAudioQualityLabel(int value);
    void SetAllowPresetSelection(bool allowed);
    void HandleFormatsQueryResult(std::variant<QSharedPointer<FormatSupport>, Message> maybeFormats);

private:
    struct ProgressState {
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

    QPropertyAnimation* sectionWidthAnim;
    QPropertyAnimation* sectionHeightAnim;
    QPropertyAnimation* windowSizeAnim;

    MediaEncoder& encoder;
    std::shared_ptr<Settings> settings;
    std::shared_ptr<Serializer> serializer;
    MetadataLoader& metadataLoader;
    Notifier& notifier;
    PlatformInfo& platformInfo;
    FormatSupportLoader& formatSupport;
};

#endif // MAINWINDOW_HPP
