#include "gateway_protocol.h"
#include "file_writer.h"

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
        slc::log() << "Unable to decode packet!";
        return;
    }

    slc::log() << "R " << lora.id << " " << packet.m().kind << " " << packet.getNodeId() << " (" << lora.size << " bytes)";

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PING: {
            delay(ReplyDelay);
            auto pong = RadioPacket{ fk_radio_PacketKind_PONG, packet.getNodeId() };
            pong.m().address = nextAddress++;
            sendPacket(std::move(pong));
            break;
        }
        case fk_radio_PacketKind_PREPARE: {
            if (writer != nullptr) {
                writer->close();
                callbacks->closeWriter(writer);
            }
            writer = callbacks->openWriter(packet);
            totalReceived = 0;
            receiveSequence = 0;
            delay(ReplyDelay);
            sendAck(lora.from);
            break;
        }
        case fk_radio_PacketKind_DATA: {
            auto data = packet.data();
            auto dupe = lora.id <= receiveSequence;
            if (!dupe) {
                if (writer != nullptr) {
                    auto written = writer->write(data.ptr, data.size);
                    assert(written == (int32_t)data.size);
                }
                totalReceived += data.size;
                receiveSequence = lora.id;
            }
            slc::log() << "R " << totalReceived << " " << lora.id << " " << receiveSequence << " " << (dupe ? "DUPE" : "");
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
