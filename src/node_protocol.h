#ifndef SLC_NODE_PROTOCOL_H_INCLUDED
#define SLC_NODE_PROTOCOL_H_INCLUDED

#include "protocol.h"

template<size_t Size>
struct HoldingBuffer {
    lws::AlignedStorageBuffer<Size> buffer;
    size_t pos{ 0 };

    lws::BufferPtr toBufferPtr() {
        return buffer.toBufferPtr();
    }

    size_t position() {
        return pos;
    }

    void position(size_t newPos) {
        pos = newPos;
    }
};

class NodeNetworkProtocol : public NetworkProtocol {
private:
    NodeLoraId nodeId;
    HoldingBuffer<242 - 24> buffer;
    lws::Reader *reader{ nullptr };

public:
    NodeNetworkProtocol(PacketRadio &radio) : NetworkProtocol(radio) {
    }

public:
    void setNodeId(NodeLoraId id) {
        nodeId = id;
    }

public:
    void tick();
    void push(LoraPacket &lora);
    void sendToGateway();

};

#endif
