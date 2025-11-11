#include "autocastcontroller.h"

#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <QRandomGenerator>
#include <QRect>
#include <QStringList>
#include <QTimer>
#include <algorithm>

#include "serverpathresolver.h"
#include "util/adbpathresolver.h"
#include "videoform.h"

// Auto detection mirrors the behaviour from scrcpy_FYC/scrcpy.c (periodic adb polling
// and hands-free session startup) so contributors can bundle a panel-free build.

AutoCastController::AutoCastController(QObject *parent)
    : QObject(parent)
    , m_bootConfig(Config::getInstance().getUserBootConfig())
{
    connect(&m_adb, &qsc::AdbProcess::adbProcessResult, this, &AutoCastController::handleAdbResult);
    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceConnected,
            this, &AutoCastController::onDeviceConnected);
    connect(&qsc::IDeviceManage::getInstance(), &qsc::IDeviceManage::deviceDisconnected,
            this, &AutoCastController::onDeviceDisconnected);

    m_pollTimer.setInterval(2000);
    connect(&m_pollTimer, &QTimer::timeout, this, &AutoCastController::queryDevices);
}

void AutoCastController::start()
{
    qInfo() << "Auto-cast mode enabled. Monitoring ADB devices...";
    emit logMessage(tr("Auto-cast watcher started."));
    emit logMessage(tr("Waiting for ADB devices..."));
    emit activeDevicesChanged(QStringList{});
    m_lastDeviceListEmpty = true;
    m_pollTimer.start();
    queryDevices();
}

void AutoCastController::queryDevices()
{
    if (m_adb.isRuning()) {
        return;
    }
    m_adb.execute("", QStringList() << "devices");
}

void AutoCastController::handleAdbResult(qsc::AdbProcess::ADB_EXEC_RESULT result)
{
    if (result != qsc::AdbProcess::AER_SUCCESS_EXEC) {
        return;
    }

    const QStringList args = m_adb.arguments();
    if (!args.contains("devices")) {
        return;
    }

    const QStringList devices = m_adb.getDevicesSerialFromStdOut();
    QSet<QString> current;
    for (const QString &serial : devices) {
        if (!serial.isEmpty()) {
            current.insert(serial);
        }
    }

    for (const QString &serial : current) {
        if (!m_castingSerials.contains(serial)) {
            startCasting(serial);
        }
    }

    const auto snapshot = m_castingSerials;
    for (const QString &serial : snapshot) {
        if (!current.contains(serial)) {
            stopCasting(serial);
        }
    }

    updateActiveDeviceList(current);
    emit deviceStatusChanged(readAdbDeviceStatuses());
}

void AutoCastController::startCasting(const QString &serial)
{
    if (serial.isEmpty()) {
        return;
    }

    qInfo() << "Auto-cast: starting mirror session for" << serial;
    emit logMessage(tr("Starting mirror session for %1").arg(serial));
    m_castingSerials.insert(serial);
    qsc::IDeviceManage::getInstance().connectDevice(buildParams(serial));
}

void AutoCastController::stopCasting(const QString &serial)
{
    if (serial.isEmpty()) {
        return;
    }

    qInfo() << "Auto-cast: stopping mirror session for" << serial;
    emit logMessage(tr("Stopping mirror session for %1").arg(serial));
    qsc::IDeviceManage::getInstance().disconnectDevice(serial);
    m_castingSerials.remove(serial);
    cleanupVideoForm(serial);
}

void AutoCastController::cleanupVideoForm(const QString &serial)
{
    auto form = m_videoForms.take(serial);
    if (!form) {
        return;
    }

    if (auto device = qsc::IDeviceManage::getInstance().getDevice(serial)) {
        device->deRegisterDeviceObserver(form);
    }

    form->close();
    form->deleteLater();
}

