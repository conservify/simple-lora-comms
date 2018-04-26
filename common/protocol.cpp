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
        sendPacket(ApplicationPacket{ PacketKind::Ping, deviceId });
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
        sendPacket(ApplicationPacket{ PacketKind::Prepare, deviceId });
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
        sendPacket(ApplicationPacket{ PacketKind::Data, deviceId });
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

void NodeNetworkProtocol::push(LoraPacket &lora, ApplicationPacket &packet) {
    auto traffic = packet.deviceId != deviceId;

    logger << "Radio: R " << lora.id << " " << packet.kind << " " << packet.deviceId << (traffic ? " TRAFFIC" : "") << "\n";

    switch (getState()) {
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
            retries().clear();
            transition(NetworkState::Prepare);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForReady: {
        switch (packet.kind) {
        case PacketKind::Ack: {
            retries().clear();
            transition(NetworkState::SendData);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForSendMore: {
        switch (packet.kind) {
        case PacketKind::Ack: {
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

void GatewayNetworkProtocol::push(LoraPacket &lora, ApplicationPacket &packet) {
    logger << "Radio: R " << lora.id << " " << packet.kind << " " << packet.deviceId << "\n";

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.kind) {
        case PacketKind::Ping: {
            delay(ReplyDelay);
            sendPacket(ApplicationPacket{ PacketKind::Pong, packet.deviceId });
            break;
        }
        case PacketKind::Prepare: {
            delay(ReplyDelay);
            sendPacket(ApplicationPacket{ PacketKind::Ack, packet.deviceId });
            break;
        }
        case PacketKind::Data: {
            delay(ReplyDelay);
            sendPacket(ApplicationPacket{ PacketKind::Ack, packet.deviceId });
            break;
        }
        }
        break;
    }
    }
}
