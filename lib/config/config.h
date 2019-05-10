// nice macro from https://stackoverflow.com/a/10966395/3474615

#define FOREACH_CONFIG_KEY(T) \
  T(SERVER) \
  T(PORT) \
  T(NICK) \
  T(IDENT) \
  T(REALNAME)

#define GENERATE_ENUM(ENUM) CIRC_ ## ENUM,
#define GENERATE_STRING(STRING) "CIRC_" #STRING,

enum CONFIG_KEY_ENUM {
  FOREACH_CONFIG_KEY(GENERATE_ENUM)
};

static const char *CONFIG_KEY_STRING[] = {
  FOREACH_CONFIG_KEY(GENERATE_STRING)
};

char* getConfigValueForKey(const char* key);
int getConfigLength();
void printConfig();
