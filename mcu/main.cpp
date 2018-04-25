#include <Arduino.h>
#include <SPI.h>
#include <RH_RF95.h>

#define RF95_FREQ                                            915.0
constexpr size_t FK_QUEUE_ENTRY_SIZE = 128;

class LoraRadio {
private:
    uint8_t pinCs;
    uint8_t pinRst;
    RH_RF95 rf95;
    bool available;
    const uint8_t pinEnable;
    uint8_t sendBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t sendLength;
    uint8_t recvBuffer[FK_QUEUE_ENTRY_SIZE];
    uint8_t recvLength;
    uint8_t tries;

public:
    LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinRst);
    bool setup();
    void tick();
    bool send(uint8_t *packet, uint8_t size);
    bool reply(uint8_t *packet, uint8_t size);
    bool resend();

    uint8_t numberOfTries() {
        return tries;
    }
    bool isAvailable() {
        return available;
    }

    const uint8_t *getPacket() {
        return recvBuffer;
    }

    uint8_t getPacketSize() {
        return recvLength;
    }

    bool hasPacket() {
        return recvLength > 0;
    }

    bool isIdle() {
        return rf95.mode() == RHGenericDriver::RHMode::RHModeIdle;
    }

    void clear() {
        recvBuffer[0] = 0;
        recvLength = 0;
    }

    void wake() {
    }

    void powerOn() {
        digitalWrite(pinEnable, HIGH);
    }

    void powerOff() {
        digitalWrite(pinEnable, LOW);
    }

    void idle() {
        rf95.setModeIdle();
    }

    void reset() {
        powerOff();
        delay(10);
        powerOn();
        delay(10);
    }

    void waitPacketSent() {
        rf95.waitPacketSent();
    }

    void sleep() {
        rf95.sleep();
    }

    void printRegisters() {
        rf95.printRegisters();
    }

    uint8_t headerFrom() { return rf95.headerFrom(); }
    uint8_t headerTo() { return rf95.headerTo(); }
    uint8_t headerFlags() { return rf95.headerFlags(); }
    uint8_t headerId() { return rf95.headerId(); }
    uint8_t lastRssi() { return rf95.lastRssi(); }

    RHGenericDriver::RHMode mode() {
        return rf95.mode();
    }

    const char *modeName() {
        switch (mode()) {
        case RHGenericDriver::RHMode::RHModeInitialising: return "Initialising";
        case RHGenericDriver::RHMode::RHModeSleep: return "Sleep";
        case RHGenericDriver::RHMode::RHModeIdle: return "Idle";
        case RHGenericDriver::RHMode::RHModeTx: return "Tx";
        case RHGenericDriver::RHMode::RHModeRx: return "Rx";
        }
        return "Unknown";
    }
};

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

    while (true) {
        char buffer[] = "Hello, World";
        if (!radio.send((uint8_t *)buffer, sizeof(buffer))) {
            Serial.println("Failed to send");
        }
        else {
            Serial.print(".");
        }
        delay(1000);
    }
}

void loop() {
}


#define MAX_RETRIES 3

LoraRadio::LoraRadio(uint8_t pinCs, uint8_t pinG0, uint8_t pinEnable, uint8_t pinRst)
    : pinCs(pinCs), pinRst(pinRst), rf95(pinCs, pinG0), pinEnable(pinEnable), available(false) {
}

bool LoraRadio::setup() {
    if (available) {
        return true;
    }

    pinMode(pinCs, OUTPUT);
    digitalWrite(pinCs, HIGH);

    pinMode(pinRst, OUTPUT);
    digitalWrite(pinRst, HIGH);

    pinMode(pinEnable, OUTPUT);
    digitalWrite(pinEnable, HIGH);

    powerOn();
    delay(10);
    reset();

    if (!rf95.init()) {
        Serial.println(F("Radio missing"));
        return false;
    }

    if (!rf95.setFrequency(RF95_FREQ)) {
        Serial.println(F("Radio setup failed"));
        return false;
    }

    rf95.setTxPower(23, false);

    available = true;
    return true;
}

bool LoraRadio::send(uint8_t *packet, uint8_t size) {
    memcpy(sendBuffer, packet, size);
    sendLength = size;
    tries = 0;
    return resend();
}

bool LoraRadio::reply(uint8_t *packet, uint8_t size) {
    rf95.setHeaderTo(rf95.headerFrom());
    bool success = send(packet, size);
    rf95.setHeaderTo(RH_BROADCAST_ADDRESS);
    return success;
}

bool LoraRadio::resend() {
    if (tries == MAX_RETRIES) {
        tries = 0;
        return false;
    }
    tries++;
    return rf95.send(sendBuffer, sendLength);
}

void LoraRadio::tick() {
    if (rf95.available()) {
        recvLength = sizeof(recvBuffer);
        rf95.recv(recvBuffer, &recvLength);
    }
    else {
        recvLength = 0;
    }
}
