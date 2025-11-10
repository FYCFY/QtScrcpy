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
    resize(440, 420);

    m_summaryLabel = new QLabel(tr("已连接设备: 0"), this);
    QFont summaryFont = m_summaryLabel->font();
    summaryFont.setPointSize(summaryFont.pointSize() + 2);
    m_summaryLabel->setFont(summaryFont);
    m_deviceList = new QListWidget(this);
    m_deviceList->setSelectionMode(QAbstractItemView::NoSelection);
    QFont listFont = m_deviceList->font();
    listFont.setPointSize(listFont.pointSize() + 1);
    m_deviceList->setFont(listFont);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    QFont infoFont = m_logView->font();
    infoFont.setPointSize(infoFont.pointSize() + 1);
    m_logView->setFont(infoFont);
    m_logView->setWordWrapMode(QTextOption::WordWrap);
    m_logView->setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(new QLabel(tr("设备列表"), this));
    layout->addWidget(m_deviceList, 1);
    layout->addWidget(new QLabel(tr("设备信息"), this));
    layout->addWidget(m_logView, 2);
    setLayout(layout);
}

void StatusWindow::setActiveDevices(const QStringList &devices)
{
    m_deviceList->clear();
    m_deviceList->addItems(devices);
    m_summaryLabel->setText(tr("已连接设备: %1").arg(devices.size()));

    // 清除已断开设备的缓存信息
    QSet<QString> current;
    for (const QString &dev : devices) {
        current.insert(dev);
    }
    for (auto it = m_deviceInfo.begin(); it != m_deviceInfo.end();) {
        if (!current.contains(it.key())) {
            it = m_deviceInfo.erase(it);
        } else {
            ++it;
        }
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
