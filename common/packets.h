#pragma once

#include <pb_encode.h>
#include <pb_decode.h>
#include <fk-radio.pb.h>

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

};

inline Logger& operator<<(Logger &log, const fk_radio_PacketKind &kind) {
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

inline bool pb_encode_data(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
    auto data = (pb_data_t *)*arg;

    if (data == nullptr) {
        return true;
    }

    if (!pb_encode_tag_for_field(stream, field)) {
        return false;
    }

    if (!pb_encode_varint(stream, data->size)) {
        return false;
    }

    if (!pb_write(stream, data->ptr, data->size)) {
        return false;
    }

    return true;
}

inline bool pb_decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
    auto data = (pb_data_t *)(*arg);

    if (data->ptr == nullptr) {
        data->ptr = (uint8_t *)malloc(stream->bytes_left);
    }

    data->size = stream->bytes_left;

    if (!pb_read(stream, (pb_byte_t *)data->ptr, stream->bytes_left)) {
        return false;
    }

    return true;
}

class RadioPacket {
private:
    fk_radio_RadioPacket message = fk_radio_RadioPacket_init_default;
    DeviceId deviceId;
    pb_data_t deviceIdInfo;
    pb_data_t dataInfo;

public:
    RadioPacket() {
    }

    RadioPacket(fk_radio_PacketKind kind) {
        message.kind = kind;
    }

    RadioPacket(fk_radio_PacketKind kind, DeviceId &deviceId) : deviceId(deviceId) {
        message.kind = kind;
    }

    RadioPacket(LoraPacket &lora) {
        auto stream = pb_istream_from_buffer(lora.data, lora.size);
        if (!pb_decode(&stream, fk_radio_RadioPacket_fields, forDecode())) {
            fklogln("Unable to decode packet! %d", lora.size);
        }
    }

public:
    void data(uint8_t *ptr, size_t size) {
        dataInfo.ptr = ptr;
        dataInfo.size = size;
    }

    pb_data_t data() {
        return dataInfo;
    }

    void clear() {
        message = fk_radio_RadioPacket_init_default;
    }

    DeviceId &getDeviceId() {
        return deviceId;
    }

    fk_radio_RadioPacket *forDecode() {
        deviceIdInfo.ptr = deviceId.raw;
        deviceIdInfo.size = 8;

        message.deviceId.funcs.decode = pb_decode_data;
        message.deviceId.arg = (void *)&deviceIdInfo;
        message.data.funcs.decode = pb_decode_data;
        message.data.arg = (void *)&dataInfo;
        return &message;
    }

    fk_radio_RadioPacket *forEncode() {
        deviceIdInfo.ptr = deviceId.raw;
        deviceIdInfo.size = 8;

        message.deviceId.funcs.encode = pb_encode_data;
        message.deviceId.arg = (void *)&deviceIdInfo;
        message.data.funcs.encode = pb_encode_data;
        message.data.arg = (void *)&dataInfo;
        return &message;
    }

    fk_radio_RadioPacket &m() {
        return message;
    }

public:
    size_t size() {
        return dataInfo.size;
    }

};
