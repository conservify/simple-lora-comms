#ifndef SLC_GATEWAY_PROTOCOL_H_INCLUDED
#define SLC_GATEWAY_PROTOCOL_H_INCLUDED

#include "protocol.h"

class GatewayNetworkCallbacks {
public:
    virtual lws::Writer *openWriter(RadioPacket &packet) = 0;
    virtual void closeWriter(lws::Writer *writer) = 0;

};

class GatewayNetworkProtocol : public NetworkProtocol {
private:
    GatewayNetworkCallbacks *callbacks{ nullptr };
    uint8_t nextAddress{ 1 };
    size_t totalReceived{ 0 };
    uint8_t receiveSequence{ 0 };
    lws::Writer *writer{ nullptr };

public:
    GatewayNetworkProtocol(PacketRadio &radio, GatewayNetworkCallbacks &callbacks) : NetworkProtocol(radio), callbacks(&callbacks) {
    }

public:
    void tick();
    void push(LoraPacket &lora);

};

#endif
