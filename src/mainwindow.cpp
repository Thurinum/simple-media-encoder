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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
	ui->setupUi(this);
	this->resize(this->minimumSizeHint());

	InitSettings("config");

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

	// menu
	QMenu *menu = new QMenu(this);
	menu->addAction(tr("Help"), &QWhatsThis::enterWhatsThisMode);
	menu->addSeparator();
	menu->addAction(tr("About"), [this]() {
		Notify(Severity::Info,
			 "About " + QApplication::applicationName(),
			 setting("Main/sAbout").toString());
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

	// parse codecs
	QList<Codec> videoCodecs;
	QList<Codec> audioCodecs;
	QList<Container> containers;

	ParseCodecs(&videoCodecs, "VideoCodecs", ui->videoCodecComboBox);
	ParseCodecs(&audioCodecs, "AudioCodecs", ui->audioCodecComboBox);
	ParseContainers(&containers, ui->containerComboBox);

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

		auto container = containers.at(ui->containerComboBox->currentIndex());

		if (QFile::exists(outputPath + "." + container.name)
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
		compressor->compress(Compressor::Options{
			.inputPath = inputPath,
			.outputPath = outputPath,
			.videoCodec = hasVideo ? videoCodecs.at(ui->videoCodecComboBox->currentIndex())
						     : optional<Codec>(),
			.audioCodec = hasAudio ? audioCodecs.at(ui->audioCodecComboBox->currentIndex())
						     : optional<Codec>(),
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
			.customArguments = ui->customCommandTextEdit->toPlainText(),
			.minVideoBitrateKbps = setting("Main/dMinBitrateVideoKbps").toDouble(),
			.minAudioBitrateKbps = setting("Main/dMinBitrateAudioKbps").toDouble(),
			.maxAudioBitrateKbps = setting("Main/dMaxBitrateAudioKbps").toDouble()});
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
				  qDebug() << fileInfo.absoluteFilePath();
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

		ui->inputFileLineEdit->setText(fileUrl.toLocalFile());
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
		double currentValue = qMax(setting("Main/dMinBitrateAudioKbps").toDouble(),
						   ui->audioQualitySlider->value() / 100.0
							   * setting("Main/dMaxBitrateAudioKbps").toDouble());
		ui->audioQualityDisplayLabel->setText(QString::number(qRound(currentValue)) + " kbps");
	});

	connect(ui->audioVideoButtonGroup, &QButtonGroup::buttonClicked, [&](QAbstractButton *button) {
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
	});

	connect(ui->codecSelectionGroupBox, &QGroupBox::clicked, [this](bool checked) {
		ui->qualityPresetComboBox->setEnabled(!checked);
	});

	LoadState();
}

MainWindow::~MainWindow()
{
	delete ui;
}

void MainWindow::ParseCodecs(QList<Codec> *codecs, const QString &type, QComboBox *comboBox)
{
	settings->beginGroup(type);

	if (settings->childKeys().empty()) {
		Notify(Severity::Critical,
			 tr("Missing codecs definition").arg(type),
			 tr("Could not find codecs list of type '%1' in the configuration file. "
			    "Please validate the configuration in %2. Exiting.")
				 .arg(type));
	}

	QString availableCodecs = compressor->availableFormats();

	for (const QString &codecLibrary : settings->childKeys()) {
		if (!availableCodecs.contains(codecLibrary)) {
			Notify(Severity::Warning,
				 tr("Unsupported codec"),
				 tr("Codec library '%1' does not appear to be supported by our version of "
				    "FFMPEG. It has been skipped. Please validate the "
				    "configuration in %2.")
					 .arg(codecLibrary, settings->fileName()));
			continue;
		}

		if (codecLibrary.contains("nvenc") && !m_isNvidia)
			continue;

		if (QRegularExpression("[^-_a-z0-9]").match(codecLibrary).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Could not parse codec"),
				 tr("Codec library '%1' could not be parsed. Please validate the "
				    "configuration in %2. Exiting.")
					 .arg(codecLibrary, settings->fileName()));
		}

		QStringList values
			= setting(codecLibrary).toString().split(QRegularExpression("(\\s*),(\\s*)"));

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
					 .arg(values.last(), settings->fileName()));
		}

		Codec codec{codecName, codecLibrary, codecMinBitrateKbps};
		codecs->append(codec);
		comboBox->addItem(codecName);
	}
	settings->endGroup();
}

