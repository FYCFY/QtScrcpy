#ifndef SERVERPATHRESOLVER_H
#define SERVERPATHRESOLVER_H

#include <QString>

// Resolve the local scrcpy-server jar path, falling back to the application
// directory when the QTSCRCPY_SERVER_PATH environment variable is not usable.
QString resolveServerBinaryPath();

#endif // SERVERPATHRESOLVER_H
