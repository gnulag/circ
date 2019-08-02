#include <glib.h>
#include "hooks.h"

static GHashTable *hooks;

void
g_free_irc_hook (gpointer hook);
void
free_irc_hook (irc_hook *hook);
irc_hook *
create_irc_hook (char *command, circ_module *mod);
static irc_hook *
get_hooks_private (char *command);
static gboolean
command_strings_equal(gconstpointer a, gconstpointer b);

void
init_hooks (void)
{
	if (hooks != NULL)
		return;

	hooks = g_hash_table_new_full(g_str_hash, command_strings_equal,
			g_free, g_free_irc_hook);
}

static gboolean
command_strings_equal(gconstpointer a, gconstpointer b)
{
	return strcmp((const char *)a, (const char *)b) == 0;
}

void
g_free_irc_hook (gpointer hook)
{
	free_irc_hook ((irc_hook *)hook);
}

void
free_irc_hook (irc_hook *hook)
{
	irc_hook *tmp;
	while (hook != NULL) {
		tmp = hook;
		free (hook->command);
		hook = hook->next;
		free(tmp);
	}
}

irc_hook *
create_irc_hook (char *command, circ_module *mod)
{
	irc_hook *hook = malloc (sizeof (irc_hook));
	hook->command = strdup(command);
	hook->mod = mod;
	hook->next = NULL;

	return hook;
}

void
add_hook (char *command, circ_module *mod)
{
	irc_hook *hook = create_irc_hook (command, mod);
	irc_hook *head = get_hooks_private (command);
	if (hooks == NULL)
		g_hash_table_insert(hooks, command, hook);
	else {
		while (head->next != NULL)
			head = head->next;
		head->next = hook;
	}
}

static irc_hook *
get_hooks_private (char *command)
{
	return (irc_hook *)g_hash_table_lookup(hooks, command);
}

const irc_hook *
get_hooks (char *command)
{
	return get_hooks_private (command);
}
