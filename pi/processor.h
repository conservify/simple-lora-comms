#ifndef SLC_PROCESSOR_H_INCLUDED
#define SLC_PROCESSOR_H_INCLUDED

#include <experimental/filesystem>
#include <thread>

#include "queue.h"

using stdpath = std::experimental::filesystem::path;

class Processor {
private:
    std::string command_;
    ConcurrentQueue<stdpath> queue_;

public:
    Processor(std::string command) : command_(command) {
    }

public:
    void start() {
        std::thread thread(std::ref(*this));
        thread.detach();
    }

    void push(stdpath path);

public:
    void operator()();

};

#endif
