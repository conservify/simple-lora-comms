#ifndef SLC_PROTOCOL_H_INCLUDED
#define SLC_PROTOCOL_H_INCLUDED

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <lwstreams/lwstreams.h>

#include "debug.h"

#include "device_id.h"
#include "packet_radio.h"

enum class NetworkState {
    Starting,
    ListenForSilence,
    Idle,

    Sleeping,

    PingGateway,
    WaitingForPong,
    SendPong,

    Prepare,
    WaitingForReady,
    ReadData,
    SendData,
    WaitingForSendMore,

    Listening,
};

inline const char *getStateName(NetworkState state) {
    switch (state) {
    case NetworkState::Starting: return "Starting";
    case NetworkState::ListenForSilence: return "ListenForSilence";
    case NetworkState::Idle: return "Idle";
    case NetworkState::Sleeping: return "Sleeping";

    case NetworkState::PingGateway: return "PingGateway";
    case NetworkState::WaitingForPong: return "WaitingForPong";
    case NetworkState::SendPong: return "SendPong";

    case NetworkState::Prepare: return "Prepare";
    case NetworkState::WaitingForReady: return "WaitingForReady";
    case NetworkState::ReadData: return "ReadData";
    case NetworkState::SendData: return "SendData";
    case NetworkState::WaitingForSendMore: return "WaitingForSendMore";

    case NetworkState::Listening: return "Listening";
    default: {
        return "Unknown";
    }
    }
}

inline LogStream& operator<<(LogStream &log, const NetworkState &state) {
    return log.print(getStateName(state));
}

class NetworkProtocol {
protected:
    static constexpr uint32_t ReceiveWindowLength = 1000;
    static constexpr uint32_t ReplyDelay = 50;
    static constexpr uint32_t IdleWindowMin = 400;
    static constexpr uint32_t IdleWindowMax = 1600;
    static constexpr uint32_t ListenForSilenceWindowLength = 5000;

    struct RetryCounter {
        uint8_t counter{ 0 };

        void clear() {
            counter = 0;
        }

        bool canRetry() {
            counter++;
            if (counter == 3) {
                counter = 0;
                return false;
            }
            return true;
        }
    };

private:
    PacketRadio *radio;
    NetworkState state{ NetworkState::Starting };
    uint32_t lastTransitionAt{ 0 };
    uint32_t timerDoneAt{ 0 };
    uint8_t sequence{ 0 };
    RetryCounter retryCounter;

public:
    NetworkProtocol(PacketRadio &radio) : radio(&radio) {
    }

public:
    bool sendPacket(RadioPacket &&packet);

    bool sendAck(uint8_t toAddress);

    void transition(NetworkState newState, uint32_t timer = 0);

    bool isTimerDone();

    bool inStateFor(uint32_t ms);

    void zeroSequence() {
        sequence = 0;
    }

    void bumpSequence() {
        sequence++;
    }

public:
    bool isSleeping() {
        return state == NetworkState::Sleeping;
    }

    bool hasBeenSleepingFor(uint32_t ms) {
        return isSleeping() && inStateFor(ms);
    }

protected:
    RetryCounter &retries() {
        return retryCounter;
    }

    NetworkState getState() {
        return state;
    }

    PacketRadio *getRadio() {
        return radio;
    }

};

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
