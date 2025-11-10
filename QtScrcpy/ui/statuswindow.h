#ifndef STATUSWINDOW_H
#define STATUSWINDOW_H

#include <QWidget>
#include <QStringList>

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
    void appendLog(const QString &message);
    void setActiveDevices(const QStringList &devices);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QListWidget *m_deviceList = nullptr;
    QPlainTextEdit *m_logView = nullptr;
    QLabel *m_summaryLabel = nullptr;
};

#endif // STATUSWINDOW_H
