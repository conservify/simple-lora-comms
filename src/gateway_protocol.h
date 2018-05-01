#ifndef SLC_GATEWAY_PROTOCOL_H_INCLUDED
#define SLC_GATEWAY_PROTOCOL_H_INCLUDED

#include "protocol.h"

class GatewayNetworkProtocol : public NetworkProtocol {
private:
    uint8_t nextAddress{ 1 };
    size_t totalReceived{ 0 };
    uint8_t receiveSequence{ 0 };
    lws::Writer *writer{ nullptr };

public:
    GatewayNetworkProtocol(PacketRadio &radio) : NetworkProtocol(radio) {
    }

public:
    void tick();
    void push(LoraPacket &lora);

};

#endif
