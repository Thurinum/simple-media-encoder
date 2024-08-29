#include <QClipboard>
#include <QFileDialog>
#include <QMenu>
#include <QMimeData>
#include <QMimeDatabase>
#include <QMovie>
#include <QThread>
#include <QTimer>
#include <QWhatsThis>

#include "encoder/encoder_options_builder.hpp"
#include "mainwindow.hpp"
#include "notifier/notifier.hpp"
#include "settings/serializer.hpp"
#include "ui_mainwindow.h"

using std::optional;

MainWindow::MainWindow(
    MediaEncoder& encoder,
    std ::shared_ptr<Settings> settings,
    std::shared_ptr<Settings> presetsSettings,
    std::shared_ptr<Serializer> serializer,
    MetadataLoader& metadata,
    Notifier& notifier,
    PlatformInfo& platformInfo,
    FormatSupportLoader& formatSupportLoader
)
    : ui(new Ui::MainWindow)
    , encoder(encoder)
    , settings(std::move(settings))
    , presetsSettings(std::move(presetsSettings))
    , serializer(std::move(serializer))
    , metadataLoader(metadata)
    , notifier(notifier)
    , platformInfo(platformInfo)
    , formatSupport(formatSupportLoader)
{
    CheckForFFmpeg();

    ui->setupUi(this);
    this->resize(this->QWidget::minimumSizeHint());
    this->warnings = new Warnings(ui->warningTooltipButton);

    overlay->setGeometry(this->rect());
    overlay->hide();
    overlay->raise();

    ui->mainHeading->setText(QApplication::applicationName());

    ui->audioVideoButtonGroup->setId(ui->radVideoAudio, 0);
    ui->audioVideoButtonGroup->setId(ui->radVideoOnly, 1);
    ui->audioVideoButtonGroup->setId(ui->radAudioOnly, 2);

    preferenceWidgets = std::make_unique<const QList<QObject*>>(QList<QObject*> {
        ui->advancedModeCheckBox,
        ui->audioVideoButtonGroup,
        ui->outputFileNameLineEdit,
        ui->outputFolderLineEdit,
        ui->autoFillCheckBox,
        ui->closeOnSuccessCheckBox,
        ui->commonFormatsOnlyCheckbox,
        ui->deleteOnSuccessCheckBox,
        ui->inputFileLineEdit,
        ui->openExplorerOnSuccessCheckBox,
        ui->outputFileNameSuffixCheckBox,
        ui->playOnSuccessCheckBox,
        ui->warnOnOverwriteCheckBox,
    });

    presetWidgets = std::make_unique<const QList<QObject*>>(QList<QObject*> {
        ui->aspectRatioSpinBoxH,
        ui->aspectRatioSpinBoxV,
        ui->audioCodecComboBox,
        ui->audioQualitySlider,
        ui->containerComboBox,
        ui->customCommandTextEdit,
        ui->fileSizeSpinBox,
        ui->fileSizeUnitComboBox,
        ui->fpsSpinBox,
        ui->heightSpinBox,
        ui->speedSpinBox,
        ui->videoCodecComboBox,
        ui->widthSpinBox,
        ui->audioChannelCountSpinbox,
    });

    videoControls = std::make_unique<const QList<QWidget*>>(QList<QWidget*> {
        ui->videoCodecComboBox,
        ui->widthSpinBox,
        ui->heightSpinBox,
        ui->aspectRatioSpinBoxH,
        ui->aspectRatioSpinBoxV,
        ui->fpsSpinBox,
        ui->speedSpinBox,
    });

    audioControls = std::make_unique<const QList<QWidget*>>(QList<QWidget*> {
        ui->audioCodecComboBox,
        ui->audioQualitySlider,
    });

    SetupAdvancedModeAnimation();
    SetupMenu();
    SetupEventCallbacks();

    QuerySupportedFormatsAsync();
}

MainWindow::~MainWindow()
{
    delete ui;
    delete warnings;
}

