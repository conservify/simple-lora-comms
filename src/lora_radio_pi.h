#ifndef SLC_LORA_RADIO_PI_INCLUDED
#define SLC_LORA_RADIO_PI_INCLUDED
#ifndef ARDUINO

#include <pthread.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <queue>

#include <wiringPi.h>
#include <wiringPiSPI.h>

#include "protocol.h"

#define RH_RF95_RSSI_CORRECTION                            157

#define SX1272_HEADER_LENGTH                               4

#define RH_RF95_REG_00_FIFO                                0x00
#define RH_RF95_REG_01_OP_MODE                             0x01
#define RH_RF95_REG_02_RESERVED                            0x02
#define RH_RF95_REG_03_RESERVED                            0x03
#define RH_RF95_REG_04_RESERVED                            0x04
#define RH_RF95_REG_05_RESERVED                            0x05
#define RH_RF95_REG_06_FRF_MSB                             0x06
#define RH_RF95_REG_07_FRF_MID                             0x07
#define RH_RF95_REG_08_FRF_LSB                             0x08
#define RH_RF95_REG_09_PA_CONFIG                           0x09
#define RH_RF95_REG_0A_PA_RAMP                             0x0a
#define RH_RF95_REG_0B_OCP                                 0x0b
#define RH_RF95_REG_0C_LNA                                 0x0c
#define RH_RF95_REG_0D_FIFO_ADDR_PTR                       0x0d
#define RH_RF95_REG_0E_FIFO_TX_BASE_ADDR                   0x0e
#define RH_RF95_REG_0F_FIFO_RX_BASE_ADDR                   0x0f
#define RH_RF95_REG_10_FIFO_RX_CURRENT_ADDR                0x10
#define RH_RF95_REG_11_IRQ_FLAGS_MASK                      0x11
#define RH_RF95_REG_12_IRQ_FLAGS                           0x12
#define RH_RF95_REG_13_RX_NB_BYTES                         0x13
#define RH_RF95_REG_14_RX_HEADER_CNT_VALUE_MSB             0x14
#define RH_RF95_REG_15_RX_HEADER_CNT_VALUE_LSB             0x15
#define RH_RF95_REG_16_RX_PACKET_CNT_VALUE_MSB             0x16
#define RH_RF95_REG_17_RX_PACKET_CNT_VALUE_LSB             0x17
#define RH_RF95_REG_18_MODEM_STAT                          0x18
#define RH_RF95_REG_19_PKT_SNR_VALUE                       0x19
#define RH_RF95_REG_1A_PKT_RSSI_VALUE                      0x1a
#define RH_RF95_REG_1B_RSSI_VALUE                          0x1b
#define RH_RF95_REG_1C_HOP_CHANNEL                         0x1c
#define RH_RF95_REG_1D_MODEM_CONFIG1                       0x1d
#define RH_RF95_REG_1E_MODEM_CONFIG2                       0x1e
#define RH_RF95_REG_1F_SYMB_TIMEOUT_LSB                    0x1f
#define RH_RF95_REG_20_PREAMBLE_MSB                        0x20
#define RH_RF95_REG_21_PREAMBLE_LSB                        0x21
#define RH_RF95_REG_22_PAYLOAD_LENGTH                      0x22
#define RH_RF95_REG_23_MAX_PAYLOAD_LENGTH                  0x23
#define RH_RF95_REG_24_HOP_PERIOD                          0x24
#define RH_RF95_REG_25_FIFO_RX_BYTE_ADDR                   0x25
#define RH_RF95_REG_26_MODEM_CONFIG3                       0x26

#define RH_RF95_REG_39_SYNC_WORD                           0x39
#define RH_RF95_REG_40_DIO_MAPPING1                        0x40
#define RH_RF95_REG_41_DIO_MAPPING2                        0x41
#define RH_RF95_REG_42_VERSION                             0x42

#define RH_RF95_REG_4B_TCXO                                0x4b
#define RH_RF95_REG_4D_PA_DAC                              0x4d
#define RH_RF95_REG_5B_FORMER_TEMP                         0x5b
#define RH_RF95_REG_61_AGC_REF                             0x61
#define RH_RF95_REG_62_AGC_THRESH1                         0x62
#define RH_RF95_REG_63_AGC_THRESH2                         0x63
#define RH_RF95_REG_64_AGC_THRESH3                         0x64

// RH_RF95_REG_01_OP_MODE                                  0x01
#define RH_RF95_LONG_RANGE_MODE                            0x80
#define RH_RF95_ACCESS_SHARED_REG                          0x40
#define RH_RF95_MODE                                       0x07
#define RH_RF95_MODE_SLEEP                                 0x00
#define RH_RF95_MODE_STDBY                                 0x01
#define RH_RF95_MODE_FSTX                                  0x02
#define RH_RF95_MODE_TX                                    0x03
#define RH_RF95_MODE_FSRX                                  0x04
#define RH_RF95_MODE_RXCONTINUOUS                          0x05
#define RH_RF95_MODE_RXSINGLE                              0x06
#define RH_RF95_MODE_CAD                                   0x07

