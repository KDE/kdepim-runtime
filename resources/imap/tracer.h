#pragma once

#include "imapresource_debug.h"

QDebug debugStream(int line, const char* file, const char* function);

#define Trace() debugStream(__LINE__, __FILE__, Q_FUNC_INFO)
