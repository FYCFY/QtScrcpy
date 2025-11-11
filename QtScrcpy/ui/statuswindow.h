#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QMap>
#include <QStringList>
#include <QWidget>
#include <QPoint>

class QListWidget;
class QPlainTextEdit;
class QLabel;
class QCloseEvent;
class QMouseEvent;

class StatusWindow : public QWidget
{
    Q_OBJECT
public:
    explicit StatusWindow(QWidget *parent = nullptr);

public slots:
    void setActiveDevices(const QStringList &devices);
    void updateDeviceInfo(const QString &serial, const QStringList &infoLines);
    void setDeviceStatuses(const QMap<QString, QString> &statuses);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void rebuildInfoPanel();
    void rebuildDeviceList();
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    QListWidget *m_deviceList = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLabel *m_summaryLabel = nullptr;
    QMap<QString, QStringList> m_deviceInfo;
    QMap<QString, QString> m_deviceStatus;
    QStringList m_currentSerials;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // STATUSWINDOW_H
