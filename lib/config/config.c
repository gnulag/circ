#include <stdlib.h>
#include "../log/log.h"
#include "config.h"

char* getConfigValueForKey(const char* key) {
  return getenv(key);
}

int getConfigLength() {
  return sizeof(CONFIG_KEY_STRING) / sizeof(CONFIG_KEY_STRING[0]);
}

void printConfig() {
  int configLength = getConfigLength();
  for (int i = 0; i < configLength; i++) {
    logInfo("%s: %s\n", CONFIG_KEY_STRING[i], getConfigValueForKey(CONFIG_KEY_STRING[i]));
  }
}
