#pragma once

#include <QMainWindow>
#include <QMap>
#include <QStringList>

class QComboBox;
class QLabel;
class QScrollArea;
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

    QComboBox *devicePicker = nullptr;
    QLabel *cameraSummary = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *emptyState = nullptr;
    QScrollArea *controlScroll = nullptr;
    QWidget *controlHost = nullptr;
    QVBoxLayout *controlLayout = nullptr;
    QString v4l2Executable;
};