#ifndef SLC_FILE_WRITER_H_INCLUDED
#define SLC_FILE_WRITER_H_INCLUDED

#include <lwstreams/lwstreams.h>

#include <experimental/filesystem>
#include <string>
#include <iostream>
#include <fstream>

using stdpath = std::experimental::filesystem::path;

class FileWriter : public lws::Writer {
private:
    stdpath path_;
    std::ofstream of_;

public:
    FileWriter(stdpath path);

public:
    bool open();
    int32_t write(uint8_t *ptr, size_t size) override;
    int32_t write(uint8_t byte) override;
    void close() override;

public:
    stdpath path() {
        return path_;
    }

};

#endif
