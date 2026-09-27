#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <lwip/napt.h>
#include <lwip/dns.h>
#include "config_storage.h"
#include "web_portal.h"

extern "C" {
#include "user_interface.h"
}

#define STATUS_LED LED_BUILTIN
#define RECONNECT_INTERVAL_MS 10000UL

ESP8266WebServer server(80);
DNSServer dnsServer;
RepeaterConfig currentConfig;

bool naptActive = false;
bool isSetupMode = true;
unsigned long lastBlink = 0;
bool ledState = false;
unsigned long scheduledReboot = 0;
int lockedChannel = 1;
unsigned long lastReconnectAttempt = 0;
bool wasConnected = false;

void setLed(bool on) {
    digitalWrite(STATUS_LED, on ? LOW : HIGH);
}

// Parses "AA:BB:CC:DD:EE:FF" into 6 raw bytes. Returns false on bad format.
bool parseMac(const String &str, uint8_t *out) {
    if (str.length() != 17) return false;
    int vals[6];
    if (sscanf(str.c_str(), "%x:%x:%x:%x:%x:%x",
               &vals[0], &vals[1], &vals[2], &vals[3], &vals[4], &vals[5]) != 6) {
        return false;
    }
    for (int i = 0; i < 6; i++) {
        if (vals[i] < 0 || vals[i] > 255) return false;
        out[i] = (uint8_t)vals[i];
    }
    return true;
}

String macToStr(const uint8_t *mac) {
    char buf[18];
    sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(buf);
}

void handleRoot() {
    server.send_P(200, "text/html", INDEX_HTML);
}

void handleScan() {
    Serial.println(F("[HTTP] Scan requested"));
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; ++i) {
        if (i > 0) json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",\"rssi\":" + String(WiFi.RSSI(i)) + ",\"enc\":" + String(WiFi.encryptionType(i) == ENC_TYPE_NONE ? "false" : "true") + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
}

void handleStatus() {
    int rssi = (WiFi.status() == WL_CONNECTED) ? WiFi.RSSI() : 0;
    int sig = (rssi <= -100) ? 0 : (rssi >= -50 ? 100 : 2 * (rssi + 100));
    String json = "{";
    json += "\"sta_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false") + ",";
    json += "\"sta_ssid\":\"" + String(currentConfig.sta_ssid) + "\",";
    json += "\"sta_ip\":\"" + (WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : "0.0.0.0") + "\",";
    json += "\"sta_mac\":\"" + WiFi.macAddress() + "\",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"signal_pct\":" + String(sig) + ",";
    json += "\"channel\":" + String(WiFi.channel()) + ",";
    json += "\"ap_ssid\":\"" + String(currentConfig.ap_ssid[0] ? currentConfig.ap_ssid : "ESP8266-Repeater-Setup") + "\",";
    json += "\"ap_clients\":" + String(WiFi.softAPgetStationNum()) + ",";
    json += "\"free_heap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"uptime_s\":" + String(millis() / 1000);
    json += "}";
    server.send(200, "application/json", json);
}

// Lists devices currently associated to our repeated (SoftAP) network: MAC + IP.
void handleClients() {
    String json = "[";
    struct station_info *stat_info = wifi_softap_get_station_info();
    bool first = true;
    while (stat_info != NULL) {
        if (!first) json += ",";
        first = false;
        IPAddress ip(stat_info->ip.addr);
        json += "{\"mac\":\"" + macToStr(stat_info->bssid) + "\",\"ip\":\"" + ip.toString() + "\"}";
        stat_info = STAILQ_NEXT(stat_info, next);
    }
    wifi_softap_free_station_info();
    json += "]";
    server.send(200, "application/json", json);
}

