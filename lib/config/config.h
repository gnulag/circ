// nice macro from https://stackoverflow.com/a/10966395/3474615

#include <stdbool.h>
#include <glib.h>

#define FOREACH_CONFIG_KEY(T) \
  T(SERVER) \
  T(PORT) \
  T(SSL) \
  T(NICK) \
  T(IDENT) \
  T(REALNAME) \
  T(SASL_ENABLED) \
  T(AUTH_USER) \
  T(AUTH_PASS) \
  T(CHANNEL)

#define GENERATE_ENUM(ENUM) CIRC_ ## ENUM,
#define GENERATE_STRING(STRING) "CIRC_" #STRING,

enum CONFIG_KEY_ENUM {
  FOREACH_CONFIG_KEY(GENERATE_ENUM)
};

static const char *CONFIG_KEY_STRING[] = {
  FOREACH_CONFIG_KEY(GENERATE_STRING)
};

typedef struct module
{
    char *name;
    char *matchers;
    char *cmd;
    char *cwd;
    char *config;
} module;

typedef struct UserType
{
    char *nickname;
    char *ident;
    char *realname;

    bool sasl_enabled;
    char *sasl_user;
    char *sasl_pass;
} UserType;

typedef struct ChannelType
{
    char channel[8000];
    struct ChannelType *next;
} ChannelType;

typedef struct ServerType
{
    char *name;
    char *host;
    char *port;
    bool secure;
    struct UserType *user;
    char *channel_string;
    struct ChannelType *channels;
} ServerType;

typedef struct ModuleType
{
    char *name;
    char *cmd;
    char *cwd;
    char *config;
    char *matchers[];
} ModuleType;

typedef struct ConfigType
{
    bool debug;
    struct ServerType *server;
    struct ModuleType *modules[];
} ConfigType;

// configstruct* config;
struct ConfigType* get_config();
int parse_config (const char *config_file_path);
char* get_config_value (const char* key);
int get_config_length ();
void print_config ();
