#include <QClipboard>
#include <QCloseEvent>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMovie>
#include <QPixmap>
#include <QProcess>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QThread>
#include <QTimer>
#include <QWhatsThis>

#include "mainwindow.hpp"
#include "notifier.hpp"
#include "serializer.hpp"
#include "ui_mainwindow.h"

using std::optional;

MainWindow::MainWindow(
    MediaEncoder& encoder,
    QSharedPointer<Settings> settings,
    QSharedPointer<Serializer> serializer,
    const Notifier& notifier,
    const PlatformInfo& platformInfo,
    QWidget* parent
)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , encoder(encoder)
    , settings(std::move(settings))
    , serializer(std::move(serializer))
    , notifier(notifier)
    , platformInfo(platformInfo)
{
    ui->setupUi(this);
    this->resize(this->minimumSizeHint());
    this->warnings = new Warnings(ui->warningTooltipButton);

    SetupAdvancedModeAnimation();

    ui->audioVideoButtonGroup->setId(ui->radVideoAudio, 0);
    ui->audioVideoButtonGroup->setId(ui->radVideoOnly, 1);
    ui->audioVideoButtonGroup->setId(ui->radAudioOnly, 2);

    CheckForBinaries();

    SetupMenu();

    ParseCodecs(&videoCodecs, "VideoCodecs", ui->videoCodecComboBox);
    ParseCodecs(&audioCodecs, "AudioCodecs", ui->audioCodecComboBox);
    ParseContainers(&containers, ui->containerComboBox);
    ParsePresets(presets, videoCodecs, audioCodecs, containers, ui->qualityPresetComboBox);

    SetupEncodingCallbacks();
    LoadState();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete warnings;
}

void MainWindow::CheckForBinaries()
{
    if (platformInfo.isWindows() && (!QFile::exists("ffmpeg.exe") || !QFile::exists("ffprobe.exe"))) {
        notifier.Notify(Severity::Critical, tr("Could not find ffmpeg binaries"), tr("If compiling from source, this is not a bug. Please download ffmpeg.exe and ffprobe.exe and "
                                                                                     "place them into the binaries directory. Otherwise, there is an error with the release; please "
                                                                                     "report this as a bug."));
    }
}

void MainWindow::SetupMenu()
{
    QMenu* menu = new QMenu(this);

    menu->addAction(tr("Help"), &QWhatsThis::enterWhatsThisMode);
    menu->addSeparator();
    menu->addAction(tr("About"), this, &MainWindow::ShowAbout);
    menu->addAction(tr("About Qt"), &QApplication::aboutQt);

    ui->infoMenuToolButton->setMenu(menu);
}

void MainWindow::SetupEncodingCallbacks()
{
    connect(&encoder, &MediaEncoder::encodingStarted, this, &MainWindow::HandleStart);
    connect(&encoder, &MediaEncoder::encodingSucceeded, this, &MainWindow::HandleSuccess);
    connect(&encoder, &MediaEncoder::encodingFailed, this, &MainWindow::HandleFailure);
    connect(&encoder, &MediaEncoder::encodingProgressUpdate, this, [this](int progress) {
        SetProgressShown(true, progress);
    });
    connect(&encoder, &MediaEncoder::metadataComputed, this, [this]() {
        SetProgressShown(false);
        ui->progressBar->setRange(0, 100);
        ui->progressBarLabel->setText(tr("Compressing..."));
    });
}

