// SPDX-License-Identifier: MIT
/*
 * Copyright (C) 2023-2025 Mathieu Carbou
 */
#include <MycilaConfig.h>
#include <Storage/MycilaPreferencesStorage.h>

Mycila::WrappedConfig config(std::make_shared<Mycila::PreferencesStorage>());

unsigned long lastHeapLog = 0;
unsigned long operationCount = 0;

void printHeap(const char* label) {
  Serial.printf("[HEAP] %s - Free: %" PRIu32 " bytes, Min: %" PRIu32 " bytes\n",
                label,
                ESP.getFreeHeap(),
                ESP.getMinFreeHeap());
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  Serial.println("\n=== BigConfig Example - 150 Keys ===\n");

  printHeap("Before configure()");

  // Configure 150 keys with various default values
  config.configure("wifi_ssid", "MyNetwork");
  config.configure("wifi_pwd", "secret123");
  config.configure("wifi_enable", true);
  config.configure("mqtt_host", "192.168.1.100");
  config.configure("mqtt_port", (uint16_t)1883);
  config.configure("mqtt_user", "admin");
  config.configure("mqtt_pwd", "mqtt_pass");
  config.configure("mqtt_enable", false);
  config.configure("ntp_server", "pool.ntp.org");
  config.configure("ntp_tz", "UTC");
  config.configure("ntp_enable", true);
  config.configure("web_port", (uint16_t)80);
  config.configure("web_user", "admin");
  config.configure("web_pwd", "admin");
  config.configure("web_enable", true);
  config.configure("ap_ssid", "ESP-Config");
  config.configure("ap_pwd", "12345678");
  config.configure("ap_enable", false);
  config.configure("hostname", "esp32-device");
  config.configure("device_name", "My ESP32");
  config.configure("device_id", "esp32-001");
  config.configure("relay1_name", "Relay 1");
  config.configure("relay1_enable", true);
  config.configure("relay2_name", "Relay 2");
  config.configure("relay2_enable", false);
  config.configure("relay3_name", "Relay 3");
  config.configure("relay3_enable", false);
  config.configure("relay4_name", "Relay 4");
  config.configure("relay4_enable", true);
  config.configure("sensor1_name", "Temperature");
  config.configure("sensor1_type", "DHT22");
  config.configure("sensor1_pin", (uint8_t)4);
  config.configure("sensor1_enbl", true);
  config.configure("sensor2_name", "Humidity");
  config.configure("sensor2_type", "DHT22");
  config.configure("sensor2_pin", (uint8_t)5);
  config.configure("sensor2_enbl", true);
  config.configure("sensor3_name", "Pressure");
  config.configure("sensor3_type", "BMP280");
  config.configure("sensor3_pin", (uint8_t)21);
  config.configure("sensor3_enbl", false);
  config.configure("led1_pin", (uint8_t)2);
  config.configure("led1_enable", true);
  config.configure("led2_pin", (uint8_t)13);
  config.configure("led2_enable", false);
  config.configure("led3_pin", (uint8_t)14);
  config.configure("led3_enable", false);
  config.configure("led4_pin", (uint8_t)15);
  config.configure("led4_enable", true);
  config.configure("btn1_pin", (uint8_t)18);
  config.configure("btn1_enable", true);
  config.configure("btn2_pin", (uint8_t)19);
  config.configure("btn2_enable", false);
  config.configure("log_level", "DEBUG");
  config.configure("log_enable", true);
  config.configure("telnet_port", (uint16_t)23);
  config.configure("telnet_enable", false);
  config.configure("serial_baud", (uint32_t)115200);
  config.configure("serial_enable", true);
  config.configure("i2c_sda", (uint8_t)21);
  config.configure("i2c_scl", (uint8_t)22);
  config.configure("i2c_freq", (uint32_t)100000);
  config.configure("i2c_enable", true);
  config.configure("spi_mosi", (uint8_t)23);
  config.configure("spi_miso", (uint8_t)19);
  config.configure("spi_sclk", (uint8_t)18);
  config.configure("spi_cs", (uint8_t)5);
  config.configure("spi_enable", false);
  config.configure("pwm1_pin", (uint8_t)25);
  config.configure("pwm1_freq", (uint32_t)5000);
  config.configure("pwm1_duty", (uint16_t)128);
  config.configure("pwm1_enable", true);
  config.configure("pwm2_pin", (uint8_t)26);
  config.configure("pwm2_freq", (uint32_t)5000);
  config.configure("pwm2_duty", (uint16_t)64);
  config.configure("pwm2_enable", false);
  config.configure("adc1_pin", (uint8_t)34);
  config.configure("adc1_enable", true);
  config.configure("adc2_pin", (uint8_t)35);
  config.configure("adc2_enable", false);
  config.configure("dac1_pin", (uint8_t)25);
  config.configure("dac1_enable", false);
  config.configure("dac2_pin", (uint8_t)26);
  config.configure("dac2_enable", false);
  config.configure("timer1_period", (uint16_t)1000);
  config.configure("timer1_enable", true);
  config.configure("timer2_period", (uint16_t)5000);
  config.configure("timer2_enable", false);
  config.configure("watchdog_time", (uint32_t)30000);
  config.configure("watchdog_enbl", true);
  config.configure("ota_port", (uint16_t)3232);
  config.configure("ota_pwd", "ota_pass");
  config.configure("ota_enable", true);
  config.configure("mdns_name", "esp32");
  config.configure("mdns_enable", true);
  config.configure("sntp_server1", "time.google.com");
  config.configure("sntp_server2", "time.nist.gov");
  config.configure("sntp_enable", true);
  config.configure("temp_unit", "C");
  config.configure("temp_offset", 0.0f);
  config.configure("pres_unit", "hPa");
  config.configure("pres_offset", 0.0f);
  config.configure("hum_offset", 0.0f);
  config.configure("altitude", 100.0f);
  config.configure("latitude", 45.5017f);
  config.configure("longitude", -73.5673f);
  config.configure("timezone_off", (int8_t)-5);
  config.configure("dst_enable", true);
  config.configure("disp_bright", (uint8_t)128);
  config.configure("disp_enable", true);
  config.configure("disp_timeout", (uint16_t)30000);
  config.configure("disp_type", "SSD1306");
  config.configure("disp_width", (uint16_t)128);
  config.configure("disp_height", (uint16_t)64);
  config.configure("disp_addr", (uint16_t)0x3C);
  config.configure("rtc_type", "DS3231");
  config.configure("rtc_enable", false);
  config.configure("sd_cs_pin", (uint8_t)5);
  config.configure("sd_enable", false);
  config.configure("sd_format", false);
  config.configure("bat_adc_pin", (uint8_t)36);
  config.configure("bat_v_divider", 2.0f);
  config.configure("bat_enable", true);
  config.configure("bat_low_volt", 3.3f);
  config.configure("bat_high_volt", 4.2f);
  config.configure("sleep_mode", "light");
  config.configure("sleep_time", (uint32_t)60000);
  config.configure("sleep_enable", false);
  config.configure("alarm1_hour", (uint8_t)7);
  config.configure("alarm1_min", (uint8_t)30);
  config.configure("alarm1_enable", true);
  config.configure("alarm2_hour", (uint8_t)19);
  config.configure("alarm2_min", (uint8_t)0);
  config.configure("alarm2_enable", false);
  config.configure("rgb_pin", (uint8_t)27);
  config.configure("rgb_count", (uint8_t)8);
  config.configure("rgb_bright", (uint8_t)50);
  config.configure("rgb_enable", true);
  config.configure("ir_rx_pin", (uint8_t)16);
  config.configure("ir_tx_pin", (uint8_t)17);
  config.configure("ir_enable", false);
  config.configure("rf_rx_pin", (uint8_t)32);
  config.configure("rf_tx_pin", (uint8_t)33);
  config.configure("rf_enable", false);
  config.configure("auth_user1", "admin");
  config.configure("auth_pwd1", "admin123");
  config.configure("auth_user2", "user");
  config.configure("auth_pwd2", "user123");

  printHeap("After configure()");
  
  config.begin("BIGCONFIG");

  printHeap("After begin()");

  // Register a change listener
  // listeners
  config.listen([](const char* key, const Mycila::ValueVariant& newValue) {
    Serial.printf("[CHANGE] %s = %s\n", key, config.toString(newValue).c_str());
  });

  Serial.println("\n=== Configuration Complete ===");
  Serial.printf("Total keys configured: %d\n", config.keys().size());
  Serial.println("Starting random operations...\n");

  lastHeapLog = millis();
}

