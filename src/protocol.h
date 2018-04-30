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
    Listening,
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
};

inline const char *getStateName(NetworkState state) {
    switch (state) {
    case NetworkState::Starting: return "Starting";
    case NetworkState::Listening: return "Listening";
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
    default: {
        return "Unknown";
    }
    }
}

inline Logger& operator<<(Logger &log, const NetworkState &state) {
    return log.print(getStateName(state));
}

class NetworkProtocol {
protected:
    static constexpr uint32_t ReceiveWindowLength = 1000;
    static constexpr uint32_t ReplyDelay = 50;
    static constexpr uint32_t IdleWindowMin = 5000;
    static constexpr uint32_t IdleWindowMax = 10000;
    static constexpr uint32_t SleepingWindowMin = 2000;
    static constexpr uint32_t SleepingWindowMax = 3000;
    static constexpr uint32_t ListeningWindowLength = 5000;

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
    DeviceId deviceId;
    HoldingBuffer<242 - 24> buffer;
    lws::Reader *reader{ nullptr };

public:
    NodeNetworkProtocol(DeviceId &deviceId, PacketRadio &radio) : NetworkProtocol(radio), deviceId(deviceId) {
    }

public:
    void tick();
    void push(LoraPacket &lora);

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