void MainWindow::LoadState()
{
    serializer->deserialize(ui->advancedModeCheckBox);
    SetAdvancedMode(ui->advancedModeCheckBox->isChecked());

    serializer->deserialize(ui->audioVideoButtonGroup);
    SetControlsState(ui->audioVideoButtonGroup->checkedButton());

    serializer->deserialize(ui->outputFileNameLineEdit);
    ValidateSelectedUrl();

    serializer->deserialize(ui->outputFolderLineEdit);
    ValidateSelectedDir();

    serializer->deserialize(ui->aspectRatioSpinBoxH);
    serializer->deserialize(ui->aspectRatioSpinBoxV);
    serializer->deserialize(ui->audioCodecComboBox);
    serializer->deserialize(ui->audioQualitySlider);
    serializer->deserialize(ui->autoFillCheckBox);
    serializer->deserialize(ui->closeOnSuccessCheckBox);
    serializer->deserialize(ui->codecSelectionGroupBox);
    serializer->deserialize(ui->containerComboBox);
    serializer->deserialize(ui->customCommandTextEdit);
    serializer->deserialize(ui->deleteOnSuccessCheckBox);
    serializer->deserialize(ui->fileSizeSpinBox);
    serializer->deserialize(ui->fileSizeUnitComboBox);
    serializer->deserialize(ui->fpsSpinBox);
    serializer->deserialize(ui->heightSpinBox);
    serializer->deserialize(ui->inputFileLineEdit);
    serializer->deserialize(ui->openExplorerOnSuccessCheckBox);
    serializer->deserialize(ui->outputFileNameSuffixCheckBox);
    serializer->deserialize(ui->playOnSuccessCheckBox);
    serializer->deserialize(ui->speedSpinBox);
    serializer->deserialize(ui->videoCodecComboBox);
    serializer->deserialize(ui->warnOnOverwriteCheckBox);
    serializer->deserialize(ui->widthSpinBox);
}

void MainWindow::SaveState()
{
    serializer->serialize(ui->advancedModeCheckBox);
    serializer->serialize(ui->aspectRatioSpinBoxH);
    serializer->serialize(ui->aspectRatioSpinBoxV);
    serializer->serialize(ui->audioCodecComboBox);
    serializer->serialize(ui->audioQualitySlider);
    serializer->serialize(ui->audioVideoButtonGroup);
    serializer->serialize(ui->autoFillCheckBox);
    serializer->serialize(ui->closeOnSuccessCheckBox);
    serializer->serialize(ui->codecSelectionGroupBox);
    serializer->serialize(ui->containerComboBox);
    serializer->serialize(ui->customCommandTextEdit);
    serializer->serialize(ui->deleteOnSuccessCheckBox);
    serializer->serialize(ui->fileSizeSpinBox);
    serializer->serialize(ui->fileSizeUnitComboBox);
    serializer->serialize(ui->fpsSpinBox);
    serializer->serialize(ui->heightSpinBox);
    serializer->serialize(ui->inputFileLineEdit);
    serializer->serialize(ui->openExplorerOnSuccessCheckBox);
    serializer->serialize(ui->outputFileNameLineEdit);
    serializer->serialize(ui->outputFileNameSuffixCheckBox);
    serializer->serialize(ui->outputFolderLineEdit);
    serializer->serialize(ui->playOnSuccessCheckBox);
    serializer->serialize(ui->speedSpinBox);
    serializer->serialize(ui->videoCodecComboBox);
    serializer->serialize(ui->warnOnOverwriteCheckBox);
    serializer->serialize(ui->widthSpinBox);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    SaveState();
    event->accept();
}

