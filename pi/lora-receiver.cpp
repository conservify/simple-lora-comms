#include <cstdlib>
#include <cstdio>

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

    printf("Starting...\n");

    LoraRadioPi radio(PIN_SELECT, PIN_RESET, PIN_DIO_0, 0);

    while (true) {
        radio.tick();

        delay(100);
    }

    printf("Done.\n");

    return 0;

}

