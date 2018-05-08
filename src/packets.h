#ifndef SLC_PACKETS_H_INCLUDED
#define SLC_PACKETS_H_INCLUDED

#include <pb_encode.h>
#include <pb_decode.h>
#include <fk-radio.pb.h>

#include "device_id.h"

struct RawPacket {
public:
    int32_t size{ 0 };
    uint8_t data[255];
    int32_t packetRssi{ 0 };
    int32_t rssi{ 0 };
    int32_t snr{ 0 };

public:
    uint8_t& operator[] (size_t i) {
        return data[i];
    }

    void clear() {
        size = 0;
    }

};

struct LoraPacket {
    static constexpr int32_t SX1272_HEADER_LENGTH = 4;

public:
    uint8_t to{ 0xff };
    uint8_t from{ 0xff };
    uint8_t id{ 0 };
    uint8_t flags{ 0 };
    int32_t size{ 0 };
    uint8_t data[255] = { 0 };

public:
    LoraPacket() {
    }

    LoraPacket(RawPacket &raw);

};

inline LogStream& operator<<(LogStream &log, const fk_radio_PacketKind &kind) {
    switch (kind) {
    case fk_radio_PacketKind_ACK: return log.print("Ack");
    case fk_radio_PacketKind_NACK: return log.print("NAck");
    case fk_radio_PacketKind_PING: return log.print("Ping");
    case fk_radio_PacketKind_PONG: return log.print("Pong");
    case fk_radio_PacketKind_PREPARE: return log.print("Prepare");
    case fk_radio_PacketKind_DATA: return log.print("Data");
    default:
        return log.print("Unknown");
    }
}

struct pb_data_t {
    uint8_t *ptr{ nullptr };
    size_t size{ 0 };
};

class RadioPacket {
private:
    fk_radio_RadioPacket message = fk_radio_RadioPacket_init_default;
    NodeLoraId nodeId;
    pb_data_t nodeIdInfo;
    pb_data_t dataInfo;

public:
    RadioPacket() {
    }

    RadioPacket(fk_radio_PacketKind kind) {
        message.kind = kind;
    }

    RadioPacket(fk_radio_PacketKind kind, NodeLoraId &nodeId) : nodeId(nodeId) {
        message.kind = kind;
    }

public:
    bool decode(LoraPacket &lora);

    fk_radio_RadioPacket *forDecode();
    fk_radio_RadioPacket *forEncode();

    void data(uint8_t *ptr, size_t size) {
        dataInfo.ptr = ptr;
        dataInfo.size = size;
    }

    pb_data_t data() {
        return dataInfo;
    }

    size_t size() {
        return dataInfo.size;
    }

    void clear() {
        message = fk_radio_RadioPacket_init_default;
    }

    NodeLoraId &getNodeId() {
        return nodeId;
    }

    fk_radio_RadioPacket &m() {
        return message;
    }

};

#endif
