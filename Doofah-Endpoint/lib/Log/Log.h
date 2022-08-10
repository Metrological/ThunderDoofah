#pragma once

#include <Arduino.h>
#include <vector>

#include <cstdlib>
#include <cxxabi.h>
#include <string>

namespace Doofhah {
#ifdef __DEBUG__
static void ToHexString(const uint8_t object[], const uint16_t length, std::string& result)
{
    const char hex_chars[] = "0123456789abcdef";

    if (object != nullptr) {

        uint16_t index = static_cast<uint16_t>(result.length());
        result.resize(index + (length * 2));

        result[1] = hex_chars[object[0] & 0xF];

        for (uint16_t i = 0, j = index; i < length; i++) {
            if ((object[i] == '\\') && ((i + 3) < length) && (object[i + 1] == 'x')) {
                result[j++] = object[i + 2];
                result[j++] = object[i + 3];
                i += 3;
            } else {
                result[j++] = hex_chars[object[i] >> 4];
                result[j++] = hex_chars[object[i] & 0xF];
            }
        }
    }
}
#else
static void ToHexString(const uint8_t[], const uint16_t, std::string&)
{
}
#endif

#ifdef __DEBUG__
class Log {
public:
    Log(const Log&) = delete;
    Log& operator=(const Log&) = delete;

    static Log& Instance()
    {
        static Log instance;
        return instance;
    }
    ~Log()
    {
        Serial2.flush();
        Serial2.end();
    }


    void Print(const char* fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        std::vector<char> logbuf(strlen(fmt));

        int sz = vsnprintf(&logbuf[0], logbuf.size(), fmt, args);

        if (sz >= int(logbuf.size())) {
            logbuf.resize(sz + 1);
            vsnprintf(&logbuf[0], logbuf.size(), fmt, args);
        }
        va_end(args);

        Serial2.println(&logbuf[0]);
        Serial2.flush();
    }
private:
    Log()
    {

        Serial2.begin(LOG_BAUDRATE, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);

    }
};
#endif
} // namespace Doofhah

#ifdef __DEBUG__
#define TRACE(msg, ...) ::Doofhah::Log::Instance().Print("%d [%s:%d (%p) %s] " msg, millis(), __FILE__, __LINE__, this, __FUNCTION__, ##__VA_ARGS__)
#define GLOBAL_TRACE(msg, ...) ::Doofhah::Log::Instance().Print("%d [%s:%d %s] " msg, millis(), __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#else
#define TRACE(msg, ...)
#define GLOBAL_TRACE(msg, ...)
#endif
