#ifndef VIDEOTOOLBAR_H
#define VIDEOTOOLBAR_H

#include <QPointer>
#include <QList>
#include <QWidget>
#include <QStringList>
#include <functional>

namespace qsc
{
    class IDevice;
}

class VideoForm;
class QPushButton;

class VideoToolbar : public QWidget
{
    Q_OBJECT
public:
    explicit VideoToolbar(VideoForm *videoForm);

    void setSerial(const QString &serial);
    void syncPosition();

protected:
    void leaveEvent(QEvent *event) override;

private:
    void invoke(const std::function<void(class qsc::IDevice*)> &operation);
    void runAdbDetached(const QStringList &args);
    void buildUi();

    QPointer<VideoForm> m_videoForm;
    QString m_serial;
    QList<QPushButton*> m_buttons;
};

#endif // VIDEOTOOLBAR_H
