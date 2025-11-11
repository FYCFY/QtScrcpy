#include "statuswindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QFrame>
#include <QFont>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QTextOption>
#include <QSet>
#include <QVBoxLayout>

StatusWindow::StatusWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("QtScrcpy 设备监控"));
    resize(240, 220);
    setWindowFlags(windowFlags() | Qt::FramelessWindowHint | Qt::Tool);
    setAttribute(Qt::WA_TranslucentBackground);
    auto *surface = new QWidget(this);
    surface->setObjectName(QStringLiteral("glassCard"));
    surface->setStyleSheet(R"(
        QWidget#glassCard {
            background: rgba(20, 20, 20, 180);
            border-radius: 16px;
            border: 1px solid rgba(255, 255, 255, 40);
        }
        QLabel, QListWidget, QPlainTextEdit {
            color: #F5F5F7;
        }
        QListWidget, QPlainTextEdit {
            background: transparent;
            border: none;
        }
    )");

    auto *cardLayout = new QVBoxLayout(surface);
    cardLayout->setContentsMargins(16, 16, 16, 16);
    cardLayout->setSpacing(6);

    m_summaryLabel = new QLabel(tr("已连接设备: 0"), surface);
    QFont summaryFont = m_summaryLabel->font();
    summaryFont.setPointSize(summaryFont.pointSize() + 2);
    m_summaryLabel->setFont(summaryFont);
    m_deviceList = new QListWidget(surface);
    m_deviceList->setSelectionMode(QAbstractItemView::NoSelection);
    QFont listFont = m_deviceList->font();
    listFont.setPointSize(listFont.pointSize() + 1);
    m_deviceList->setFont(listFont);

    m_logView = new QPlainTextEdit(surface);
    m_logView->setReadOnly(true);
    QFont infoFont = m_logView->font();
    infoFont.setPointSize(infoFont.pointSize() + 1);
    m_logView->setFont(infoFont);
    m_logView->setWordWrapMode(QTextOption::WordWrap);
    m_logView->setFrameShape(QFrame::NoFrame);

    cardLayout->addWidget(m_summaryLabel);
    cardLayout->addWidget(new QLabel(tr("设备列表"), surface));
    cardLayout->addWidget(m_deviceList, 1);
    cardLayout->addWidget(new QLabel(tr("设备信息"), surface));
    cardLayout->addWidget(m_logView, 2);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->addWidget(surface);
    setLayout(rootLayout);
}

void StatusWindow::setActiveDevices(const QStringList &devices)
{
    m_deviceList->clear();
    m_deviceList->addItems(devices);
    m_summaryLabel->setText(tr("已连接设备: %1").arg(devices.size()));

    for (const QString &serial : devices) {
        if (!serial.isEmpty()) {
            m_deviceStatus.insert(serial, tr("ADB 在线"));
        }
    }

    // 清除已断开设备的缓存信息
    QSet<QString> current;
    for (const QString &dev : devices) {
        current.insert(dev);
    }
    QList<QString> removed;
    for (auto it = m_deviceInfo.begin(); it != m_deviceInfo.end();) {
        if (!current.contains(it.key())) {
            removed << it.key();
            it = m_deviceInfo.erase(it);
        } else {
            ++it;
        }
    }
    for (const QString &serial : removed) {
        m_deviceStatus.remove(serial);
    }
    rebuildInfoPanel();
}

void StatusWindow::updateDeviceInfo(const QString &serial, const QStringList &infoLines)
{
    if (serial.isEmpty()) {
        return;
    }
    m_deviceInfo.insert(serial, infoLines);
    rebuildInfoPanel();
}

void StatusWindow::rebuildInfoPanel()
{
    QStringList blocks;
    for (auto it = m_deviceInfo.constBegin(); it != m_deviceInfo.constEnd(); ++it) {
        QStringList section;
        section << tr("序列号: %1").arg(it.key());
        const QString status = m_deviceStatus.value(it.key(), tr("未知"));
        section << tr("状态: %1").arg(status);
        for (const QString &line : it.value()) {
            section << QStringLiteral("  • %1").arg(line);
        }
        blocks << section.join('\n');
    }
    m_logView->setPlainText(blocks.join("\n\n"));
}

void StatusWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    if (qApp) {
        qApp->quit();
    }
}
