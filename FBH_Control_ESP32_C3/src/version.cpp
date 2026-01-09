#include <Arduino.h>
#include "version.h"

String generateBuildVersion()
{
    const char* months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char m[4] = { __DATE__[0], __DATE__[1], __DATE__[2], 0 };

    int month = (strstr(months, m) - months) / 3 + 1;
    int day   = atoi(__DATE__ + 4);
    int year  = atoi(__DATE__ + 7) % 100;
    int hour  = atoi(__TIME__);
    int min   = atoi(__TIME__ + 3);

    char buf[32];
    snprintf(buf, sizeof(buf),
        "%02d%02d%02d%02d%02d-%s",
        year, month, day, hour, min, FW_MAJOR_VERSION);

    return String(buf);
}

