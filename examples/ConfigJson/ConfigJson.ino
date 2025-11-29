#include <ArduinoJson.h>
#include <WrappedConfig.h>
#include <StreamString.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

WrappedConfig::WrappedConfig config(std::make_shared<WrappedConfig::Storage::Preferences>());

uint8_t getLogLevel() {
  return config.get<bool>(KEY_DEBUG_ENABLE) ? ARDUHAL_LOG_LEVEL_DEBUG : ARDUHAL_LOG_LEVEL_INFO;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    continue;

  config.configure(KEY_DEBUG_ENABLE, false);
  config.configure(KEY_WIFI_SSID, "My_wifi");
  config.configure(KEY_WIFI_PWD, "Super_Secret_Password");
  config.begin();

  // Important for this example
  // Clear the old configuration
  config.clear();
}

void loop() {
  // print backup as json
  JsonDocument doc;
  config.toJson(doc.to<JsonObject>());
  Serial.println("======= BACKUP JSON =======");
  serializeJson(doc, Serial);
  Serial.println();
  Serial.println("===== END BACKUP JSON =====\n");

  // print backup as string
  StreamString content;
  content.reserve(1024);
  config.backup(content);
  Serial.println("======= BACKUP =======");
  Serial.println(content);
  Serial.println("===== END BACKUP =====\n");
  content.clear();

  // ^^^ OR:
  // Serial.println("======= BACKUP =======");
  // config.backup(Serial);
  // Serial.println("===== END BACKUP =====\n");

  // invert 'debug' key
  config.set(KEY_DEBUG_ENABLE, !config.get<bool>(KEY_DEBUG_ENABLE));

  // set new wifi pwd
  assert(config.set(KEY_WIFI_PWD, "pwd_" + std::to_string(millis())));

  Serial.printf("Free heap: %lu, min: %lu\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
  delay(5000);
}