void MainWindow::ParseContainers(QList<Container> *containers, QComboBox *comboBox)
{
	settings->beginGroup("Containers");

	if (settings->childKeys().empty()) {
		Notify(Severity::Critical,
			 tr("Missing containers definition"),
			 tr("Could not find list of container in the configuration file. Please "
			    "validate the configuration in %1. Exiting.")
				 .arg(settings->fileName()));
	}

	for (const QString &containerName : settings->childKeys()) {
		if (QRegularExpression("[^-_a-z0-9]").match(containerName).hasMatch()) {
			Notify(Severity::Critical,
				 tr("Invalid container name"),
				 tr("Name of container '%1' could not be parsed. Please "
				    "validate the configuration in %2. Exiting.")
					 .arg(containerName, settings->fileName()));
		}

		QStringList supportedCodecs
			= setting(containerName).toString().split(QRegularExpression("(\\s*),(\\s*)"));
		Container container{containerName, supportedCodecs};
		containers->append(container);
		comboBox->addItem(containerName.toUpper());
	}
	settings->endGroup();
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
		ui->warningTooltipButton->setToolTip(
			tr("A custom aspect ratio has been set.\nIt will take priority over the "
			   "one\ndefined by custom scaling."));
		ui->warningTooltipButton->setVisible(true);
	} else {
		ui->warningTooltipButton->setVisible(false);
	}
}

void MainWindow::InitSettings(const QString &fileName)
{
	QString defaultFileName = fileName + "_default.ini";
	QString currentFileName = fileName + ".ini";

	if (!QFile::exists(defaultFileName)) {
		Notify(Severity::Critical,
			 tr("Cannot find default settings"),
			 tr("Default configuration file %1 was not found. Please reinstall the program.")
				 .arg(defaultFileName));
	}

	if (!QFile::exists(currentFileName))
		QFile::copy(defaultFileName, currentFileName);

	settings = new QSettings(currentFileName, QSettings::IniFormat);
}

QVariant MainWindow::setting(const QString &key)
{
	if (!settings->contains(key)) {
		Notify(Severity::Critical,
			 tr("Missing configuration"),
			 tr("Configuration file is missing key '%1'. Please validate the "
			    "configuration in %2. Exiting.")
				 .arg(key, settings->fileName()),
			 tr("Missing key: %1\nWorking directory: %2").arg(key, QDir::currentPath()));
	}

	return settings->value(key);
}

void MainWindow::setSetting(const QString &key, const QVariant &value)
{
	if (!settings->contains(key)) {
		Notify(Severity::Warning,
			 tr("New setting created"),
			 tr("Request to set configuration key '%1' which does not already "
			    "exist. Please report this warning to the developers.")
				 .arg(key));
	}

	settings->setValue(key, value);
}

void MainWindow::SetProgressShown(bool shown, int progressPercent)
{
	auto *valueAnimation = new QPropertyAnimation(ui->progressBar, "value");
	valueAnimation->setDuration(setting("Main/iProgressBarAnimDurationMs").toInt());
	valueAnimation->setEasingCurve(QEasingCurve::InOutQuad);

	auto *heightAnimation = new QPropertyAnimation(ui->progressWidget, "maximumHeight");
	valueAnimation->setDuration(setting("Main/iProgressWidgetAnimDurationMs").toInt());
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
	int duration = setting("Main/iSectionAnimDurationMs").toInt();

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
		QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);
}

