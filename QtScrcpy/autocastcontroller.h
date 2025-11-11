#ifndef AUTOCASTCONTROLLER_H
#define AUTOCASTCONTROLLER_H

#include <QObject>
#include <QHash>
#include <QMap>
#include <QSet>
#include <QSize>
#include <QTimer>
#include <QStringList>

#include "config.h"
#include "adbprocess.h"
#include "../QtScrcpyCore/include/QtScrcpyCore.h"

class VideoForm;

class AutoCastController : public QObject
{
    Q_OBJECT
public:
    explicit AutoCastController(QObject *parent = nullptr);
    void start();

signals:
    void logMessage(const QString &message);
    void activeDevicesChanged(const QStringList &devices);
    void deviceInfoReady(const QString &serial, const QStringList &info);
    void deviceStatusChanged(const QMap<QString, QString> &statusMap);

private slots:
    void queryDevices();
    void handleAdbResult(qsc::AdbProcess::ADB_EXEC_RESULT result);
    void onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size);
    void onDeviceDisconnected(const QString &serial);

private:
    void startCasting(const QString &serial);
    void stopCasting(const QString &serial);
    void cleanupVideoForm(const QString &serial);
    void ensureVideoForm(const QString &serial, const QSize &size);
    void updateActiveDeviceList(const QSet<QString> &serials);
    void emitActiveDeviceSnapshot();
    void emitStatusSnapshot();
    QMap<QString, QString> readAdbDeviceStatuses();
    QStringList collectDeviceInfo(const QString &serial);
    QStringList collectFastbootInfo(const QString &serial);
    enum class FastbootMode {
        Bootloader,
        Userspace
    };
    void refreshFastbootSnapshot();
    FastbootMode detectFastbootMode(const QString &serial, const QString &transportHint) const;
    QString runAdbCommandSync(const QStringList &args, int timeoutMs = 3000) const;
    QString runFastbootCommandSync(const QStringList &args, int timeoutMs = 3000) const;
    QString readDeviceProperty(const QString &serial, const QString &prop);
    QString readFastbootVariable(const QString &serial, const QString &prop) const;
    quint16 resolveMaxSize() const;
    QString resolveRecordFormat() const;
    qsc::DeviceParams buildParams(const QString &serial);

    qsc::AdbProcess m_adb;
    QTimer m_pollTimer;
    QSet<QString> m_castingSerials;
    QHash<QString, VideoForm*> m_videoForms;
    QSet<QString> m_lastAdbSerials;
    QSet<QString> m_fastbootSerials;
    QMap<QString, QString> m_lastAdbStatuses;
    QMap<QString, QString> m_lastFastbootStatuses;
    UserBootConfig m_bootConfig;
    bool m_lastDeviceListEmpty = true;
};

#endif // AUTOCASTCONTROLLER_H
