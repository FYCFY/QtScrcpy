#include "adbpathresolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "config.h"

namespace
{
QString normalizePath(const QString &path)
{
    if (path.isEmpty()) {
        return {};
    }

    QFileInfo info(path);
    if (!info.isAbsolute()) {
        QDir base(QCoreApplication::applicationDirPath());
#ifdef Q_OS_WIN
        QString normalized = path;
        normalized.replace('\\', '/');
        info.setFile(base.absoluteFilePath(normalized));
#else
        info.setFile(base.absoluteFilePath(path));
#endif
    }
    return QDir::cleanPath(info.absoluteFilePath());
}
}

QString resolveAdbExecutable()
{
    QStringList candidates;

    candidates << normalizePath(Config::getInstance().getAdbPath());
    candidates << normalizePath(QString::fromLocal8Bit(qgetenv("QTSCRCPY_ADB_PATH")));

#if defined(Q_OS_WIN)
    candidates << normalizePath("adb.exe");
    candidates << normalizePath("../Resources/adb.exe");
    candidates << normalizePath("../../QtScrcpy/QtScrcpyCore/src/third_party/adb/win/adb.exe");
#elif defined(Q_OS_OSX)
    candidates << normalizePath("adb");
    candidates << normalizePath("../Resources/adb");
    candidates << normalizePath("../../../../QtScrcpy/QtScrcpyCore/src/third_party/adb/mac/adb");
#else
    candidates << normalizePath("adb");
    candidates << normalizePath("../lib/qtscrcpy/adb");
    candidates << normalizePath("../../QtScrcpy/QtScrcpyCore/src/third_party/adb/linux/adb");
#endif

    candidates << QStringLiteral("adb");

    for (const QString &candidate : candidates) {
        if (candidate.isEmpty()) {
            continue;
        }
        QFileInfo info(candidate);
        if (info.exists() && info.isFile()) {
#ifdef Q_OS_WIN
            if (info.suffix().toLower() == "exe" || info.fileName().toLower().endsWith(".exe")) {
                return info.absoluteFilePath();
            }
#else
            if (info.isExecutable()) {
                return info.absoluteFilePath();
            }
#endif
        }
    }

    return QStringLiteral("adb");
}
