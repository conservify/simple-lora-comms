#include "gateway_protocol.h"

struct PacketLogMessage {
    LoraPacket &lora;
    RadioPacket &packet;
};

inline LogStream& operator<<(LogStream &log, const PacketLogMessage &plm) {
    return log << plm.packet.m().kind << " " << plm.packet.getNodeId() << " " << plm.lora.id << " p(" << plm.lora.size << " bytes)";
}

bool CurrentNodeTracker::canPongPing(RadioPacket &packet) {
    if (id_ == packet.getNodeId()) {
        return true;
    }

    if (millis() - lastPing_ < 1000) {
        return false;
    }

    lastPing_ = millis();
    id_ = packet.getNodeId();

    return true;
}

DownloadTracker::DownloadTracker(GatewayNetworkCallbacks &callbacks) : callbacks_(&callbacks) {
}

bool DownloadTracker::prepare(LogStream &log, LoraPacket &lora, RadioPacket &packet) {
    if (writer_ != nullptr) {
        writer_->close();
        callbacks_->closeWriter(writer_, false);
    }
    writer_ = callbacks_->openWriter(packet);
    received_ = 0;
    expected_ = packet.m().size;
    receiveSequence_ = 0;
    return true;
}

bool DownloadTracker::download(LogStream &log, LoraPacket &lora, RadioPacket &packet) {
    auto dupe = (lora.id != (uint8_t)(receiveSequence_ + 1));
    auto data = packet.data();
    auto closed = data.size == 0;
    if (!dupe) {
        if (writer_ != nullptr) {
            if (data.size > 0) {
                auto written = writer_->write(data.ptr, data.size);
                assert(written == (int32_t)data.size);
            }
            else {
                callbacks_->closeWriter(writer_, true);
                writer_ = nullptr;
            }
        }
        received_ += data.size;
        receiveSequence_ = lora.id;
    }
    auto mismatch = closed && (received_ != expected_);
    log << " data(" << data.size << " bytes) total(" << received_ << "/" << expected_ << " bytes)"
        << (dupe ? " DUPE" : "") << (closed ? " CLOSED" : "") << (mismatch ? " MISMATCH" : "");
    return true;
}

void GatewayNetworkProtocol::tick() {
    switch (getState()) {
    case NetworkState::Starting: {
        transition(NetworkState::Listening);
        break;
    }
    case NetworkState::Idle: {
        getRadio()->setModeIdle();
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

void GatewayNetworkProtocol::push(LoraPacket &lora) {
    auto packet = RadioPacket{ };
    if (!packet.decode(lora)) {
        slc::log() << "Malformed packet.";
        return;
    }

    auto le = slc::log();

    le << "R " << PacketLogMessage{ lora, packet };

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PING: {
            if (!currentNode.canPongPing(packet)) {
                le << " IGNORE";
                le.flush();
                break;
            }
            le.flush();
            delay(ReplyDelay);
            auto pong = RadioPacket{ fk_radio_PacketKind_PONG, packet.getNodeId() };
            pong.m().address = currentNode.address();
            sendPacket(std::move(pong));
            break;
        }
        case fk_radio_PacketKind_PREPARE: {
            le.flush();
            download.prepare(le, lora, packet);
            delay(ReplyDelay);
            sendAck(lora.from);
            break;
        }
        case fk_radio_PacketKind_DATA: {
            download.download(le, lora, packet);
            delay(ReplyDelay);
            sendAck(lora.from);
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