void AutoCastController::ensureVideoForm(const QString &serial, const QSize &size)
{
    VideoForm *form = m_videoForms.value(serial, nullptr);
    const bool needsInit = (form == nullptr);

    if (!form) {
        form = new VideoForm(m_bootConfig.framelessWindow, Config::getInstance().getSkin(), m_bootConfig.showToolbar);
        form->setSerial(serial);
        form->showFPS(m_bootConfig.showFPS);
        if (m_bootConfig.windowOnTop) {
            form->staysOnTop();
        }
        m_videoForms.insert(serial, form);
    }

    if (needsInit) {
        if (auto device = qsc::IDeviceManage::getInstance().getDevice(serial)) {
            device->setUserData(static_cast<void*>(form));
            device->registerDeviceObserver(form);
        }
    }

    QString name = Config::getInstance().getNickName(serial);
    if (name.isEmpty()) {
        name = Config::getInstance().getTitle();
    }
    form->setWindowTitle(name + "-" + serial);
    form->updateShowSize(size);

    const bool deviceVertical = size.height() > size.width();
    QRect stored = Config::getInstance().getRect(serial);
    const bool rectVertical = stored.height() > stored.width();
    if (stored.isValid() && (deviceVertical == rectVertical)) {
        form->resize(stored.size());
        form->setGeometry(stored);
    }

#ifdef Q_OS_WIN32
    QTimer::singleShot(200, form, [form]() { form->show(); });
#else
    if (needsInit) {
        form->show();
    }
#endif
}

void AutoCastController::updateActiveDeviceList(const QSet<QString> &serials)
{
    QStringList devices = serials.values();
    std::sort(devices.begin(), devices.end());
    emit activeDevicesChanged(devices);

    const bool emptyNow = devices.isEmpty();
    if (emptyNow && !m_lastDeviceListEmpty) {
        emit logMessage(tr("Waiting for ADB devices..."));
    } else if (!emptyNow && m_lastDeviceListEmpty) {
        emit logMessage(tr("Detected %1 ADB device(s).").arg(devices.size()));
    }
    m_lastDeviceListEmpty = emptyNow;
}

QStringList AutoCastController::collectDeviceInfo(const QString &serial)
{
    QStringList details;

    auto addLine = [&](const QString &label, const QString &value) {
        const QString text = value.trimmed().isEmpty() ? tr("未知") : value.trimmed();
        details << QStringLiteral("%1: %2").arg(label, text);
    };

    addLine(tr("设备代号"), readDeviceProperty(serial, "ro.product.device"));
    addLine(tr("设备型号"), readDeviceProperty(serial, "ro.product.model"));
    addLine(tr("安卓版本"), readDeviceProperty(serial, "ro.build.version.release"));

    QString slot = readDeviceProperty(serial, "ro.boot.slot_suffix");
    if (slot.trimmed().isEmpty()) {
        slot = tr("未分区");
    }
    addLine(tr("活动卡槽"), slot);

    const QString blValue = readDeviceProperty(serial, "ro.boot.flash.locked");
    QString blText = blValue.trimmed() == QStringLiteral("0") ? tr("已解锁") : tr("未解锁");
    addLine(tr("BL 锁状态"), blText);

    return details;
}

QString AutoCastController::readDeviceProperty(const QString &serial, const QString &prop)
{
    QStringList args;
    if (!serial.isEmpty()) {
        args << "-s" << serial;
    }
    args << "shell" << "getprop" << prop;
    return runAdbCommandSync(args);
}

