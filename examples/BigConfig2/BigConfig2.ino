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
  config.configure("wifi_ssid", "MyNetwork")
        .configure("wifi_pwd", "secret123")
        .configure("wifi_enable", true)
        .configure("mqtt_host", "192.168.1.100")
        .configure<uint16_t>("mqtt_port", 1883u)
        .configure("mqtt_user", "admin")
        .configure("mqtt_pwd", "mqtt_pass")
        .configure("mqtt_enable", false)
        .configure("ntp_server", "pool.ntp.org")
        .configure("ntp_tz", "UTC")
        .configure("ntp_enable", true)
        .configure<uint16_t>("web_port", 80u)
        .configure("web_user", "admin")
        .configure("web_pwd", "admin")
        .configure("web_enable", true)
        .configure("ap_ssid", "ESP-Config")
        .configure("ap_pwd", "12345678")
        .configure("ap_enable", false)
        .configure("hostname", "esp32-device")
        .configure("device_name", "My ESP32")
        .configure("device_id", "esp32-001")
        .configure("relay1_name", "Relay 1")
        .configure("relay1_enable", true)
        .configure("relay2_name", "Relay 2")
        .configure("relay2_enable", false)
        .configure("relay3_name", "Relay 3")
        .configure("relay3_enable", false)
        .configure("relay4_name", "Relay 4")
        .configure("relay4_enable", true)
        .configure("sensor1_name", "Temperature")
        .configure("sensor1_type", "DHT22")
        .configure<uint8_t>("sensor1_pin", 4u)
        .configure("sensor1_enbl", true)
        .configure("sensor2_name", "Humidity")
        .configure("sensor2_type", "DHT22")
        .configure<uint8_t>("sensor2_pin", 5u)
        .configure("sensor2_enbl", true)
        .configure("sensor3_name", "Pressure")
        .configure("sensor3_type", "BMP280")
        .configure("sensor3_pin", (uint8_t)21)
        .configure("sensor3_enbl", false)
        .configure("led1_pin", (uint8_t)2)
        .configure("led1_enable", true)
        .configure("led2_pin", (uint8_t)13)
        .configure("led2_enable", false)
        .configure("led3_pin", (uint8_t)14)
        .configure("led3_enable", false)
        .configure("led4_pin", (uint8_t)15)
        .configure("led4_enable", true)
        .configure("btn1_pin", (uint8_t)18)
        .configure("btn1_enable", true)
        .configure("btn2_pin", (uint8_t)19)
        .configure("btn2_enable", false)
        .configure("log_level", "DEBUG")
        .configure("log_enable", true)
        .configure("telnet_port", (uint16_t)23)
        .configure("telnet_enable", false)
        .configure("serial_baud", (uint32_t)115200)
        .configure("serial_enable", true)
        .configure("i2c_sda", (uint8_t)21)
        .configure("i2c_scl", (uint8_t)22)
        .configure("i2c_freq", (uint32_t)100000)
        .configure("i2c_enable", true)
        .configure("spi_mosi", (uint8_t)23)
        .configure("spi_miso", (uint8_t)19)
        .configure("spi_sclk", (uint8_t)18)
        .configure("spi_cs", (uint8_t)5)
        .configure("spi_enable", false)
        .configure("pwm1_pin", (uint8_t)25)
        .configure("pwm1_freq", (uint32_t)5000)
        .configure("pwm1_duty", (uint16_t)128)
        .configure("pwm1_enable", true)
        .configure("pwm2_pin", (uint8_t)26)
        .configure("pwm2_freq", (uint32_t)5000)
        .configure("pwm2_duty", (uint16_t)64)
        .configure("pwm2_enable", false)
        .configure("adc1_pin", (uint8_t)34)
        .configure("adc1_enable", true)
        .configure("adc2_pin", (uint8_t)35)
        .configure("adc2_enable", false)
        .configure("dac1_pin", (uint8_t)25)
        .configure("dac1_enable", false)
        .configure("dac2_pin", (uint8_t)26)
        .configure("dac2_enable", false)
        .configure("timer1_period", (uint16_t)1000)
        .configure("timer1_enable", true)
        .configure("timer2_period", (uint16_t)5000)
        .configure("timer2_enable", false)
        .configure("watchdog_time", (uint32_t)30000)
        .configure("watchdog_enbl", true)
        .configure("ota_port", (uint16_t)3232)
        .configure("ota_pwd", "ota_pass")
        .configure("ota_enable", true)
        .configure("mdns_name", "esp32")
        .configure("mdns_enable", true)
        .configure("sntp_server1", "time.google.com")
        .configure("sntp_server2", "time.nist.gov")
        .configure("sntp_enable", true)
        .configure("temp_unit", "C")
        .configure("temp_offset", 0.0f)
        .configure("pres_unit", "hPa")
        .configure("pres_offset", 0.0f)
        .configure("hum_offset", 0.0f)
        .configure("altitude", 100.0f)
        .configure("latitude", 45.5017f)
        .configure("longitude", -73.5673f)
        .configure("timezone_off", (int8_t)-5)
        .configure("dst_enable", true)
        .configure("disp_bright", (uint8_t)128)
        .configure("disp_enable", true)
        .configure("disp_timeout", (uint16_t)30000)
        .configure("disp_type", "SSD1306")
        .configure("disp_width", (uint16_t)128)
        .configure("disp_height", (uint16_t)64)
        .configure("disp_addr", (uint16_t)0x3C)
        .configure("rtc_type", "DS3231")
        .configure("rtc_enable", false)
        .configure("sd_cs_pin", (uint8_t)5)
        .configure("sd_enable", false)
        .configure("sd_format", false)
        .configure("bat_adc_pin", (uint8_t)36)
        .configure("bat_v_divider", 2.0f)
        .configure("bat_enable", true)
        .configure("bat_low_volt", 3.3f)
        .configure("bat_high_volt", 4.2f)
        .configure("sleep_mode", "light")
        .configure("sleep_time", (uint32_t)60000)
        .configure("sleep_enable", false)
        .configure("alarm1_hour", (uint8_t)7)
        .configure("alarm1_min", (uint8_t)30)
        .configure("alarm1_enable", true)
        .configure("alarm2_hour", (uint8_t)19)
        .configure("alarm2_min", (uint8_t)0)
        .configure("alarm2_enable", false)
        .configure("rgb_pin", (uint8_t)27)
        .configure("rgb_count", (uint8_t)8)
        .configure("rgb_bright", (uint8_t)50)
        .configure("rgb_enable", true)
        .configure("ir_rx_pin", (uint8_t)16)
        .configure("ir_tx_pin", (uint8_t)17)
        .configure("ir_enable", false)
        .configure("rf_rx_pin", (uint8_t)32)
        .configure("rf_tx_pin", (uint8_t)33)
        .configure("rf_enable", false)
        .configure("auth_user1", "admin")
        .configure("auth_pwd1", "admin123")
        .configure("auth_user2", "user")
        .configure("auth_pwd2", "user123");

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
