#pragma once

struct RawPacket {
public:
    int32_t size{ 0 };
    uint8_t data[256];
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
    uint8_t data[256] = { 0 };

public:
    LoraPacket() {
    }

    LoraPacket(RawPacket &raw) {
        if (raw.size < (int32_t)SX1272_HEADER_LENGTH) {
            return;
        }
        if (raw.size > (int32_t)sizeof(data)) {
            return;
        }

        size = raw.size - SX1272_HEADER_LENGTH;
        memcpy(data, raw.data + SX1272_HEADER_LENGTH, size);

        to = raw.data[0];
        from = raw.data[1];
        id = raw.data[2];
        flags = raw.data[3];
    }

    template<class T>
    void set(T &body) {
        to = 0x00;
        from = 0xff;
        memcpy(data, (uint8_t *)&body, sizeof(T));
        size = sizeof(T);
    }

};

enum class PacketKind {
    Ack,
    Nack,
    Ping,
    Pong,
    Prepare,
    Data,
};

inline Logger& operator<<(Logger &log, const PacketKind &kind) {
    switch (kind) {
    case PacketKind::Ack: return log.print("Ack");
    case PacketKind::Nack: return log.print("Nack");
    case PacketKind::Ping: return log.print("Ping");
    case PacketKind::Pong: return log.print("Pong");
    case PacketKind::Prepare: return log.print("Prepare");
    case PacketKind::Data: return log.print("Data");
    default:
        return log.print("Unknown");
    }
}

struct ApplicationPacket {
    PacketKind kind;
    DeviceId deviceId;

    ApplicationPacket(PacketKind kind) : kind(kind) {
    }

    ApplicationPacket(PacketKind kind, DeviceId deviceId) : kind(kind), deviceId(deviceId) {
    }

    ApplicationPacket(LoraPacket &lora) {
        assert(lora.size >= (int32_t)sizeof(ApplicationPacket));
        memcpy((void *)this, (void *)&lora.data, sizeof(ApplicationPacket));
    }
};
