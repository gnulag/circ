#include "../modules/module.h"

typedef struct irc_hook {
	char *command;
	circ_module *mod;
	struct irc_hook *next;
} irc_hook;

void
init_hooks (void);
void
add_hook (char *command, circ_module *mod);
const irc_hook *
get_hooks (char *command);
