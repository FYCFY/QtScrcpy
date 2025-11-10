#include "statuswindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QDateTime>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>

StatusWindow::StatusWindow(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(tr("QtScrcpy AutoCast Monitor"));
    resize(420, 360);

    m_summaryLabel = new QLabel(tr("Active devices: 0"), this);
    m_deviceList = new QListWidget(this);
    m_deviceList->setSelectionMode(QAbstractItemView::NoSelection);

    m_logView = new QPlainTextEdit(this);
    m_logView->setReadOnly(true);
    m_logView->setMaximumBlockCount(500);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_summaryLabel);
    layout->addWidget(m_deviceList, 1);
    layout->addWidget(new QLabel(tr("ADB monitor log:"), this));
    layout->addWidget(m_logView, 1);
    setLayout(layout);
}

void StatusWindow::appendLog(const QString &message)
{
    const QString stamp = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_logView->appendPlainText(QString("[%1] %2").arg(stamp, message));
}

void StatusWindow::setActiveDevices(const QStringList &devices)
{
    m_deviceList->clear();
    m_deviceList->addItems(devices);
    m_summaryLabel->setText(tr("Active devices: %1").arg(devices.size()));
}

void StatusWindow::closeEvent(QCloseEvent *event)
{
    QWidget::closeEvent(event);
    if (qApp) {
        qApp->quit();
    }
}
