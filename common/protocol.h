#pragma once

#include <cstdint>

enum class LoraPacketKind {
    Ack,
    Ping,
    Pong
};

struct fk_lora_packet_t {
    LoraPacketKind kind;
    uint8_t deviceId[8];
};
