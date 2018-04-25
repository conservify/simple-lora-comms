#include <cstring>
#include <cstdio>

#include "lora_radio_pi.h"

modem_config_t Bw125Cr45Sf128 = { 0x72, 0x74, 0x00};

lora_packet_t *lora_packet_new(size_t size) {
    auto lora = (lora_packet_t *)malloc(sizeof(lora_packet_t) + size);
    memset((void *)lora, 0, sizeof(lora_packet_t) + size);

    lora->size = size;
    lora->data = ((uint8_t *)lora) + sizeof(lora_packet_t);
    lora->to = 0xff;
    lora->from = 0xff;

    return lora;
}

lora_packet_t *lora_packet_create_from(raw_packet_t *raw) {
    if (raw->size <= SX1272_HEADER_LENGTH) {
        return NULL;
    }

    auto lora = (lora_packet_t *)malloc(sizeof(lora_packet_t));
    lora->size = raw->size - SX1272_HEADER_LENGTH;
    lora->data = raw->data + SX1272_HEADER_LENGTH;
    lora->to = raw->data[0];
    lora->from = raw->data[1];
    lora->id = raw->data[2];
    lora->flags = raw->data[3];

    return lora;
}

void log_raw_packet(FILE *fp, raw_packet_t *raw_packet, lora_packet_t *lora_packet) {
    fprintf(fp, "%02x,%02x,%02x,%02x,", lora_packet->to, lora_packet->from, lora_packet->id, lora_packet->flags);
    fprintf(fp, "%4d,", raw_packet->packet_rssi);
    fprintf(fp, "%4d,", raw_packet->rssi);
    fprintf(fp, "%2d,", raw_packet->snr);
    fprintf(fp, "%d,", (int32_t)raw_packet->size);
    fprintf(fp, "%d,", (int32_t)lora_packet->size);
    fprintf(fp, " ");

    #if 1
    for (auto i = 0; i < lora_packet->size; ++i) {
        fprintf(fp, "%02x '%c' ", lora_packet->data[i], lora_packet->data[i]);
    }
    #endif
    fprintf(fp, "\n");
}

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
    pinMode(pinCs, OUTPUT);
    pinMode(pinDio0, INPUT);
    pinMode(pinReset, OUTPUT);

    radios_for_isr[0] = this;

    wiringPiISR(pinDio0, INT_EDGE_RISING, handle_isr);

    reset();

    if (!detectChip()) {
        return false;
    }

    pthread_mutex_init(&mutex, NULL);

    spiWrite(RH_RF95_REG_01_OP_MODE, RH_RF95_MODE_SLEEP | RH_RF95_LONG_RANGE_MODE);
    delay(10);

    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, spiRead(RH_RF95_REG_0F_FIFO_RX_BASE_ADDR));
    spiWrite(RH_RF95_REG_23_MAX_PAYLOAD_LENGTH, 0x80);
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

void LoraRadioPi::sendPacket(lora_packet_t *packet) {
    setModeIdle();

    spiWrite(RH_RF95_REG_0E_FIFO_TX_BASE_ADDR, 0);
    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, 0);

    spiWrite(RH_RF95_REG_00_FIFO, packet->to);
    spiWrite(RH_RF95_REG_00_FIFO, packet->from);
    spiWrite(RH_RF95_REG_00_FIFO, packet->id);
    spiWrite(RH_RF95_REG_00_FIFO, packet->flags);

    for (auto i = 0; i < packet->size; ++i) {
        spiWrite(RH_RF95_REG_00_FIFO, packet->data[i]);
    }

    spiWrite(RH_RF95_REG_22_PAYLOAD_LENGTH, packet->size + SX1272_HEADER_LENGTH);

    setModeTx();
}

void LoraRadioPi::service() {
    pthread_mutex_lock(&mutex);

    uint8_t flags = spiRead(RH_RF95_REG_12_IRQ_FLAGS);

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

void LoraRadioPi::receive() {
    auto raw_packet = readRawPacket();
    if (raw_packet != NULL) {
        auto lora_packet = lora_packet_create_from(raw_packet);
        if (lora_packet != NULL) {
            log_raw_packet(stdout, raw_packet, lora_packet);

            free(lora_packet);
            lora_packet = NULL;
        }

        free(raw_packet);
        raw_packet = NULL;
    }
}

void LoraRadioPi::tick() {
    lock();

    if (!available) {
        if (setup()) {
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

raw_packet_t *LoraRadioPi::readRawPacket() {
    auto currentAddress = spiRead(RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR);
    auto receivedBytes = spiRead(RH_RF95_REG_13_RX_NB_BYTES);

    auto pkt = (raw_packet_t *)malloc(sizeof(raw_packet_t) + receivedBytes + 1);
    pkt->size = receivedBytes;
    pkt->data = ((uint8_t *)pkt) + sizeof(raw_packet_t);

    spiWrite(RH_RF95_REG_0D_FIFO_ADDR_PTR, currentAddress);

    for (auto i = 0; i < receivedBytes; i++) {
        pkt->data[i] = spiRead(RH_RF95_REG_00_FIFO);
    }

    pkt->packet_rssi = getPacketRssi();
    pkt->rssi = getRssi();
    pkt->snr = getSnr();

    return pkt;
}

int8_t LoraRadioPi::spiRead(int8_t address) {
    uint8_t buffer[2];

    buffer[0] = address & 0x7F;
    buffer[1] = 0x00;

    digitalWrite(pinCs, LOW);
    wiringPiSPIDataRW(spiChannel, buffer, 2);
    digitalWrite(pinCs, HIGH);

    return buffer[1];
}

void LoraRadioPi::spiWrite(int8_t address, int8_t value) {
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
