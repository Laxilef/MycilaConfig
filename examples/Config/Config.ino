#include <MycilaConfig.h>

Mycila::Config config;
Preferences prefs;

static void assertEquals(const Mycila::ValueVariant& actual, const Mycila::ValueVariant& expected) {
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
  Serial.begin(115200);
  while (!Serial)
    continue;

  // prepare storage for tests
  prefs.begin("CONFIG", false);
  prefs.clear();
  prefs.putString("key4", "bar");
  prefs.end();
  prefs.begin("CONFIG", true);

  // listeners

  config.listen([](const char* key, const Mycila::ValueVariant& newValue) {
    Serial.printf("(listen) '%s' => '%s'\n", key, config.toString(newValue).c_str());
  });

  config.listen([]() {
    Serial.println("(restored)");
  });

  // begin()

  config.begin();

  // configure()

  config.configure("key1", false);
  config.configure("key2", "");
  config.configure("key3");
  config.configure("key4", "foo");
  config.configure("key5", "baz");
  config.configure("key6", std::to_string(6));

  // tests

  assertEquals(config.get<bool>("key1"), false);
  assertEquals(config.get<std::string>("key2", "not_empty").c_str(), "");
  assertEquals(config.get("key3", "abcd"), "");

  // check exists key
  assert(config.exists("key4"));

  // set global validator
  assert(config.setValidator([](const char* key, const Mycila::ValueVariant& newValue) {
    Serial.printf("(global validator) '%s' => '%s'\n", key, config.toString(newValue).c_str());
    return true;
  }));

  // set key
  assert(config.set("key1", true));
  assertEquals(config.get("key1").has_value(), true);
  assertEquals(config.get("key1").value(), true);
  assertEquals(config.get<bool>("key1"), true);
  assert(prefs.isKey("key1"));

  // set key to same value => no change
  assert(config.set("key1", true) == Mycila::Config::Result::ALREADY_PERSISTED);
  assert(!config.set("key1", true));

  // cache stored key
  assertEquals(config.get<std::string>("key4"), "bar"); // load key and cache

  // set key to same value => no change
  assert(config.set("key4", "bar") == Mycila::Config::Result::ALREADY_PERSISTED);
  assert(!config.set("key4", "bar"));

  // set stored key to default value
  assert(config.set("key4", "foo"));
  assertEquals(config.get("key4", ""), "foo");
  assert(prefs.isKey("key4"));

  // set stored key to other value
  assert(config.set("key4", "bar"));
  assertEquals(config.get("key4", ""), "bar");

  // unset global validator
  assert(config.setValidator(nullptr));

  // unset stored key
  assert(config.unset("key4"));
  assert(!prefs.isKey("key4"));
  assertEquals(config.get("key4", ""), "foo");

  // unset non-existing key => noop
  assert(!config.unset("key4"));
  assertEquals(config.get("key4").value(), "foo");

  // set validator
  assert(config.setValidator("key4", [](const char* key, const Mycila::ValueVariant& newValue) {
    Serial.printf("(validator) '%s' => '%s'\n", key, config.toString(newValue).c_str());
    return std::holds_alternative<std::string>(newValue) && std::get<std::string>(newValue) == "baz";
    //return newValue == (Mycila::ValueVariant)"baz";
  }));

  // try set a permitted value
  assert(config.set("key4", "baz"));
  assertEquals(config.get("key4", ""), "baz");

  // try set a NOT permitted value
  assert(config.set("key4", "bar") == Mycila::Config::Result::INVALID_VALUE);
  assert(!config.set("key4", "bar"));
  assertEquals(config.get("key4").value(), "baz");

  // unset validator
  assert(config.setValidator("key4", nullptr));

  // set un-stored to default value => no change
  assert(config.set("key5", "baz") == Mycila::Config::Result::SAME_AS_DEFAULT);
  assert(!config.set("key5", "baz"));

  // unset non stored key => noop
  assert(!config.unset("key5"));

  config.backup(Serial);

  config.set("key1", "value1");
  config.set("key2", "value2");

  config.backup(Serial);
  config.restore("key1=\nkey2=\nkey3=value3\nkey4=foo\n");

  assertEquals(config.get("key6").value(), "6");
  config.set("key6", std::to_string(7));
  assertEquals(config.get<std::string>("key6"), "7");


  for (int i = 100; i < 180; i++) {
    std::string key = "key_" + std::to_string(i);
    config.configure(key.c_str(), "foo");
    assert(config.set(key.c_str(), "bar"));
    delay(10);
  }
}

void loop() {
  Serial.printf("Free heap: %lu, min: %lu\n", ESP.getFreeHeap(), ESP.getMinFreeHeap());
  delay(5000);
}
