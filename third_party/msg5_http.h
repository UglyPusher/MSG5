// MSG5_Core/ThirdParty/Http.h
#pragma once
// ВАЖНО: дефайны должны быть ДО инклюда httplib.h
// Включить TLS (если нужен)
////#define CPPHTTPLIB_OPENSSL_SUPPORT
// Включить сжатие (если нужно)
//// #define CPPHTTPLIB_ZLIB_SUPPORT

#pragma warning(push)
#pragma warning(disable: 6262 26495 26819 28020) // что шумит у MSVC/CA

#include <cpp-httplib/httplib.h>

#pragma warning(pop)