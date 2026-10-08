#include "CrashHandler.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <fcntl.h>
#include <execinfo.h>
#include <sys/wait.h>
#include <exception>

static char s_crashLogPath[512] = "/tmp/s-shot-crash.log";
static char s_appLogPath[512] = "/tmp/s-shot.log";

static void signalHandler(int sig, siginfo_t* info, void* /*ctx*/) {
    int fd = open(s_crashLogPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        const char header[] = "\n================== S-SHOT CRASH REPORT ==================\n";
        write(fd, header, sizeof(header) - 1);

        const char* sigName = "UNKNOWN";
        switch (sig) {
            case SIGSEGV: sigName = "SIGSEGV (Segmentation Fault)"; break;
            case SIGABRT: sigName = "SIGABRT (Aborted)"; break;
            case SIGFPE:  sigName = "SIGFPE (Floating Point Exception)"; break;
            case SIGILL:  sigName = "SIGILL (Illegal Instruction)"; break;
            case SIGBUS:  sigName = "SIGBUS (Bus Error)"; break;
        }

        char line[256];
        int len = snprintf(line, sizeof(line), "Signal: %d [%s]\nFault address: %p\n", sig, sigName, info ? info->si_addr : nullptr);
        if (len > 0) write(fd, line, len);

        const char btHeader[] = "Call stack:\n";
        write(fd, btHeader, sizeof(btHeader) - 1);

        void* array[64];
        int size = backtrace(array, 64);
        backtrace_symbols_fd(array, size, fd);

        const char footer[] = "=========================================================\n";
        write(fd, footer, sizeof(footer) - 1);
        close(fd);
    }

    // Also write brief message to stderr
    fprintf(stderr, "\n[S-Shot Error] Fatal signal %d received. Crash report written to: %s\n", sig, s_crashLogPath);

    // Fork a child process to show a GUI notification if possible
    pid_t pid = fork();
    if (pid == 0) {
        char msg[1024];
        snprintf(msg, sizeof(msg),
                 "S-Shot encountered a fatal error and has terminated.\n\nCrash log written to:\n%s",
                 s_crashLogPath);
        // Try zenity (common on GNOME/Cinnamon/Mint)
        execlp("zenity", "zenity", "--error", "--title=S-Shot Fatal Error", "--text", msg, nullptr);
        // Fallback to kdialog (KDE)
        execlp("kdialog", "kdialog", "--error", msg, nullptr);
        // Fallback to xmessage
        execlp("xmessage", "xmessage", "-center", msg, nullptr);
        _exit(0);
    }

    // Reset signal handler to default and re-raise
    signal(sig, SIG_DFL);
    raise(sig);
}

static void terminateHandler() {
    fprintf(stderr, "\n[S-Shot Error] Unhandled exception occurred.\n");
    int fd = open(s_crashLogPath, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd >= 0) {
        const char msg[] = "\n[ERROR] Terminate handler invoked: unhandled C++ exception.\n";
        write(fd, msg, sizeof(msg) - 1);
        void* array[64];
        int size = backtrace(array, 64);
        backtrace_symbols_fd(array, size, fd);
        close(fd);
    }
    abort();
}

static void qtLogMessageHandler(QtMsgType type, const QMessageLogContext& /*context*/, const QString& msg) {
    const char* typeStr = "DEBUG";
    switch (type) {
        case QtDebugMsg:    typeStr = "DEBUG"; break;
        case QtInfoMsg:     typeStr = "INFO"; break;
        case QtWarningMsg:  typeStr = "WARNING"; break;
        case QtCriticalMsg: typeStr = "CRITICAL"; break;
        case QtFatalMsg:    typeStr = "FATAL"; break;
    }

    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
    QString line = QString("[%1] [%2] %3\n").arg(timeStr, typeStr, msg);

    // Print to standard stderr
    fprintf(stderr, "%s", qPrintable(line));

    // Append to s-shot.log
    FILE* f = fopen(s_appLogPath, "a");
    if (f) {
        fprintf(f, "%s", qPrintable(line));
        fclose(f);
    }
}

void CrashHandler::init() {
    QString configDir = logDir();
    QDir().mkpath(configDir);

    QString crashFile = configDir + "/crash.log";
    QString appFile = configDir + "/s-shot.log";

    strncpy(s_crashLogPath, crashFile.toUtf8().constData(), sizeof(s_crashLogPath) - 1);
    s_crashLogPath[sizeof(s_crashLogPath) - 1] = '\0';

    strncpy(s_appLogPath, appFile.toUtf8().constData(), sizeof(s_appLogPath) - 1);
    s_appLogPath[sizeof(s_appLogPath) - 1] = '\0';

    // Install signal handlers with alternate signal stack
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = signalHandler;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGFPE,  &sa, nullptr);
    sigaction(SIGILL,  &sa, nullptr);
    sigaction(SIGBUS,  &sa, nullptr);

    std::set_terminate(terminateHandler);
    qInstallMessageHandler(qtLogMessageHandler);
}

QString CrashHandler::logPath() {
    return QString::fromUtf8(s_appLogPath);
}

QString CrashHandler::crashLogPath() {
    return QString::fromUtf8(s_crashLogPath);
}

QString CrashHandler::logDir() {
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.config";
    }
    return baseDir + "/s-shot";
}
