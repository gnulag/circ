#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <glib.h>

#define DEBUG (get_config() && get_config()->debug)

typedef struct module
{
    char *name;
    char *matchers;
    char *cmd;
    char *cwd;
    char *config;
} module;

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

#endif /* CONFIG_H */
