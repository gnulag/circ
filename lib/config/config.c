#include "config.h"
#include "log/log.h"
#include <stdlib.h>

char*
get_config_value (const char* key)
{
	return getenv (key);
}

int
get_config_length ()
{
	return sizeof (CONFIG_KEY_STRING) / sizeof (CONFIG_KEY_STRING[0]);
}

void
print_config ()
{
	int config_length = get_config_length ();

	for (int i = 0; i < config_length; i++) {
		log_info ("%s: %s\n",
		          CONFIG_KEY_STRING[i],
		          get_config_value (CONFIG_KEY_STRING[i]));
	}
}