void MainWindow::StartEncoding()
{
    QString inputPath = ui->inputFileLineEdit->text();

    if (!QFile::exists(inputPath)) {
        notifier.Notify(Severity::Info, tr("File not found"), tr("File '%1' does not exist. Please select a valid file.").arg(inputPath));
        return;
    }

    double sizeKbpsConversionFactor = -1;
    switch (ui->fileSizeUnitComboBox->currentIndex()) {
    case 0: // KB to kb
        sizeKbpsConversionFactor = 8;
        break;
    case 1: // MB to kb
        sizeKbpsConversionFactor = 8000;
        break;
    case 2: // GB to kb
        sizeKbpsConversionFactor = 8e+6;
        break;
    }

    bool hasVideo = ui->radVideoAudio->isChecked() || ui->radVideoOnly->isChecked();
    bool hasAudio = ui->radVideoAudio->isChecked() || ui->radAudioOnly->isChecked();
    QString outputPath = getOutputPath(inputPath);

    // get codecs
    optional<Codec> videoCodec;
    optional<Codec> audioCodec;
    optional<Container> container;

    videoCodec = hasVideo ? ui->videoCodecComboBox->currentData().value<Codec>() : optional<Codec>();
    audioCodec = hasAudio ? ui->audioCodecComboBox->currentData().value<Codec>() : optional<Codec>();
    container = containers.value(ui->containerComboBox->currentText());

    if (QFile::exists(outputPath + "." + container->name) && ui->warnOnOverwriteCheckBox->isChecked()
        && QMessageBox::question(this,
                                 "Overwrite?",
                                 "Output at path '" + outputPath + "' already exists. Overwrite it?")
            == QMessageBox::No) {
        notifier.Notify(Severity::Info, "Operation canceled", "Compression aborted.");
        return;
    }

    encoder.Encode(MediaEncoder::Options {
        .inputPath = inputPath,
        .outputPath = outputPath,
        .videoCodec = videoCodec,
        .audioCodec = audioCodec,
        .container = container,
        .sizeKbps = isAutoValue(ui->fileSizeSpinBox)
                      ? std::optional<double>()
                      : ui->fileSizeSpinBox->value() * sizeKbpsConversionFactor,
        .audioQualityPercent = ui->audioQualitySlider->value() / 100.0,
        .outputWidth = ui->widthSpinBox->value() == 0 ? std::optional<int>()
                                                      : ui->widthSpinBox->value(),
        .outputHeight = ui->heightSpinBox->value() == 0 ? std::optional<int>()
                                                        : ui->heightSpinBox->value(),
        .aspectRatio = ui->aspectRatioSpinBoxH->value() != 0 && ui->aspectRatioSpinBoxV->value() != 0
                         ? QPoint(ui->aspectRatioSpinBoxH->value(), ui->aspectRatioSpinBoxV->value())
                         : optional<QPoint>(),
        .fps = ui->fpsSpinBox->value() == 0 ? optional<int>() : ui->fpsSpinBox->value(),
        .speed = ui->speedSpinBox->value() == 0 ? optional<double>() : ui->speedSpinBox->value(),
        .customArguments = ui->customCommandTextEdit->toPlainText(),
        .inputMetadata = metadata.has_value() ? metadata : optional<Metadata>(),
        .minVideoBitrateKbps = settings->get("Main/dMinBitrateVideoKbps").toDouble(),
        .minAudioBitrateKbps = settings->get("Main/dMinBitrateAudioKbps").toDouble(),
        .maxAudioBitrateKbps = settings->get("Main/dMaxBitrateAudioKbps").toDouble(),
    });
}

void MainWindow::HandleStart(double videoBitrateKbps, double audioBitrateKbps)
{
    SetProgressShown(true);

    ui->progressBarLabel->setText(QString(tr("Video bitrate: %1 kbps | Audio bitrate: %2 kbps"))
                                      .arg(QString::number(qRound(videoBitrateKbps)),
                                           QString::number(qRound(audioBitrateKbps))));
}