void MainWindow::SaveState()
{
	setSetting("LastDesired/bAdvancedMode", ui->advancedModeCheckBox->isChecked());

	setSetting("LastDesired/sInputFile", ui->inputFileLineEdit->text());
	setSetting("LastDesired/sOutputDir", ui->outputFolderLineEdit->text());
	setSetting("LastDesired/dFileSize", ui->fileSizeSpinBox->value());
	setSetting("LastDesired/sFileSizeUnit", ui->fileSizeUnitComboBox->currentText());
	setSetting("LastDesired/sFileNameOrSuffix", ui->outputFileNameLineEdit->text());
	setSetting("LastDesired/iQualityRatio", ui->audioQualitySlider->value());

	setSetting("LastDesired/sVideoCodec", ui->videoCodecComboBox->currentText());
	setSetting("LastDesired/sAudioCodec", ui->audioCodecComboBox->currentText());
	setSetting("LastDesired/sContainer", ui->containerComboBox->currentText());

	setSetting("LastDesired/iWidth", ui->widthSpinBox->value());
	setSetting("LastDesired/iHeight", ui->heightSpinBox->value());

	setSetting("LastDesired/iAspectRatioH", ui->aspectRatioSpinBoxH->value());
	setSetting("LastDesired/iAspectRatioV", ui->aspectRatioSpinBoxV->value());

	setSetting("LastDesired/sCustomParameters", ui->customCommandTextEdit->toPlainText());

	setSetting("LastDesired/bHasFileSuffix", ui->outputFileNameSuffixCheckBox->isChecked());
	setSetting("LastDesired/bWarnOnOverwrite", ui->warnOnOverwriteCheckBox->isChecked());

	setSetting("LastDesired/iFps", ui->fpsSpinBox->value());

	setSetting("LastDesired/bDeleteInputOnSuccess", ui->deleteOnSuccessCheckBox->isChecked());

	auto *streamSelection = ui->audioVideoButtonGroup->checkedButton();
	if (streamSelection == ui->radVideoAudio)
		setSetting("LastDesired/iSelectedStreams", 0);
	else if (streamSelection == ui->radVideoOnly)
		setSetting("LastDesired/iSelectedStreams", 1);
	else if (streamSelection == ui->radAudioOnly)
		setSetting("LastDesired/iSelectedStreams", 2);

	setSetting("LastDesired/bOpenInExplorerOnSuccess",
		     ui->openExplorerOnSuccessCheckBox->isChecked());
	setSetting("LastDesired/bPlayResultOnSuccess", ui->playOnSuccessCheckBox->isChecked());
	setSetting("LastDesired/bCloseOnSuccess", ui->closeOnSuccessCheckBox->isChecked());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	SaveState();
	event->accept();
}

void MainWindow::LoadState()
{
	ui->warningTooltipButton->setVisible(false);

	if (!setting("LastDesired/bAdvancedMode").toBool())
		ui->advancedModeCheckBox->click();

	QString selectedUrl = setting("LastDesired/sInputFile").toString();
	QString selectedDir = setting("LastDesired/sOutputDir").toString();

	if (selectedUrl == "" || QFile::exists(selectedUrl))
		ui->inputFileLineEdit->setText(selectedUrl);
	if (selectedDir == "" || QDir(selectedDir).exists())
		ui->outputFolderLineEdit->setText(selectedDir);

	ui->outputFileNameLineEdit->setText(setting("LastDesired/sFileNameOrSuffix").toString());

	ui->fileSizeSpinBox->setValue(setting("LastDesired/dFileSize").toDouble());
	ui->fileSizeUnitComboBox->setCurrentText(setting("LastDesired/sFileSizeUnit").toString());
	ui->audioQualitySlider->setValue(setting("LastDesired/iQualityRatio").toInt());
	emit ui->audioQualitySlider->valueChanged(ui->audioQualitySlider->value());

	ui->videoCodecComboBox->setCurrentText(setting("LastDesired/sVideoCodec").toString());
	ui->audioCodecComboBox->setCurrentText(setting("LastDesired/sAudioCodec").toString());
	ui->containerComboBox->setCurrentText(setting("LastDesired/sContainer").toString());

	ui->widthSpinBox->setValue(setting("LastDesired/iWidth").toInt());
	ui->heightSpinBox->setValue(setting("LastDesired/iHeight").toInt());

	ui->aspectRatioSpinBoxH->setValue(setting("LastDesired/iAspectRatioH").toInt());
	ui->aspectRatioSpinBoxV->setValue(setting("LastDesired/iAspectRatioV").toInt());

	ui->customCommandTextEdit->setPlainText(setting("LastDesired/sCustomParameters").toString());

	ui->outputFileNameSuffixCheckBox->setChecked(setting("LastDesired/bHasFileSuffix").toBool());
	ui->warnOnOverwriteCheckBox->setChecked(setting("LastDesired/bWarnOnOverwrite").toBool());

	ui->fpsSpinBox->setValue(setting("LastDesired/iFps").toInt());

	ui->deleteOnSuccessCheckBox->setChecked(setting("LastDesired/bDeleteInputOnSuccess").toBool());

	switch (setting("LastDesired/iSelectedStreams").toInt()) {
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
		setting("LastDesired/bOpenInExplorerOnSuccess").toBool());
	ui->playOnSuccessCheckBox->setChecked(setting("LastDesired/bPlayResultOnSuccess").toBool());
	ui->closeOnSuccessCheckBox->setChecked(setting("LastDesired/bCloseOnSuccess").toBool());
}
