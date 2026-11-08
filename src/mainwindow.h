#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QProcess>
#include <QStringList>

class QComboBox;
class QCheckBox;
class QDialog;
class QLabel;
class QMouseEvent;
class QPlainTextEdit;
class QPushButton;
class QScrollArea;
class QToolButton;
class QTimer;
class QVBoxLayout;

struct CameraControl
{
    QString name;
    QString type;
    QString category;
    QString flags;
    qlonglong minimum = 0;
    qlonglong maximum = 0;
    qlonglong step = 1;
    qlonglong defaultValue = 0;
    qlonglong value = 0;
    bool hasRange = false;
    QMap<qlonglong, QString> menuItems;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private:
    void buildInterface();
    void scanDevices();
    void loadControls();
    void populateControls(const QList<CameraControl> &controls);
    bool runV4l2(const QStringList &arguments, QString *output, QString *error) const;
    QString selectedDevice() const;
    void setStatus(const QString &message, bool isError = false);
    void applyControl(const QString &name, const QString &value);
    void resetDefaults();
    void startPreview();
    void stopPreview();
    void pausePreview();
    void resumePreview();
    void toggleRecording();
    void capturePhoto();
    void startTimelapse();
    void stopTimelapse();
    void updatePreview();
    void updateTimer();
    void finishCaptureProcess(int exitCode, QProcess::ExitStatus status);
    void appendCaptureLog(const QByteArray &output);
    void requestCaptureStop();
    void chooseDefaultLocation();
    QString outputDirectory(const QString &category, QString *error = nullptr) const;
    QString nextOutputPath(const QString &extension, const QString &category, QString *error = nullptr) const;

    QComboBox *devicePicker = nullptr;
    QLabel *cameraSummary = nullptr;
    QLabel *previewLabel = nullptr;
    QLabel *timerLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QDialog *logsDialog = nullptr;
    QPlainTextEdit *logsView = nullptr;
    QLabel *emptyState = nullptr;
    QScrollArea *controlScroll = nullptr;
    QWidget *controlHost = nullptr;
    QVBoxLayout *controlLayout = nullptr;
    QWidget *controlPanel = nullptr;
    QComboBox *modePicker = nullptr;
    QPushButton *recordButton = nullptr;
    QPushButton *photoButton = nullptr;
    QPushButton *timelapseButton = nullptr;
    QProcess *previewProcess = nullptr;
    QProcess *captureProcess = nullptr;
    QTimer *recordTimer = nullptr;
    QByteArray previewBuffer;
    QString activeOutputPath;
    QDateTime recordingStarted;
    bool recording = false;
    QString v4l2Executable;
    QString ffmpegExecutable;
};