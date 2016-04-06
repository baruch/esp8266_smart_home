#ifndef CONFIG_H
#define CONFIG_H

#define NUM_CONFIG_ENTRIES 10

typedef enum ConfigType {
  CONFIG_TYPE_UNKNOWN,
  CONFIG_TYPE_INT,
  CONFIG_TYPE_STRING,
} ConfigType;

/* This class holds configuration data and can read/write it to storage.

   A key is always a string.
   A type is string or int
   A value is as it is typed.
*/
class Config {
  public:
    Config(const char *filename);
    ~Config();

    bool readFile();
    bool writeFile();

    ConfigType getType(const char *key);
    bool keyExists(const char *key);
    const char *getValueStr(const char *key);
    int getValueInt(const char *key);

    void setValueStr(const char *key, const char *value);
    void setValueInt(const char *key, int value);

  private:
    int find_key(const char *key);
    void reset(void);
    int allocate_entry(const char *key, ConfigType ctype);

    const char *_filename;
    char *keys[NUM_CONFIG_ENTRIES];
    char types[NUM_CONFIG_ENTRIES];
    void *values[NUM_CONFIG_ENTRIES];
};

#endif
