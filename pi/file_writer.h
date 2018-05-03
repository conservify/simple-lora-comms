#ifndef SLC_FILE_WRITER_H_INCLUDED
#define SLC_FILE_WRITER_H_INCLUDED

#include <lwstreams/lwstreams.h>

#include <experimental/filesystem>
#include <string>
#include <iostream>
#include <fstream>

class FileWriter : public lws::Writer {
private:
    std::experimental::filesystem::path path;
    std::ofstream of;

public:
    FileWriter(std::experimental::filesystem::path path);

public:
    bool open();
    int32_t write(uint8_t *ptr, size_t size) override;
    int32_t write(uint8_t byte) override;
    void close() override;

};

#endif
