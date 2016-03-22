#include "config.h"

Config::Config(const char *filename)
{
  _filename = filename;
  int i;
  for (i = 0; i < NUM_CONFIG_ENTRIES; i++) {
    types[i] = CONFIG_TYPE_UNKNOWN;
    values[i] = NULL;
    keys[i] = NULL;
  }
}

Config::~Config() {
  reset();
}

void Config::reset() {
  int i;
  for (i = 0; i < NUM_CONFIG_ENTRIES; i++) {
    if (types[i] != CONFIG_TYPE_UNKNOWN && keys[i]) {
      free(keys[i]);
      keys[i] = NULL;

      if (types[i] == CONFIG_TYPE_STRING) {
        free(values[i]);
      }
      values[i] = NULL;
      types[i] = CONFIG_TYPE_UNKNOWN;
    }
  }
}

bool Config::readFile() {
  int ch;
  bool res = true;

  // Clear all existing values
  reset();

  // If file doesn't exist we are done
  if (!SPIFFS.exists(_filename)) {
    Serial.print("Config file ");
    Serial.print(_filename);
    Serial.println(" doesn't exist");
    return false;
  }
  
  File configFile = SPIFFS.open(_filename, "r");
  ch = configFile.read();
  if (ch != 'C') {
    Serial.print("Config Error: Invalid header, got ");
    Serial.println(ch, HEX);
    res = false;
  }

  while (res && configFile.available()) {
    char t = configFile.read();
    uint8_t l = configFile.read();

    if (l == 0 || l > 31) {
      Serial.print("Config Error: Invalid length ");
      Serial.println(l);
      res = false;
      break;
    }

    char key[32];
    configFile.read((uint8_t*)key, l);
    key[l] = 0;

    l = configFile.read();

    switch (t) {
      case 'I':
        if (l == sizeof(int)) {
          int val = 0;
          configFile.read((uint8_t*)&val, sizeof(int));
          setValueInt(key, val);
        } else {
          Serial.println("Config error: integer type not of sizeof(int)");
          res = false;
        }
        break;
      case 'S':
        {
          char value[64];
          if (l > sizeof(value)-1) {
            Serial.print("Config error: string length invalid");
            Serial.println(l);
            res = false;
          } else {
            configFile.read((uint8_t*)value, l);
            value[l] = 0;
            setValueStr(key, value);
          }
        }
        break;
      default:
        Serial.print("Config Error: Unknown field type '");
        Serial.print(t);
        Serial.print("'=0x");
        Serial.println(t, HEX);
        res = false;
        break;
    }
  }
  
  configFile.close();
  return res;
}

bool Config::writeFile() {
  int i;
  File configFile = SPIFFS.open(_filename, "w");
  configFile.write('C');
  for (i = 0; i < NUM_CONFIG_ENTRIES; i++) {
    switch (types[i]) {
      case CONFIG_TYPE_UNKNOWN: break;
      case CONFIG_TYPE_STRING:
        {
          uint8_t l;
          configFile.write('S');

          l = strlen(keys[i]);
          configFile.write(l);
          configFile.write((uint8_t*)keys[i], l);
          
          l = strlen((char*)values[i]);
          configFile.write(l);
          configFile.write((uint8_t*)values[i], l);
        }
        break;
      case CONFIG_TYPE_INT:
        configFile.write('I');
        
        uint8_t l = strlen(keys[i]);
        configFile.write(l);
        configFile.write((uint8_t*)keys[i], l);
          
        configFile.write(sizeof(int));
        configFile.write((uint8_t*)&values[i], sizeof(int));
        break;
    }
  }
  configFile.flush();
  configFile.close();
}

int Config::find_key(const char *key) {
  int i;
  for (i = 0; i < NUM_CONFIG_ENTRIES; i++) {
    if (types[i] != CONFIG_TYPE_UNKNOWN && keys[i] && strcmp(keys[i], key)) {
      return i;
    }
  }
  return -1;
}

ConfigType Config::getType(const char *key) {
  int i = find_key(key);
  if (i == -1)
    return CONFIG_TYPE_UNKNOWN;
  else
    return (ConfigType)types[i];
}

bool Config::keyExists(const char *key) {
  return find_key(key) != -1;
}

const char *Config::getValueStr(const char *key) {
  int i = find_key(key);
  if (i == -1)
    return "";

  if (values[i] == NULL)
    return "";

  return (const char *)values[i];
}

int Config::getValueInt(const char *key) {
  int i = find_key(key);
  if (i == -1)
    return 0;

  return (int)values[i];
}

int Config::allocate_entry(const char *key, ConfigType ctype) {
  int i = find_key(key);
  if (i == -1) {
    for (i = 0; i < NUM_CONFIG_ENTRIES; i++) {
      if (types[i] == CONFIG_TYPE_UNKNOWN)
      break;
    }
    // No free entry found
    if (i == NUM_CONFIG_ENTRIES) {
      Serial.println("BUG: Failed to find empty space in config");
      return -1;
    }
  }

  if (types[i] == CONFIG_TYPE_UNKNOWN) {
    keys[i] = strdup(key);
    types[i] = ctype;
  } else if (types[i] == CONFIG_TYPE_STRING) {
    free(values[i]);
    values[i] = NULL;
  } else if (types[i] == CONFIG_TYPE_INT) {
    values[i] = NULL;
  }
}

void Config::setValueInt(const char *key, int value) {
  int i = allocate_entry(key, CONFIG_TYPE_INT);
  if (i == -1)
    return;

  values[i] = (void*)value;
}

void Config::setValueStr(const char *key, const char *value) {
  int i = allocate_entry(key, CONFIG_TYPE_STRING);
  if (i == -1)
    return;

 values[i] = strdup(value);
}