QString AutoCastController::runAdbCommandSync(const QStringList &args, int timeoutMs) const
{
    const QString adb = resolveAdbExecutable();
    if (adb.isEmpty()) {
        return {};
    }

    QProcess process;
    process.start(adb, args);
    if (!process.waitForStarted(timeoutMs)) {
        qWarning() << "AutoCastController: adb command start timeout" << adb << args;
        process.kill();
        process.waitForFinished();
        return {};
    }
    if (!process.waitForFinished(timeoutMs)) {
        qWarning() << "AutoCastController: adb command timeout" << adb << args;
        process.kill();
        process.waitForFinished();
        return {};
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

QMap<QString, QString> AutoCastController::readAdbDeviceStatuses()
{
    QMap<QString, QString> statuses;
    const QString output = runAdbCommandSync(QStringList() << "devices");
    const QStringList lines = output.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    for (const QString &line : lines) {
        if (line.contains("List of devices", Qt::CaseInsensitive)) {
            continue;
        }
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }
        const QStringList parts = trimmed.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.isEmpty()) {
            continue;
        }
        const QString serial = parts.first();
        QString status = tr("ADB 在线");
        if (parts.size() >= 2) {
            const QString flag = parts.at(1).toLower();
            if (flag.contains("unauthorized")) {
                status = tr("未授权");
            } else if (flag.contains("offline")) {
                status = tr("离线");
            } else if (flag.contains("device")) {
                status = tr("ADB 在线");
            }
        }
        statuses.insert(serial, status);
    }
    return statuses;
}

quint16 AutoCastController::resolveMaxSize() const
{
    static const quint16 sizes[] = {640, 720, 1080, 1280, 1920, 0};
    const int sizeCount = static_cast<int>(sizeof(sizes) / sizeof(quint16));
    int index = m_bootConfig.maxSizeIndex;
    if (index < 0 || index >= sizeCount) {
        index = 2; // default to 1080p
    }
    return sizes[index];
}

QString AutoCastController::resolveRecordFormat() const
{
    static const QStringList formats = {"mp4", "mkv"};
    int index = m_bootConfig.recordFormatIndex;
    if (index < 0 || index >= formats.size()) {
        index = 0;
    }
    return formats.at(index);
}

qsc::DeviceParams AutoCastController::buildParams(const QString &serial)
{
    qsc::DeviceParams params;
    params.serial = serial;
    params.maxSize = resolveMaxSize();
    params.bitRate = m_bootConfig.bitRate > 0 ? m_bootConfig.bitRate : 8000000;
    params.maxFps = static_cast<quint32>(Config::getInstance().getMaxFps());
    params.closeScreen = m_bootConfig.autoOffScreen;
    params.useReverse = m_bootConfig.reverseConnect;
    params.display = true; // auto mode always displays video
    params.renderExpiredFrames = Config::getInstance().getRenderExpiredFrames();
    if (m_bootConfig.lockOrientationIndex > 0) {
        params.captureOrientationLock = 1;
        params.captureOrientation = (m_bootConfig.lockOrientationIndex - 1) * 90;
    }
    params.stayAwake = m_bootConfig.keepAlive;
    params.recordFile = m_bootConfig.recordScreen;
    params.recordPath = m_bootConfig.recordPath;
    params.recordFileFormat = resolveRecordFormat();
    params.serverLocalPath = resolveServerBinaryPath();
    params.serverRemotePath = Config::getInstance().getServerPath();
    params.pushFilePath = Config::getInstance().getPushFilePath();
    params.gameScript = "";
    params.logLevel = Config::getInstance().getLogLevel();
    params.codecOptions = Config::getInstance().getCodecOptions();
    params.codecName = Config::getInstance().getCodecName();
    params.scid = QRandomGenerator::global()->bounded(1, 10000) & 0x7FFFFFFF;
    return params;
}

void AutoCastController::onDeviceConnected(bool success, const QString &serial, const QString &deviceName, const QSize &size)
{
    Q_UNUSED(deviceName);
    if (!success) {
        qWarning() << "Auto-cast: failed to attach" << serial;
        emit logMessage(tr("Failed to attach %1").arg(serial));
        m_castingSerials.remove(serial);
        return;
    }

    ensureVideoForm(serial, size);
    const QString info = tr("Mirroring %1 (%2x%3)").arg(serial).arg(size.width()).arg(size.height());
    qInfo() << info;
    emit logMessage(info);
    emit deviceInfoReady(serial, collectDeviceInfo(serial));
}

void AutoCastController::onDeviceDisconnected(const QString &serial)
{
    qInfo() << "Auto-cast: device detached" << serial;
    emit logMessage(tr("Device detached %1").arg(serial));
    m_castingSerials.remove(serial);
    cleanupVideoForm(serial);
}
