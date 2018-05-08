#include "packets.h"

LoraPacket::LoraPacket(RawPacket &raw) {
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

bool pb_encode_data(pb_ostream_t *stream, const pb_field_t *field, void *const *arg) {
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

bool pb_decode_data(pb_istream_t *stream, const pb_field_t *field, void **arg) {
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

fk_radio_RadioPacket *RadioPacket::forDecode() {
    nodeIdInfo.ptr = nodeId.ptr;
    nodeIdInfo.size = nodeId.size;

    message.nodeId.funcs.decode = pb_decode_data;
    message.nodeId.arg = (void *)&nodeIdInfo;
    message.data.funcs.decode = pb_decode_data;
    message.data.arg = (void *)&dataInfo;
    return &message;
}

fk_radio_RadioPacket *RadioPacket::forEncode() {
    nodeIdInfo.ptr = nodeId.ptr;
    nodeIdInfo.size = nodeId.size;

    message.nodeId.funcs.encode = pb_encode_data;
    message.nodeId.arg = (void *)&nodeIdInfo;
    message.data.funcs.encode = pb_encode_data;
    message.data.arg = (void *)&dataInfo;
    return &message;
}

bool RadioPacket::decode(LoraPacket &lora) {
    if (lora.size > 0) {
        auto stream = pb_istream_from_buffer(lora.data, lora.size);
        if (!pb_decode(&stream, fk_radio_RadioPacket_fields, forDecode())) {
            return false;
        }
    }
    else {
        if (lora.flags == 1) {
            message.kind = fk_radio_PacketKind_ACK;
        }
    }
    return true;
}
