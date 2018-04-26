#include <cstdarg>

#include <Arduino.h>

#include "lora_radio.h"
#include "protocol.h"

constexpr size_t FK_DEBUG_LINE_MAX = 256;

void fklog(const char *f, ...) {
    char buffer[FK_DEBUG_LINE_MAX];
    va_list args;

    va_start(args, f);
    vsnprintf(buffer, FK_DEBUG_LINE_MAX, f, args);
    va_end(args);

    Serial.print(buffer);
}

// 0xD1,0x0B,0x7D,0x76,0xC2,0xE1
// 0xA6,0x03,0x11,0x03,0x14,0xBD

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        delay(10);
    }

    Serial.println("lora-test: Hello!");

    LoraRadio radio{ 5, 2, 0, 3 };
    if (!radio.setup()) {
        while (true);
    }

    Serial.println("lora-test: Ready");

    pinMode(13, OUTPUT);

    radio.setHeaderTo(0xff);
    radio.setHeaderFrom(0x00);
    radio.setThisAddress(0x00);

    while (true) {
        digitalWrite(13, HIGH);

        fklog("lora-test: Tx\n");

        fk_lora_packet_t packet{ LoraPacketKind::Ping };
        if (!radio.send((uint8_t *)&packet, sizeof(fk_lora_packet_t))) {
            fklog("lora-test: Failed to send\n");
        }
        else {
            radio.waitPacketSent();
        }

        auto started = millis();
        while (true) {
            radio.tick();

            if (radio.hasPacket()) {
                fklog("lora-test: Got packet! %x %x\n", radio.headerTo(), radio.headerFrom());
                break;
            }

            if (millis() - started > 1000) {
                fklog("lora-test: Timeout!\n");
                break;
            }
        }

        digitalWrite(13, LOW);

        delay(100);
    }
}

void loop() {
}


