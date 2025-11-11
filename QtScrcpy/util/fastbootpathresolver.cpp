#include "fastbootpathresolver.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

#include "adbpathresolver.h"

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

QString siblingFastbootPath()
{
    const QString adb = resolveAdbExecutable();
    if (adb.isEmpty()) {
        return {};
    }
    QFileInfo adbInfo(adb);
    const QDir baseDir = adbInfo.dir();
#ifdef Q_OS_WIN
    return baseDir.absoluteFilePath(QStringLiteral("fastboot.exe"));
#else
    return baseDir.absoluteFilePath(QStringLiteral("fastboot"));
#endif
}
}

QString resolveFastbootExecutable()
{
    QStringList candidates;

    candidates << normalizePath(QString::fromLocal8Bit(qgetenv("QTSCRCPY_FASTBOOT_PATH")));
    candidates << normalizePath(siblingFastbootPath());

#if defined(Q_OS_WIN)
    candidates << normalizePath("fastboot.exe");
    candidates << normalizePath("../Resources/fastboot.exe");
    candidates << normalizePath("../../QtScrcpy/QtScrcpyCore/src/third_party/platform-tools/win/fastboot.exe");
#elif defined(Q_OS_OSX)
    candidates << normalizePath("fastboot");
    candidates << normalizePath("../Resources/fastboot");
    candidates << normalizePath("../../../../QtScrcpy/QtScrcpyCore/src/third_party/platform-tools/mac/fastboot");
#else
    candidates << normalizePath("fastboot");
    candidates << normalizePath("../lib/qtscrcpy/fastboot");
    candidates << normalizePath("../../QtScrcpy/QtScrcpyCore/src/third_party/platform-tools/linux/fastboot");
#endif

    candidates << QStringLiteral("fastboot");

    for (const QString &candidate : candidates) {
        if (candidate.isEmpty()) {
            continue;
        }
        QFileInfo info(candidate);
        if (!info.exists() || !info.isFile()) {
            continue;
        }
#ifdef Q_OS_WIN
        if (info.suffix().compare(QStringLiteral("exe"), Qt::CaseInsensitive) != 0
            && !info.fileName().toLower().endsWith(".exe")) {
            continue;
        }
        return info.absoluteFilePath();
#else
        if (!info.isExecutable()) {
            continue;
        }
        return info.absoluteFilePath();
#endif
    }

    return QStringLiteral("fastboot");
}
