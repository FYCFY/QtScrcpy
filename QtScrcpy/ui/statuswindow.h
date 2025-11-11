#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QMap>
#include <QStringList>
#include <QWidget>

class QListWidget;
class QPlainTextEdit;
class QLabel;
class QCloseEvent;

class StatusWindow : public QWidget
{
    Q_OBJECT
public:
    explicit StatusWindow(QWidget *parent = nullptr);

public slots:
    void setActiveDevices(const QStringList &devices);
    void updateDeviceInfo(const QString &serial, const QStringList &infoLines);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void rebuildInfoPanel();
    QListWidget *m_deviceList = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QMap<QString, QStringList> m_deviceInfo;
    QMap<QString, QString> m_deviceStatus;
};

#endif // STATUSWINDOW_H