void MainWindow::HandleSuccess(const MediaEncoder::Options& options,
                               const MediaEncoder::ComputedOptions& computed,
                               QFile& output)
{
    SetProgressShown(true, 100);

    QString summary;
    QString videoBitrate = computed.videoBitrateKbps.has_value()
        ? QString::number(*computed.videoBitrateKbps) + "kbps"
        : "auto-set bitrate";

    if (options.videoCodec.has_value()) {
        summary += tr("Using video codec %1 at %2 with container %3.\n")
                       .arg(options.videoCodec->name, videoBitrate, options.container->name);
    }
    if (options.audioCodec.has_value()) {
        summary += tr("Using audio codec %1 at %2kbps.\n")
                       .arg(options.audioCodec->name, QString::number(*computed.audioBitrateKbps));
    }
    if (options.sizeKbps.has_value()) {
        summary += tr("Requested size was %1 kb.\nActual "
                      "compression achieved is %2 kb.")
                       .arg(QString::number(*options.sizeKbps), QString::number(output.size() / 125.0));
    }

    notifier.Notify(Severity::Info, tr("Compressed successfully"), summary);

    SetProgressShown(false);

    QFileInfo fileInfo(output);
    QString command = platformInfo.isWindows() ? "explorer.exe" : "xdg-open";

    if (ui->deleteOnSuccessCheckBox->isChecked()) {
        QFile input(options.inputPath);
        input.open(QIODevice::WriteOnly);

        if (!(input.remove())) {
            notifier.Notify(Severity::Error, tr("Failed to remove input file"), output.errorString() + "\n\n" + options.inputPath);
        } else {
            ui->inputFileLineEdit->clear();
        }

        input.close();
    }

    if (ui->openExplorerOnSuccessCheckBox->isChecked()) {
        QProcess::execute(QString(R"(%1 "%2")").arg(command, fileInfo.dir().path()));
    }

    if (ui->playOnSuccessCheckBox->isChecked()) {
        QProcess::execute(QString(R"(%1 "%2")").arg(command, fileInfo.absoluteFilePath()));
    }

    if (ui->closeOnSuccessCheckBox->isChecked()) {
        QApplication::exit(0);
    }
}

void MainWindow::HandleFailure(const QString& shortError, const QString& longError)
{
    notifier.Notify(Severity::Warning, tr("Compression failed"), shortError, longError);
    SetProgressShown(false);
}

void MainWindow::CheckAspectRatioConflict()
{
    bool hasCustomScale = ui->aspectRatioSpinBoxH->value() != 0 || ui->aspectRatioSpinBoxV->value() != 0;
    bool hasCustomAspect = ui->widthSpinBox->value() != 0 || ui->heightSpinBox->value() != 0;

    if (hasCustomScale && hasCustomAspect) {
        warnings->Add("aspectRatioConflict", tr("A custom aspect ratio has been set. It will take priority over the "
                                                "one defined by custom scaling."));
    } else {
        warnings->Remove("aspectRatioConflict");
    }
}

void MainWindow::CheckSpeedConflict()
{
    bool hasSpeed = ui->speedSpinBox->value() != 0;
    bool hasFps = ui->fpsSpinBox->value() != 0;

    if (hasSpeed && hasFps) {
        warnings->Add("speedConflict", tr("A custom speed was set alongside a custom fps; this may cause "
                                          "strange behavior. Note that fps is automatically compensated when "
                                          "changing the speed."));
    } else {
        warnings->Remove("speedConflict");
    }
}

void MainWindow::SelectOutputDirectory()
{
    QDir dir = QFileDialog::getExistingDirectory(this, tr("Select output directory"), QDir::currentPath());
    ui->outputFolderLineEdit->setText(dir.absolutePath());
}

void MainWindow::UpdateAudioQualityLabel(int value)
{
    static double minBitrate = settings->get("Main/dMinBitrateAudioKbps").toDouble();
    static double maxBitrate = settings->get("Main/dMaxBitrateAudioKbps").toDouble();
    double currentValue = qMax(minBitrate, value / 100.0 * maxBitrate);

    ui->audioQualityDisplayLabel->setText(QString::number(qRound(currentValue)) + " kbps");
}

void MainWindow::SetAllowPresetSelection(bool allowed)
{
    ui->qualityPresetComboBox->setEnabled(!allowed);
}

