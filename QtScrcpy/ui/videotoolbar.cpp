#include "videotoolbar.h"

#include <QBoxLayout>
#include <QPushButton>
#include <QStyle>
#include <functional>

#include "videoform.h"
#include "../QtScrcpyCore/include/QtScrcpyCore.h"

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
    card->setObjectName("toolbarCard");
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

    auto addButton = [&](const QString &text, const std::function<void(qsc::IDevice*)> &handler) {
        QPushButton *btn = new QPushButton(text, card);
        btn->setFocusPolicy(Qt::NoFocus);
        connect(btn, &QPushButton::clicked, this, [this, handler]() { invoke(handler); });
        layout->addWidget(btn);
        m_buttons.append(btn);
    };

    addButton(tr("Back"), [](qsc::IDevice *device) { device->postGoBack(); });
    addButton(tr("Home"), [](qsc::IDevice *device) { device->postGoHome(); });
    addButton(tr("Recent"), [](qsc::IDevice *device) { device->postAppSwitch(); });
    addButton(tr("Menu"), [](qsc::IDevice *device) { device->postGoMenu(); });
    addButton(tr("Power"), [](qsc::IDevice *device) { device->postPower(); });

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
