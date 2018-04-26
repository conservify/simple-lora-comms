#include <utility>

#include "protocol.h"

void NodeNetworkProtocol::tick() {
    switch (getState()) {
    case NetworkState::Starting: {
        retries().clear();
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
        retries().clear();
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
        sendPacket(RadioPacket{ fk_radio_PacketKind_PING, deviceId });
        transition(NetworkState::WaitingForPong);
        break;
    }
    case NetworkState::WaitingForPong: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                fklogln("Radio: FAIL!");
                transition(NetworkState::Listening);
            }
        }
        break;
    }
    case NetworkState::Prepare: {
        auto prepare = RadioPacket{ fk_radio_PacketKind_PREPARE, deviceId };
        prepare.m().size = 2048;
        sendPacket(std::move(prepare));
        transition(NetworkState::WaitingForReady);
        break;
    }
    case NetworkState::WaitingForReady: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                if (retries().canRetry()) {
                    fklogln("Radio: RETRY!");
                    transition(NetworkState::Prepare);
                }
                else {
                    fklogln("Radio: FAIL!");
                    transition(NetworkState::Listening);
                }
            }
        }
        break;
    }
    case NetworkState::SendData: {
        sendPacket(RadioPacket{ fk_radio_PacketKind_DATA, deviceId });
        transition(NetworkState::WaitingForSendMore);
        break;
    }
    case NetworkState::WaitingForSendMore: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
            if (inStateFor(ReceiveWindowLength)) {
                if (retries().canRetry()) {
                    fklogln("Radio: RETRY!");
                    transition(NetworkState::SendData);
                }
                else {
                    fklogln("Radio: FAIL!");
                    transition(NetworkState::Listening);
                }
            }
        }
        break;
    }
    default: {
        break;
    }
    }
}

void NodeNetworkProtocol::push(LoraPacket &lora, RadioPacket &packet) {
    auto traffic = packet.getDeviceId() != deviceId;

    logger << "Radio: R " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << (traffic ? " TRAFFIC" : "") << "\n";

    switch (getState()) {
    case NetworkState::Listening: {
        if (traffic) {
            transition(NetworkState::Sleeping, random(SleepingWindowMin, SleepingWindowMax));
            return;
        }
        break;
    }
    case NetworkState::WaitingForPong: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PONG: {
            retries().clear();
            transition(NetworkState::Prepare);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForReady: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_ACK: {
            retries().clear();
            transition(NetworkState::SendData);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForSendMore: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_ACK: {
            retries().clear();
            transition(NetworkState::SendData);
            break;
        }
        }
        break;
    }
    }

#ifdef ARDUINO
    randomSeed(millis());
#endif
}

void GatewayNetworkProtocol::tick() {
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
    }
}

void GatewayNetworkProtocol::push(LoraPacket &lora, RadioPacket &packet) {
    logger << "Radio: R " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << "\n";

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PING: {
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_PONG, packet.getDeviceId() });
            break;
        }
        case fk_radio_PacketKind_PREPARE: {
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_ACK, packet.getDeviceId() });
            break;
        }
        case fk_radio_PacketKind_DATA: {
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_ACK, packet.getDeviceId() });
            break;
        }
        }
        break;
    }
    }
}
