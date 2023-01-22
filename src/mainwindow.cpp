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
#include <QTimer>
#include <QWhatsThis>

#include "mainwindow.hpp"
#include "ui_mainwindow.h"

using std::optional;
using Error = Compressor::Error;
using Metadata = Compressor::Metadata;

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->resize(this->minimumSizeHint());
	this->warnings = new Warnings(ui->warningTooltipButton);

	connect(&settings, &Settings::configNotFound, [this](const QString &fileName) {
		Notify(Severity::Critical,
			 tr("Config not found"),
			 tr("Default configuration file '%1' is missing. Please reinstall the program.")
				 .arg(fileName));
	});
	connect(&settings, &Settings::keyCreated, [this](const QString &key) {
		Notify(Severity::Warning,
			 tr("Config key created"),
			 tr("A new configuration key '%1' has been created.").arg(key));
	});
	connect(&settings, &Settings::keyFallbackUsed, [this](const QString &key) {
		Notify(Severity::Warning,
			 tr("Config fallback used"),
			 tr("Configuration key '%1' was missing in '%2' and was created from the "
			    "fallback value in the default configuration file.")
				 .arg(key, settings.fileName()));
	});
	connect(&settings, &Settings::keyNotFound, [this](const QString &key) {
		Notify(Severity::Critical,
			 tr("Config key not found"),
			 tr("Configuration key '%1' is missing. Please add it manually in %2 or "
			    "reinstall the program.")
				 .arg(key, settings.fileName()));
	});
	connect(&settings, &Settings::notInitialized, [this]() {
		Notify(Severity::Critical,
			 tr("Config not initialized"),
			 tr("Configuration has not been initialized. Please contact the developers."));
	});

	settings.Init("config");

	connect(ui->qualityPresetComboBox, &QComboBox::currentIndexChanged, [this]() {
		if (ui->codecSelectionGroupBox->isChecked())
			return;

		Preset p = qvariant_cast<Preset>(ui->qualityPresetComboBox->currentData());

		ui->videoCodecComboBox->setCurrentText(p.videoCodec.name);
		ui->audioCodecComboBox->setCurrentText(p.audioCodec.name);
		ui->containerComboBox->setCurrentText(p.container.name);
	});

	connect(ui->widthSpinBox,
		  &QSpinBox::valueChanged,
		  this,
		  &MainWindow::CheckAspectRatioConflict);
	connect(ui->heightSpinBox,
		  &QSpinBox::valueChanged,
		  this,
		  &MainWindow::CheckAspectRatioConflict);
	connect(ui->aspectRatioSpinBoxH,
		  &QSpinBox::valueChanged,
		  this,
		  &MainWindow::CheckAspectRatioConflict);
	connect(ui->aspectRatioSpinBoxV,
		  &QSpinBox::valueChanged,
		  this,
		  &MainWindow::CheckAspectRatioConflict);

	connect(ui->speedSpinBox,
		  &QDoubleSpinBox::valueChanged,
		  this,
		  &MainWindow::CheckSpeedConflict);
	connect(ui->fpsSpinBox, &QSpinBox::valueChanged, this, &MainWindow::CheckSpeedConflict);

	// menu
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Help"), &QWhatsThis::enterWhatsThisMode);
	menu->addSeparator();
	menu->addAction(tr("About"), [this]() {
		Notify(Severity::Info,
			 "About " + QApplication::applicationName(),
			 settings.get("Main/sAbout").toString());
	});
	menu->addAction(tr("About Qt"), &QApplication::aboutQt);
	ui->infoMenuToolButton->setMenu(menu);
	connect(ui->infoMenuToolButton,
		  &QToolButton::pressed,
		  ui->infoMenuToolButton,
		  &QToolButton::showMenu);

	// detect nvidia
	QProcess process;
	process.setProcessChannelMode(QProcess::MergedChannels);

	if (IS_WINDOWS)
		process.startCommand("wmic path win32_VideoController get name");
	else
		process.startCommand("lspci");

	process.waitForFinished();

	if (process.readAllStandardOutput().toLower().contains("nvidia"))
		m_isNvidia = true;

	ParseCodecs(&videoCodecs, "VideoCodecs", ui->videoCodecComboBox);
	ParseCodecs(&audioCodecs, "AudioCodecs", ui->audioCodecComboBox);
	ParseContainers(&containers, ui->containerComboBox);
	ParsePresets(presets, videoCodecs, audioCodecs, containers, ui->qualityPresetComboBox);

	connect(ui->advancedModeCheckBox, &QCheckBox::clicked, this, &MainWindow::SetAdvancedMode);

	// start compression button
	connect(ui->startCompressionButton, &QPushButton::clicked, [=, this]() {
		QString inputPath = ui->inputFileLineEdit->text();

		if (!QFile::exists(inputPath)) {
			Notify(Severity::Info,
				 tr("File not found"),
				 tr("File '%1' does not exist. Please select a valid file.").arg(inputPath));
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

		videoCodec = hasVideo ? ui->videoCodecComboBox->currentData().value<Codec>()
					    : optional<Codec>();
		audioCodec = hasAudio ? ui->audioCodecComboBox->currentData().value<Codec>()
					    : optional<Codec>();
		container = containers.value(ui->containerComboBox->currentText());

		if (QFile::exists(outputPath + "." + container->name)
		    && ui->warnOnOverwriteCheckBox->isChecked()
		    && QMessageBox::question(this,
						     "Overwrite?",
						     "Output at path '" + outputPath
							     + "' already exists. Overwrite it?")
				 == QMessageBox::No) {
			Notify(Severity::Info, "Operation canceled", "Compression aborted.");
			return;
		}

		SetProgressShown(true);
		compressor->Compress(Compressor::Options{
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
			.aspectRatio = ui->aspectRatioSpinBoxH->value() != 0
							   && ui->aspectRatioSpinBoxV->value() != 0
						   ? QPoint(ui->aspectRatioSpinBoxH->value(),
								ui->aspectRatioSpinBoxV->value())
						   : optional<QPoint>(),
			.fps = ui->fpsSpinBox->value() == 0 ? optional<int>() : ui->fpsSpinBox->value(),
			.speed = ui->speedSpinBox->value() == 0 ? optional<double>()
									    : ui->speedSpinBox->value(),
			.customArguments = ui->customCommandTextEdit->toPlainText(),
			.inputMetadata = m_metadata.has_value() ? m_metadata
									    : optional<Compressor::Metadata>(),
			.minVideoBitrateKbps = settings.get("Main/dMinBitrateVideoKbps").toDouble(),
			.minAudioBitrateKbps = settings.get("Main/dMinBitrateAudioKbps").toDouble(),
			.maxAudioBitrateKbps = settings.get("Main/dMaxBitrateAudioKbps").toDouble(),
		});
	});

	// handle displaying target bitrates during compression
	connect(compressor,
		  &Compressor::compressionStarted,
		  [this](double videoBitrateKbps, double audioBitrateKbps) {
			  ui->progressBarLabel->setText(
				  QString(tr("Video bitrate: %1 kbps | Audio bitrate: %2 kbps"))
					  .arg(QString::number(qRound(videoBitrateKbps)),
						 QString::number(qRound(audioBitrateKbps))));
		  });

	// handle successful compression xd
	connect(compressor,
		  &Compressor::compressionSucceeded,
		  [this](const Compressor::Options &options,
			   const Compressor::ComputedOptions &computed,
			   QFile &output) {
			  SetProgressShown(true, 100);

			  QString summary;
			  QString videoBitrate = computed.videoBitrateKbps.has_value()
								 ? QString::number(*computed.videoBitrateKbps)
									   + "kbps"
								 : "auto-set bitrate";

			  if (options.videoCodec.has_value()) {
				  summary += tr("Using video codec %1 at %2 with container %3.\n")
							 .arg(options.videoCodec->name,
								videoBitrate,
								options.container->name);
			  }
			  if (options.audioCodec.has_value()) {
				  summary += tr("Using audio codec %1 at %2kbps.\n")
							 .arg(options.audioCodec->name,
								QString::number(*computed.audioBitrateKbps));
			  }
			  if (options.sizeKbps.has_value()) {
				  summary += tr("Requested size was %1 kb.\nActual "
						    "compression achieved is %2 kb.")
							 .arg(QString::number(*options.sizeKbps),
								QString::number(output.size() / 125.0));
			  }

			  Notify(Severity::Info, tr("Compressed successfully"), summary);

			  SetProgressShown(false);

			  QFileInfo fileInfo(output);
			  QString command = IS_WINDOWS ? "explorer.exe" : "xdg-open";

			  if (ui->deleteOnSuccessCheckBox->isChecked()) {
				  QFile input(options.inputPath);
				  input.open(QIODevice::WriteOnly);

				  if (!(input.remove())) {
					  Notify(Severity::Error,
						   tr("Failed to remove input file"),
						   output.errorString() + "\n\n" + options.inputPath);
				  } else {
					  ui->inputFileLineEdit->clear();
				  }

				  input.close();
			  }

			  if (ui->openExplorerOnSuccessCheckBox->isChecked()) {
				  QProcess::execute(
					  QString(R"(%1 "%2")").arg(command, fileInfo.dir().path()));
			  }

			  if (ui->playOnSuccessCheckBox->isChecked()) {
				  QProcess::execute(
					  QString(R"(%1 "%2")").arg(command, fileInfo.absoluteFilePath()));
			  }

			  if (ui->closeOnSuccessCheckBox->isChecked()) {
				  QApplication::exit(0);
			  }
		  });

	// handle failed compression
	connect(compressor,
		  &Compressor::compressionFailed,
		  [this](const QString &shortError, const QString &longError) {
			  Notify(Severity::Warning, tr("Compression failed"), shortError, longError);
			  SetProgressShown(false);
		  });

	// handle compression progress updates
	connect(compressor, &Compressor::compressionProgressUpdate, this, [this](int progress) {
		SetProgressShown(true, progress);
	});

	// show name of file picked with file dialog
	connect(ui->inputFileButton, &QPushButton::clicked, [this]() {
		QUrl fileUrl = QFileDialog::getOpenFileUrl(this,
									 tr("Select file to compress"),
									 QDir::currentPath(),
									 "*");

		QString path = fileUrl.toLocalFile();
		ui->inputFileLineEdit->setText(path);

		ParseMetadata(path);

		if (ui->autoFillCheckBox->isChecked()) {
			ui->widthSpinBox->setValue(m_metadata->width);
			ui->heightSpinBox->setValue(m_metadata->height);
			ui->speedSpinBox->setValue(1);
			ui->fileSizeSpinBox->setValue(m_metadata->sizeKbps);
			ui->fileSizeUnitComboBox->setCurrentIndex(0);
			ui->aspectRatioSpinBoxH->setValue(m_metadata->aspectRatioX);
			ui->aspectRatioSpinBoxV->setValue(m_metadata->aspectRatioY);
			ui->fpsSpinBox->setValue(m_metadata->frameRate);

			int qualityPercent = m_metadata->audioBitrateKbps * 100 / 256; // TODO parametrize
			ui->audioQualitySlider->setValue(qualityPercent);
		}
	});

	connect(ui->statisticsButton, &QPushButton::clicked, [this]() {
		if (!m_metadata.has_value()) {
			Notify(Severity::Info,
				 tr("No file selected"),
				 tr("Please select a file to continue."));
			return;
		}

		Notify(Severity::Info,
			 tr("Metadata"),
			 tr("Video codec: %1\nAudio codec: %2\nContainer: %3\nAudio bitrate: %4 kbps\n\n "
			    "(complete data to be implemented in a future update)")
				 .arg(m_metadata->videoCodec,
					m_metadata->audioCodec,
					"N/A",
					QString::number(m_metadata->audioBitrateKbps)));
	});

	// show name of file picked with file dialog
	connect(ui->outputFolderButton, &QPushButton::clicked, [this]() {
		QDir dir = QFileDialog::getExistingDirectory(this,
									   tr("Select output directory"),
									   QDir::currentPath());

		ui->outputFolderLineEdit->setText(dir.absolutePath());
	});

	// show value in kbps of audio quality slider
	connect(ui->audioQualitySlider, &QSlider::valueChanged, [this]() {
		double currentValue = qMax(settings.get("Main/dMinBitrateAudioKbps").toDouble(),
						   ui->audioQualitySlider->value() / 100.0
							   * settings.get("Main/dMaxBitrateAudioKbps").toDouble());
		ui->audioQualityDisplayLabel->setText(QString::number(qRound(currentValue)) + " kbps");
	});

	connect(ui->audioVideoButtonGroup,
		  &QButtonGroup::buttonClicked,
		  this,
		  &MainWindow::SetControlsState);

	connect(ui->codecSelectionGroupBox, &QGroupBox::clicked, [this](bool checked) {
		ui->qualityPresetComboBox->setEnabled(!checked);
	});

	LoadState();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::ParseCodecs(QHash<QString, Codec> *codecs, const QString &type, QComboBox *comboBox)
{
	QStringList keys = settings.keysInGroup(type);

	if (keys.empty()) {
		Notify(Severity::Critical,
			 tr("Missing codecs definition").arg(type),
			 tr("Could not find codecs list of type '%1' in the configuration file. "
			    "Please validate the configuration in %2. Exiting.")
				 .arg(type));
	}

	QString availableCodecs = compressor->availableFormats();

	for (const QString &codecLibrary : keys) {
		if (!availableCodecs.contains(codecLibrary)) {
			Notify(Severity::Warning,
				 tr("Unsupported codec"),
				 tr("Codec library '%1' does not appear to be supported by our version of "
				    "FFMPEG. It has been skipped. Please validate the "
				    "configuration in %2.")
					 .arg(codecLibrary, settings.fileName()));
			continue;
		}

		if (codecLibrary.contains("nvenc") && !m_isNvidia)
			continue;

		if (QRegularExpression("[^-_a-z0-9]").match(codecLibrary).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Could not parse codec"),
				 tr("Codec library '%1' could not be parsed. Please validate the "
				    "configuration in %2. Exiting.")
					 .arg(codecLibrary, settings.fileName()));
		}

		QStringList values = settings.get(type + "/" + codecLibrary)
						   .toString()
						   .split(QRegularExpression("(\\s*),(\\s*)"));

		QString codecName = values.first();

		bool isConversionOk = false;
		double codecMinBitrateKbps = values.size() == 2
							     ? values.last().toDouble(&isConversionOk)
							     : 0;

		if (values.size() == 2 && !isConversionOk) {
			Notify(Severity::Critical,
				 tr("Invalid codec bitrate"),
				 tr("Minimum bitrate of codec '%1' could not be parsed. Please "
				    "validate the configuration in %2. Exiting.")
					 .arg(values.last(), settings.fileName()));
		}

		Codec codec{codecName, codecLibrary, codecMinBitrateKbps};
		codecs->insert(codec.library, codec);

		QVariant data;
		data.setValue(codec);

		comboBox->addItem(codecName, data);
	}
}

