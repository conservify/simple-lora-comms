#include "file_writer.h"

FileWriter::FileWriter(const char *fn) : fn(fn) {
}

bool FileWriter::open() {
    fp = fopen(fn, "w+");
    if (fp == nullptr) {
        return false;
    }
    return true;
}

int32_t FileWriter::write(uint8_t *ptr, size_t size) {
    auto bytes =  fwrite(ptr, 1, size, fp);
    fflush(fp);
    return bytes;
}

int32_t FileWriter::write(uint8_t byte) {
    if (fp == nullptr) {
        return 0;
    }
    return fputc(byte, fp);
}

void FileWriter::close() {
    if (fp != nullptr) {
        fflush(fp);
        fclose(fp);
        fp = nullptr;
    }
}
