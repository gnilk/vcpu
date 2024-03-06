//
// Hex dump utility class
//

#ifndef GNILK_HEXDUMP_H
#define GNILK_HEXDUMP_H

#ifdef GNILK_HAVE_LOGGER
#include "logger.h"
#endif
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <functional>

class HexDump {
public:
    static void Write(std::function<void(const char *str)> printer, const uint8_t *pData, const size_t szData);
#ifdef GNILK_HAVE_LOGGER
    static void ToLog(gnilk::ILogger *pLogger, const void *pData, const size_t szData);
#endif
    static void ToConsole(const void *pData, const size_t szData);
    static void ToFile(FILE *f, const void *pData, const size_t szData);
};


#endif //GNILK_HEXDUMP_H