void MainWindow::ParseContainers(QHash<QString, Container> *containers, QComboBox *comboBox)
{
	QStringList keys = settings.keysInGroup("Containers");

	if (keys.empty()) {
		Notify(Severity::Critical,
			 tr("Missing containers definition"),
			 tr("Could not find list of container in the configuration file. Please "
			    "validate the configuration in %1. Exiting.")
				 .arg(settings.fileName()));
	}

	for (const QString &containerName : keys) {
		if (QRegularExpression("[^-_a-z0-9]").match(containerName).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Invalid container name"),
				 tr("Name of container '%1' could not be parsed. Please "
				    "validate the configuration in %2. Exiting.")
					 .arg(containerName, settings.fileName()));
		}

		QStringList supportedCodecs = settings.get("Containers/" + containerName)
								.toString()
								.split(QRegularExpression("(\\s*),(\\s*)"));
		Container container{containerName, supportedCodecs};
		containers->insert(containerName, container);

		QVariant data;
		data.setValue(container);

		comboBox->addItem(containerName, data);
	}
}

void MainWindow::ParsePresets(QHash<QString, Preset> &presets,
					QHash<QString, Codec> &videoCodecs,
					QHash<QString, Codec> &audioCodecs,
					QHash<QString, Container> &containers,
					QComboBox *comboBox)
{
	QStringList keys = settings.keysInGroup("Presets");
	QString supportedCodecs = compressor->availableFormats();

	for (const QString &key : keys) {
		Preset preset;
		preset.name = key;

		QString data = settings.get("Presets/" + key).toString();
		QStringList librariesData = data.split("+");

		if (librariesData.size() != 3) {
			Notify(Severity::Error,
				 tr("Could not parse presets"),
				 tr("Failed to parse presets as the data is malformed. Please check the "
				    "configuration in %1.")
					 .arg(settings.fileName()));
		}

		optional<Codec> videoCodec;
		optional<Codec> audioCodec;
		QStringList videoLibrary = librariesData[0].split("|");

		for (const QString &library : videoLibrary) {
			if (supportedCodecs.contains(library)) {
				videoCodec = videoCodecs.value(library);
				break;
			}
		}

		QStringList audioLibrary = librariesData[1].split("|");

		for (const QString &library : audioLibrary) {
			if (supportedCodecs.contains(library)) {
				audioCodec = audioCodecs.value(library);
				break;
			}
		}

		if (!videoCodec.has_value() || !audioCodec.has_value()) {
			Notify(Severity::Warning,
				 tr("Preset not supported"),
				 tr("Skipped encoding quality preset '%1' preset because it contains no "
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

void MainWindow::ParseMetadata(const QString &path)
{
	std::variant<Metadata, Compressor::Error> result = compressor->getMetadata(path);

	if (std::holds_alternative<Compressor::Error>(result)) {
		Compressor::Error error = std::get<Compressor::Error>(result);

		Notify(Severity::Error,
			 tr("Failed to parse media metadata"),
			 error.summary,
			 error.details);
		ui->inputFileLineEdit->clear();
		return;
	}

	m_metadata = std::get<Metadata>(result);
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

bool MainWindow::isAutoValue(QAbstractSpinBox *spinBox)
{
	return spinBox->text() == spinBox->specialValueText();
}

void MainWindow::CheckAspectRatioConflict()
{
	bool hasCustomScale = ui->aspectRatioSpinBoxH->value() != 0
				    || ui->aspectRatioSpinBoxV->value() != 0;
	bool hasCustomAspect = ui->widthSpinBox->value() != 0 || ui->heightSpinBox->value() != 0;

	if (hasCustomScale && hasCustomAspect) {
		warnings->Add("aspectRatioConflict",
				  tr("A custom aspect ratio has been set. It will take priority over the "
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
		warnings->Add("speedConflict",
				  tr("A custom speed was set alongside a custom fps; this may cause "
				     "strange behavior. Note that fps is automatically compensated when "
				     "changing the speed."));
	} else {
		warnings->Remove("speedConflict");
	}
}

void MainWindow::SetProgressShown(bool shown, int progressPercent)
{
	auto *valueAnimation = new QPropertyAnimation(ui->progressBar, "value");
	valueAnimation->setDuration(settings.get("Main/iProgressBarAnimDurationMs").toInt());
	valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

	auto *heightAnimation = new QPropertyAnimation(ui->progressWidget, "maximumHeight");
	valueAnimation->setDuration(settings.get("Main/iProgressWidgetAnimDurationMs").toInt());
	valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

	if (shown && ui->progressWidget->maximumHeight() == 0) {
		ui->centralWidget->setEnabled(false);
		ui->startCompressionButton->setText(tr("Compressing..."));
		ui->progressWidgetTopSpacer->changeSize(0, 10);
		heightAnimation->setStartValue(0);
		heightAnimation->setEndValue(500);
		heightAnimation->start();
	}

	if (!shown && ui->progressWidget->maximumHeight() > 0) {
		ui->centralWidget->setEnabled(true);
		ui->startCompressionButton->setText(tr("Start compression"));
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
	auto *sectionWidthAnim = new QPropertyAnimation(ui->advancedSection, "maximumWidth", this);
	auto *sectionHeightAnim = new QPropertyAnimation(ui->advancedSection, "maximumHeight", this);
	auto *windowSizeAnim = new QPropertyAnimation(this, "size");
	int duration = settings.get("Main/iSectionAnimDurationMs").toInt();

	sectionWidthAnim->setDuration(duration);
	sectionWidthAnim->setEasingCurve(QEasingCurve::InOutQuad);
	sectionHeightAnim->setDuration(duration);
	sectionHeightAnim->setEasingCurve(QEasingCurve::InOutQuad);
	windowSizeAnim->setDuration(duration);
	windowSizeAnim->setEasingCurve(QEasingCurve::InOutQuad);

	connect(sectionWidthAnim, &QPropertyAnimation::finished, [this, windowSizeAnim]() {
		windowSizeAnim->setStartValue(this->size());
		windowSizeAnim->setEndValue(this->minimumSizeHint());
		windowSizeAnim->start();
	});

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

void MainWindow::SetControlsState(QAbstractButton *button)
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

void MainWindow::Notify(Severity severity,
				const QString &title,
				const QString &message,
				const QString &details)
{
	QFont font;
	font.setBold(true);

	QMessageBox dialog;
	dialog.setWindowTitle(ui->centralWidget->windowTitle());
	dialog.setIcon(severity == Severity::Critical ? QMessageBox::Critical
								    : (QMessageBox::Icon) severity);
	dialog.setText("<font size=5><b>" + title + ".</b></font>");
	dialog.setInformativeText(message);
	dialog.setDetailedText(details);
	dialog.setStandardButtons(QMessageBox::Ok);
	dialog.exec();

	// wait until event loop has begun before attempting to exit
	if (severity == Severity::Critical)
		QMetaObject::invokeMethod(QApplication::instance(), "exit", Qt::QueuedConnection);
}

void MainWindow::SaveState()
{
	settings.Set("LastDesired/bAdvancedMode", ui->advancedModeCheckBox->isChecked());

	settings.Set("LastDesired/sInputFile", ui->inputFileLineEdit->text());
	settings.Set("LastDesired/sOutputDir", ui->outputFolderLineEdit->text());
	settings.Set("LastDesired/dFileSize", ui->fileSizeSpinBox->value());
	settings.Set("LastDesired/sFileSizeUnit", ui->fileSizeUnitComboBox->currentText());
	settings.Set("LastDesired/sFileNameOrSuffix", ui->outputFileNameLineEdit->text());
	settings.Set("LastDesired/iQualityRatio", ui->audioQualitySlider->value());

	settings.Set("LastDesired/sVideoCodec", ui->videoCodecComboBox->currentText());
	settings.Set("LastDesired/sAudioCodec", ui->audioCodecComboBox->currentText());
	settings.Set("LastDesired/sContainer", ui->containerComboBox->currentText());

	settings.Set("LastDesired/iWidth", ui->widthSpinBox->value());
	settings.Set("LastDesired/iHeight", ui->heightSpinBox->value());

	settings.Set("LastDesired/iAspectRatioH", ui->aspectRatioSpinBoxH->value());
	settings.Set("LastDesired/iAspectRatioV", ui->aspectRatioSpinBoxV->value());

	settings.Set("LastDesired/sCustomParameters", ui->customCommandTextEdit->toPlainText());

	settings.Set("LastDesired/bHasFileSuffix", ui->outputFileNameSuffixCheckBox->isChecked());
	settings.Set("LastDesired/bWarnOnOverwrite", ui->warnOnOverwriteCheckBox->isChecked());

	settings.Set("LastDesired/dSpeed", ui->speedSpinBox->value());
	settings.Set("LastDesired/iFps", ui->fpsSpinBox->value());

	settings.Set("LastDesired/bDeleteInputOnSuccess", ui->deleteOnSuccessCheckBox->isChecked());

	settings.Set("LastDesired/bUseCustomCodecs", ui->codecSelectionGroupBox->isChecked());

	settings.Set("LastDesired/bAutoFillMetadata", ui->autoFillCheckBox->isChecked());

	auto *streamSelection = ui->audioVideoButtonGroup->checkedButton();
	if (streamSelection == ui->radVideoAudio)
		settings.Set("LastDesired/iSelectedStreams", 0);
	else if (streamSelection == ui->radVideoOnly)
		settings.Set("LastDesired/iSelectedStreams", 1);
	else if (streamSelection == ui->radAudioOnly)
		settings.Set("LastDesired/iSelectedStreams", 2);

	settings.Set("LastDesired/bOpenInExplorerOnSuccess",
			 ui->openExplorerOnSuccessCheckBox->isChecked());
	settings.Set("LastDesired/bPlayResultOnSuccess", ui->playOnSuccessCheckBox->isChecked());
	settings.Set("LastDesired/bCloseOnSuccess", ui->closeOnSuccessCheckBox->isChecked());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	SaveState();
	event->accept();
}

void MainWindow::LoadState()
{
	ui->warningTooltipButton->setVisible(false);

	if (!settings.get("LastDesired/bAdvancedMode").toBool())
		ui->advancedModeCheckBox->click();

	QString selectedUrl = settings.get("LastDesired/sInputFile").toString();
	QString selectedDir = settings.get("LastDesired/sOutputDir").toString();

	if (selectedUrl == "" || QFile::exists(selectedUrl))
		ui->inputFileLineEdit->setText(selectedUrl);
	if (selectedDir == "" || QDir(selectedDir).exists())
		ui->outputFolderLineEdit->setText(selectedDir);

	if (QFile::exists(selectedUrl))
		ParseMetadata(selectedUrl);

	ui->outputFileNameLineEdit->setText(settings.get("LastDesired/sFileNameOrSuffix").toString());

	ui->fileSizeSpinBox->setValue(settings.get("LastDesired/dFileSize").toDouble());
	ui->fileSizeUnitComboBox->setCurrentText(settings.get("LastDesired/sFileSizeUnit").toString());
	ui->audioQualitySlider->setValue(settings.get("LastDesired/iQualityRatio").toInt());
	emit ui->audioQualitySlider->valueChanged(ui->audioQualitySlider->value());

	ui->videoCodecComboBox->setCurrentText(settings.get("LastDesired/sVideoCodec").toString());
	ui->audioCodecComboBox->setCurrentText(settings.get("LastDesired/sAudioCodec").toString());
	ui->containerComboBox->setCurrentText(settings.get("LastDesired/sContainer").toString());

	ui->widthSpinBox->setValue(settings.get("LastDesired/iWidth").toInt());
	ui->heightSpinBox->setValue(settings.get("LastDesired/iHeight").toInt());

	ui->aspectRatioSpinBoxH->setValue(settings.get("LastDesired/iAspectRatioH").toInt());
	ui->aspectRatioSpinBoxV->setValue(settings.get("LastDesired/iAspectRatioV").toInt());

	ui->customCommandTextEdit->setPlainText(
		settings.get("LastDesired/sCustomParameters").toString());

	ui->outputFileNameSuffixCheckBox->setChecked(
		settings.get("LastDesired/bHasFileSuffix").toBool());
	ui->warnOnOverwriteCheckBox->setChecked(settings.get("LastDesired/bWarnOnOverwrite").toBool());

	ui->speedSpinBox->setValue(settings.get("LastDesired/dSpeed").toDouble());
	ui->fpsSpinBox->setValue(settings.get("LastDesired/iFps").toInt());

	ui->autoFillCheckBox->setChecked(settings.get("LastDesired/bAutoFillMetadata").toBool());

	ui->deleteOnSuccessCheckBox->setChecked(
		settings.get("LastDesired/bDeleteInputOnSuccess").toBool());

	switch (settings.get("LastDesired/iSelectedStreams").toInt()) {
	case 0:
		ui->radVideoAudio->setChecked(true);
		break;
	case 1:
		ui->radVideoOnly->setChecked(true);
		break;
	case 2:
		ui->radAudioOnly->setChecked(true);
		break;
	}

	ui->openExplorerOnSuccessCheckBox->setChecked(
		settings.get("LastDesired/bOpenInExplorerOnSuccess").toBool());
	ui->playOnSuccessCheckBox->setChecked(
		settings.get("LastDesired/bPlayResultOnSuccess").toBool());
	ui->closeOnSuccessCheckBox->setChecked(settings.get("LastDesired/bCloseOnSuccess").toBool());

	ui->codecSelectionGroupBox->setChecked(settings.get("LastDesired/bUseCustomCodecs").toBool());

	SetControlsState(ui->audioVideoButtonGroup->checkedButton());
}

Warnings::Warnings(QWidget *widget) : m_widget{widget} {}

void Warnings::Add(QString name, QString description)
{
	this->insert(name, description);
	this->Update();
}

void Warnings::Remove(QString name)
{
	this->remove(name);
	this->Update();
}

void Warnings::Update()
{
	if (this->isEmpty()) {
		m_widget->setVisible(false);
	} else {
		m_widget->setToolTip(this->values().join("\n\n"));
		m_widget->setVisible(true);
	}
}
