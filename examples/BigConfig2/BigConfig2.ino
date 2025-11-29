#include <WrappedConfig.h>
#include <Storage/Preferences.h>

WrappedConfig::WrappedConfig config(std::make_shared<WrappedConfig::Storage::Preferences>(), 150);

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
  while (!Serial) {
    continue;
  }

  Serial.println("\n=== BigConfig Example - 150 Keys ===\n");

  Serial.printf(
    "Size `ValueVariant`: %zu, Size `Value`: %zu, Size `LazyString`: %zu\n",
    sizeof(WrappedConfig::ValueVariant), sizeof(WrappedConfig::Value), sizeof(WrappedConfig::LazyString)
  );

  printHeap("Before configure()");

  // Configure 150 keys with various default values
  config.configure("wifi_ssid", "MyNetwork")
        .configure("wifi_pwd", "secret123")
        .configure<bool>("wifi_enable", true)
        .configure("mqtt_host", "192.168.1.100")
        .configure<uint16_t>("mqtt_port", 1883u)
        .configure("mqtt_user", "admin")
        .configure("mqtt_pwd", "mqtt_pass")
        .configure<bool>("mqtt_enable", false)
        .configure("ntp_server", "pool.ntp.org")
        .configure("ntp_tz", "UTC")
        .configure<bool>("ntp_enable", true)
        .configure<uint16_t>("web_port", 80u)
        .configure("web_user", "admin")
        .configure("web_pwd", "admin")
        .configure<bool>("web_enable", true)
        .configure("ap_ssid", "ESP-Config")
        .configure("ap_pwd", "12345678")
        .configure<bool>("ap_enable", false)
        .configure("hostname", "esp32-device")
        .configure("device_name", "My ESP32")
        .configure("device_id", "esp32-001")
        .configure("relay1_name", "Relay 1")
        .configure<bool>("relay1_enable", true)
        .configure("relay2_name", "Relay 2")
        .configure<bool>("relay2_enable", false)
        .configure("relay3_name", "Relay 3")
        .configure<bool>("relay3_enable", false)
        .configure("relay4_name", "Relay 4")
        .configure<bool>("relay4_enable", true)
        .configure("sensor1_name", "Temperature")
        .configure("sensor1_type", "DHT22")
        .configure<uint8_t>("sensor1_pin", 4u)
        .configure<bool>("sensor1_enbl", true)
        .configure("sensor2_name", "Humidity")
        .configure("sensor2_type", "DHT22")
        .configure<uint8_t>("sensor2_pin", 5u)
        .configure<bool>("sensor2_enbl", true)
        .configure("sensor3_name", "Pressure")
        .configure("sensor3_type", "BMP280")
        .configure<uint8_t>("sensor3_pin", 21u)
        .configure<bool>("sensor3_enbl", false)
        .configure<uint8_t>("led1_pin", 2u)
        .configure<bool>("led1_enable", true)
        .configure<uint8_t>("led2_pin", 13u)
        .configure<bool>("led2_enable", false)
        .configure<uint8_t>("led3_pin", 14u)
        .configure<bool>("led3_enable", false)
        .configure<uint8_t>("led4_pin", 15u)
        .configure<bool>("led4_enable", true)
        .configure<uint8_t>("btn1_pin", 18u)
        .configure<bool>("btn1_enable", true)
        .configure<uint8_t>("btn2_pin", 19u)
        .configure<bool>("btn2_enable", false)
        .configure("log_level", "DEBUG")
        .configure<bool>("log_enable", true)
        .configure<uint16_t>("telnet_port", 23u)
        .configure<bool>("telnet_enable", false)
        .configure<uint32_t>("serial_baud", 115200u)
        .configure<bool>("serial_enable", true)
        .configure<uint8_t>("i2c_sda", 21u)
        .configure<uint8_t>("i2c_scl", 22u)
        .configure<uint32_t>("i2c_freq", 100000u)
        .configure<bool>("i2c_enable", true)
        .configure<uint8_t>("spi_mosi", 23u)
        .configure<uint8_t>("spi_miso", 19u)
        .configure<uint8_t>("spi_sclk", 18u)
        .configure<uint8_t>("spi_cs", 5u)
        .configure<bool>("spi_enable", false)
        .configure<uint8_t>("pwm1_pin", 25u)
        .configure<uint32_t>("pwm1_freq", 5000u)
        .configure<uint16_t>("pwm1_duty", 128u)
        .configure<bool>("pwm1_enable", true)
        .configure<uint8_t>("pwm2_pin", 26u)
        .configure<uint32_t>("pwm2_freq", 5000u)
        .configure<uint16_t>("pwm2_duty", 64u)
        .configure<bool>("pwm2_enable", false)
        .configure<uint8_t>("adc1_pin", 34u)
        .configure<bool>("adc1_enable", true)
        .configure<uint8_t>("adc2_pin", 35u)
        .configure<bool>("adc2_enable", false)
        .configure<uint8_t>("dac1_pin", 25u)
        .configure<bool>("dac1_enable", false)
        .configure<uint8_t>("dac2_pin", 26u)
        .configure<bool>("dac2_enable", false)
        .configure<uint16_t>("timer1_period", 1000u)
        .configure<bool>("timer1_enable", true)
        .configure<uint16_t>("timer2_period", 5000u)
        .configure<bool>("timer2_enable", false)
        .configure<uint32_t>("watchdog_time", 30000u)
        .configure<bool>("watchdog_enbl", true)
        .configure<uint16_t>("ota_port", 3232u)
        .configure("ota_pwd", "ota_pass")
        .configure<bool>("ota_enable", true)
        .configure("mdns_name", "esp32")
        .configure<bool>("mdns_enable", true)
        .configure("sntp_server1", "time.google.com")
        .configure("sntp_server2", "time.nist.gov")
        .configure<bool>("sntp_enable", true)
        .configure("temp_unit", "C")
        .configure<float>("temp_offset", 0.0f)
        .configure("pres_unit", "hPa")
        .configure<float>("pres_offset", 0.0f)
        .configure<float>("hum_offset", 0.0f)
        .configure<float>("altitude", 100.0f)
        .configure<float>("latitude", 45.5017f)
        .configure<float>("longitude", -73.5673f)
        .configure<int8_t>("timezone_off", -5)
        .configure<bool>("dst_enable", true)
        .configure<uint8_t>("disp_bright", 128u)
        .configure<bool>("disp_enable", true)
        .configure<uint16_t>("disp_timeout", 30000u)
        .configure("disp_type", "SSD1306")
        .configure<uint16_t>("disp_width", 128u)
        .configure<uint16_t>("disp_height", 64u)
        .configure<uint16_t>("disp_addr", 0x3Cu)
        .configure("rtc_type", "DS3231")
        .configure<bool>("rtc_enable", false)
        .configure<uint8_t>("sd_cs_pin", 5u)
        .configure<bool>("sd_enable", false)
        .configure<bool>("sd_format", false)
        .configure<uint8_t>("bat_adc_pin", 36u)
        .configure<float>("bat_v_divider", 2.0f)
        .configure<bool>("bat_enable", true)
        .configure<float>("bat_low_volt", 3.3f)
        .configure<float>("bat_high_volt", 4.2f)
        .configure("sleep_mode", "light")
        .configure<uint32_t>("sleep_time", 60000u)
        .configure<bool>("sleep_enable", false)
        .configure<uint8_t>("alarm1_hour", 7u)
        .configure<uint8_t>("alarm1_min", 30u)
        .configure<bool>("alarm1_enable", true)
        .configure<uint8_t>("alarm2_hour", 19u)
        .configure<uint8_t>("alarm2_min", 0u)
        .configure<bool>("alarm2_enable", false)
        .configure<uint8_t>("rgb_pin", 27u)
        .configure<uint8_t>("rgb_count", 8u)
        .configure<uint8_t>("rgb_bright", 50u)
        .configure<bool>("rgb_enable", true)
        .configure<uint8_t>("ir_rx_pin", 16u)
        .configure<uint8_t>("ir_tx_pin", 17u)
        .configure<bool>("ir_enable", false)
        .configure<uint8_t>("rf_rx_pin", 32u)
        .configure<uint8_t>("rf_tx_pin", 33u)
        .configure<bool>("rf_enable", false)
        .configure("auth_user1", "admin")
        .configure("auth_pwd1", "admin123")
        .configure("auth_user2", "user")
        .configure("auth_pwd2", "user123");

  printHeap("After configure()");
  
  config.begin("BIGCONFIG");

  printHeap("After begin()");

  // Register a change listener
  // listeners
  config.listen([](const char* key, const WrappedConfig::Value& newValue) {
    Serial.printf("[CHANGE] %s = %s\n", key, newValue.toString().data());
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
    Serial.printf("[GET] %s = %s\n", key, value.toString().data());

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
