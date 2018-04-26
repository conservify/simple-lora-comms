#include <cstring>
#include <cstdio>

#include "lora_radio_pi.h"
#include "protocol.h"

modem_config_t Bw125Cr45Sf128 = { 0x72, 0x74, 0x00};

static LoraRadioPi *radios_for_isr[1] = { nullptr };

static void handle_isr() {
    auto radio = radios_for_isr[0];
    if (radio != nullptr) {
        radio->service();
    }
}

LoraRadioPi::LoraRadioPi(uint8_t pinCs, uint8_t pinReset, uint8_t pinDio0, uint8_t spiChannel) : pinCs(pinCs), pinReset(pinReset), pinDio0(pinDio0), spiChannel(spiChannel) {
}

LoraRadioPi::~LoraRadioPi() {
    pthread_mutex_destroy(&mutex);
}

bool LoraRadioPi::setup() {
    pthread_mutex_init(&mutex, NULL);

    return true;
}

bool LoraRadioPi::begin() {
    pinMode(pinCs, OUTPUT);
    pinMode(pinDio0, INPUT);
    pinMode(pinReset, OUTPUT);

    radios_for_isr[0] = this;

    wiringPiISR(pinDio0, INT_EDGE_RISING, handle_isr);

    reset();

    if (!detectChip()) {
        return false;
    }

    spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);
    delay(10);

    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, spiRead(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));
    spiWrite(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0xF2);
    spiWrite(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spiWrite(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR, 0);

    setModeIdle();

    setModemConfig(&Bw125Cr45Sf128);
    setFrequency(915.0f);
    setPreambleLength(8);
    setTxPower(13);

    return true;
}

bool LoraRadioPi::detectChip() {
    uint8_t version = spiRead(RH_RF95_REG_42_VERSION);
    if (version == 0) {
        // fprintf(stderr, "No radio found\n");
        return false;
    }
    if (version != 0x12) {
        // fprintf(stderr, "Unrecognized transceiver (Version: 0x%x)\n", version);
        return false;
    }
    return true;
}

void LoraRadioPi::lock() {
    pthread_mutex_lock(&mutex);
}

void LoraRadioPi::unlock() {
    pthread_mutex_unlock(&mutex);
}

uint8_t LoraRadioPi::getMode() {
    return spiRead(RH_RF95_REG_01_OP_MODE);
}

void LoraRadioPi::setModeRx() {
    if (mode != RH_RF95_MODE_RXCONTINUOUS) {
        spiWrite(RH_RF95_REG_40_DIO_MAPPING1, 0x00); // IRQ on RxDone
        spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_RXCONTINUOUS);
        mode = RH_RF95_MODE_RXCONTINUOUS;
    }
}

void LoraRadioPi::setModeIdle() {
    if (mode != RH_RF95_MODE_STDBY) {
        spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_STDBY);
        mode = RH_RF95_MODE_STDBY;
    }
}

void LoraRadioPi::setModeTx() {
    if (mode != RH_RF95_MODE_TX) {
        spiWrite(RH_RF95_REG_40_DIO_MAPPING1, 0x40); // IRQ on TxDone
        spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_TX);
        mode = RH_RF95_MODE_TX;
    }
}

bool LoraRadioPi::isModeStandby() {
    return mode == RH_RF95_MODE_STDBY;
}

bool LoraRadioPi::isModeRx() {
    return mode == RH_RF95_MODE_RXCONTINUOUS;
}

bool LoraRadioPi::isAvailable() {
    return available;
}

