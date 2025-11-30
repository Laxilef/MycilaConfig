#include <WrappedConfig.h>
#include <Storage/Dummy.h>

WrappedConfig::WrappedConfig config(std::make_shared<WrappedConfig::Storage::Dummy>());

static void assertEquals(const WrappedConfig::Value& actual, const WrappedConfig::Value& expected) {
  if (actual != expected) {
    if (actual.isNull()) {
      Serial.printf("Expected '%s' but got NULL\n", expected.toString().c_str());

    } else {
      Serial.printf("Expected '%s' but got '%s'\n", expected.toString().c_str(), actual.toString().c_str());
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

  // listeners
  config.listen([](const char* key, const WrappedConfig::Value& newValue) {
    if (newValue.isNull()) {
      Serial.printf("(listen) '%s' => NULL\n", key);

    } else {
      Serial.printf("(listen) '%s' => '%s'\n", key, newValue.toString().c_str());
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
  assertEquals(config.get("key4"), "foo");
  assertEquals(config.get("key5", ""), "baz");

  // check exists key
  assert(config.exists("key4"));

  // set key
  assert(config.set("key1", true));
  assertEquals(config.get("key1"), true);
  assertEquals(config.get("key1"), true);
  assertEquals(config.get<bool>("key1"), true);

  auto key5 = std::string("key").append("5");
  assert(config.set(key5.c_str(), "woof"));
  auto sameKey5 = std::string("key").append("5");
  assertEquals(config.get<const char*>(sameKey5.c_str()), "woof");

  // set key to same value => no change
  assert(config.set("key1", true) == WrappedConfig::Status::SAME_AS_PERSISTED);
  assert(config.set("key1", true));

  // set key to same value => no change
  assert(config.set("key4", "bar") == WrappedConfig::Status::PERSISTED);
  assert(config.set("key4", "bar"));

  // set stored key to default value
  assert(config.set("key4", "foo") == WrappedConfig::Status::SAME_AS_DEFAULT);
  assertEquals(config.get<const char*>("key4", ""), "foo");

  // set stored key to other value
  assert(config.set("key4", "bar"));
  assertEquals(config.get<const char*>("key4", ""), "bar");

  // unset stored key
  assert(config.unset("key4"));
  assertEquals(config.get<const char*>("key4", ""), "foo");

  // unset non-existing key => noop
  assert(!config.unset("key4"));
  assertEquals(config.get<const char*>("key4"), "foo");

  // set un-stored to default value => no change
  assert(config.unset("key5"));
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
}

void loop() {
  Serial.printf(
    "Free heap: %lu, min: %lu, time consumption: %lu ms\n",
    ESP.getFreeHeap(), ESP.getMinFreeHeap(), millis()
  );
  vTaskDelete(NULL);
}
