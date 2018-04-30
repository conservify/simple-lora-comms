#include <cstdarg>

#include <Arduino.h>
#include <Wire.h>

#include <lwstreams/streams.h>

#include "lora_radio.h"
#include "protocol.h"
#include "debug.h"

class MacAddressEeprom {
private:
    TwoWire *bus;
    uint8_t address{ 0x50 };

public:
    MacAddressEeprom(TwoWire &bus) : bus(&bus) {
    }

public:
    bool read128bMac(DeviceId &id) {
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

    DeviceId id;
    if (!eeprom.read128bMac(id)) {
        fklogln("lora-test: No address");
        while (true);
    }
    fklog("lora-test: Address: ");
    for (auto i = 0; i < 8; ++i) {
        fklog("%02x", id[i]);
    }
    fklogln("");

    LoraRadio radio{ 5, 2, 0, 3 };
    if (!radio.setup()) {
        fklogln("lora-test: No radio");
        while (true);
    }

    fklogln("lora-test: Ready");

    pinMode(13, OUTPUT);

    radio.setHeaderTo(0xff);
    radio.setHeaderFrom(0x00);
    radio.setThisAddress(0x00);

    auto protocol = NodeNetworkProtocol{ id, radio };

    while (true) {
        radio.tick();
        protocol.tick();

        if (radio.hasPacket()) {
            auto lora = radio.getLoraPacket();
            protocol.push(lora);
        }

        delay(10);
    }
}

void loop() {
}


