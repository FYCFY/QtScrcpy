#ifndef AUTOCASTCONTROLLER_H
#define AUTOCASTCONTROLLER_H

#include <QObject>
#include <QHash>
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
    quint16 resolveMaxSize() const;
    QString resolveRecordFormat() const;
    qsc::DeviceParams buildParams(const QString &serial);

    qsc::AdbProcess m_adb;
    QTimer m_pollTimer;
    QSet<QString> m_castingSerials;
    QHash<QString, VideoForm*> m_videoForms;
    UserBootConfig m_bootConfig;
    bool m_lastDeviceListEmpty = true;
};

#endif // AUTOCASTCONTROLLER_H
