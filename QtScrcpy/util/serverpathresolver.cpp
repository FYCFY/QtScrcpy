#include "serverpathresolver.h"

#include <QCoreApplication>
#include <QFileInfo>

QString resolveServerBinaryPath()
{
    static QString serverPath;
    if (!serverPath.isEmpty()) {
        return serverPath;
    }

    serverPath = QString::fromLocal8Bit(qgetenv("QTSCRCPY_SERVER_PATH"));
    QFileInfo fileInfo(serverPath);
    if (serverPath.isEmpty() || !fileInfo.isFile()) {
        serverPath = QCoreApplication::applicationDirPath() + "/scrcpy-server";
    }

    return serverPath;
}
