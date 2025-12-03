#include <WrappedConfig.h>
#include <WrappedConfig/Storage/Preferences.h>

auto configStorage = WrappedConfig::Storage::Preferences();
auto config = WrappedConfig::Config(configStorage);
Preferences prefs;

static void assertEquals(const WrappedConfig::Value& actual, const WrappedConfig::Value& expected) {
  if (actual != expected) {
    if (actual.isNull()) {
      Serial.printf("Expected '%s' but got NULL\n", expected.as<const char*>());

    } else {
      Serial.printf("Expected '%s' but got '%s'\n", expected.as<const char*>(), actual.as<const char*>());
    }

    assert(false);
  }
}

static void assertEquals(const char* actual, const char* expected) {
  auto _actual = actual != nullptr ? actual : "";
  auto _expected = expected != nullptr ? expected : "";

  if (strcmp(_actual, _expected) != 0) {
    Serial.printf("Expected '%s' but got '%s'\n", _expected, _actual);
    assert(false);
  }
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    continue;
  }

  // prepare storage for tests
  prefs.begin("config", false);
  prefs.clear();
  prefs.putString("key4", "bar");
  prefs.end();
  prefs.begin("config", true);

  // listeners
  config.listen([](const char* key, const WrappedConfig::Value& newValue) {
    if (newValue.isNull()) {
      Serial.printf("(listen) '%s' => NULL\n", key);

    } else {
      Serial.printf("(listen) '%s' => '%s'\n", key, newValue.as<const char*>());
    }
  });

  config.listen([]() {
    Serial.println("(restored)");
  });

  // configure()
  config.configure("key1", false);
  config.configure("key2", "");
  config.configure("key3", "");
  config.configure("key4", "foo");
  config.configure("key5", "baz");
  config.configure("key6", std::to_string(6));

  // begin()
  config.begin();

  // tests
  assertEquals(config.get<bool>("key1", true), false);
  assertEquals(config.get<WrappedConfig::LazyString>("key2", "not_empty"), "");
  assertEquals(config.get<const char*>("key3"), "");
  assertEquals(config.get<const char*>("key3", "abcd"), "");

  // check exists key
  assert(config.exists("key4"));

  // set global validator
  assert(config.setValidator([](const char* key, const WrappedConfig::Value& newValue) {
    if (newValue.isNull()) {
      Serial.printf("(global validator) '%s' => NULL\n", key);

    } else {
      Serial.printf("(global validator) '%s' => '%s'\n", key, newValue.as<const char*>());
    }

    return true;
  }));


  // set key
  assert(config.set("key1", true));
  assertEquals(config.get("key1"), true);
  assertEquals(config.get("key1"), true);
  assertEquals(config.get<bool>("key1"), true);
  assert(prefs.isKey("key1"));

  // set key to same value => no change
  assert(config.set("key1", true) == WrappedConfig::Status::SAME_AS_PERSISTED);
  assert(config.get("key1").as<int>() == 1);
  assert(config.set("key1", false));
  assert(config.get("key1").as<uint8_t>() == 0);
  assert(config.set("key1", true));
  assertEquals(config.get<std::string>("key1"), "true");

  // cache stored key
  assertEquals(config.get<WrappedConfig::LazyString>("key4"), "bar"); // load key and cache

  // set key to same value => no change
  assert(config.set("key4", "bar") == WrappedConfig::Status::SAME_AS_PERSISTED);
  assert(config.set("key4", "bar"));

  // set stored key to default value
  assert(config.set("key4", "foo") == WrappedConfig::Status::SAME_AS_DEFAULT);
  assertEquals(config.get<const char*>("key4", ""), "foo");
  assert(!prefs.isKey("key4"));

  // set stored key to other value
  assert(config.set("key4", "bar"));
  assertEquals(config.get<const char*>("key4", ""), "bar");

  // unset global validator
  assert(config.setValidator(nullptr));

  // unset stored key
  assert(config.unset("key4"));
  assert(!prefs.isKey("key4"));
  assertEquals(config.get<const char*>("key4", ""), "foo");

  // unset non-existing key => noop
  assert(!config.unset("key4"));
  assertEquals(config.get<const char*>("key4"), "foo");

  // set validator
  assert(config.setValidator("key4", [](const char* key, const WrappedConfig::Value& newValue) {
    if (newValue.isNull()) {
      Serial.printf("(validator) '%s' => NULL\n", key);

    } else {
      Serial.printf("(validator) '%s' => '%s'\n", key, newValue.as<const char*>());
    }

    return newValue == "baz";
  }));

  // try set a permitted value
  assert(config.set("key4", "baz"));
  assertEquals(config.get<const char*>("key4", ""), "baz");

  // try set a NOT permitted value
  assert(config.set("key4", "bar") == WrappedConfig::Status::INVALID_VALUE);
  assert(!config.set("key4", "bar"));
  assertEquals(config.get<const char*>("key4"), "baz");

  // unset validator
  assert(config.setValidator("key4", nullptr));

  // set un-stored to default value => no change
  assert(config.set("key5", "baz") == WrappedConfig::Status::SAME_AS_DEFAULT);
  assert(config.set("key5", "baz"));

  // unset non stored key => noop
  assert(!config.unset("key5"));

  Serial.println("======= BACKUP =======");
  config.backup(Serial);
  Serial.println("===== END BACKUP =====\n");

  config.set("key1", false);
  config.set("key3", "woof");

  Serial.println("======= BACKUP =======");
  config.backup(Serial);
  Serial.println("===== END BACKUP =====\n");
  config.restore("key1=false\nkey2=\nkey3=value3\nkey4=foo\n");

  assertEquals(config.get<const char*>("key6"), "6");
  config.set("key6", std::to_string(7));
  assertEquals(config.get<WrappedConfig::LazyString>("key6"), "7");
  assert(config.get<int16_t>("key6") == 7);
}

void loop() {
  Serial.printf(
    "Free heap: %lu, min: %lu, time consumption: %lu ms\n",
    ESP.getFreeHeap(), ESP.getMinFreeHeap(), millis()
  );
  vTaskDelete(NULL);
}
