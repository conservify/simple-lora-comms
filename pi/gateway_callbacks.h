#ifndef SLC_GATEWAY_CALLBACKS_H_INCLUDED
#define SLC_GATEWAY_CALLBACKS_H_INCLUDED

#include <string>
#include <queue>
#include <experimental/filesystem>

#include "gateway_protocol.h"
#include "file_writer.h"

using stdpath = std::experimental::filesystem::path;

class ArchivingGatewayCallbacks : public GatewayNetworkCallbacks {
private:
    std::string path_;
    std::queue<stdpath> pending_;

public:
    ArchivingGatewayCallbacks(std::string path) : path_(path) {
    }

public:
    std::queue<stdpath> &pending() {
        return pending_;
    }

public:
    lws::Writer *openWriter(RadioPacket &packet) override;
    void closeWriter(lws::Writer *writer, bool success) override;

private:
    std::string getNodeId(RadioPacket &packet);
    stdpath getPath(RadioPacket &packet);

};

#endif
