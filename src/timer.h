#ifndef SLC_TIMER_H_INCLUDED
#define SLC_TIMER_H_INCLUDED

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <wiringPi.h>
#endif

class Timer {
private:
    uint32_t started{ 0 };
    uint32_t total{ 0 };
    uint32_t samples{ 0 };

public:
    void clear() {
        started = 0;
        samples = 0;
        total = 0;
    }

    bool isRunning() {
        return started > 0;
    }

    void begin() {
        started = millis();
    }

    void end() {
        if (started > 0) {
            total += millis() - started;
            started = 0;
            samples++;
        }
    }

public:
    friend LogStream& operator<<(LogStream &log, const Timer &timer) {
        auto average = timer.samples > 0 ? ((float)timer.total / (float)timer.samples) : 0.0f;
        log.printf("Timer<%d samples, %.2f average>", timer.samples, average);
        return log;
    }

};

#endif
