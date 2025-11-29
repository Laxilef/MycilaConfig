#include <WrappedConfig.h>

WrappedConfig::WrappedConfig config(std::make_shared<WrappedConfig::Storage::LittleFS>());
std::vector<std::string> dynamicKeys;

static void assertEquals(const WrappedConfig::Value& actual, const WrappedConfig::Value& expected) {
  if (actual != expected) {
    Serial.printf("Expected '%s' but got '%s'\n", config.toString(expected).c_str(), config.toString(actual).c_str());
    assert(false);
  }
}

static void assertEquals(const char* actual, const char* expected) {
  if (strcmp(actual, expected) != 0) {
    Serial.printf("Expected '%s' but got '%s'\n", expected, actual);
    assert(false);
  }
}

void setup() {
  dynamicKeys.reserve(300); // avoid reallocation
  Serial.begin(115200);
  while (!Serial)
    continue;

  // Important for this example
  // Clear the old configuration
  assert(LittleFS.begin());
  LittleFS.format();

  // start time
  auto startTime = millis();

  // listeners
  config.listen([](const char* key, const WrappedConfig::Value& newValue) {
    Serial.printf("(listen) '%s' => '%s'\n", key, config.toString(newValue).c_str());
  });
  config.listen([]() {
    Serial.println("(restored)");
  });

  // configure()
  config.configure("some_key", "some_value");
  config.configure("amount_dynamic_keys", (uint16_t) 0);

  for (uint16_t i = 0; i <= 299; i++) {
    dynamicKeys.push_back("dynamic_key_" + std::to_string(i));
  }

  uint16_t i = 0;
  for (const auto& key : dynamicKeys) {
    config.configure(key.c_str(), "foo " + std::to_string(i++));
  }

  // begin()
  config.begin();

  // tests
  assertEquals(config.get<WrappedConfig::LazyString>("some_key"), "some_value");
  assertEquals(config.get("some_key", "another"), "some_value");
  assertEquals(config.get<uint16_t>("amount_dynamic_keys"), (uint16_t) 0);

  // set key
  assert(config.set("amount_dynamic_keys", (uint16_t) 10));
  assert(!config.set("amount_dynamic_keys", "nope"));
  assert(config.unset("amount_dynamic_keys"));

  i = 0;
  for (const auto& key : dynamicKeys) {
    // test
    assertEquals(config.get<WrappedConfig::LazyString>(key.c_str()), "foo " + std::to_string(i));

    // set key to new value
    assert(config.set(key.c_str(), "baz"));
    assertEquals(config.get<WrappedConfig::LazyString>(key.c_str()), "baz");

    i++;
    delay(10);
  }
  dynamicKeys.clear();

  config.set("amount_dynamic_keys", i);

  Serial.println("======= BACKUP =======");
  config.backup(Serial);
  Serial.println("===== END BACKUP =====\n");

  Serial.printf("Time consumption: %lu ms\n", millis() - startTime);
}

void loop() {
  Serial.printf("Free heap: %lu, min: %lu\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());

  // Required to write the config to a file
  // To optimize memory wear
  if (config.flush()) {
    Serial.println("Config has been successfully saved to file");
  }

  delay(5000);
}
