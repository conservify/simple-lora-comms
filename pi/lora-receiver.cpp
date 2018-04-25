#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <unistd.h>

#include "lora_radio_pi.h"

constexpr uint8_t PIN_SELECT = 6;
constexpr uint8_t PIN_DIO_0 = 7;
constexpr uint8_t PIN_RESET = 0;

void write_pid() {
    FILE *fp = fopen("/var/run/lora-receiver.pid", "w");
    fprintf(fp, "%d", getpid());
    fclose(fp);
}

int32_t main(int32_t argc, const char **argv) {
    if (argc > 1) {
        if (daemon(1, 0) == 0) {
            chdir(argv[1]);
            write_pid();
        }
        else {
            return 1;
        }
    }

    wiringPiSetup();
    wiringPiSPISetup(0, 500000);

    LoraRadioPi radio(PIN_SELECT, PIN_RESET, PIN_DIO_0, 0);

    printf("Starting...\n");

    time_t checkRadioEvery = 1000;
    time_t checkedAt = 0;

    bool haveRadio = false;
    while (true) {
        radio.lock();

        if (!haveRadio) {
            if (radio.setup()) {
                radio.setModeRx();
                haveRadio = true;
            }
            else {
                printf("Radio unavailable.\n");
            }
        }
        else {
            if (time(NULL) - checkedAt > checkRadioEvery) {
                haveRadio = radio.detectChip();
                checkedAt = time(NULL);
            }

            if (radio.isModeStandby()) {
                radio.setModeRx();
            }
        }

        radio.unlock();

        delay(100);
    }

    printf("Done.\n");

    return 0;

}

