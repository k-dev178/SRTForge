#include "SystemMemory.h"

#if defined(Q_OS_MACOS)
#include <sys/sysctl.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#elif defined(Q_OS_LINUX)
#include <sys/sysinfo.h>
#endif

quint64 SystemMemory::totalBytes() {
#if defined(Q_OS_MACOS)
    quint64 value = 0;
    size_t size = sizeof(value);
    if (sysctlbyname("hw.memsize", &value, &size, nullptr, 0) == 0) {
        return value;
    }
#elif defined(Q_OS_WIN)
    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    if (GlobalMemoryStatusEx(&status)) {
        return static_cast<quint64>(status.ullTotalPhys);
    }
#elif defined(Q_OS_LINUX)
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        return static_cast<quint64>(info.totalram) * static_cast<quint64>(info.mem_unit);
    }
#endif
    return 0;
}

double SystemMemory::totalGiB() {
    const quint64 bytes = totalBytes();
    if (bytes == 0) {
        return 0.0;
    }
    return static_cast<double>(bytes) / 1024.0 / 1024.0 / 1024.0;
}