void MainWindow::ShowAbout()
{
    static const QString msg = "\r\n<h4>Acknowledgements</h4>\r\n"

                               "A convenient graphical frontend for <i>ffmpeg</i>, the media encoding and decoding suite.<br /><br />\r\n"
                               "On Linux, uses system <a href=https://ffmpeg.org/ffprobe.html>ffprobe</a> and <a href=https://ffmpeg.org>ffmpeg</a>.<br />\r\n"
                               "On Windows, uses pre-compiled binaries of <i>ffprobe.exe</i> and <i>ffmpeg.exe</i> from <a href=https://github.com/GyanD/codexffmpeg/releases/>gyan.dev</a>.<br /><br />\r\nBuilt with <a href=https://www.qt.io>Qt</a>, the complete toolkit for cross-platform application development, under GPLv3 licensing. See About Qt.<br /><br />\r\n"
                               "For more information, visit our <a href=https://github.com/Thurinum/free-video-compressor>GitHub repository</a>.\r\n"

                               "<h4>Credits</h4>\r\n"
                               "Special thanks to Theodore L'Heureux for his helpful advice on refactoring and beta-testing of the product.\r\n";

    notifier.Notify(Severity::Info, "About " + QApplication::applicationName(), msg);
}

void MainWindow::SetProgressShown(bool shown, int progressPercent)
{
    auto* valueAnimation = new QPropertyAnimation(ui->progressBar, "value");
    valueAnimation->setDuration(settings->get("Main/iProgressBarAnimDurationMs").toInt());
    valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    auto* heightAnimation = new QPropertyAnimation(ui->progressWidget, "maximumHeight");
    valueAnimation->setDuration(settings->get("Main/iProgressWidgetAnimDurationMs").toInt());
    valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    if (shown && ui->progressWidget->maximumHeight() == 0) {
        ui->centralWidget->setEnabled(false);
        ui->startCompressionButton->setText(tr("Encoding..."));
        ui->progressWidgetTopSpacer->changeSize(0, 10);
        heightAnimation->setStartValue(0);
        heightAnimation->setEndValue(500);
        heightAnimation->start();
    }

    if (!shown) {
        ui->centralWidget->setEnabled(true);
        ui->startCompressionButton->setText(tr("Start encoding"));
        ui->progressWidgetTopSpacer->changeSize(0, 0);
        heightAnimation->setStartValue(ui->progressWidget->height());
        heightAnimation->setEndValue(0);
        heightAnimation->start();
    }

    valueAnimation->setStartValue(ui->progressBar->value());
    valueAnimation->setEndValue(progressPercent);
    valueAnimation->start();
}

void MainWindow::SetAdvancedMode(bool enabled)
{
    if (enabled) {
        sectionWidthAnim->setStartValue(0);
        sectionWidthAnim->setEndValue(1000);
        sectionHeightAnim->setStartValue(0);
        sectionHeightAnim->setEndValue(1000);
    } else {
        sectionWidthAnim->setStartValue(ui->advancedSection->width());
        sectionWidthAnim->setEndValue(0);
        sectionHeightAnim->setStartValue(ui->advancedSection->width());
        sectionHeightAnim->setEndValue(0);
    }

    sectionWidthAnim->start();
    sectionHeightAnim->start();
}

void MainWindow::SetControlsState(QAbstractButton* button)
{
    if (button == ui->radVideoAudio) {
        ui->widthSpinBox->setEnabled(true);
        ui->heightSpinBox->setEnabled(true);
        ui->speedSpinBox->setEnabled(true);
        ui->videoCodecComboBox->setEnabled(true);
        ui->audioCodecComboBox->setEnabled(true);
        ui->containerComboBox->setEnabled(true);
        ui->audioQualitySlider->setEnabled(true);
        ui->aspectRatioSpinBoxH->setEnabled(true);
        ui->aspectRatioSpinBoxV->setEnabled(true);
        ui->fpsSpinBox->setEnabled(true);
    } else if (button == ui->radVideoOnly) {
        ui->widthSpinBox->setEnabled(true);
        ui->heightSpinBox->setEnabled(true);
        ui->speedSpinBox->setEnabled(true);
        ui->videoCodecComboBox->setEnabled(true);
        ui->audioCodecComboBox->setEnabled(false);
        ui->containerComboBox->setEnabled(true);
        ui->audioQualitySlider->setEnabled(false);
        ui->aspectRatioSpinBoxH->setEnabled(true);
        ui->aspectRatioSpinBoxV->setEnabled(true);
        ui->fpsSpinBox->setEnabled(true);
    } else if (button == ui->radAudioOnly) {
        ui->widthSpinBox->setEnabled(false);
        ui->heightSpinBox->setEnabled(false);
        ui->speedSpinBox->setEnabled(false);
        ui->videoCodecComboBox->setEnabled(false);
        ui->audioCodecComboBox->setEnabled(true);
        ui->containerComboBox->setEnabled(false);
        ui->audioQualitySlider->setEnabled(true);
        ui->aspectRatioSpinBoxH->setEnabled(false);
        ui->aspectRatioSpinBoxV->setEnabled(false);
        ui->fpsSpinBox->setEnabled(false);
    }
}

