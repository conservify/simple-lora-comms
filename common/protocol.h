#pragma once

#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <cassert>

#include <lwstreams/lwstreams.h>

#include "debug.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include <wiringPi.h>
#endif

#ifndef ARDUINO
inline int32_t random(int32_t min, int32_t max) {
    return (rand() % (max - min)) + min;
}
#endif

#include "device_id.h"
#include "packets.h"

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

class PacketRadio {
public:
    virtual bool isModeRx() = 0;
    virtual bool isModeTx() = 0;
    virtual bool isIdle() = 0;
    virtual void setModeRx() = 0;
    virtual void setModeIdle() = 0;
    virtual bool sendPacket(RadioPacket &packet) = 0;

};

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
    RetryCounter retryCounter;

public:
    NetworkProtocol(PacketRadio &radio) : radio(&radio) {
    }

public:
    void transition(NetworkState newState, uint32_t timer = 0) {
        lastTransitionAt = millis();
        fklogln("Radio: %s -> %s", getStateName(state), getStateName(newState));
        state = newState;
        if (timer > 0) {
            timerDoneAt = millis() + timer;
        }
        else {
            timerDoneAt = 0;
        }
    }

    bool isTimerDone() {
        return timerDoneAt > 0 && millis() > timerDoneAt;
    }

    bool inStateFor(uint32_t ms) {
        return millis() - lastTransitionAt > ms;
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

    bool sendPacket(RadioPacket &&packet) {
        return radio->sendPacket(packet);
    }

};

template<size_t Size>
struct HoldingBuffer {
    lws::AlignedStorageBuffer<Size> buffer;
    size_t pos{ 0 };

    lws::BufferPtr toBufferPtr() {
        return buffer.toBufferPtr();
    }

    size_t getPosition() {
        return pos;
    }

    void setPosition(size_t newPos) {
        pos = newPos;
    }
};

class NodeNetworkProtocol : public NetworkProtocol {
private:
    DeviceId deviceId;
    HoldingBuffer<128> buffer;
    lws::Reader *reader{ nullptr };

public:
    NodeNetworkProtocol(DeviceId &deviceId, PacketRadio &radio) : NetworkProtocol(radio), deviceId(deviceId) {
    }

public:
    void tick();
    void push(LoraPacket &lora, RadioPacket &packet);

};

class GatewayNetworkProtocol : public NetworkProtocol {
private:
    size_t totalReceived{ 0 };
    lws::Writer *writer{ nullptr };

public:
    GatewayNetworkProtocol(PacketRadio &radio) : NetworkProtocol(radio) {
    }

public:
    void tick();
    void push(LoraPacket &lora, RadioPacket &packet);

};