void MainWindow::CheckForFFmpeg() const
{
    const QString ffmpeg = platformInfo.isWindows() ? "ffmpeg.exe" : "ffmpeg";
    const QString ffprobe = platformInfo.isWindows() ? "ffprobe.exe" : "ffprobe";

    const int ffmpegReturnCode = QProcess::execute(ffmpeg, QStringList() << "-version");
    const int ffprobeReturnCode = QProcess::execute(ffprobe, QStringList() << "-version");

    if (ffmpegReturnCode == 0 && ffprobeReturnCode == 0)
        return;

    notifier.Notify(
        Critical, tr("Could not locate FFmpeg"),
        tr("No valid install of FFmpeg was located. Please make sure FFmpeg and FFprobe are in your PATH, or directly "
           "in the application directory.")
    );
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

void MainWindow::SetupEventCallbacks()
{
    connect(&formatSupport, &FormatSupportLoader::queryCompleted, this, &MainWindow::HandleFormatsQueryResult);

    connect(&encoder, &MediaEncoder::encodingStarted, this, &MainWindow::HandleStart);
    connect(&encoder, &MediaEncoder::encodingSucceeded, this, &MainWindow::HandleSuccess);
    connect(&encoder, &MediaEncoder::encodingFailed, this, &MainWindow::HandleFailure);
    connect(&encoder, &MediaEncoder::encodingProgressUpdate, this, [this](int progress)
            { SetProgressShown({ .status = tr("Compressing..."), .progressPercent = progress }); });
}

void MainWindow::QuerySupportedFormatsAsync()
{
    SetProgressShown({ .status = tr("Querying supported formats...") });
    formatSupport.QuerySupportedFormatsAsync();
}

void MainWindow::LoadState()
{
    const QString key = "Preferences";

    serializer->deserialize(ui->advancedModeCheckBox, settings, key);
    SetAdvancedMode(ui->advancedModeCheckBox->isChecked());

    serializer->deserialize(ui->audioVideoButtonGroup, settings, key);
    SetControlsState(ui->audioVideoButtonGroup->checkedButton());

    serializer->deserialize(ui->outputFileNameLineEdit, settings, key);
    LoadSelectedUrl();

    serializer->deserialize(ui->outputFolderLineEdit, settings, key);
    ValidateSelectedDir();

    serializer->deserialize(ui->commonFormatsOnlyCheckbox, settings, "Preferences");
    UpdateCodecsList(ui->commonFormatsOnlyCheckbox->isChecked());

    LoadPresetNames();

    const QList<QObject*> widgets = {
        ui->autoFillCheckBox,
        ui->closeOnSuccessCheckBox,
        ui->deleteOnSuccessCheckBox,
        ui->inputFileLineEdit,
        ui->openExplorerOnSuccessCheckBox,
        ui->outputFileNameSuffixCheckBox,
        ui->playOnSuccessCheckBox,
        ui->warnOnOverwriteCheckBox,
    };

    serializer->deserializeMany(widgets, settings, key);
    serializer->deserializeMany(*presetWidgets, settings, "PreviousSettings");
}

void MainWindow::LoadPresetNames() const
{
    ui->qualityPresetComboBox->addItems(presetsSettings->groups());
}

void MainWindow::SaveState() const
{
    serializer->serializeMany(*preferenceWidgets, settings, "Preferences");
    serializer->serializeMany(*presetWidgets, settings, "PreviousSettings");
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (!event->mimeData()->hasUrls())
    {
        event->ignore();
        return;
    }

    const QMimeDatabase database;
    const QUrl url = event->mimeData()->urls().first();
    const QMimeType mimeType = database.mimeTypeForFile(url.toLocalFile());

    isValidMimeForDrop = mimeType.name().startsWith("image/")
                      || mimeType.name().startsWith("video/")
                      || mimeType.name().startsWith("audio/");
    if (isValidMimeForDrop)
    {
        overlay->setBackgroundColor(QColor(0, 0, 0, 128));
        overlay->setText("Drop to select file");
    }
    else
    {
        overlay->setBackgroundColor(QColor(128, 0, 0, 128));
        overlay->setText("Invalid media file");
    }

    if (!isDragging)
    {
        overlay->showWithFade();
        isDragging = true;
    }

    event->accept();
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (!event->mimeData()->hasUrls())
        return;

    if (isValidMimeForDrop)
    {
        const QUrl url = event->mimeData()->urls().first();
        LoadInputFile(url);
    }

    overlay->hideWithFade();
    isDragging = false;
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event)
{
    overlay->hideWithFade();
    isDragging = false;
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    overlay->setGeometry(this->rect());
    QMainWindow::resizeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    SaveState();
    event->accept();
}

void MainWindow::StartEncoding()
{
    EncoderOptionsBuilder builder;

    const QString inputPath = ui->inputFileLineEdit->text();
    const QString outputPath = getOutputPath(inputPath);

    // FIXME: emit fileExists() signal
    // if (QFile::exists(outputPath + "." + container->formatName) && ui->warnOnOverwriteCheckBox->isChecked()
    //     && QMessageBox::question(this, "Overwrite?", "Output at path '" + outputPath + "' already exists. Overwrite
    //     it?")
    //            == QMessageBox::No) {
    //     notifier.Notify(Severity::Info, "Operation canceled", "Compression aborted.");
    //     return;
    // }

    if (metadata.has_value())
        builder.useMetadata(*metadata);

    const auto streamType = static_cast<StreamType>(ui->audioVideoButtonGroup->checkedId());
    const bool hasVideo = streamType == VideoAudio || streamType == VideoOnly;
    const bool hasAudio = streamType == VideoAudio || streamType == AudioOnly;

    if (hasVideo)
        builder.withVideoCodec(ui->videoCodecComboBox->currentData().value<Codec>());

    if (hasAudio)
        builder.withAudioCodec(ui->audioCodecComboBox->currentData().value<Codec>());

    builder.inputFrom(inputPath)
        .outputTo(outputPath)
        .withContainer(ui->containerComboBox->currentData().value<Container>())
        .withTargetOutputSize(getOutputSizeKbps())
        .withAudioQuality(ui->audioQualitySlider->value() / 100.0)
        .withAudioChannelsCount(ui->audioChannelCountSpinbox->value())
        .withOutputWidth(ui->widthSpinBox->value())
        .withOutputHeight(ui->heightSpinBox->value())
        .withAspectRatio(QPoint(ui->aspectRatioSpinBoxH->value(), ui->aspectRatioSpinBoxV->value()))
        .atFps(ui->fpsSpinBox->value())
        .atSpeed(ui->speedSpinBox->value())
        .withCustomArguments(ui->customCommandTextEdit->toPlainText())
        .withMinVideoBitrate(settings->get("Main/dMinBitrateVideoKbps").toDouble())
        .withMinAudioBitrate(settings->get("Main/dMinBitrateAudioKbps").toDouble())
        .withMaxAudioBitrate(settings->get("Main/dMaxBitrateAudioKbps").toDouble());

    const auto maybeOptions = builder.build();
    if (std::holds_alternative<QList<QString>>(maybeOptions))
    {
        notifier.Notify(Severity::Error, "Invalid encoding options", std::get<QList<QString>>(maybeOptions).join("\n"));
        return;
    }

    const EncoderOptions options = std::get<EncoderOptions>(maybeOptions);
    encoder.Encode(options);
}

void MainWindow::HandleStart(double videoBitrateKbps, double audioBitrateKbps)
{
    SetProgressShown({ .status = tr("Compressing..."), .progressPercent = 0 });

    ui->progressBarLabel->setText(
        QString(tr("Video bitrate: %1 kbps | Audio bitrate: %2 kbps"))
            .arg(QString::number(qRound(videoBitrateKbps)), QString::number(qRound(audioBitrateKbps)))
    );
}

void MainWindow::HandleSuccess(
    const EncoderOptions& options, const MediaEncoder::ComputedOptions& computed, QFile& output
)
{
    SetProgressShown({ .status = tr("Compression complete"), .progressPercent = 100 });

    QString summary;
    QString videoBitrate = computed.videoBitrateKbps.has_value() ? QString::number(*computed.videoBitrateKbps) + "kbps"
                                                                 : "auto-set bitrate";

    if (options.videoCodec.has_value())
    {
        summary += tr("Using video codec %1 at %2 with container %3.\n")
                       .arg(options.videoCodec->displayName, videoBitrate, options.container.displayName);
    }
    if (options.audioCodec.has_value())
    {
        summary += tr("Using audio codec %1 at %2kbps.\n")
                       .arg(options.audioCodec->displayName, QString::number(*computed.audioBitrateKbps));
    }
    if (options.sizeKbps.has_value())
    {
        summary += tr("Requested size was %1 kb.\nActual "
                      "compression achieved is %2 kb.")
                       .arg(QString::number(*options.sizeKbps), QString::number(output.size() / 125.0));
    }

    notifier.Notify(Severity::Info, tr("Compressed successfully"), summary);

    SetProgressShown({});

    QFileInfo fileInfo(output);
    QString command = platformInfo.isWindows() ? "explorer.exe" : "xdg-open";

    if (ui->deleteOnSuccessCheckBox->isChecked())
    {
        QFile input(options.inputPath);
        input.open(QIODevice::WriteOnly);

        if (!(input.remove()))
        {
            notifier.Notify(
                Severity::Error, tr("Failed to remove input file"), output.errorString() + "\n\n" + options.inputPath
            );
        }
        else
        {
            ui->inputFileLineEdit->clear();
        }

        input.close();
    }

    if (ui->openExplorerOnSuccessCheckBox->isChecked())
    {
        QProcess::execute(QString(R"(%1 "%2")").arg(command, fileInfo.dir().path()));
    }

    if (ui->playOnSuccessCheckBox->isChecked())
    {
        QProcess::execute(QString(R"(%1 "%2")").arg(command, fileInfo.absoluteFilePath()));
    }

    if (ui->closeOnSuccessCheckBox->isChecked())
    {
        QApplication::exit(0);
    }
}

void MainWindow::HandleFailure(const QString& shortError, const QString& longError)
{
    notifier.Notify(Severity::Warning, tr("Compression failed"), shortError, longError);
    SetProgressShown({});
}

void MainWindow::CheckAspectRatioConflict()
{
    bool hasCustomScale = ui->aspectRatioSpinBoxH->value() != 0 || ui->aspectRatioSpinBoxV->value() != 0;
    bool hasCustomAspect = ui->widthSpinBox->value() != 0 || ui->heightSpinBox->value() != 0;

    if (hasCustomScale && hasCustomAspect)
    {
        warnings->Add(
            "aspectRatioConflict", tr("A custom aspect ratio has been set. It will take priority over the "
                                      "one defined by custom scaling.")
        );
    }
    else
    {
        warnings->Remove("aspectRatioConflict");
    }
}

void MainWindow::CheckSpeedConflict()
{
    bool hasSpeed = ui->speedSpinBox->value() != 0;
    bool hasFps = ui->fpsSpinBox->value() != 0;

    if (hasSpeed && hasFps)
    {
        warnings->Add(
            "speedConflict", tr("A custom speed was set alongside a custom fps; this may cause "
                                "strange behavior. Note that fps is automatically compensated when "
                                "changing the speed.")
        );
    }
    else
    {
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

void MainWindow::SetAllowPresetSelection(bool allowed) { ui->qualityPresetComboBox->setEnabled(!allowed); }

void MainWindow::HandleFormatsQueryResult(const std::variant<QSharedPointer<FormatSupport>, Message>& maybeFormats)
{
    if (std::holds_alternative<Message>(maybeFormats))
    {
        notifier.Notify(std::get<Message>(maybeFormats));
        return;
    }

    const auto formats = std::get<QSharedPointer<FormatSupport>>(maybeFormats);
    formatSupportCache = formats;
    SetProgressShown({});

    LoadState();
    LoadSelectedUrl();
}

void MainWindow::UpdateCodecsList(const bool commonOnly) const
{
    static QStringList commonVideoCodecs = settings->get("FormatSelection/sCommonVideoCodecs").toStringList();
    static QStringList commonAudioCodecs = settings->get("FormatSelection/sCommonAudioCodecs").toStringList();
    static QStringList commonContainers = settings->get("FormatSelection/sCommonContainers").toStringList();

    ui->videoCodecComboBox->clear();
    ui->audioCodecComboBox->clear();
    ui->containerComboBox->clear();

    // passing “copy” as codec name will make ffmpeg use the same codec as the input file
    static Codec passthroughCodec = { .displayName = "Passthrough", .libraryName = "copy", .isAudioCodec = false };
    ui->videoCodecComboBox->addItem(passthroughCodec.displayName);
    ui->videoCodecComboBox->setItemData(0, QVariant::fromValue(passthroughCodec));

    for (const Codec& videoCodec : formatSupportCache->videoCodecs)
    {
        QString name = videoCodec.libraryName;
        if (commonOnly && !commonVideoCodecs.contains(name))
            continue;

        ui->videoCodecComboBox->addItem(name);
        ui->videoCodecComboBox->setItemData(ui->videoCodecComboBox->count() - 1, QVariant::fromValue(videoCodec));
        ui->videoCodecComboBox->setItemData(ui->videoCodecComboBox->count() - 1, videoCodec.displayName, Qt::ToolTipRole);
    }

    static Codec passthroughAudioCodec = { .displayName = "Passthrough", .libraryName = "copy", .isAudioCodec = true };
    ui->audioCodecComboBox->addItem(passthroughAudioCodec.displayName);
    ui->audioCodecComboBox->setItemData(0, QVariant::fromValue(passthroughAudioCodec));
    for (const Codec& audioCodec : formatSupportCache->audioCodecs)
    {
        QString name = audioCodec.libraryName;
        if (commonOnly && !commonAudioCodecs.contains(name))
            continue;

        ui->audioCodecComboBox->addItem(name);
        ui->audioCodecComboBox->setItemData(ui->audioCodecComboBox->count() - 1, QVariant::fromValue(audioCodec));
        ui->audioCodecComboBox->setItemData(ui->audioCodecComboBox->count() - 1, audioCodec.displayName, Qt::ToolTipRole);
    }

    for (const Container& container : formatSupportCache->containers)
    {
        QString name = container.formatName;
        if (commonOnly && !commonContainers.contains(name))
            continue;

        ui->containerComboBox->addItem(name);
        ui->containerComboBox->setItemData(ui->containerComboBox->count() - 1, QVariant::fromValue(container));
        ui->containerComboBox->setItemData(ui->containerComboBox->count() - 1, container.displayName, Qt::ToolTipRole);
    }
}

void MainWindow::ShowAbout() const
{
    static const QString msg
        = "\r\n<h4>Acknowledgements</h4>\r\n"

          "A convenient graphical frontend for <i>ffmpeg</i>, the media encoding and decoding suite.<br /><br />\r\n"
          "On Linux, uses system <a href=https://ffmpeg.org/ffprobe.html>ffprobe</a> and <a "
          "href=https://ffmpeg.org>ffmpeg</a>.<br />\r\n"
          "On Windows, uses pre-compiled binaries of <i>ffprobe.exe</i> and <i>ffmpeg.exe</i> from <a "
          "href=https://github.com/GyanD/codexffmpeg/releases/>gyan.dev</a>.<br /><br />\r\nBuilt with <a "
          "href=https://www.qt.io>Qt</a>, the complete toolkit for cross-platform application development, under GPLv3 "
          "licensing. See About Qt.<br /><br />\r\n"
          "For more information, visit our <a href=https://github.com/Thurinum/free-video-compressor>GitHub "
          "repository</a>.\r\n"

          "<h4>Credits</h4>\r\n"
          "Special thanks to Theodore L'Heureux for his helpful advice on refactoring and beta-testing of the "
          "product.\r\n";

    notifier.Notify(Severity::Info, "About " + QApplication::applicationName(), msg);
}

void MainWindow::SetProgressShown(ProgressState state)
{
    auto* valueAnimation = new QPropertyAnimation(ui->progressBar, "value");
    valueAnimation->setDuration(settings->get("Main/iProgressBarAnimDurationMs").toInt());
    valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    auto* heightAnimation = new QPropertyAnimation(ui->progressWidget, "maximumHeight");
    valueAnimation->setDuration(settings->get("Main/iProgressWidgetAnimDurationMs").toInt());
    valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

    if (state.status.has_value() && ui->progressWidget->maximumHeight() == 0)
    {
        ui->centralWidget->setEnabled(false);

        QString taskName = *state.status;
        if (ui->startCompressionButton->text() != taskName)
            ui->startCompressionButton->setText(taskName);

        ui->progressWidgetTopSpacer->changeSize(0, 10);
        heightAnimation->setStartValue(0);
        heightAnimation->setEndValue(500);
    }
    else if (!state.status)
    {
        ui->centralWidget->setEnabled(true);
        ui->startCompressionButton->setText(tr("Start encoding"));
        ui->progressWidgetTopSpacer->changeSize(0, 0);
        heightAnimation->setStartValue(ui->progressWidget->height());
        heightAnimation->setEndValue(0);
    }

    heightAnimation->start();

    if (!state.progressPercent.has_value())
    {
        ui->progressBar->setRange(0, 0);
        return;
    }

    ui->progressBar->setRange(0, 100);
    valueAnimation->setStartValue(ui->progressBar->value());
    valueAnimation->setEndValue(*state.progressPercent);
    valueAnimation->start();
}

void MainWindow::SetAdvancedMode(bool enabled)
{
    if (enabled)
    {
        sectionWidthAnim->setStartValue(0);
        sectionWidthAnim->setEndValue(1000);
        sectionHeightAnim->setStartValue(0);
        sectionHeightAnim->setEndValue(1000);
    }
    else
    {
        sectionWidthAnim->setStartValue(ui->advancedSection->width());
        sectionWidthAnim->setEndValue(0);
        sectionHeightAnim->setStartValue(ui->advancedSection->width());
        sectionHeightAnim->setEndValue(0);
    }

    sectionWidthAnim->start();
    sectionHeightAnim->start();
}

void MainWindow::SetControlsState([[maybe_unused]] QAbstractButton* button) const
{
    SetControlsState();
}

void MainWindow::SetControlsState() const
{
    const auto state = static_cast<StreamType>(ui->audioVideoButtonGroup->checkedId());
    const bool isVideoAudio = state == VideoAudio;
    const bool isVideoOnly = state == VideoOnly;
    const bool isAudioOnly = state == AudioOnly;

    const bool isVideoPassthrough = ui->videoCodecComboBox->currentIndex() == 0;
    const bool isAudioPassthrough = ui->audioCodecComboBox->currentIndex() == 0;

    for (QWidget* control : *videoControls)
        control->setEnabled((isVideoAudio || isVideoOnly) && (!isVideoPassthrough || control == ui->videoCodecComboBox));

    for (QWidget* control : *audioControls)
        control->setEnabled((isVideoAudio || isAudioOnly) && (!isAudioPassthrough || control == ui->audioCodecComboBox));
}

void MainWindow::ShowMetadata()
{
    if (!metadata.has_value())
    {
        notifier.Notify(Severity::Info, tr("No file selected"), tr("Please select a file to continue."));
        return;
    }

    notifier.Notify(
        Severity::Info, tr("Metadata"),
        tr("Video codec: %1\nAudio codec: %2\nContainer: %3\nAudio bitrate: %4 kbps\n\n "
           "(complete data to be implemented in a future update)")
            .arg(metadata->videoCodec, metadata->audioCodec, "N/A", QString::number(metadata->audioBitrateKbps))
    );
}

void MainWindow::LoadPreset(const int index) const
{
    if (index < 0)
        return;

    const QString presetName = ui->qualityPresetComboBox->itemText(index);
    if (!presetsSettings->groups().contains(presetName))
        return;

    serializer->deserializeMany(*presetWidgets, presetsSettings, presetName);
}

void MainWindow::SelectVideoCodec(const int index) const
{
    SetControlsState(ui->audioVideoButtonGroup->checkedButton());
}

void MainWindow::SelectAudioCodec(const int index) const
{
    const bool isPassthrough = index == 0;

    for (QWidget* control : *audioControls)
    {
        if (control == ui->audioCodecComboBox)
            continue;

        control->setEnabled(!isPassthrough);
    }
}

void MainWindow::OpenInputFile()
{
    const QUrl fileUrl = QFileDialog::getOpenFileUrl(this, tr("Select file to compress"), QDir::currentPath(), "*");
    LoadInputFile(fileUrl);
}

void MainWindow::QueryMediaMetadataAsync(const QString& path)
{
    SetProgressShown({ .status = tr("Parsing metadata...") });

    connect(
        &metadataLoader, &MetadataLoader::loadAsyncComplete, this, &MainWindow::ReceiveMediaMetadata,
        Qt::UniqueConnection
    );

    metadataLoader.loadAsync(path);
}

void MainWindow::ReceiveMediaMetadata(MetadataResult result)
{
    SetProgressShown({});

    if (std::holds_alternative<Message>(result))
    {
        Message error = std::get<Message>(result);

        notifier.Notify(error);
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

    if (fileNameOrSuffix.isEmpty())
    {
        resolvedFileName = inputFile.baseName();
    }
    else if (hasSuffix)
    {
        if (!fileNameOrSuffix[0].isLetterOrNumber())
            fileNameOrSuffix.remove(0, 1);

        resolvedFileName = inputFile.baseName() + "_" + fileNameOrSuffix;
    }
    else
    {
        resolvedFileName = fileNameOrSuffix;
    }

    return resolvedFolder.filePath(resolvedFileName);
}

bool MainWindow::isAutoValue(QAbstractSpinBox* spinBox) { return spinBox->text() == spinBox->specialValueText(); }

void MainWindow::LoadSelectedUrl()
{
    const QString selectedUrl = ui->inputFileLineEdit->text();
    bool isValidInput = QFile::exists(selectedUrl);
    if (isValidInput)
        QueryMediaMetadataAsync(selectedUrl);
    else
        ui->inputFileLineEdit->clear();
}
void MainWindow::LoadInputFile(const QUrl& url)
{
    const QString path = url.toLocalFile();
    ui->inputFileLineEdit->setText(path);

    QueryMediaMetadataAsync(path);

    if (ui->autoFillCheckBox->isChecked())
    {
        ui->widthSpinBox->setValue(metadata->width);
        ui->heightSpinBox->setValue(metadata->height);
        ui->speedSpinBox->setValue(1);
        ui->fileSizeSpinBox->setValue(metadata->sizeKbps);
        ui->fileSizeUnitComboBox->setCurrentIndex(0);
        ui->aspectRatioSpinBoxH->setValue(metadata->aspectRatioX);
        ui->aspectRatioSpinBoxV->setValue(metadata->aspectRatioY);
        ui->fpsSpinBox->setValue(metadata->frameRate);

        const int qualityPercent = metadata->audioBitrateKbps * 100 / 256;
        ui->audioQualitySlider->setValue(qualityPercent);
    }
}

void MainWindow::ValidateSelectedDir() const
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

    connect(sectionWidthAnim, &QPropertyAnimation::finished, windowSizeAnim, [this]()
            {
        windowSizeAnim->setStartValue(this->size());
        windowSizeAnim->setEndValue(this->minimumSizeHint());
        windowSizeAnim->start(); });
}

double MainWindow::getOutputSizeKbps()
{
    double sizeKbpsConversionFactor = 0;

    switch (ui->fileSizeUnitComboBox->currentIndex())
    {
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

    return ui->fileSizeSpinBox->value() * sizeKbpsConversionFactor;
}
