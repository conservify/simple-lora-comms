#include <lwstreams/lwstreams.h>
#include <utility>
#include <cstdio>

#include "protocol.h"

bool NetworkProtocol::sendPacket(RadioPacket &&packet) {
    size_t required = 0;
    if (!pb_get_encoded_size(&required, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    char buffer[required];
    auto stream = pb_ostream_from_buffer((uint8_t *)buffer, required);
    if (!pb_encode(&stream, fk_radio_RadioPacket_fields, packet.forEncode())) {
        return false;
    }

    LoraPacket lora;
    memcpy(lora.data, buffer, stream.bytes_written);
    lora.size = stream.bytes_written;
    logger << "Radio: S " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " (" << stream.bytes_written << " bytes)\n";
    return radio->sendPacket(lora);
}

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
        if (reader != nullptr) {
            delete reader;
        }
        reader = new lws::CountingReader(2048);
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
    case NetworkState::ReadData: {
        auto bp = buffer.toBufferPtr();
        auto bytes = reader->read(bp.ptr, bp.size);
        if (bytes < 0) {
            transition(NetworkState::Idle, random(IdleWindowMin, IdleWindowMax));
        }
        else if (bytes > 0) {
            buffer.position(bytes);
            transition(NetworkState::SendData);
        }
        break;
    }
    case NetworkState::SendData: {
        auto packet = RadioPacket{ fk_radio_PacketKind_DATA, deviceId };
        packet.data(buffer.toBufferPtr().ptr, buffer.position());
        sendPacket(std::move(packet));
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

    logger << "Radio: R " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " " << lora.size << (traffic ? " TRAFFIC" : "") << "\n";

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
            transition(NetworkState::ReadData);
            break;
        }
        }
        break;
    }
    case NetworkState::WaitingForSendMore: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_ACK: {
            retries().clear();
            transition(NetworkState::ReadData);
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

class FileWriter : public lws::Writer {
private:
    FILE *fp;

public:
    FileWriter(const char *fn) {
        fp = fopen(fn, "w+");
        if (fp == nullptr) {
            fklogln("Failed to open %s\n", fn);
        }
    }

public:
    int32_t write(uint8_t *ptr, size_t size) override {
        auto bytes =  fwrite(ptr, 1, size, fp);
        fflush(fp);
        return bytes;
    }

    int32_t write(uint8_t byte) override {
        if (fp == nullptr) {
            return 0;
        }
        return fputc(byte, fp);
    }

    void close() override {
        if (fp != nullptr) {
            fflush(fp);
            fclose(fp);
            fp = nullptr;
        }
    }

};

void GatewayNetworkProtocol::push(LoraPacket &lora, RadioPacket &packet) {
    logger << "Radio: R " << lora.id << " " << packet.m().kind << " " << packet.getDeviceId() << " " << lora.size << "\n";

    switch (getState()) {
    case NetworkState::Listening: {
        switch (packet.m().kind) {
        case fk_radio_PacketKind_PING: {
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_PONG, packet.getDeviceId() });
            break;
        }
        case fk_radio_PacketKind_PREPARE: {
            totalReceived = 0;
            if (writer != nullptr) {
                writer->close();
                delete writer;
            }
            writer = new FileWriter("DATA");
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_ACK, packet.getDeviceId() });
            break;
        }
        case fk_radio_PacketKind_DATA: {
            auto data = packet.data();
            totalReceived += data.size;
            logger << "Radio: R " << totalReceived << "\n";
            if (writer != nullptr) {
                auto written = writer->write(data.ptr, data.size);
                assert(written == data.size);
            }
            delay(ReplyDelay);
            sendPacket(RadioPacket{ fk_radio_PacketKind_ACK, packet.getDeviceId() });
            break;
        }
        }
        break;
    }
    }
}
