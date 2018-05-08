#ifndef SLC_GATEWAY_PROTOCOL_H_INCLUDED
#define SLC_GATEWAY_PROTOCOL_H_INCLUDED

#include <alogging/alogging.h>

#include "protocol.h"
#include "device_id.h"

class GatewayNetworkCallbacks {
public:
    virtual lws::Writer *openWriter(RadioPacket &packet) = 0;
    virtual void closeWriter(lws::Writer *writer, bool success) = 0;

};

class CurrentNodeTracker {
private:
    uint32_t lastPing_{ 0 };
    uint8_t address_{ 1 };
    NodeLoraId id_;

public:
    uint8_t address() {
        return address_;
    }

public:
    bool canPongPing(RadioPacket &radio);

};

class DownloadTracker {
private:
    GatewayNetworkCallbacks *callbacks_{ nullptr };
    size_t received_{ 0 };
    size_t expected_{ 0 };
    uint8_t receiveSequence_{ 0 };
    lws::Writer *writer_{ nullptr };

public:
    DownloadTracker(GatewayNetworkCallbacks &callbacks);

public:
    bool prepare(LogStream &log, LoraPacket &lora, RadioPacket &radio);
    bool download(LogStream &log, LoraPacket &lora, RadioPacket &packet);

};

class GatewayNetworkProtocol : public NetworkProtocol {
private:
    CurrentNodeTracker currentNode;
    DownloadTracker download;

public:
    GatewayNetworkProtocol(PacketRadio &radio, GatewayNetworkCallbacks &callbacks) : NetworkProtocol(radio), download(callbacks) {
    }

public:
    void tick();
    void push(LoraPacket &lora);

};

#endif
