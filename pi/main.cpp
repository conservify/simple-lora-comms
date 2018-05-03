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

#include "lora_radio_pi.h"
#include "gateway_protocol.h"
#include "file_writer.h"

constexpr uint8_t PIN_SELECT = 6;
constexpr uint8_t PIN_DIO_0 = 7;
constexpr uint8_t PIN_RESET = 0;

using path = std::experimental::filesystem::path;

class ArchivingGatewayCallbacks : public GatewayNetworkCallbacks {
private:
    std::string archivePath;

public:
    ArchivingGatewayCallbacks(std::string archivePath) : archivePath(archivePath) {
    }

public:
    lws::Writer *openWriter(RadioPacket &packet) override {
        auto path = getPath(packet);
        return new FileWriter(path);
    }

    void closeWriter(lws::Writer *writer) override {
        if (writer != nullptr) {
            writer->close();
        }
    }

private:
    std::string getNodeId(RadioPacket &packet) {
        auto& id = packet.getNodeId();
        std::stringstream ss;
        for (auto i = 0; i < (int32_t)id.size; ++i) {
            ss << std::setfill('0') << std::setw(2) << std::hex << (int32_t)id.ptr[i];
        }
        return ss.str();
    }

    path getPath(RadioPacket &packet) {
        std::time_t t = std::time(nullptr);
        std::tm tm = *std::localtime(&t);
        std::stringstream buffer;
        buffer << archivePath << "/";
        buffer << getNodeId(packet) << "/";
        buffer << std::put_time(&tm, "%Y%m%d/%H%M%S") << "/";
        buffer << "fkn.fkpb";
        return path{ buffer.str() };
    }

};

int32_t main(int32_t argc, const char **argv) {
    wiringPiSetup();
    wiringPiSPISetup(0, 500000);

    LoraRadioPi radio(PIN_SELECT, PIN_RESET, PIN_DIO_0, 0);

    // This is purely to ensure the mutex inside is ready. This can be forgiving
    // until you start using the heap, etc...
    radio.setup();

    auto callbacks = ArchivingGatewayCallbacks{ "./archive" };
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

