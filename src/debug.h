#ifndef SLC_DEBUG_H_INCLUDED
#define SLC_DEBUG_H_INCLUDED

#include <cstdarg>

class Logger {
public:

public:
    Logger& printf(const char *f, ...);
    Logger& print(const char *str);

public:
    Logger& operator<<(uint8_t i) {
        printf("%d", i);
        return *this;
    }

    Logger& operator<<(uint32_t i) {
        printf("%lu", i);
        return *this;
    }

    Logger& operator<<(int8_t i) {
        printf("%d", i);
        return *this;
    }

    Logger& operator<<(int32_t i) {
        printf("%d", i);
        return *this;
    }

    #ifdef ARDUINO // Ick
    Logger& operator<<(size_t i) {
        printf("%d", i);
        return *this;
    }
    #endif

    Logger& operator<<(const char *s) {
        printf("%s", s);
        return *this;
    }

};

extern Logger logger;

#endif
