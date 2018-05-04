#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cstdarg>
#include <unistd.h>

#include <experimental/filesystem>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>

#include "lora_radio_pi.h"
#include "gateway_protocol.h"
#include "gateway_callbacks.h"
#include "file_writer.h"
#include "processor.h"

constexpr uint8_t PIN_SELECT = 6;
constexpr uint8_t PIN_DIO_0 = 7;
constexpr uint8_t PIN_RESET = 0;

int32_t main(int32_t argc, const char **argv) {
    auto command = "";
    auto archive = "./archive";
    for (auto i = 0; i < argc; ++i) {
        auto arg = std::string(argv[i]);
        if (arg == "--command") {
            if (i + 1 < argc) {
                command = argv[++i];
                slc::log() << "Using command: " << command;
            }
        }
        if (arg == "--archive") {
            if (i + 1 < argc) {
                archive = argv[++i];
                slc::log() << "Using directory: " << archive;
            }
        }
    }

    wiringPiSetup();
    wiringPiSPISetup(0, 500000);

    LoraRadioPi radio(PIN_SELECT, PIN_RESET, PIN_DIO_0, 0);

    // This is purely to ensure the mutex inside is ready. This can be forgiving
    // until you start using the heap, etc...
    radio.setup();

    Processor processor{ command };
    auto callbacks = ArchivingGatewayCallbacks{ archive };
    auto protocol = GatewayNetworkProtocol{ radio, callbacks };

    processor.start();

    while (true) {
        radio.tick();
        protocol.tick();

        auto &incoming = radio.getIncoming();
        if (incoming.size() > 0) {
            auto lora = incoming.front();
            incoming.pop();
            protocol.push(lora);

            auto &pending = callbacks.pending();
            while (pending.size() > 0) {
                processor.push(pending.front());
                pending.pop();
            }
        }

        delay(10);
    }

    return 0;

}

