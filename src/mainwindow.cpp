#include "mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenuBar>
#include <QMouseEvent>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsBlurEffect>
#include <QImage>
#include <QPixmap>
#include <QPushButton>
#include <QProcess>
#include <QSettings>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QStyle>
#include <QTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <limits>

namespace
{
bool flagText(const QString &flags, const QString &flag)
{
    const QStringList parts = flags.split(',', Qt::SkipEmptyParts);
    return parts.contains(flag);
}

QString displayName(const QString &name)
{
    QString result = name;
    result.replace('_', ' ');
    return result;
}

QLabel *makeEyebrow(const QString &text, QWidget *parent)
{
    auto *label = new QLabel(text, parent);
    label->setObjectName("eyebrow");
    return label;
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      v4l2Executable(QStandardPaths::findExecutable("v4l2-ctl"))
      , ffmpegExecutable(QStandardPaths::findExecutable("ffmpeg"))
{
    buildInterface();
    scanDevices();
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && windowHandle()->startSystemMove()) {
        event->accept();
        return;
    }

    QMainWindow::mousePressEvent(event);
}

void MainWindow::buildInterface()
{
    setWindowTitle("KamKontroller | Camera");
    setMinimumSize(900, 600);
    resize(1180, 760);
    setStyleSheet(R"(
        QWidget { color: #dce5df; background: #151a18; font-family: "Noto Sans", "DejaVu Sans", sans-serif; font-size: 12px; }
        QMainWindow, #mainPane { background: #191f1c; }
        #preview { background: #0b0e0d; color: #748078; font-size: 15px; }
        #overlayPanel { background: rgba(25, 31, 28, 245); border: 1px solid #536149; }
        #modePicker { font-size: 13px; padding: 8px 12px; }
        #recordButton { background: #a8473c; color: white; border-color: #d06a5d; padding: 9px 16px; }
        #recordButton:checked { background: #6d2d29; }
        #timer { color: #f3d9d3; font-size: 18px; font-weight: 700; background: transparent; }
        #sideButton { min-width: 116px; padding: 9px 12px; }
        #brand { color: #c6e36b; font-size: 14px; font-weight: 700; }
        #eyebrow { color: #84958b; font-size: 9px; font-weight: 700; }
        #pageTitle { color: #edf3ee; font-size: 22px; font-weight: 700; }
        #cameraName { color: #e6eee8; font-size: 13px; font-weight: 700; }
        #cameraPath { color: #91a098; font-family: monospace; font-size: 11px; }
        #sectionTitle { color: #dce5df; font-size: 12px; font-weight: 700; }
        QComboBox, QSpinBox, QLineEdit {
            background: #202824; color: #e3ebe5; border: 1px solid #39463f; border-radius: 3px;
            padding: 4px 7px; min-height: 18px; selection-background-color: #506331;
        }
        QComboBox:focus, QSpinBox:focus, QLineEdit:focus { border-color: #9ab84c; }
        QComboBox::drop-down { background: transparent; border: 0; width: 22px; }
        QComboBox QAbstractItemView { background: #202824; color: #e3ebe5; border: 1px solid #39463f; selection-background-color: #40532c; }
        QSpinBox::up-button, QSpinBox::down-button { background: #29332d; border: 0; width: 15px; }
        QPushButton {
            background: #2b382d; color: #d8e8ad; border: 1px solid #485b37;
            border-radius: 3px; padding: 6px 9px; font-weight: 700;
        }
        QPushButton:hover { background: #35452f; border-color: #667d3d; }
        QPushButton:pressed { background: #263226; }
        QPushButton:disabled { color: #69756e; background: #202622; border-color: #303a34; }
        QCheckBox { spacing: 6px; background: transparent; }
        QCheckBox::indicator { width: 14px; height: 14px; background: #202824; border: 1px solid #536159; border-radius: 2px; }
        QCheckBox::indicator:checked { background: #9ab84c; border: 1px solid #9ab84c; border-radius: 2px; }
        QSlider::groove:horizontal { height: 3px; background: #38433d; border-radius: 1px; }
        QSlider::sub-page:horizontal { background: #9ab84c; border-radius: 1px; }
        QSlider::handle:horizontal { background: #dce8c5; border: 2px solid #9ab84c; width: 11px; height: 11px; margin: -5px 0; border-radius: 6px; }
        QScrollArea, QScrollArea > QWidget > QWidget { background: transparent; border: 0; }
        QScrollBar:vertical { background: transparent; width: 7px; margin: 0; }
        QScrollBar::handle:vertical { background: #46534b; border-radius: 3px; min-height: 24px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        QToolTip { color: #e3ebe5; background: #29332d; border: 1px solid #46534b; }
        #status { color: #92a198; padding: 3px 0; background: transparent; }
        #status[error="true"] { color: #f08d7e; }
        #emptyState { color: #91a098; font-size: 12px; padding: 20px 2px; background: transparent; }
        QFrame[role="divider"] { background: #303a34; max-height: 1px; }
    )");

    auto *root = new QWidget(this);
    auto *settingsMenu = menuBar()->addMenu("Settings");
    auto *locationAction = settingsMenu->addAction("Default save location...");
    connect(locationAction, &QAction::triggered, this, &MainWindow::chooseDefaultLocation);

    auto *mainPane = new QWidget(root);
    mainPane->setObjectName("mainPane");
    auto *mainLayout = new QVBoxLayout(mainPane);
    mainLayout->setContentsMargins(18, 14, 18, 12);
    mainLayout->setSpacing(10);

    auto *topRow = new QHBoxLayout;
    auto *brand = new QLabel("KAMKONTROLLER", mainPane);
    brand->setObjectName("brand");
    topRow->addWidget(brand);
    topRow->addSpacing(14);
    auto *changeButton = new QPushButton("Change camera", mainPane);
    changeButton->setObjectName("sideButton");
    topRow->addWidget(changeButton);
    auto *controlButton = new QPushButton("Controls", mainPane);
    controlButton->setObjectName("sideButton");
    topRow->addWidget(controlButton);
    topRow->addSpacing(12);
    devicePicker = new QComboBox(mainPane);
    devicePicker->setMinimumWidth(280);
    topRow->addWidget(devicePicker);
    auto *refreshButton = new QPushButton("Scan", mainPane);
    topRow->addWidget(refreshButton);
    cameraSummary = new QLabel("No device selected", mainPane);
    cameraSummary->setObjectName("cameraPath");
    cameraSummary->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    topRow->addWidget(cameraSummary, 1);
    mainLayout->addLayout(topRow);

    auto *previewFrame = new QFrame(mainPane);
    previewFrame->setObjectName("preview");
    auto *previewLayout = new QVBoxLayout(previewFrame);
    previewLayout->setContentsMargins(0, 0, 0, 0);
    previewLabel = new QLabel("Connect a camera to start the live preview", previewFrame);
    previewLabel->setObjectName("preview");
    previewLabel->setAlignment(Qt::AlignCenter);
    previewLabel->setMinimumHeight(360);
    previewLayout->addWidget(previewLabel);
    timerLabel = new QLabel("00:00:00", previewFrame);
    timerLabel->setObjectName("timer");
    timerLabel->setAlignment(Qt::AlignCenter);
    timerLabel->hide();
    previewLayout->addWidget(timerLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(previewFrame, 1);

    auto *bottomRow = new QHBoxLayout;
    modePicker = new QComboBox(mainPane);
    modePicker->setObjectName("modePicker");
    modePicker->addItems({"Video", "Camera", "Timelapse"});
    bottomRow->addWidget(modePicker);
    recordButton = new QPushButton("Start recording", mainPane);
    recordButton->setObjectName("recordButton");
    recordButton->setCheckable(true);
    bottomRow->addWidget(recordButton);
    photoButton = new QPushButton("Take photo", mainPane);
    photoButton->hide();
    bottomRow->addWidget(photoButton);
    timelapseButton = new QPushButton("Start timelapse", mainPane);
    timelapseButton->hide();
    bottomRow->addWidget(timelapseButton);
    bottomRow->addStretch();
    statusLabel = new QLabel("Ready", mainPane);
    statusLabel->setObjectName("status");
    bottomRow->addWidget(statusLabel);
    mainLayout->addLayout(bottomRow);

    controlPanel = new QWidget(mainPane);
    controlPanel->setObjectName("overlayPanel");
    controlPanel->setMinimumWidth(430);
    controlPanel->setMaximumWidth(520);
    auto *panelLayout = new QVBoxLayout(controlPanel);
    panelLayout->setContentsMargins(16, 12, 12, 12);
    auto *panelHeader = new QHBoxLayout;
    auto *panelTitle = new QLabel("Camera controls", controlPanel);
    panelTitle->setObjectName("pageTitle");
    panelHeader->addWidget(panelTitle);
    panelHeader->addStretch();
    auto *resetButton = new QPushButton("Reset defaults", controlPanel);
    panelHeader->addWidget(resetButton);
    auto *closeButton = new QPushButton("Close", controlPanel);
    panelHeader->addWidget(closeButton);
    panelLayout->addLayout(panelHeader);
    controlScroll = new QScrollArea(controlPanel);
    controlScroll->setWidgetResizable(true);
    controlHost = new QWidget;
    controlLayout = new QVBoxLayout(controlHost);
    controlLayout->setContentsMargins(0, 0, 5, 0);
    controlLayout->setSpacing(0);
    emptyState = new QLabel("Select a camera to read its available controls.", controlHost);
    emptyState->setObjectName("emptyState");
    emptyState->setWordWrap(true);
    controlLayout->addWidget(emptyState);
    controlScroll->setWidget(controlHost);
    panelLayout->addWidget(controlScroll, 1);
    controlPanel->hide();
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(mainPane, 1);
    rootLayout->addWidget(controlPanel);
    setCentralWidget(root);

    auto *refreshControlsButton = new QPushButton("Refresh controls", mainPane);
    refreshControlsButton->hide();

    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::scanDevices);
    connect(devicePicker, &QComboBox::currentIndexChanged, this, [this](int) { loadControls(); startPreview(); });
    connect(refreshControlsButton, &QPushButton::clicked, this, &MainWindow::loadControls);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetDefaults);
    connect(controlButton, &QPushButton::clicked, controlPanel, &QWidget::setVisible);
    connect(closeButton, &QPushButton::clicked, controlPanel, &QWidget::hide);
    connect(changeButton, &QPushButton::clicked, this, [this] {
        if (devicePicker->count() > 1)
            devicePicker->setCurrentIndex((devicePicker->currentIndex() + 1) % devicePicker->count());
    });
    connect(recordButton, &QPushButton::clicked, this, &MainWindow::toggleRecording);
    connect(photoButton, &QPushButton::clicked, this, &MainWindow::capturePhoto);
    connect(timelapseButton, &QPushButton::clicked, this, [this] {
        recording ? stopTimelapse() : startTimelapse();
    });
    connect(modePicker, &QComboBox::currentTextChanged, this, [this](const QString &mode) {
        const bool time = mode == "Timelapse";
        recordButton->setVisible(!time);
        photoButton->setVisible(mode == "Camera");
        timelapseButton->setVisible(time);
    });
    previewProcess = new QProcess(this);
    captureProcess = new QProcess(this);
    recordTimer = new QTimer(this);
    recordTimer->setInterval(1000);
    connect(recordTimer, &QTimer::timeout, this, &MainWindow::updateTimer);
    connect(previewProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::updatePreview);
    connect(captureProcess, qOverload<int, QProcess::ExitStatus>(&QProcess::finished),
            this, &MainWindow::finishCaptureProcess);
}

bool MainWindow::runV4l2(const QStringList &arguments, QString *output, QString *error) const
{
    if (v4l2Executable.isEmpty()) {
        *error = "Could not find v4l2-ctl. Install the v4l-utils package and try again.";
        return false;
    }

    QProcess process;
    process.start(v4l2Executable, arguments);
    if (!process.waitForStarted(3000)) {
        *error = process.errorString();
        return false;
    }
    if (!process.waitForFinished(10000)) {
        process.kill();
        process.waitForFinished();
        *error = "v4l2-ctl timed out while communicating with the camera.";
        return false;
    }

    *output = QString::fromLocal8Bit(process.readAllStandardOutput());
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        *error = QString::fromLocal8Bit(process.readAllStandardError()).trimmed();
        if (error->isEmpty())
            *error = "v4l2-ctl could not complete the request.";
        return false;
    }
    return true;
}

void MainWindow::scanDevices()
{
    QString output;
    QString error;
    if (!runV4l2({"--list-devices"}, &output, &error)) {
        stopPreview();
        devicePicker->clear();
        cameraSummary->setText("No camera selected");
        setStatus(error, true);
        populateControls({});
        return;
    }

    const QString previousDevice = selectedDevice();
    devicePicker->blockSignals(true);
    devicePicker->clear();

    QString currentName;
    const QRegularExpression deviceLine(R"(^\s*(/dev/video\d+)\s*$)");
    const QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty())
            continue;
        const auto match = deviceLine.match(line);
        if (match.hasMatch()) {
            const QString path = match.captured(1);
            devicePicker->addItem(QString("%1  |  %2").arg(currentName, path), path);
        } else if (trimmed.endsWith(':')) {
            currentName = trimmed.left(trimmed.size() - 1);
        }
    }

    const int previousIndex = devicePicker->findData(previousDevice);
    if (previousIndex >= 0)
        devicePicker->setCurrentIndex(previousIndex);
    devicePicker->blockSignals(false);

    if (devicePicker->count() == 0) {
        stopPreview();
        cameraSummary->setText("No camera detected");
        populateControls({});
        setStatus("No V4L2 video devices were found.");
        return;
    }

    loadControls();
    startPreview();
}

QString MainWindow::selectedDevice() const
{
    return devicePicker->currentData().toString();
}

void MainWindow::loadControls()
{
    const QString device = selectedDevice();
    if (device.isEmpty()) {
        populateControls({});
        return;
    }

    const QString cameraLabel = devicePicker->currentText();
    cameraSummary->setText(cameraLabel.section("  |  ", 1, 1));

    QString output;
    QString error;
    if (!runV4l2({"--device=" + device, "--list-ctrls-menus"}, &output, &error)) {
        populateControls({});
        setStatus(error, true);
        return;
    }

    QList<CameraControl> controls;
    QString category = "Camera Controls";
    int currentControl = -1;
    const QRegularExpression controlPattern(R"(^\s*(\S+)\s+0x[0-9a-fA-F]+\s+\(([^)]+)\)\s*:\s*(.*)$)");
    const QRegularExpression propertyPattern(R"((min|max|step|default|value)=(-?\d+))");
    const QRegularExpression flagsPattern(R"(flags=([^\s]+))");
    const QRegularExpression menuPattern(R"(^\s*(\d+):\s*(.*)$)");

    for (const QString &line : output.split('\n')) {
        const QString trimmed = line.trimmed();
        const auto match = controlPattern.match(line);
        if (match.hasMatch()) {
            CameraControl control;
            control.name = match.captured(1);
            control.type = match.captured(2);
            control.category = category;
            const QString properties = match.captured(3);

            auto iterator = propertyPattern.globalMatch(properties);
            while (iterator.hasNext()) {
                const auto property = iterator.next();
                const qlonglong value = property.captured(2).toLongLong();
                const QString key = property.captured(1);
                if (key == "min") {
                    control.minimum = value;
                    control.hasRange = true;
                } else if (key == "max") {
                    control.maximum = value;
                    control.hasRange = true;
                } else if (key == "step") {
                    control.step = qMax<qlonglong>(1, value);
                } else if (key == "default") {
                    control.defaultValue = value;
                } else if (key == "value") {
                    control.value = value;
                }
            }
            const auto flagsMatch = flagsPattern.match(properties);
            if (flagsMatch.hasMatch())
                control.flags = flagsMatch.captured(1);

            controls.append(control);
            currentControl = controls.size() - 1;
            continue;
        }

        if (trimmed.endsWith("Controls")) {
            category = trimmed;
            currentControl = -1;
            continue;
        }

        if (currentControl >= 0) {
            const auto menuMatch = menuPattern.match(line);
            if (menuMatch.hasMatch())
                controls[currentControl].menuItems.insert(menuMatch.captured(1).toLongLong(), menuMatch.captured(2).trimmed());
        }
    }

    populateControls(controls);
    setStatus(QString("%1 controls loaded from %2.").arg(controls.size()).arg(device));
}

void MainWindow::populateControls(const QList<CameraControl> &controls)
{
    while (QLayoutItem *item = controlLayout->takeAt(0)) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    if (controls.isEmpty()) {
        emptyState = new QLabel(devicePicker->count() == 0
                                    ? "Connect a camera to see its V4L2 controls here."
                                    : "This device did not report any available controls.",
                                controlHost);
        emptyState->setObjectName("emptyState");
        emptyState->setWordWrap(true);
        controlLayout->addWidget(emptyState);
        controlLayout->addStretch();
        return;
    }

    QString activeCategory;
    for (const CameraControl &control : controls) {
        if (control.category != activeCategory) {
            activeCategory = control.category;
            auto *section = new QLabel(activeCategory.toUpper(), controlHost);
            section->setObjectName("eyebrow");
            section->setContentsMargins(0, 8, 0, 4);
            controlLayout->addWidget(section);
        }

        const bool readOnly = flagText(control.flags, "read-only");
        const bool disabled = flagText(control.flags, "disabled")
            || flagText(control.flags, "inactive") || readOnly;
        auto *row = new QWidget(controlHost);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 4, 0, 4);
        rowLayout->setSpacing(12);

        auto *nameLabel = new QLabel(displayName(control.name), row);
        nameLabel->setMinimumWidth(160);
        nameLabel->setStyleSheet("font-weight:600;");
        rowLayout->addWidget(nameLabel);

        auto *editorHost = new QWidget(row);
        auto *editorLayout = new QHBoxLayout(editorHost);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(9);

        if (control.type == "bool") {
            auto *checkBox = new QCheckBox(editorHost);
            checkBox->setChecked(control.value != 0);
            checkBox->setEnabled(!disabled);
            connect(checkBox, &QCheckBox::toggled, this, [this, name = control.name](bool checked) {
                applyControl(name, checked ? "1" : "0");
            });
            editorLayout->addWidget(checkBox);
            editorLayout->addStretch();
        } else if (control.type == "menu" && !control.menuItems.isEmpty()) {
            auto *combo = new QComboBox(editorHost);
            combo->setMinimumWidth(190);
            for (auto it = control.menuItems.cbegin(); it != control.menuItems.cend(); ++it)
                combo->addItem(it.value(), it.key());
            const int index = combo->findData(control.value);
            if (index >= 0)
                combo->setCurrentIndex(index);
            combo->setEnabled(!disabled);
            connect(combo, &QComboBox::currentIndexChanged, this, [this, combo, name = control.name](int index) {
                if (index >= 0)
                    applyControl(name, combo->currentData().toString());
            });
            editorLayout->addWidget(combo);
            editorLayout->addStretch();
        } else if (control.hasRange && control.minimum <= control.maximum
                   && control.minimum >= std::numeric_limits<int>::min()
                   && control.maximum <= std::numeric_limits<int>::max()) {
            auto *spin = new QSpinBox(editorHost);
            spin->setRange(static_cast<int>(control.minimum), static_cast<int>(control.maximum));
            spin->setSingleStep(static_cast<int>(qMin<qlonglong>(control.step, std::numeric_limits<int>::max())));
            spin->setValue(static_cast<int>(qBound(control.minimum, control.value, control.maximum)));
            spin->setEnabled(!disabled);
            editorLayout->addWidget(spin);

            const qlonglong span = control.maximum - control.minimum;
            if (span <= 50000) {
                auto *slider = new QSlider(Qt::Horizontal, editorHost);
                slider->setRange(static_cast<int>(control.minimum), static_cast<int>(control.maximum));
                slider->setSingleStep(static_cast<int>(qMin<qlonglong>(control.step, std::numeric_limits<int>::max())));
                slider->setPageStep(static_cast<int>(qMax<qlonglong>(control.step, span / 10)));
                slider->setValue(spin->value());
                slider->setEnabled(!disabled);
                editorLayout->addWidget(slider, 1);
                connect(slider, &QSlider::valueChanged, spin, &QSpinBox::setValue);
                connect(spin, qOverload<int>(&QSpinBox::valueChanged), slider, &QSlider::setValue);
                connect(slider, &QSlider::sliderReleased, this, [this, spin, name = control.name]() {
                    applyControl(name, QString::number(spin->value()));
                });
            } else {
                editorLayout->addStretch();
            }
            connect(spin, &QSpinBox::editingFinished, this, [this, spin, name = control.name]() {
                applyControl(name, QString::number(spin->value()));
            });
        } else {
            auto *editor = new QLineEdit(QString::number(control.value), editorHost);
            editor->setEnabled(!disabled);
            editorLayout->addWidget(editor, 1);
            auto *applyButton = new QPushButton("Apply", editorHost);
            applyButton->setEnabled(!disabled);
            editorLayout->addWidget(applyButton);
            connect(applyButton, &QPushButton::clicked, this, [this, editor, name = control.name]() {
                applyControl(name, editor->text());
            });
            connect(editor, &QLineEdit::returnPressed, applyButton, &QPushButton::click);
        }

        rowLayout->addWidget(editorHost, 1);
        controlLayout->addWidget(row);
        auto *divider = new QFrame(controlHost);
        divider->setProperty("role", "divider");
        divider->setFrameShape(QFrame::HLine);
        controlLayout->addWidget(divider);
    }
    controlLayout->addStretch();
}

void MainWindow::applyControl(const QString &name, const QString &value)
{
    const QString device = selectedDevice();
    if (device.isEmpty())
        return;

    QString output;
    QString error;
    if (!runV4l2({"--device=" + device, "--set-ctrl=" + name + "=" + value}, &output, &error)) {
        setStatus(QString("Could not set %1: %2").arg(displayName(name), error), true);
        return;
    }
    setStatus(QString("%1 set to %2.").arg(displayName(name), value));
}

void MainWindow::resetDefaults()
{
    const QString device = selectedDevice();
    if (device.isEmpty())
        return;

    QString output;
    QString error;
    if (!runV4l2({"--device=" + device, "--list-ctrls-menus"}, &output, &error)) {
        setStatus(error, true);
        return;
    }

    const QRegularExpression controlPattern(R"(^\s*(\S+)\s+0x[0-9a-fA-F]+\s+\(([^)]+)\)\s*:\s*(.*)$)");
    const QRegularExpression defaultPattern(R"(default=(-?\d+))");
    QStringList defaults;
    for (const QString &line : output.split('\n')) {
        const auto match = controlPattern.match(line);
        if (!match.hasMatch())
            continue;
        const auto defaultMatch = defaultPattern.match(match.captured(3));
        const QString flags = match.captured(3).section("flags=", 1);
        if (defaultMatch.hasMatch() && !flagText(flags, "read-only")
            && !flagText(flags, "disabled") && !flagText(flags, "inactive"))
            defaults.append(match.captured(1) + "=" + defaultMatch.captured(1));
    }

    if (defaults.isEmpty()) {
        setStatus("No resettable defaults were reported for this camera.");
        return;
    }
    if (!runV4l2({"--device=" + device, "--set-ctrl=" + defaults.join(',')}, &output, &error)) {
        setStatus(QString("Could not reset camera controls: %1").arg(error), true);
        return;
    }

    loadControls();
    setStatus(QString("Reset %1 controls to their reported defaults.").arg(defaults.size()));
}

void MainWindow::setStatus(const QString &message, bool isError)
{
    statusLabel->setText(message);
    statusLabel->setProperty("error", isError ? "true" : "false");
    statusLabel->style()->unpolish(statusLabel);
    statusLabel->style()->polish(statusLabel);
}

void MainWindow::startPreview()
{
    stopPreview();
    const QString device = selectedDevice();
    if (device.isEmpty() || ffmpegExecutable.isEmpty())
        return;
    previewBuffer.clear();
    previewProcess->start(ffmpegExecutable, {"-hide_banner", "-loglevel", "error", "-f", "v4l2",
                                              "-i", device, "-f", "mjpeg", "-q:v", "6", "pipe:1"});
    if (!previewProcess->waitForStarted(2000))
        setStatus("Could not start FFmpeg preview: " + previewProcess->errorString(), true);
}

void MainWindow::stopPreview()
{
    if (!previewProcess || previewProcess->state() == QProcess::NotRunning)
        return;
    previewProcess->terminate();
    if (!previewProcess->waitForFinished(1000)) {
        previewProcess->kill();
        previewProcess->waitForFinished();
    }
}

void MainWindow::pausePreview()
{
    stopPreview();
    if (previewLabel->pixmap().isNull()) {
        previewLabel->setText("Camera preview paused");
        return;
    }

    if (!previewLabel->graphicsEffect()) {
        auto *blur = new QGraphicsBlurEffect;
        blur->setBlurRadius(12);
        previewLabel->setGraphicsEffect(blur);
    }
}

void MainWindow::resumePreview()
{
    if (previewLabel->graphicsEffect())
        previewLabel->setGraphicsEffect(nullptr);
    if (previewLabel->pixmap().isNull())
        previewLabel->setText("Reconnecting to camera preview...");
    startPreview();
}

void MainWindow::updatePreview()
{
    previewBuffer.append(previewProcess->readAllStandardOutput());
    while (true) {
        const int start = previewBuffer.indexOf("\xff\xd8");
        if (start < 0) {
            if (previewBuffer.size() > 1024 * 1024)
                previewBuffer.clear();
            return;
        }
        const int end = previewBuffer.indexOf("\xff\xd9", start + 2);
        if (end < 0) {
            if (start > 0)
                previewBuffer.remove(0, start);
            return;
        }
        const QByteArray jpeg = previewBuffer.mid(start, end - start + 2);
        previewBuffer.remove(0, end + 2);
        QImage image;
        if (image.loadFromData(jpeg, "JPEG"))
            previewLabel->setPixmap(QPixmap::fromImage(image).scaled(previewLabel->size(),
                                                                       Qt::KeepAspectRatio,
                                                                       Qt::SmoothTransformation));
    }
}

QString MainWindow::outputDirectory(const QString &category, QString *error) const
{
    const QSettings settings;
    const QString configuredDirectory = settings.value("capture/defaultLocation").toString();
    const QString directory = configuredDirectory.isEmpty()
        ? QDir::home().filePath("kamkontroller/" + category)
        : configuredDirectory;
    if (!QDir().mkpath(directory) || !QDir(directory).exists()) {
        if (error)
            *error = "Could not create output directory: " + directory;
        return {};
    }
    if (!QFileInfo(directory).isWritable()) {
        if (error)
            *error = "Output directory is not writable: " + directory;
        return {};
    }
    return QDir::cleanPath(directory);
}

void MainWindow::chooseDefaultLocation()
{
    QSettings settings;
    const QString configuredDirectory = settings.value("capture/defaultLocation").toString();
    const QString initialDirectory = configuredDirectory.isEmpty()
        ? QDir::home().filePath("kamkontroller")
        : configuredDirectory;
    const QString selected = QFileDialog::getExistingDirectory(
        this, "Choose default save location", initialDirectory,
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (selected.isEmpty())
        return;

    settings.setValue("capture/defaultLocation", QDir::cleanPath(selected));
    setStatus("Default save location set to " + QDir::cleanPath(selected));
}

QString MainWindow::nextOutputPath(const QString &extension, const QString &category, QString *error) const
{
    const QString directory = outputDirectory(category, error);
    if (directory.isEmpty())
        return {};
    return QDir(directory).filePath(
        QString("kamkontroller_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"), extension));
}

void MainWindow::toggleRecording()
{
    if (recording) {
        captureProcess->terminate();
        return;
    }
    const QString device = selectedDevice();
    if (device.isEmpty()) {
        setStatus("Select a camera before recording.", true);
        recordButton->setChecked(false);
        return;
    }
    if (captureProcess->state() != QProcess::NotRunning) {
        setStatus("Wait for the current capture to finish before recording.", true);
        recordButton->setChecked(false);
        return;
    }
    if (ffmpegExecutable.isEmpty()) {
        setStatus("Could not find ffmpeg. Install FFmpeg and try again.", true);
        recordButton->setChecked(false);
        return;
    }
    QString pathError;
    activeOutputPath = nextOutputPath("mp4", "videos", &pathError);
    if (activeOutputPath.isEmpty()) {
        setStatus(pathError, true);
        recordButton->setChecked(false);
        return;
    }
    pausePreview();
    captureProcess->start(ffmpegExecutable, {"-hide_banner", "-loglevel", "error", "-f", "v4l2",
                                              "-i", device, "-c:v", "libx264", "-preset", "ultrafast",
                                              "-pix_fmt", "yuv420p", activeOutputPath});
    if (!captureProcess->waitForStarted(2000)) {
        setStatus("Could not start recording: " + captureProcess->errorString(), true);
        recordButton->setChecked(false);
        resumePreview();
        return;
    }
    recording = true;
    recordingStarted = QDateTime::currentDateTime();
    modePicker->setEnabled(false);
    recordTimer->start();
    timerLabel->show();
    recordButton->setText("Stop recording");
    setStatus("Recording to " + activeOutputPath);
}

void MainWindow::capturePhoto()
{
    if (captureProcess->state() != QProcess::NotRunning)
        return;
    const QString device = selectedDevice();
    if (device.isEmpty() || ffmpegExecutable.isEmpty()) {
        setStatus(ffmpegExecutable.isEmpty() ? "Could not find ffmpeg." : "Select a camera first.", true);
        return;
    }
    QString pathError;
    activeOutputPath = nextOutputPath("jpg", "pictures", &pathError);
    if (activeOutputPath.isEmpty()) {
        setStatus(pathError, true);
        return;
    }
    pausePreview();
    captureProcess->start(ffmpegExecutable, {"-hide_banner", "-loglevel", "error", "-f", "v4l2",
                                              "-i", device, "-frames:v", "1", "-y", activeOutputPath});
    if (!captureProcess->waitForStarted(2000)) {
        setStatus("Could not capture photo: " + captureProcess->errorString(), true);
        if (!(recording && modePicker->currentText() == "Timelapse"))
            resumePreview();
    }
}

void MainWindow::startTimelapse()
{
    const QString device = selectedDevice();
    if (device.isEmpty()) {
        setStatus("Select a camera before starting a timelapse.", true);
        return;
    }
    if (ffmpegExecutable.isEmpty()) {
        setStatus("Could not find ffmpeg. Install FFmpeg and try again.", true);
        return;
    }
    if (captureProcess->state() != QProcess::NotRunning) {
        setStatus("Wait for the current capture to finish before starting a timelapse.", true);
        return;
    }
    QString pathError;
    activeOutputPath = nextOutputPath("mp4", "timelapse", &pathError);
    if (activeOutputPath.isEmpty()) {
        setStatus(pathError, true);
        return;
    }
    pausePreview();
    captureProcess->start(ffmpegExecutable, {"-f", "v4l2", "-i", device,
                                              "-vf", "fps=24,setpts=N/1920/TB",
                                              "-c:v", "libx264", "-preset", "fast",
                                              "-crf", "23", "-t", "75600", activeOutputPath});
    if (!captureProcess->waitForStarted(2000)) {
        setStatus("Could not start timelapse: " + captureProcess->errorString(), true);
        resumePreview();
        return;
    }
    recording = true;
    recordingStarted = QDateTime::currentDateTime();
    modePicker->setEnabled(false);
    recordTimer->start();
    timerLabel->show();
    timelapseButton->setText("Stop timelapse");
    setStatus("Recording timelapse to " + activeOutputPath);
}

void MainWindow::stopTimelapse()
{
    captureProcess->terminate();
    setStatus("Stopping timelapse...");
}

void MainWindow::updateTimer()
{
    if (!recordingStarted.isValid())
        return;
    const qint64 seconds = recordingStarted.secsTo(QDateTime::currentDateTime());
    timerLabel->setText(QTime(0, 0).addSecs(static_cast<int>(seconds)).toString("hh:mm:ss"));
}

void MainWindow::finishCaptureProcess(int exitCode, QProcess::ExitStatus status)
{
    const bool wasVideoRecording = recording && modePicker->currentText() == "Video";
    const bool wasTimelapse = recording && modePicker->currentText() == "Timelapse";
    if (wasVideoRecording || wasTimelapse) {
        recording = false;
        recordTimer->stop();
        timerLabel->hide();
        modePicker->setEnabled(true);
        if (wasVideoRecording) {
            recordButton->setChecked(false);
            recordButton->setText("Start recording");
        }
        if (wasTimelapse)
            timelapseButton->setText("Start timelapse");
    }
    if (wasVideoRecording || wasTimelapse || !recording)
        resumePreview();
    if (status != QProcess::NormalExit || exitCode != 0) {
        setStatus("FFmpeg capture failed: " + QString::fromLocal8Bit(captureProcess->readAllStandardError()).trimmed(), true);
        return;
    }
    if (!activeOutputPath.isEmpty())
        setStatus("Saved " + activeOutputPath);
}