void MainWindow::ShowMetadata()
{
    if (!metadata.has_value()) {
        notifier.Notify(Severity::Info, tr("No file selected"), tr("Please select a file to continue."));
        return;
    }

    notifier.Notify(Severity::Info, tr("Metadata"), tr("Video codec: %1\nAudio codec: %2\nContainer: %3\nAudio bitrate: %4 kbps\n\n "
                                                       "(complete data to be implemented in a future update)")
                                                        .arg(metadata->videoCodec, metadata->audioCodec, "N/A", QString::number(metadata->audioBitrateKbps)));
}

void MainWindow::SelectPresetCodecs()
{
    if (ui->codecSelectionGroupBox->isChecked())
        return;

    Preset p = qvariant_cast<Preset>(ui->qualityPresetComboBox->currentData());

    ui->videoCodecComboBox->setCurrentText(p.videoCodec.name);
    ui->audioCodecComboBox->setCurrentText(p.audioCodec.name);
    ui->containerComboBox->setCurrentText(p.container.name);
}

void MainWindow::OpenInputFile()
{
    QUrl fileUrl = QFileDialog::getOpenFileUrl(this, tr("Select file to compress"), QDir::currentPath(), "*");

    QString path = fileUrl.toLocalFile();
    ui->inputFileLineEdit->setText(path);

    ui->progressBar->setRange(0, 0);
    ui->progressBarLabel->setText(tr("Parsing metadata..."));
    SetProgressShown(true);

    ParseMetadata(path);

    if (ui->autoFillCheckBox->isChecked()) {
        ui->widthSpinBox->setValue(metadata->width);
        ui->heightSpinBox->setValue(metadata->height);
        ui->speedSpinBox->setValue(1);
        ui->fileSizeSpinBox->setValue(metadata->sizeKbps);
        ui->fileSizeUnitComboBox->setCurrentIndex(0);
        ui->aspectRatioSpinBoxH->setValue(metadata->aspectRatioX);
        ui->aspectRatioSpinBoxV->setValue(metadata->aspectRatioY);
        ui->fpsSpinBox->setValue(metadata->frameRate);

        int qualityPercent = metadata->audioBitrateKbps * 100 / 256;
        ui->audioQualitySlider->setValue(qualityPercent);
    }
}

