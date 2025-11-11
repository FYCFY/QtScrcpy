#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QMap>
#include <QStringList>
#include <QWidget>
#include <QPoint>

class QPlainTextEdit;
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
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void rebuildInfoPanel();
    QPlainTextEdit *m_logView = nullptr;
    QMap<QString, QStringList> m_deviceInfo;
    QMap<QString, QString> m_deviceStatus;
    bool m_dragging = false;
    QPoint m_dragOffset;
};

#endif // STATUSWINDOW_H
