#include <cstdarg>

#include <Arduino.h>
#include <Wire.h>

#include <lwstreams/streams.h>

#include "lora_radio_rh.h"
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
    bool read128bMac(DeviceIdBuffer &id) {
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

    DeviceIdBuffer deviceId;
    if (!eeprom.read128bMac(deviceId)) {
        logger.print("lora-test: No address\n");
        while (true);
    }
    logger.print("lora-test: Address: ");
    for (auto i = 0; i < 8; ++i) {
        logger.printf("%02x", deviceId[i]);
    }
    logger.print("\n");

    LoraRadioRadioHead radio{ 5, 2, 0, 3 };
    if (!radio.setup()) {
        logger.print("lora-test: No radio\n");
        while (true);
    }

    logger.print("lora-test: Ready\n");

    pinMode(13, OUTPUT);

    radio.setHeaderTo(0xff);
    radio.setHeaderFrom(0x00);
    radio.setThisAddress(0x00);

    auto protocol = NodeNetworkProtocol{ radio };

    protocol.setDeviceId(deviceId);

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


