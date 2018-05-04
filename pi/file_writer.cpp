#include "file_writer.h"
#include "packet_radio.h"

FileWriter::FileWriter(std::experimental::filesystem::path path) : path_(path) {
}

bool FileWriter::open() {
    auto directory = path_.parent_path();

    if (!std::experimental::filesystem::exists(directory)) {
        if (!std::experimental::filesystem::create_directories(directory)) {
            std::cerr << "Unable to create directories: " << directory << std::endl;
            return false;
        }
    }

    slc::log() << "Creating " << path_.c_str();

    of_.open(path_, std::ios::binary);

    return of_.is_open();
}

int32_t FileWriter::write(uint8_t *ptr, size_t size) {
    if (!of_.is_open()) {
        if (!open()) {
            return 0;
        }
    }
    of_.write(reinterpret_cast<char*>(ptr), size);
    of_.flush();
    return size;
}

int32_t FileWriter::write(uint8_t byte) {
    if (!of_.is_open()) {
        if (!open()) {
            return 0;
        }
    }
    of_.put(byte);
    return 1;
}

void FileWriter::close() {
    if (of_.is_open()) {
        of_.close();
    }
}
