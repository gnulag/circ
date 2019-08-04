// nice macro from https://stackoverflow.com/a/10966395/3474615

#include <stdbool.h>
#include <glib.h>

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
    char channel[1024];
    struct ChannelType *next;
} ChannelType;

typedef struct ServerType
{
    char *name;
    char *host;
    char *port;
    bool secure;
    struct UserType *user;
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
void free_config ();
