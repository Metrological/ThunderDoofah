#pragma once
#include <Arduino.h>
#include <vector>

namespace {
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
#ifdef __DEBUG__
        Serial2.flush();
        Serial2.end();
#endif
    }

#ifdef __DEBUG__
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
    }
#else
    void Print(const char*, ...)
    {
    }
#endif
private:
    Log()
    {
#ifdef __DEBUG__
        Serial2.begin(LOG_BAUDRATE, SERIAL_8N1, LOG_RX_PIN, LOG_TX_PIN);
#endif
    }
};
} // namespace