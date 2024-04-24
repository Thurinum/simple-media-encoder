#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "encoder.hpp"
#include "notifier.hpp"
#include "platform_info.hpp"
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
        const QPointer<Settings> settings,
        const Notifier& notifier,
        const PlatformInfo& platformInfo,
        QWidget* parent = nullptr
    );
    ~MainWindow() override;

protected:
    // initialization
    void CheckForBinaries();
    void SetupMenu();
    void SetupUiInteractions();

    void LoadState();
    void SaveState();
    void closeEvent(QCloseEvent* event) override;

    // core logic
    void StartEncoding();
    void HandleStart(double videoBitrateKbps, double audioBitrateKbps);
    void HandleSuccess(const MediaEncoder::Options& options,
                       const MediaEncoder::ComputedOptions& computed,
                       QFile& output);
    void HandleFailure(const QString& shortError, const QString& longError);

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

    Ui::MainWindow* ui;
    Warnings* warnings;

    QHash<QString, Codec> videoCodecs;
    QHash<QString, Codec> audioCodecs;
    QHash<QString, Container> containers;
    QHash<QString, Preset> presets;
    optional<Metadata> metadata;

    // FIXME: Modify so it can be const (services should be immutable)
    MediaEncoder& encoder;
    QPointer<Settings> settings;
    const Notifier& notifier;
    const PlatformInfo& platformInfo;
};

#endif // MAINWINDOW_HPP
