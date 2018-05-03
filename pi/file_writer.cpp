#include "file_writer.h"
#include "packet_radio.h"

FileWriter::FileWriter(std::experimental::filesystem::path path) : path(path) {
}

bool FileWriter::open() {
    auto directory = path.parent_path();

    if (!std::experimental::filesystem::create_directories(directory)) {
        std::cerr << "Unable to create directories: " << directory << std::endl;
        return false;
    }

    slc::log() << "Creating " << path.c_str();

    of.open(path, std::ios::binary);

    return of.is_open();
}

int32_t FileWriter::write(uint8_t *ptr, size_t size) {
    if (!of.is_open()) {
        if (!open()) {
            return 0;
        }
    }
    of.write(reinterpret_cast<char*>(ptr), size);
    of.flush();
    return size;
}

int32_t FileWriter::write(uint8_t byte) {
    if (!of.is_open()) {
        if (!open()) {
            return 0;
        }
    }
    of.put(byte);
    return 1;
}

void FileWriter::close() {
    if (of.is_open()) {
        of.close();
    }
}