void handleSave() {
    String body = server.arg("plain");
    RepeaterConfig newCfg;
    memset(&newCfg, 0, sizeof(newCfg));

    auto getVal = [&](const String &key, char *out, size_t maxLen) {
        if (server.hasArg(key)) {
            strncpy(out, server.arg(key).c_str(), maxLen - 1);
            return;
        }
        int k = body.indexOf("\"" + key + "\"");
        if (k != -1) {
            int c = body.indexOf(':', k);
            int q1 = body.indexOf('"', c);
            int q2 = body.indexOf('"', q1 + 1);
            if (q1 != -1 && q2 != -1) {
                String v = body.substring(q1 + 1, q2);
                strncpy(out, v.c_str(), maxLen - 1);
            }
        }
    };

    getVal("sta_ssid", newCfg.sta_ssid, sizeof(newCfg.sta_ssid));
    getVal("sta_pass", newCfg.sta_pass, sizeof(newCfg.sta_pass));
    getVal("ap_ssid", newCfg.ap_ssid, sizeof(newCfg.ap_ssid));
    getVal("ap_pass", newCfg.ap_pass, sizeof(newCfg.ap_pass));

    if (strlen(newCfg.sta_ssid) == 0 || strlen(newCfg.ap_ssid) == 0) {
        server.send(400, "text/plain", "Required fields are empty");
        return;
    }

    char macBuf[32];
    memset(macBuf, 0, sizeof(macBuf));
    getVal("custom_mac", macBuf, sizeof(macBuf));
    String macStr = String(macBuf);
    macStr.trim();
    if (macStr.length() > 0) {
        uint8_t macBytes[6];
        if (!parseMac(macStr, macBytes)) {
            server.send(400, "text/plain", "Invalid MAC format. Use AA:BB:CC:DD:EE:FF");
            return;
        }
        newCfg.use_custom_mac = true;
        memcpy(newCfg.custom_mac, macBytes, 6);
    } else {
        newCfg.use_custom_mac = false;
    }

    ConfigStorage::save(newCfg);
    server.send(200, "text/plain", "OK");
    Serial.println(F("[CONFIG] Saved. Restarting in 1s..."));
    scheduledReboot = millis() + 1000;
}

void handleReset() {
    ConfigStorage::clear();
    server.send(200, "text/plain", "Reset OK");
    Serial.println(F("[CONFIG] Reset. Restarting in 1s..."));
    scheduledReboot = millis() + 1000;
}

void handleReboot() {
    server.send(200, "text/plain", "Rebooting");
    scheduledReboot = millis() + 500;
}

void applyLowLatencyTuning() {
    // 1. Force hardware sleep OFF at SDK level
    wifi_set_sleep_type(NONE_SLEEP_T);
    WiFi.setSleepMode(WIFI_NONE_SLEEP);

    // 2. Maximize RF output power
    WiFi.setOutputPower(20.5f);

    // 3. Set DTIM and Beacon parameters for minimal delivery latency
    struct softap_config ap_conf;
    wifi_softap_get_config(&ap_conf);
    ap_conf.beacon_interval = 100; // 100ms beacon interval
    wifi_softap_set_config_current(&ap_conf);
}

