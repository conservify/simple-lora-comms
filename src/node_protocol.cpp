#include "node_protocol.h"

#ifndef ARDUINO
inline int32_t random(int32_t min, int32_t max) {
    return (rand() % (max - min)) + min;
}
#endif

void NodeNetworkProtocol::sendToGateway() {
    transition(NetworkState::Idle, random(IdleWindowMin, IdleWindowMax));
}

void NodeNetworkProtocol::tick() {
    if (getRadio()->isModeTx()) {
        if (!transmitting.isRunning()) {
            transmitting.begin();
        }
    }
    else if (transmitting.isRunning()) {
        transmitting.end();
    }
    switch (getState()) {
    case NetworkState::Starting: {
        retries().clear();
        transition(NetworkState::Sleeping);
        break;
    }
    case NetworkState::Idle: {
        getRadio()->setModeIdle();
        if (isTimerDone()) {
            transition(NetworkState::ListenForSilence);
        }
        break;
    }
    case NetworkState::Sleeping: {
        getRadio()->sleep();
        break;
    }
    case NetworkState::ListenForSilence: {
        retries().clear();
        getRadio()->setModeRx();
        if (inStateFor(ListenForSilenceWindowLength)) {
            transition(NetworkState::PingGateway);
        }
        break;
    }
    case NetworkState::PingGateway: {
        sendPacket(RadioPacket{ fk_radio_PacketKind_PING, nodeId });
        transition(NetworkState::WaitingForPong);
        break;
    }
    case NetworkState::WaitingForPong: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
        }
        if (inStateFor(ReceiveWindowLength)) {
            slc::log() << "FAIL!";
            transition(NetworkState::ListenForSilence);
        }
        break;
    }
    case NetworkState::Prepare: {
        if (reader != nullptr) {
            callbacks->closeReader(reader);
        }
        reader = callbacks->openReader();
        auto prepare = RadioPacket{ fk_radio_PacketKind_PREPARE, nodeId };
        sendPacket(std::move(prepare));
        transition(NetworkState::WaitingForReady);
        waitingOnAck.begin();
        break;
    }
    case NetworkState::WaitingForReady: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
        }
        if (inStateFor(ReceiveWindowLength)) {
            if (retries().canRetry()) {
                slc::log() << "RETRY!";
                transition(NetworkState::Prepare);
            }
            else {
                slc::log() << "FAIL!";
                transition(NetworkState::SendFailure);
            }
        }
        break;
    }
    case NetworkState::ReadData: {
        auto bp = buffer.toBufferPtr();
        auto bytes = reader->read(bp.ptr, bp.size);
        if (bytes < 0) {
            transition(NetworkState::SendClose);
            slc::log() << "Done! waitingOnAck: " << waitingOnAck << " transmitting: " << transmitting;
        }
        else if (bytes > 0) {
            buffer.position(bytes);
            transition(NetworkState::SendData);
        }
        break;
    }
    case NetworkState::SendData: {
        auto packet = RadioPacket{ fk_radio_PacketKind_DATA, nodeId };
        packet.data(buffer.toBufferPtr().ptr, buffer.position());
        sendPacket(std::move(packet));
        transition(NetworkState::WaitingForSendMore);
        waitingOnAck.begin();
        break;
    }
    case NetworkState::WaitingForSendMore: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
        }
        if (inStateFor(ReceiveWindowLength)) {
            if (retries().canRetry()) {
                slc::log() << "RETRY!";
                transition(NetworkState::SendData);
            }
            else {
                slc::log() << "FAIL!";
                transition(NetworkState::SendFailure);
            }
        }
        break;
    }
    case NetworkState::SendClose: {
        auto packet = RadioPacket{ fk_radio_PacketKind_DATA, nodeId };
        sendPacket(std::move(packet));
        transition(NetworkState::WaitingForClosed);
        waitingOnAck.begin();
        break;
    }
    case NetworkState::WaitingForClosed: {
        if (!getRadio()->isModeTx()) {
            getRadio()->setModeRx();
        }
        if (inStateFor(ReceiveWindowLength)) {
            if (retries().canRetry()) {
                slc::log() << "RETRY!";
                transition(NetworkState::SendClose);
            }
            else {
                slc::log() << "FAIL!";
                transition(NetworkState::SendFailure);
            }
        }
        break;
    }
    default: {
        break;
    }
    }
}

void NodeNetworkProtocol::push(LoraPacket &lora) {
    auto packet = RadioPacket{ };
    if (!packet.decode(lora)) {
        slc::log() << "Unable to decode packet!";
        return;
    }
    auto traffic = packet.m().kind != fk_radio_PacketKind_ACK && packet.getNodeId() != nodeId;

    slc::log() << "R " << lora.id << " " << packet.m().kind << " (" << lora.size << " bytes)" << (traffic ? " TRAFFIC" : "");

    switch (getState()) {
    case NetworkState::ListenForSilence: {
        if (traffic) {
            transition(NetworkState::Sleeping);
            return;
        }
        break;
    }
    case NetworkState::WaitingForPong: {
        if (packet.m().kind == fk_radio_PacketKind_PONG) {
            retries().clear();
            slc::log() << "Pong: My address: " << packet.m().address;
            transition(NetworkState::Prepare);
        }
        break;
    }
    case NetworkState::WaitingForReady: {
        if (packet.m().kind == fk_radio_PacketKind_ACK) {
            waitingOnAck.end();
            zeroSequence();
            bumpSequence();
            retries().clear();
            transition(NetworkState::ReadData);
        }
        break;
    }
    case NetworkState::WaitingForSendMore: {
        if (packet.m().kind == fk_radio_PacketKind_ACK) {
            waitingOnAck.end();
            bumpSequence();
            retries().clear();
            transition(NetworkState::ReadData);
        }
        break;
    }
    case NetworkState::WaitingForClosed: {
        if (packet.m().kind == fk_radio_PacketKind_ACK) {
            waitingOnAck.end();
            bumpSequence();
            retries().clear();
            transition(NetworkState::Sleeping);
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
