#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "encoder/encoder.hpp"
#include "formats/format_support_loader.hpp"
#include "notifier/notifier.hpp"
#include "settings/serializer.hpp"
#include "settings/settings.hpp"
#include "utils/platform_info.hpp"
#include "utils/warnings.hpp"

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
        FormatSupportLoader& formatSupportLoader,
        QWidget* parent = nullptr
    );
    ~MainWindow() override;

protected:
    void CheckForBinaries();
    void SetupMenu();
    void SetupEventCallbacks();
    void QuerySupportedFormats();

    void LoadState();
    void SaveState();
    void closeEvent(QCloseEvent* event) override;

    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const MediaEncoder::Options& options,
                       const MediaEncoder::ComputedOptions& computed,
                       QFile& output);
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
    void ParseMetadata(const QString& path);
    QString getOutputPath(QString inputFilePath);
    inline bool isAutoValue(QAbstractSpinBox* spinBox);
    void SetProgressShown(bool shown, int progressPercent = 0);
    void SetProgressIndeterminate(bool indeterminate);
    void ValidateSelectedUrl();
    void ValidateSelectedDir();
    void SetupAdvancedModeAnimation();

    Ui::MainWindow* ui;
    Warnings* warnings;

    optional<Metadata> metadata;

    QPropertyAnimation* sectionWidthAnim;
    QPropertyAnimation* sectionHeightAnim;
    QPropertyAnimation* windowSizeAnim;

    MediaEncoder& encoder;
    QSharedPointer<Settings> settings;
    QSharedPointer<Serializer> serializer;
    const Notifier& notifier;
    const PlatformInfo& platformInfo;
    FormatSupportLoader& formatSupport;
};

#endif // MAINWINDOW_HPP