void setup() {
    Serial.begin(115200);
    delay(200);
    pinMode(STATUS_LED, OUTPUT);
    setLed(false);

    Serial.println();
    Serial.println(F("========================================"));
    Serial.println(F(" ESP8266 Low-Latency Turbo Repeater     "));
    Serial.println(F("========================================"));

    ConfigStorage::begin();
    bool hasConfig = ConfigStorage::load(currentConfig);

    IPAddress apIP(192, 168, 4, 1);
    IPAddress netMsk(255, 255, 255, 0);

    // Don't let the SDK auto-persist Wi-Fi state to flash on every connect;
    // we manage our own EEPROM config instead. Reduces flash wear & boot time.
    WiFi.persistent(false);

    if (hasConfig) {
        Serial.printf("[BOOT] Starting Low-Latency Repeater: STA='%s' -> AP='%s'\n", currentConfig.sta_ssid, currentConfig.ap_ssid);
        isSetupMode = false;
        WiFi.mode(WIFI_AP_STA);

        if (currentConfig.use_custom_mac) {
            wifi_set_macaddr(STATION_IF, currentConfig.custom_mac);
            Serial.printf("[MAC] Custom STA MAC applied: %s\n", macToStr(currentConfig.custom_mac).c_str());
        }

        applyLowLatencyTuning();

        WiFi.softAPConfig(apIP, apIP, netMsk);
        if (strlen(currentConfig.ap_pass) >= 8) {
            WiFi.softAP(currentConfig.ap_ssid, currentConfig.ap_pass);
        } else {
            WiFi.softAP(currentConfig.ap_ssid);
        }

        WiFi.setAutoReconnect(true);
        WiFi.begin(currentConfig.sta_ssid, currentConfig.sta_pass);
    } else {
        Serial.println(F("[BOOT] Mode Setup: SSID 'ESP8266-Repeater-Setup'"));
        isSetupMode = true;
        WiFi.mode(WIFI_AP);
        applyLowLatencyTuning();

        WiFi.softAPConfig(apIP, apIP, netMsk);
        WiFi.softAP("ESP8266-Repeater-Setup");
        dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
        dnsServer.start(53, "*", apIP);
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/scan", HTTP_GET, handleScan);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/clients", HTTP_GET, handleClients);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/reset", HTTP_POST, handleReset);
    server.on("/reboot", HTTP_POST, handleReboot);

    server.on("/generate_204", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.on("/hotspot-detect.html", HTTP_GET, []() {
        server.sendHeader("Location", "http://192.168.4.1/", true);
        server.send(302, "text/plain", "");
    });
    server.onNotFound([]() {
        String host = server.hostHeader();
        if (host.length() > 0 && !host.startsWith("192.168.4.1")) {
            server.sendHeader("Location", "http://192.168.4.1/", true);
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not Found");
        }
    });

    server.begin();
    Serial.println(F("[BOOT] Ready at http://192.168.4.1"));
}

void loop() {
    server.handleClient();
    
    if (isSetupMode) {
        dnsServer.processNextRequest();
    }

    unsigned long now = millis();

    if (!isSetupMode) {
        if (WiFi.status() == WL_CONNECTED) {
            wasConnected = true;
            if (!naptActive) {
                int ch = WiFi.channel();
                Serial.printf("[WIFI] Connected! WAN IP: %s on Channel %d\n", WiFi.localIP().toString().c_str(), ch);
                
                // Align SoftAP to exact same radio channel as upstream router to eliminate channel-switching hopping
                if (ch != lockedChannel && ch > 0) {
                    lockedChannel = ch;
                    IPAddress apIP(192, 168, 4, 1);
                    IPAddress netMsk(255, 255, 255, 0);
                    WiFi.softAPConfig(apIP, apIP, netMsk);
                    if (strlen(currentConfig.ap_pass) >= 8) {
                        WiFi.softAP(currentConfig.ap_ssid, currentConfig.ap_pass, lockedChannel);
                    } else {
                        WiFi.softAP(currentConfig.ap_ssid, "", lockedChannel);
                    }
                    Serial.printf("[WIFI] SoftAP locked to synchronized Channel %d (zero hopping latency)\n", lockedChannel);
                }

                err_t ret = ip_napt_init(1000, 32);
                if (ret == ERR_OK) {
                    ret = ip_napt_enable_no(SOFTAP_IF, 1);
                    if (ret == ERR_OK) {
                        naptActive = true;
                        Serial.println(F("[NAPT] 160MHz LwIP NAPT active and forwarding packets!"));
                    }
                }
                auto& dhcp = WiFi.softAPDhcpServer();
                dhcp.setDns(WiFi.dnsIP(0));
            }
            setLed((now % 2000) < 1900);
        } else {
            naptActive = false;
            if (wasConnected) {
                Serial.println(F("[WIFI] Upstream link lost. Will retry..."));
                wasConnected = false;
                lastReconnectAttempt = now;
            }
            // Belt-and-suspenders reconnect: force a fresh attempt periodically
            // in case the SDK's own auto-reconnect stalls (e.g. after the
            // upstream AP briefly disappears or changes channel).
            if (now - lastReconnectAttempt > RECONNECT_INTERVAL_MS) {
                WiFi.reconnect();
                lastReconnectAttempt = now;
            }
            if (now - lastBlink > 400) {
                ledState = !ledState;
                setLed(ledState);
                lastBlink = now;
            }
        }
    } else {
        if (now - lastBlink > 150) {
            ledState = !ledState;
            setLed(ledState);
            lastBlink = now;
        }
    }

    if (scheduledReboot > 0 && now >= scheduledReboot) {
        Serial.println(F("[SYSTEM] Restarting..."));
        ESP.restart();
    }

    // Zero delay for immediate packet processing
    yield();
}
