#include <cstdarg>

#include <Arduino.h>
#include <Wire.h>

#include <lwstreams/streams.h>
#include <alogging/alogging.h>

#include "lora_radio_rh.h"
#include "node_protocol.h"

class MacAddressEeprom {
private:
    TwoWire *bus;
    uint8_t address{ 0x50 };

public:
    MacAddressEeprom(TwoWire &bus) : bus(&bus) {
    }

public:
    bool read128bMac(NodeLoraId &id) {
        bus->beginTransmission(address);
        bus->write(0xf8);
        if (bus->endTransmission() != 0) {
            return false;
        }

        bus->requestFrom(address, 8);

        size_t bytes = 0;
        while (bus->available()) {
            id[bytes++] = bus->read();
            if (bytes == 8) {
                break;
            }
        }

        return true;
    }
};

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(10);
    }

    Wire.begin();

    MacAddressEeprom eeprom{ Wire };

    NodeLoraId nodeId;
    if (!eeprom.read128bMac(nodeId)) {
        slc::log() << "lora-test: No address";
        while (true);
    }

    LoraRadioRadioHead radio{ 5, 2, 0, 3 };
    if (!radio.setup()) {
        slc::log() << "lora-test: No radio";
        while (true);
    }

    slc::log() << "lora-test: Ready";

    pinMode(13, OUTPUT);

    radio.setHeaderTo(0xff);
    radio.setHeaderFrom(0x00);
    radio.setThisAddress(0x00);

    auto protocol = NodeNetworkProtocol{ radio };

    protocol.setNodeId(nodeId);

    while (true) {
        protocol.tick();

        if (protocol.hasBeenSleepingFor(20000))  {
            protocol.sendToGateway();
        }

        if (radio.hasPacket()) {
            auto lora = radio.getLoraPacket();
            protocol.push(lora);
        }

        delay(10);
    }
}

void loop() {
}


