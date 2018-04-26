#pragma once

#include <cstdint>
#include <cassert>

#include "debug.h"

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

    template<class T>
    void set(T &body) {
        to = 0x00;
        from = 0xff;
        memcpy(data, (uint8_t *)&body, sizeof(T));
        size = sizeof(T);
    }

};

struct DeviceId {
private:
    uint8_t raw[8] = { 0 };

public:
    uint8_t& operator[] (int32_t i) {
        return raw[i];
    }

    uint8_t operator[] (int32_t i) const {
        return raw[i];
    }

public:
    friend Logger& operator<<(Logger &log, const DeviceId &deviceId) {
        log.printf("%02x%02x%02x%02x%02x%02x%02x%02x",
                   deviceId[0], deviceId[1], deviceId[2], deviceId[3],
                   deviceId[4], deviceId[5], deviceId[6], deviceId[7]);
        return log;
    }
};

inline bool operator==(const DeviceId& lhs, const DeviceId& rhs) {
    for (auto i = 0; i < 8; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

inline bool operator!=(const DeviceId& lhs, const DeviceId& rhs) {
    return !(lhs == rhs);
}

enum class PacketKind {
    Ack,
    Ping,
    Pong,
    Prepare,
};

inline Logger& operator<<(Logger &log, const PacketKind &kind) {
    switch (kind) {
    case PacketKind::Ack: return log.print("Ack");
    case PacketKind::Ping: return log.print("Ping");
    case PacketKind::Pong: return log.print("Pong");
    case PacketKind::Prepare: return log.print("Prepare");
    default:
        return log.print("Unknown");
    }
}

struct ApplicationPacket {
    PacketKind kind;
    DeviceId deviceId;

    ApplicationPacket(PacketKind kind) : kind(kind) {
    }

    ApplicationPacket(PacketKind kind, DeviceId deviceId) : kind(kind), deviceId(deviceId) {
    }

    ApplicationPacket(LoraPacket &lora) {
        assert(lora.size >= (int32_t)sizeof(ApplicationPacket));
        memcpy((void *)this, (void *)&lora.data, sizeof(ApplicationPacket));
    }
};

enum class NetworkState {
    Starting,
    Idle,
    Listening,
    Sleeping,
    PingGateway,
    WaitingForPong,
    SendPong,
};

inline const char *getStateName(NetworkState state) {
    switch (state) {
    case NetworkState::Starting: return "Starting";
    case NetworkState::Idle: return "Idle";
    case NetworkState::Listening: return "Listening";
    case NetworkState::Sleeping: return "Sleeping";
    case NetworkState::PingGateway: return "PingGateway";
    case NetworkState::WaitingForPong: return "WaitingForPong";
    case NetworkState::SendPong: return "SendPong";
    default: {
        return "Unknown";
    }
    }
}

class PacketRadio {
public:
    virtual bool isModeRx() = 0;
    virtual bool isModeTx() = 0;
    virtual bool isIdle() = 0;
    virtual void setModeRx() = 0;
    virtual void setModeIdle() = 0;
    virtual bool sendPacket(ApplicationPacket &packet) = 0;

};

class NetworkProtocol {
protected:
    static constexpr uint32_t ReceiveWindowLength = 500;
    static constexpr uint32_t ReplyDelay = 50;
    static constexpr uint32_t IdleWindowMin = 5000;
    static constexpr uint32_t IdleWindowMax = 10000;
    static constexpr uint32_t SleepingWindowMin = 2000;
    static constexpr uint32_t SleepingWindowMax = 3000;
    static constexpr uint32_t ListeningWindowLength = 5000;

private:
    PacketRadio *radio;
    NetworkState state{ NetworkState::Starting };
    uint32_t lastTransitionAt{ 0 };
    uint32_t timerDoneAt{ 0 };

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

protected:
    NetworkState getState() {
        return state;
    }

    PacketRadio *getRadio() {
        return radio;
    }

    bool sendPacket(ApplicationPacket &&packet) {
        return radio->sendPacket(packet);
    }

};

#ifndef ARDUINO
inline int32_t random(int32_t min, int32_t max) {
    return (rand() % (max - min)) + min;
}
#endif

class NodeNetworkProtocol : public NetworkProtocol {
private:
    DeviceId deviceId;

public:
    NodeNetworkProtocol(DeviceId &deviceId, PacketRadio &radio) : NetworkProtocol(radio), deviceId(deviceId) {
    }

public:
    void tick() {
        switch (getState()) {
        case NetworkState::Starting: {
            transition(NetworkState::Listening);
            break;
        }
        case NetworkState::Idle: {
            getRadio()->setModeRx();
            if (isTimerDone()) {
                transition(NetworkState::Listening);
            }
            break;
        }
        case NetworkState::Listening: {
            getRadio()->setModeRx();
            if (inStateFor(ListeningWindowLength)) {
                transition(NetworkState::PingGateway);
            }
            break;
        }
        case NetworkState::Sleeping: {
            getRadio()->setModeIdle();
            if (isTimerDone()) {
                transition(NetworkState::Listening);
            }
            break;
        }
        case NetworkState::PingGateway: {
            sendPacket(ApplicationPacket{ PacketKind::Ping, deviceId });
            transition(NetworkState::WaitingForPong);
            break;
        }
        case NetworkState::WaitingForPong: {
            if (getRadio()->isModeTx()) {
            }
            else {
                getRadio()->setModeRx();
                if (inStateFor(ReceiveWindowLength)) {
                    fklogln("Radio: NO PONG!");
                    transition(NetworkState::Listening);
                }
            }
            break;
        }
        default: {
            break;
        }
        }
    }

    void push(LoraPacket &lora, ApplicationPacket &packet) {
        auto traffic = packet.deviceId != deviceId;

        logger << "Radio: R " << lora.id << " " << packet.kind << " " << packet.deviceId << (traffic ? " TRAFFIC" : "") << "\n";

        switch (getState()) {
        case NetworkState::Idle: {
            break;
        }
        case NetworkState::Sleeping: {
            break;
        }
        case NetworkState::Listening: {
            if (traffic) {
                transition(NetworkState::Sleeping, random(SleepingWindowMin, SleepingWindowMax));
                return;
            }
            break;
        }
        case NetworkState::WaitingForPong: {
            switch (packet.kind) {
            case PacketKind::Pong: {
                transition(NetworkState::Idle, random(IdleWindowMin, IdleWindowMax));
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        default: {
            break;
        }
        }

        #ifdef ARDUINO
        randomSeed(millis());
        #endif
    }

};

class GatewayNetworkProtocol : public NetworkProtocol {
public:
    GatewayNetworkProtocol(PacketRadio &radio) : NetworkProtocol(radio) {
    }

public:
    void tick() {
        switch (getState()) {
        case NetworkState::Starting: {
            transition(NetworkState::Listening);
            break;
        }
        case NetworkState::Idle: {
            getRadio()->setModeRx();
            break;
        }
        case NetworkState::Listening: {
            getRadio()->setModeRx();
            break;
        }
        case NetworkState::SendPong: {
            break;
        }
        default: {
            break;
        }
        }
    }

    void push(LoraPacket &lora, ApplicationPacket &packet) {
        logger << "Radio: R " << lora.id << " " << packet.kind << " " << packet.deviceId << "\n";

        switch (getState()) {
        case NetworkState::Listening: {
            switch (packet.kind) {
            case PacketKind::Ping: {
                delay(ReplyDelay);
                sendPacket(ApplicationPacket{ PacketKind::Pong, packet.deviceId });
                transition(NetworkState::Listening);
                break;
            }
            default: {
                break;
            }
            }
            break;
        }
        default: {
            break;
        }
        }
    }

};
