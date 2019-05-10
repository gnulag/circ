#include <stdlib.h>
#include <stdio.h>
#include "config.h"

char* getConfigValueForKey(char* key) {
  return getenv(key);
}

char* getConfigLength() {
  return sizeof(CONFIG_KEY_STRING) / sizeof(CONFIG_KEY_STRING[0]);
}

void printConfig() {
  int configLength = getConfigLength();
  for (int i = 0; i < configLength; i++) {
    getConfigValueForKey(CONFIG_KEY_STRING[i]);
  }
}