#define RH_RF95_LNA_MAX_GAIN                               0x23
#define RH_RF95_LNA_OFF_GAIN                               0x00
#define RH_RF95_LNA_LOW_GAIN		    	                     0x20

// RH_RF95_REG_0C_LNA                                      0x0c
#define RH_RF95_LNA_GAIN                                   0xe0
#define RH_RF95_LNA_BOOST                                  0x03
#define RH_RF95_LNA_BOOST_DEFAULT                          0x00
#define RH_RF95_LNA_BOOST_150PC                            0x11

// RH_RF95_REG_11_IRQ_FLAGS_MASK                           0x11
#define RH_RF95_RX_TIMEOUT_MASK                            0x80
#define RH_RF95_RX_DONE_MASK                               0x40
#define RH_RF95_PAYLOAD_CRC_ERROR_MASK                     0x20
#define RH_RF95_VALID_HEADER_MASK                          0x10
#define RH_RF95_TX_DONE_MASK                               0x08
#define RH_RF95_CAD_DONE_MASK                              0x04
#define RH_RF95_FHSS_CHANGE_CHANNEL_MASK                   0x02
#define RH_RF95_CAD_DETECTED_MASK                          0x01

// RH_RF95_REG_12_IRQ_FLAGS                                0x12
#define RH_RF95_RX_TIMEOUT                                 0x80
#define RH_RF95_RX_DONE                                    0x40
#define RH_RF95_PAYLOAD_CRC_ERROR                          0x20
#define RH_RF95_VALID_HEADER                               0x10
#define RH_RF95_TX_DONE                                    0x08
#define RH_RF95_CAD_DONE                                   0x04
#define RH_RF95_FHSS_CHANGE_CHANNEL                        0x02
#define RH_RF95_CAD_DETECTED                               0x01


#define SX72_MC2_FSK                                       0x00
#define SX72_MC2_SF7                                       0x70
#define SX72_MC2_SF8                                       0x80
#define SX72_MC2_SF9                                       0x90
#define SX72_MC2_SF10                                      0xA0
#define SX72_MC2_SF11                                      0xB0
#define SX72_MC2_SF12                                      0xC0

#define SX72_MC1_LOW_DATA_RATE_OPTIMIZE                    0x01 // mandated for SF11 and SF12

// RH_RF95_REG_09_PA_CONFIG                                0x09
#define RH_RF95_PA_SELECT                                  0x80
#define RH_RF95_OUTPUT_POWER                               0x0f

// set frequency
// uint64_t frf = ((uint64_t)freq << 19) / 32000000;
// The crystal oscillator frequency of the module
#define RH_RF95_FXOSC 32000000.0

// The Frequency Synthesizer step = RH_RF95_FXOSC / 2^^19
#define RH_RF95_FSTEP  (RH_RF95_FXOSC / 524288)

struct modem_config_t {
    uint8_t reg_1d;
    uint8_t reg_1e;
    uint8_t reg_26;
};

class LoraRadioPi : public PacketRadio {
private:
    pthread_mutex_t mutex;
    uint8_t number;
    uint8_t mode;
    uint8_t pinCs;
    uint8_t pinReset;
    uint8_t pinDio0;
    uint8_t spiChannel;
    uint8_t thisAddress{ 0xff };
    bool available{ false };
    uint32_t checkedAt{ 0 };
    uint32_t checkRadioEvery{ 1000 };
    std::queue<LoraPacket> incoming;
    std::queue<LoraPacket> outgoing;

public:
    LoraRadioPi(uint8_t pinCs, uint8_t pinReset, uint8_t pinDio0, uint8_t spiChannel);
    virtual ~LoraRadioPi();

public:
    bool setup();
    bool begin();
    bool detectChip();

    uint8_t getMode();
    void setModeTx();
    void setModeRx() override;
    void setModeIdle() override;

    bool isModeRx() override;

    bool isModeTx() override {
        return mode == RH_RF95_MODE_TX;
    }

    bool isIdle() override {
        return isModeStandby();
    }

    bool isModeStandby();
    bool isAvailable();

    void setThisAddress(uint8_t address) override;
    bool sendPacket(LoraPacket &packet) override;
    void service();

    void tick();

    std::queue<LoraPacket>& getIncoming() {
        return incoming;
    }

private:
    void lock();
    void unlock();

    uint8_t spiRead(int8_t address);
    void spiWrite(int8_t address, uint8_t value);

    void setFrequency(float centre);
    void setModemConfig(modem_config_t *config);
    void setTxPower(int8_t power);
    void setPreambleLength(uint16_t length);
    void reset();

    int32_t getSnr() ;
    int32_t getPacketRssi();
    int32_t getRssi();

    bool readRawPacket(RawPacket &raw);
    void receive();

};

#endif
#endif
