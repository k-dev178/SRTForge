#pragma once

#include <QtGlobal>

class SystemMemory {
public:
    static quint64 totalBytes();
    static double totalGiB();
};