void MainWindow::ParseCodecs(QHash<QString, Codec>* codecs, const QString& type, QComboBox* comboBox)
{
    QStringList keys = settings->keysInGroup(type);

    if (keys.empty()) {
        notifier.Notify(Severity::Critical, tr("Missing codecs definition").arg(type), tr("Could not find codecs list of type '%1' in the configuration file. "
                                                                                          "Please validate the configuration in %2. Exiting.")
                                                                                           .arg(type));
    }

    QString availableCodecs = encoder.getAvailableFormats();

    for (const QString& codecLibrary : keys) {
        if (!availableCodecs.contains(codecLibrary)) {
            notifier.Notify(Severity::Warning, tr("Unsupported codec"), tr("Codec library '%1' does not appear to be supported by our version of "
                                                                           "FFMPEG. It has been skipped. Please validate the "
                                                                           "configuration in %2.")
                                                                            .arg(codecLibrary, settings->fileName()));
            continue;
        }

        if (codecLibrary.contains("nvenc") && !platformInfo.isNvidia())
            continue;

        static QRegularExpression expression("[^-_a-z0-9]");
        if (expression.match(codecLibrary).hasMatch()) {
            notifier.Notify(Severity::Critical, tr("Could not parse codec"), tr("Codec library '%1' could not be parsed. Please validate the "
                                                                                "configuration in %2. Exiting.")
                                                                                 .arg(codecLibrary, settings->fileName()));
        }

        static QRegularExpression splitExpression("(\\s*),(\\s*)");
        QStringList values = settings->get(type + "/" + codecLibrary)
                                 .toString()
                                 .split(splitExpression);

        QString codecName = values.first();

        bool isConversionOk = false;
        double codecMinBitrateKbps = values.size() == 2 ? values.last().toDouble(&isConversionOk) : 0;

        if (values.size() == 2 && !isConversionOk) {
            notifier.Notify(Severity::Critical, tr("Invalid codec bitrate"), tr("Minimum bitrate of codec '%1' could not be parsed. Please "
                                                                                "validate the configuration in %2. Exiting.")
                                                                                 .arg(values.last(), settings->fileName()));
        }

        Codec codec { codecName, codecLibrary, codecMinBitrateKbps };
        codecs->insert(codec.library, codec);

        QVariant data;
        data.setValue(codec);

        comboBox->addItem(codecName, data);
    }
}

void MainWindow::ParseContainers(QHash<QString, Container>* containers, QComboBox* comboBox)
{
    QStringList keys = settings->keysInGroup("Containers");

    if (keys.empty()) {
        notifier.Notify(Severity::Critical, tr("Missing containers definition"), tr("Could not find list of container in the configuration file. Please "
                                                                                    "validate the configuration in %1. Exiting.")
                                                                                     .arg(settings->fileName()));
    }

    for (const QString& containerName : keys) {
        static QRegularExpression expression("[^-_a-z0-9]");
        if (expression.match(containerName).hasMatch()) {
            notifier.Notify(Severity::Critical, tr("Invalid container name"), tr("Name of container '%1' could not be parsed. Please "
                                                                                 "validate the configuration in %2. Exiting.")
                                                                                  .arg(containerName, settings->fileName()));
        }

        static QRegularExpression splitExpression("(\\s*),(\\s*)");
        QStringList supportedCodecs = settings->get("Containers/" + containerName).toString().split(splitExpression);
        Container container { containerName, supportedCodecs };
        containers->insert(containerName, container);

        QVariant data;
        data.setValue(container);

        comboBox->addItem(containerName, data);
    }
}

void MainWindow::ParsePresets(QHash<QString, Preset>& presets,
                              QHash<QString, Codec>& videoCodecs,
                              QHash<QString, Codec>& audioCodecs,
                              QHash<QString, Container>& containers,
                              QComboBox* comboBox)
{
    QStringList keys = settings->keysInGroup("Presets");
    QString supportedCodecs = encoder.getAvailableFormats();

    for (const QString& key : keys) {
        Preset preset;
        preset.name = key;

        QString data = settings->get("Presets/" + key).toString();
        QStringList librariesData = data.split("+");

        if (librariesData.size() != 3) {
            notifier.Notify(Severity::Error, tr("Could not parse presets"), tr("Failed to parse presets as the data is malformed. Please check the "
                                                                               "configuration in %1.")
                                                                                .arg(settings->fileName()));
        }

        optional<Codec> videoCodec;
        optional<Codec> audioCodec;
        QStringList videoLibrary = librariesData[0].split("|");

        for (const QString& library : videoLibrary) {
            if (supportedCodecs.contains(library)) {
                videoCodec = videoCodecs.value(library);
                break;
            }
        }

        QStringList audioLibrary = librariesData[1].split("|");

        for (const QString& library : audioLibrary) {
            if (supportedCodecs.contains(library)) {
                audioCodec = audioCodecs.value(library);
                break;
            }
        }

        if (!videoCodec.has_value() || !audioCodec.has_value()) {
            notifier.Notify(Severity::Warning, tr("Preset not supported"), tr("Skipped encoding quality preset '%1' preset because it contains no "
                                                                              "available encoders.")
                                                                               .arg(preset.name),
                            tr("Available codecs: %1\nPreset data: %2").arg(supportedCodecs, data));
            continue;
        }

        preset.videoCodec = *videoCodec;
        preset.audioCodec = *audioCodec;
        preset.container = containers.value(librariesData[2]);

        presets.insert(preset.name, preset);

        QVariant itemData;
        itemData.setValue(preset);
        comboBox->addItem(preset.name, itemData);
    }
}

