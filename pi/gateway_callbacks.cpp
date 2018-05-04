#include <iomanip>

#include "gateway_callbacks.h"

lws::Writer *ArchivingGatewayCallbacks::openWriter(RadioPacket &packet) {
    auto path = getPath(packet);
    return new FileWriter(path);
}

void ArchivingGatewayCallbacks::closeWriter(lws::Writer *writer, bool success) {
    if (writer != nullptr) {
        writer->close();
        if (success) {
            auto fileWriter = reinterpret_cast<FileWriter*>(writer);
            pending_.emplace(fileWriter->path());
        }
    }
}

std::string ArchivingGatewayCallbacks::getNodeId(RadioPacket &packet) {
    auto& id = packet.getNodeId();
    std::stringstream ss;
    for (auto i = 0; i < (int32_t)id.size; ++i) {
        ss << std::setfill('0') << std::setw(2) << std::hex << (int32_t)id.ptr[i];
    }
    return ss.str();
}

stdpath ArchivingGatewayCallbacks::getPath(RadioPacket &packet) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream buffer;
    buffer << path_ << "/";
    buffer << getNodeId(packet) << "/";
    buffer << std::put_time(&tm, "%Y%m%d/%H%M%S") << "_";
    buffer << "fkn.fkpb";
    return stdpath{ buffer.str() };
}
