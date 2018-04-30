#ifndef FK_DEVICE_ID_H_INCLUDED
#define FK_DEVICE_ID_H_INCLUDED

struct DeviceId {
public:
    uint8_t raw[8] = { 0 };

public:
    uint8_t& operator[] (int32_t i) {
        return raw[i];
    }

    uint8_t operator[] (int32_t i) const {
        return raw[i];
    }

public:
    friend Logger& operator<<(Logger &log, const DeviceId &deviceId) {
        log.printf("%02x%02x%02x%02x%02x%02x%02x%02x",
                   deviceId[0], deviceId[1], deviceId[2], deviceId[3],
                   deviceId[4], deviceId[5], deviceId[6], deviceId[7]);
        return log;
    }
};

inline bool operator==(const DeviceId& lhs, const DeviceId& rhs) {
    for (auto i = 0; i < 8; ++i) {
        if (lhs[i] != rhs[i]) {
            return false;
        }
    }
    return true;
}

inline bool operator!=(const DeviceId& lhs, const DeviceId& rhs) {
    return !(lhs == rhs);
}

#endif
