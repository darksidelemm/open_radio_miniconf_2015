#ifndef SETTINGS_H
#define SETTINGS_H

#define SETTINGS_VER    1
#define SETTINGS_MAGIC  (0x7AD10000 | SETTINGS_VER)

struct settings {
        uint32_t tx_freq;
        uint32_t rx_freq;
        int32_t calibration;
        uint32_t magic;
};

#endif /* SETTINGS_H */
