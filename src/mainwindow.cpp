#include "mainwindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QProcess>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>
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
{
    buildInterface();
    scanDevices();
}

void MainWindow::buildInterface()
{
    setWindowTitle("KamKontroller | Camera controls");
    setMinimumSize(820, 560);
    resize(1120, 760);
    setStyleSheet(R"(
        QWidget { color: #202723; font-family: "Noto Sans", "DejaVu Sans", sans-serif; font-size: 13px; }
        QMainWindow, #mainPane { background: #f4f5f1; }
        #sidebar { background: #202b27; }
        #sidebar QLabel { color: #edf1e9; }
        #brand { color: #d9ed77; font-size: 15px; font-weight: 700; }
        #eyebrow { color: #77827b; font-size: 10px; font-weight: 700; letter-spacing: 1px; }
        #sidebar #eyebrow { color: #a1b0a5; }
        #pageTitle { color: #202723; font-size: 27px; font-weight: 700; }
        #cameraName { color: #202723; font-size: 15px; font-weight: 700; }
        #cameraPath { color: #77827b; font-family: monospace; font-size: 12px; }
        #sectionTitle { color: #35423b; font-size: 13px; font-weight: 700; }
        QComboBox, QSpinBox, QLineEdit {
            background: #ffffff; border: 1px solid #d9dfd8; border-radius: 4px;
            padding: 7px 9px; min-height: 19px;
        }
        QComboBox:focus, QSpinBox:focus, QLineEdit:focus { border-color: #788c40; }
        #sidebar QComboBox { background: #303c36; color: #f2f4ef; border-color: #4a5850; }
        QComboBox::drop-down { border: 0; width: 25px; }
        QPushButton {
            background: #e2ebbd; color: #293321; border: 1px solid #cfdda0;
            border-radius: 4px; padding: 8px 12px; font-weight: 700;
        }
        QPushButton:hover { background: #d4e398; }
        QPushButton:disabled { color: #8b938c; background: #e7eae5; border-color: #dce0da; }
        #sidebar QPushButton { background: #303c36; border-color: #4a5850; color: #edf1e9; }
        #sidebar QPushButton:hover { background: #3a4941; }
        QCheckBox { spacing: 8px; }
        QCheckBox::indicator { width: 17px; height: 17px; }
        QCheckBox::indicator:checked { background: #829c42; border: 1px solid #829c42; border-radius: 3px; }
        QSlider::groove:horizontal { height: 4px; background: #dce1d8; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #829c42; border-radius: 2px; }
        QSlider::handle:horizontal { background: #ffffff; border: 2px solid #829c42; width: 13px; height: 13px; margin: -6px 0; border-radius: 7px; }
        QScrollArea { background: transparent; border: 0; }
        QScrollBar:vertical { background: transparent; width: 9px; margin: 0; }
        QScrollBar::handle:vertical { background: #cbd2c8; border-radius: 4px; min-height: 25px; }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        #status { color: #657168; padding: 5px 0; }
        #status[error="true"] { color: #a13c30; }
        #emptyState { color: #69746c; font-size: 14px; padding: 28px 4px; }
        QFrame[role="divider"] { background: #e2e6df; max-height: 1px; }
    )");

    auto *root = new QWidget(this);
    auto *rootLayout = new QHBoxLayout(root);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto *sidebar = new QWidget(root);
    sidebar->setObjectName("sidebar");
    sidebar->setFixedWidth(252);
    auto *sideLayout = new QVBoxLayout(sidebar);
    sideLayout->setContentsMargins(22, 25, 22, 22);
    sideLayout->setSpacing(13);

    auto *brand = new QLabel("KAM / KONTROLLER", sidebar);
    brand->setObjectName("brand");
    sideLayout->addWidget(brand);
    sideLayout->addSpacing(32);
    sideLayout->addWidget(makeEyebrow("CONNECTED CAMERAS", sidebar));

    devicePicker = new QComboBox(sidebar);
    devicePicker->setMinimumWidth(200);
    sideLayout->addWidget(devicePicker);

    auto *refreshButton = new QPushButton("Scan cameras", sidebar);
    sideLayout->addWidget(refreshButton);
    sideLayout->addSpacing(20);
    sideLayout->addWidget(makeEyebrow("DEVICE", sidebar));
    cameraSummary = new QLabel("No camera selected", sidebar);
    cameraSummary->setObjectName("cameraName");
    cameraSummary->setWordWrap(true);
    sideLayout->addWidget(cameraSummary);
    auto *pathHint = new QLabel("Control values are read from the selected V4L2 device.", sidebar);
    pathHint->setWordWrap(true);
    pathHint->setStyleSheet("color:#a1b0a5; line-height:1.35;");
    sideLayout->addWidget(pathHint);
    sideLayout->addStretch();

    auto *utilityLabel = new QLabel(v4l2Executable.isEmpty()
                                        ? "v4l2-ctl not found"
                                        : QString("v4l2-ctl  %1").arg(QFileInfo(v4l2Executable).fileName()),
                                    sidebar);
    utilityLabel->setStyleSheet("color:#a1b0a5; font-family:monospace; font-size:11px;");
    sideLayout->addWidget(utilityLabel);

    auto *mainPane = new QWidget(root);
    mainPane->setObjectName("mainPane");
    auto *mainLayout = new QVBoxLayout(mainPane);
    mainLayout->setContentsMargins(38, 31, 38, 20);
    mainLayout->setSpacing(12);

    auto *topRow = new QHBoxLayout;
    auto *headingLayout = new QVBoxLayout;
    headingLayout->setSpacing(5);
    headingLayout->addWidget(makeEyebrow("VIDEO INPUT  /  DEVICE SETTINGS", mainPane));
    auto *title = new QLabel("Camera controls", mainPane);
    title->setObjectName("pageTitle");
    headingLayout->addWidget(title);
    topRow->addLayout(headingLayout);
    topRow->addStretch();
    auto *resetButton = new QPushButton("Reset defaults", mainPane);
    topRow->addWidget(resetButton, 0, Qt::AlignVCenter);
    mainLayout->addLayout(topRow);
    mainLayout->addSpacing(10);

    controlScroll = new QScrollArea(mainPane);
    controlScroll->setWidgetResizable(true);
    controlHost = new QWidget;
    controlLayout = new QVBoxLayout(controlHost);
    controlLayout->setContentsMargins(0, 0, 8, 0);
    controlLayout->setSpacing(0);
    emptyState = new QLabel("Select a camera to read its available controls.", controlHost);
    emptyState->setObjectName("emptyState");
    emptyState->setWordWrap(true);
    controlLayout->addWidget(emptyState);
    controlScroll->setWidget(controlHost);
    mainLayout->addWidget(controlScroll, 1);

    auto *footer = new QHBoxLayout;
    statusLabel = new QLabel("Ready", mainPane);
    statusLabel->setObjectName("status");
    footer->addWidget(statusLabel);
    footer->addStretch();
    auto *refreshControlsButton = new QPushButton("Refresh controls", mainPane);
    footer->addWidget(refreshControlsButton);
    mainLayout->addLayout(footer);

    rootLayout->addWidget(sidebar);
    rootLayout->addWidget(mainPane, 1);
    setCentralWidget(root);

    connect(refreshButton, &QPushButton::clicked, this, &MainWindow::scanDevices);
    connect(devicePicker, &QComboBox::currentIndexChanged, this, [this](int) { loadControls(); });
    connect(refreshControlsButton, &QPushButton::clicked, this, &MainWindow::loadControls);
    connect(resetButton, &QPushButton::clicked, this, &MainWindow::resetDefaults);
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
        cameraSummary->setText("No camera detected");
        populateControls({});
        setStatus("No V4L2 video devices were found.");
        return;
    }

    loadControls();
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
    cameraSummary->setText(cameraLabel.section("  |  ", 0, 0));

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
            section->setContentsMargins(0, 12, 0, 8);
            controlLayout->addWidget(section);
        }

        const bool readOnly = flagText(control.flags, "read-only");
        const bool disabled = flagText(control.flags, "disabled")
            || flagText(control.flags, "inactive") || readOnly;
        auto *row = new QWidget(controlHost);
        auto *rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 8, 0, 8);
        rowLayout->setSpacing(18);

        auto *nameLabel = new QLabel(displayName(control.name), row);
        nameLabel->setMinimumWidth(190);
        nameLabel->setStyleSheet("font-weight:600;");
        rowLayout->addWidget(nameLabel);

        auto *editorHost = new QWidget(row);
        auto *editorLayout = new QHBoxLayout(editorHost);
        editorLayout->setContentsMargins(0, 0, 0, 0);
        editorLayout->setSpacing(12);

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