void MainWindow::ParseMetadata(const QString& path)
{
    std::variant<Metadata, Metadata::Error> result = encoder.getMetadata(path);

    if (std::holds_alternative<Metadata::Error>(result)) {
        Metadata::Error error = std::get<Metadata::Error>(result);

        notifier.Notify(Severity::Error, tr("Failed to parse media metadata"), error.summary, error.details);
        ui->inputFileLineEdit->clear();
        return;
    }

    metadata = std::get<Metadata>(result);
}

QString MainWindow::getOutputPath(QString inputFilePath)
{
    QString folder = ui->outputFolderLineEdit->text();
    bool hasSuffix = ui->outputFileNameSuffixCheckBox->isChecked();
    QString fileNameOrSuffix = ui->outputFileNameLineEdit->text();
    QFileInfo inputFile = QFileInfo(inputFilePath);

    QDir resolvedFolder = folder.isEmpty() ? inputFile.dir() : QDir(folder);
    QString resolvedFileName;

    if (fileNameOrSuffix.isEmpty()) {
        resolvedFileName = inputFile.baseName();
    } else if (hasSuffix) {
        if (!fileNameOrSuffix[0].isLetterOrNumber())
            fileNameOrSuffix.remove(0, 1);

        resolvedFileName = inputFile.baseName() + "_" + fileNameOrSuffix;
    } else {
        resolvedFileName = fileNameOrSuffix;
    }

    return resolvedFolder.filePath(resolvedFileName);
}

bool MainWindow::isAutoValue(QAbstractSpinBox* spinBox)
{
    return spinBox->text() == spinBox->specialValueText();
}

void MainWindow::ValidateSelectedUrl()
{
    const QString selectedUrl = ui->inputFileLineEdit->text();
    bool isValidInput = QFile::exists(selectedUrl);
    if (isValidInput)
        ParseMetadata(selectedUrl);
    else
        ui->inputFileLineEdit->clear();
}

void MainWindow::ValidateSelectedDir()
{
    const QString selectedDir = ui->outputFolderLineEdit->text();
    bool isValidOutput = QDir(selectedDir).exists();
    if (!isValidOutput)
        ui->outputFolderLineEdit->clear();

    ui->warningTooltipButton->setVisible(false);
}

void MainWindow::SetupAdvancedModeAnimation()
{
    const int duration = settings->get("Main/iSectionAnimDurationMs").toInt();

    sectionWidthAnim = new QPropertyAnimation(ui->advancedSection, "maximumWidth", this);
    sectionWidthAnim->setDuration(duration);
    sectionWidthAnim->setEasingCurve(QEasingCurve::InOutQuad);

    sectionHeightAnim = new QPropertyAnimation(ui->advancedSection, "maximumHeight", this);
    sectionHeightAnim->setDuration(duration);
    sectionHeightAnim->setEasingCurve(QEasingCurve::InOutQuad);

    windowSizeAnim = new QPropertyAnimation(this, "size");
    windowSizeAnim->setDuration(duration);
    windowSizeAnim->setEasingCurve(QEasingCurve::InOutQuad);

    connect(sectionWidthAnim, &QPropertyAnimation::finished, windowSizeAnim, [this]() {
        windowSizeAnim->setStartValue(this->size());
        windowSizeAnim->setEndValue(this->minimumSizeHint());
        windowSizeAnim->start();
    });
}
