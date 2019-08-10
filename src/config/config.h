#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define DEBUG (get_config () && get_config ()->debug)

typedef struct module_t
{
	char *name;
	char *cmd;
	char *cwd;
	char *config;
	char *matchers[];
} module_t;

typedef struct config_t
{
	bool debug;
	char *cmd_prefix;
	char *db_path;
	char *scheme_mod_dir;
	struct irc_server *server;
	struct module_t *modules[];
} config_t;

// configstruct* config;
struct config_t *
get_config ();
int
parse_config (const char *config_file_path);
void
free_config ();

#endif /* CONFIG_H */
