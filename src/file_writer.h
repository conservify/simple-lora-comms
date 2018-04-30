#ifndef FK_FILE_WRITER_H_INCLUDED
#define FK_FILE_WRITER_H_INCLUDED

#include <lwstreams/lwstreams.h>

#include <cstdio>

class FileWriter : public lws::Writer {
private:
    const char *fn{ nullptr };
    FILE *fp{ nullptr };

public:
    FileWriter(const char *fn);

public:
    bool open();
    int32_t write(uint8_t *ptr, size_t size) override;
    int32_t write(uint8_t byte) override;
    void close() override;

};

#endif
