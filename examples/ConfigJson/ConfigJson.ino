#include <ArduinoJson.h>
#include <MycilaConfig.h>
#include <StreamString.h>

#define KEY_DEBUG_ENABLE "debug_enable"
#define KEY_WIFI_SSID "wifi_ssid"
#define KEY_WIFI_PWD "wifi_pwd"

Mycila::WrappedConfig config(std::make_shared<Mycila::PreferencesStorage>());

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
  JsonDocument doc;
  config.toJson(doc.to<JsonObject>());
  serializeJson(doc, Serial);
  Serial.println();

  StreamString content;
  content.reserve(1024);
  config.backup(content);
  Serial.println(content);

  assert(config.set(KEY_DEBUG_ENABLE, !config.get<bool>(KEY_DEBUG_ENABLE)));

  delay(5000);
}
