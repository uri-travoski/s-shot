#pragma once

#include <QString>

class CrashHandler {
public:
    static void init();
    static QString logPath();
    static QString crashLogPath();
    static QString logDir();
};
