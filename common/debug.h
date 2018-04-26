#pragma once

#include <cstdarg>

void fklog(const char *f, ...);
void fklogln(const char *f, ...);

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

    Logger& operator<<(const char *s) {
        printf("%s", s);
        return *this;
    }

};

extern Logger logger;
