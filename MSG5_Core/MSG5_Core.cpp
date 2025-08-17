// MSG5_Core.cpp : Определяет функции для статической библиотеки.
//

#include "pch.h"
#include "MSG5_Core.h"

// TODO: Это пример библиотечной функции.

namespace msg5 {
    const char* core_version() noexcept {
        return "MSG5 Core 0.1.0 (" __DATE__ " " __TIME__ ")";
    }
}