bool LoraRadioPi::sendPacket(RadioPacket &packet) {
    size_t required = 0;
    if (!pb_get_encoded_size(&required, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    char buffer[required];
    auto stream = pb_ostream_from_buffer((uint8_t *)buffer, required);
    if (!pb_encode(&stream, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    LoraPacket lora;
    memcpy(lora.data, buffer, stream.bytes_written);
    lora.size = stream.bytes_written;
    logger << "Radio: S " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " (" << stream.bytes_written << " bytes)\n";
    sendPacket(lora);
    return true;
}

void LoraRadioPi::sendPacket(LoraPacket &packet) {
    setModeIdle();

    spiWrite(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);

    spiWrite(RH_RF95_REG_00_FIFO, packet.to);
    spiWrite(RH_RF95_REG_00_FIFO, packet.from);
    spiWrite(RH_RF95_REG_00_FIFO, packet.id);
    spiWrite(RH_RF95_REG_00_FIFO, packet.flags);

    for (auto i = 0; i < packet.size; ++i) {
        spiWrite(RH_RF95_REG_00_FIFO, packet.data[i]);
    }

    spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, packet.size + SX1272_HEADER_LENGTH);

    setModeTx();
}

void LoraRadioPi::service() {
    pthread_mutex_lock(&mutex);

    auto flags = spiRead(RH_RF95_REG_12_IRQ_FLAGS);

    if ((flags & RH_RF95_PAYLOAD_CRC_ERROR_MASK) == RH_RF95_PAYLOAD_CRC_ERROR_MASK) {
        // Oh no
    }
    else if ((flags & RH_RF95_RX_DONE) == RH_RF95_RX_DONE) {
        receive();
        setModeIdle();
    }
    else if ((flags & RH_RF95_TX_DONE) == RH_RF95_TX_DONE) {
        setModeIdle();
    }

    spiWrite(RH_RF95_REG_12_IRQ_FLAGS, 0xff); // clear all IRQ flags

    pthread_mutex_unlock(&mutex);
}

void LoraRadioPi::waitPacketSent() {
    while (!isModeStandby()) {
    }
}

void LoraRadioPi::receive() {
    RawPacket raw;
    if (readRawPacket(raw)) {
        LoraPacket lora(raw);
        if (lora.size > 0) {
            incoming.emplace(lora);
            return;
        }
    }
}

void LoraRadioPi::tick() {
    lock();

    if (!available) {
        if (begin()) {
            setModeRx();
            available = true;
        }
    }
    else {
        if (time(NULL) - checkedAt > checkRadioEvery) {
            available = detectChip();
            checkedAt = time(NULL);
        }

        if (isModeStandby()) {
            setModeRx();
        }
    }

    unlock();
}

bool LoraRadioPi::readRawPacket(RawPacket &raw) {
    auto currentAddress = spiRead(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
    auto receivedBytes = spiRead(RH_RF95_REG_13_RX_NB_BYTES);

    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, currentAddress);

    for (auto i = 0; i < receivedBytes; i++) {
        raw[i] = spiRead(RH_RF95_REG_00_FIFO);
    }
    raw.size = receivedBytes;
    raw.packetRssi = getPacketRssi();
    raw.rssi = getRssi();
    raw.snr = getSnr();

    return true;
}

uint8_t LoraRadioPi::spiRead(int8_t address) {
    uint8_t buffer[2];

    buffer[0] = address & 0x7F;
    buffer[1] = 0x00;

    digitalWrite(pinCs, LOW);
    wiringPiSPIDataRW(spiChannel, buffer, 2);
    digitalWrite(pinCs, HIGH);

    return buffer[1];
}

void LoraRadioPi::spiWrite(int8_t address, uint8_t value) {
    uint8_t buffer[2];

    buffer[0] = address | 0x80;
    buffer[1] = value;

    digitalWrite(pinCs, LOW);
    wiringPiSPIDataRW(spiChannel, buffer, 2);
    digitalWrite(pinCs, HIGH);
}

void LoraRadioPi::setFrequency(float centre) {
    uint32_t frf = (centre * 1000000.0) / RH_RF95_FSTEP;
    spiWrite(RH_RF95_REG_06_FRF_MSB, (uint8_t)((frf >> 16) & 0xff));
    spiWrite(RH_RF95_REG_07_FRF_MID, (uint8_t)((frf >> 8) & 0xff));
    spiWrite(RH_RF95_REG_08_FRF_LSB, (uint8_t)((frf) & 0xff));
}

void LoraRadioPi::setModemConfig(modem_config_t *config) {
    spiWrite(RH_RF95_REG_1D_MODEM_CONFIG1, config->reg_1d);
    spiWrite(RH_RF95_REG_1E_MODEM_CONFIG2, config->reg_1e);
    spiWrite(RH_RF95_REG_26_MODEM_CONFIG3, config->reg_26);
}

void LoraRadioPi::setTxPower(int8_t power) {
    if (power > 20)
        power = 20;
    if (power < 5)
        power = 5;
    spiWrite(RH_RF95_REG_09_PA_CONFIG, RH_RF95_PA_SELECT | (power - 5));
}

void LoraRadioPi::setPreambleLength(uint16_t length) {
    spiWrite(RH_RF95_REG_20_PREAMBLE_MSB, length >> 8);
    spiWrite(RH_RF95_REG_21_PREAMBLE_LSB, length & 0xff);
}

int32_t LoraRadioPi::getSnr() {
    auto value = spiRead(RH_RF95_REG_19_PKT_SNR_VALUE);
    if (value & 0x80) {
        value = ((~value + 1) & 0xFF) >> 2;
        return -value;
    }
    else {
        return (value & 0xFF) >> 2;
    }
}

int32_t LoraRadioPi::getPacketRssi() {
    return spiRead(RH_RF95_REG_1A_PKT_RSSI_VALUE) - RH_RF95_RSSI_CORRECTION;
}

int32_t LoraRadioPi::getRssi() {
    return spiRead(RH_RF95_REG_1B_RSSI_VALUE) - RH_RF95_RSSI_CORRECTION;
}

void LoraRadioPi::reset() {
    digitalWrite(pinReset, LOW);
    delay(100);
    digitalWrite(pinReset, HIGH);
    delay(100);
}
