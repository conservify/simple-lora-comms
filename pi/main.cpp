#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstdarg>

#include <unistd.h>

#include "lora_radio_pi.h"
#include "gateway_protocol.h"
#include "file_writer.h"

constexpr uint8_t PIN_SELECT = 6;
constexpr uint8_t PIN_DIO_0 = 7;
constexpr uint8_t PIN_RESET = 0;

void write_pid() {
    FILE *fp = fopen("/var/run/lora-receiver.pid", "w");
    fprintf(fp, "%d", getpid());
    fclose(fp);
}

class DumbGatewayCallbacks : public GatewayNetworkCallbacks {
public:
    lws::Writer *openWriter(RadioPacket &packet) override {
        auto writer = new FileWriter("DATA");
        if (!writer->open()) {
            delete writer;
            writer = nullptr;
        }
        return writer;
    }

    void closeWriter(lws::Writer *writer) override {
    }

};

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

    // This is purely to ensure the mutex inside is ready. This can be forgiving
    // until you start using the heap, etc...
    radio.setup();

    auto callbacks = DumbGatewayCallbacks{ };
    auto protocol = GatewayNetworkProtocol{ radio, callbacks };

    while (true) {
        radio.tick();
        protocol.tick();

        auto &incoming = radio.getIncoming();
        if (incoming.size() > 0) {
            auto lora = incoming.front();
            incoming.pop();
            protocol.push(lora);
        }

        delay(10);
    }

    return 0;

}

