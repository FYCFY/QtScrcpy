#include "videotoolbar.h"

#include <QBoxLayout>
#include <QDebug>
#include <QProcess>
#include <QPushButton>
#include <QStyle>

#include "config.h"
#include "videoform.h"
#include "../QtScrcpyCore/include/QtScrcpyCore.h"

namespace
{
QString resolveAdbBinary()
{
    QString path = Config::getInstance().getAdbPath();
    if (path.isEmpty()) {
        const QByteArray env = qgetenv("QTSCRCPY_ADB_PATH");
        if (!env.isEmpty()) {
            path = QString::fromLocal8Bit(env);
        } else {
            path = QStringLiteral("adb");
        }
    }
    return path;
}
}

VideoToolbar::VideoToolbar(VideoForm *videoForm)
    : QWidget(nullptr)
    , m_videoForm(videoForm)
{
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::NoFocus);

    buildUi();
}

void VideoToolbar::buildUi()
{
    auto *card = new QWidget(this);
    card->setObjectName(QStringLiteral("toolbarCard"));
    card->setStyleSheet(R"(
        QWidget#toolbarCard {
            background-color: rgba(30, 30, 30, 190);
            border-radius: 8px;
        }
        QPushButton {
            color: #FFFFFF;
            background-color: rgba(255, 255, 255, 25);
            border: none;
            padding: 6px 12px;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 55);
        }
    )");

    auto *layout = new QVBoxLayout(card);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto addButton = [&](const QString &text, const QString &tooltip, const std::function<void(qsc::IDevice*)> &handler) {
        QPushButton *btn = new QPushButton(text, card);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setToolTip(tooltip);
        connect(btn, &QPushButton::clicked, this, [this, handler]() { invoke(handler); });
        layout->addWidget(btn);
        m_buttons.append(btn);
    };

    auto addAdbButton = [&](const QString &text, const QStringList &args) {
        QPushButton *btn = new QPushButton(text, card);
        btn->setFocusPolicy(Qt::NoFocus);
        connect(btn, &QPushButton::clicked, this, [this, args]() { runAdbDetached(args); });
        layout->addWidget(btn);
        m_buttons.append(btn);
    };

    QFont navFont;
    navFont.setPointSize(18);

    auto makeNavButton = [&](const QString &symbol, const QString &tooltip, const std::function<void(qsc::IDevice*)> &handler) {
        QPushButton *btn = new QPushButton(symbol, card);
        btn->setFont(navFont);
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setToolTip(tooltip);
        connect(btn, &QPushButton::clicked, this, [this, handler]() { invoke(handler); });
        layout->addWidget(btn);
        m_buttons.append(btn);
    };

    makeNavButton(QStringLiteral("△"), tr("返回"), [](qsc::IDevice *device) { device->postGoBack(); });
    makeNavButton(QStringLiteral("□"), tr("主页"), [](qsc::IDevice *device) { device->postGoHome(); });
    makeNavButton(QStringLiteral("≡"), tr("最近任务"), [](qsc::IDevice *device) { device->postAppSwitch(); });

    addButton(tr("菜单"), tr("打开菜单"), [](qsc::IDevice *device) { device->postGoMenu(); });
    addButton(tr("电源"), tr("电源键"), [](qsc::IDevice *device) { device->postPower(); });

    addAdbButton(tr("重启系统"), QStringList() << "reboot");
    addAdbButton(tr("重启Bootloader"), QStringList() << "reboot" << "bootloader");
    addAdbButton(tr("重启Fastbootd"), QStringList() << "reboot" << "fastboot");
    addAdbButton(tr("重启9008"), QStringList() << "reboot" << "edl");
    addAdbButton(tr("开启USB调试安全"), QStringList() << "shell" << "su" << "-c" << "setprop persist.security.adbinput 1");

    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->addWidget(card);
    setLayout(outer);
}

void VideoToolbar::setSerial(const QString &serial)
{
    m_serial = serial;
}

void VideoToolbar::syncPosition()
{
    if (!m_videoForm) {
        return;
    }
    const QRect parentRect = m_videoForm->rect();
    QPoint topRight = m_videoForm->mapToGlobal(parentRect.topRight());
    QPoint target = topRight + QPoint(12, 40);
    move(target);
}

void VideoToolbar::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    clearFocus();
}

void VideoToolbar::invoke(const std::function<void(qsc::IDevice*)> &operation)
{
    auto device = qsc::IDeviceManage::getInstance().getDevice(m_serial);
    if (!device) {
        return;
    }
    operation(device);
}

void VideoToolbar::runAdbDetached(const QStringList &args)
{
    if (m_serial.isEmpty()) {
        return;
    }

    QString adbPath = resolveAdbBinary();
    QStringList fullArgs;
    fullArgs << "-s" << m_serial;
    fullArgs << args;
    if (!QProcess::startDetached(adbPath, fullArgs)) {
        qWarning() << "VideoToolbar: failed to run adb command" << fullArgs;
    }
}
