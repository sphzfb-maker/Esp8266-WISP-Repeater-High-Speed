#pragma once
#include <Arduino.h>
#include <EEPROM.h>

#define CONFIG_MAGIC 0xAA55B008
#define EEPROM_SIZE 512

struct RepeaterConfig {
    uint32_t magic;
    char sta_ssid[33];
    char sta_pass[65];
    char ap_ssid[33];
    char ap_pass[65];
    bool use_custom_mac;
    uint8_t custom_mac[6];
    uint32_t checksum;
};

class ConfigStorage {
public:
    static void begin() {
        EEPROM.begin(EEPROM_SIZE);
    }

    static uint32_t calcChecksum(const RepeaterConfig &cfg) {
        uint32_t sum = 0;
        const uint8_t *p = (const uint8_t *)&cfg;
        size_t len = sizeof(RepeaterConfig) - sizeof(uint32_t);
        for (size_t i = 0; i < len; i++) {
            sum = (sum * 31) + p[i];
        }
        return sum;
    }

    static bool load(RepeaterConfig &cfg) {
        EEPROM.get(0, cfg);
        if (cfg.magic != CONFIG_MAGIC) {
            return false;
        }
        if (cfg.checksum != calcChecksum(cfg)) {
            return false;
        }
        return (strlen(cfg.sta_ssid) > 0);
    }

    static bool save(const RepeaterConfig &cfg) {
        RepeaterConfig toSave = cfg;
        toSave.magic = CONFIG_MAGIC;
        toSave.checksum = calcChecksum(toSave);
        EEPROM.put(0, toSave);
        return EEPROM.commit();
    }

    static void clear() {
        RepeaterConfig emptyCfg;
        memset(&emptyCfg, 0, sizeof(emptyCfg));
        EEPROM.put(0, emptyCfg);
        EEPROM.commit();
    }
};