void loop() {
  // Log heap every 2 seconds
  if (millis() - lastHeapLog >= 2000) {
    printHeap("Loop");
    Serial.printf("Operations completed: %lu\n\n", operationCount);
    lastHeapLog = millis();
  }

  // Perform random operations
  int op = random(0, 100);

  if (op < 40) {
    // 40% chance: GET operation
    const auto& keys = config.keys();
    const char* key = keys[random(0, keys.size())];
    auto value = config.get(key);
    Serial.printf("[GET] %s = %s\n", key, config.toString(value).c_str());

  } else if (op < 70) {
    // 30% chance: SET operation
    const char* keys[] = {
      "wifi_ssid",
      "mqtt_host",
      "device_name",
      "sensor1_name",
      "log_level",
      "hostname",
      "ntp_server",
      "temp_unit"};
    const char* values[] = {
      "Network1",
      "broker.local",
      "ESP Device",
      "TempSensor",
      "INFO",
      "my-esp32",
      "time.cloudflare.com",
      "F"};
    int idx = random(0, 8);
    auto result = config.set(keys[idx], values[idx]);
    Serial.printf("[SET] %s = %s (result: %s)\n",
                  keys[idx],
                  values[idx],
                  result ? "OK" : "FAILED");

  } else if (op < 85) {
    // 15% chance: SET bool
    const char* keys[] = {
      "wifi_enable",
      "mqtt_enable",
      "ntp_enable",
      "web_enable",
      "relay1_enable",
      "sensor1_enbl",
      "log_enable"};
    const char* key = keys[random(0, 7)];
    bool value = random(0, 2);
    config.set(key, (bool)value);
    Serial.printf("[SET_BOOL] %s = %s\n", key, value ? "true" : "false");

  } else {
    // 15% chance: UNSET operation
    const char* keys[] = {
      "pwm1_duty",
      "pwm2_duty",
      "temp_offset",
      "pres_offset",
      "hum_offset"};
    const char* key = keys[random(0, 5)];
    auto result = config.unset(key);
    Serial.printf("[UNSET] %s (result: %s)\n",
                  key,
                  result ? "OK" : "FAILED");
  }

  operationCount++;

  // Add small delay to avoid overwhelming the serial output
  delay(random(100, 500));
}
