#pragma once

#include <QDebug>

QDebug debugStream(int line, const char* file, const char* function);

#define Trace() debugStream(__LINE__, __FILE__, Q_FUNC_INFO)
