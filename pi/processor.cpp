#include <iostream>
#include <string>

#include "processor.h"
#include "packet_radio.h"

class ProcessMonitor {
private:
    Logger log{ "Process "};

public:
    bool runSynchronously(std::string cmdline) {
        auto stream = popen(cmdline.c_str(), "r");
        if (stream == nullptr) {
            return false;
        }

        char buffer[256] = { 0 };
        while (NULL != fgets(buffer, sizeof(buffer) - 1, stream)) {
            std::string line(buffer);
            auto trimmed = line.substr(0, line.find_last_not_of("\n") + 1);
            log() << trimmed;
        }

        /*
        if (feof(stream)) {
            slc::log() << "Command finished successfully";
        }
        */

        auto status = pclose(stream);
        if (status != -1) {
            log() << "Command finished in error";
            return false;
        }

        return true;
    }

};

void Processor::operator()() {
    while (true) {
        auto path = queue_.pop();

        slc::log() << "Processing " << path.c_str();

        if (command_.size() > 0) {
            auto running = command_;
            running += " ";
            running += path;
            running += " 2>&1";

            slc::log() << "Running " << running;
            auto process = ProcessMonitor();
            process.runSynchronously(running);
        }
    }
}

void Processor::push(stdpath path) {
    queue_.push(path);
}
