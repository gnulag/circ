#include <glib.h>
#include <stdio.h>

#include "hooks.h"

static GHashTable* hooks;

void
g_free_irc_hook (gpointer hook);
void
free_irc_hook (irc_hook* hook);
irc_hook*
create_irc_hook (const char* command,
                 void (*f) (irc_server*, IrciumMessage*));
static irc_hook*
get_hooks_private (const char* command);

void
init_hooks (void)
{
	if (hooks != NULL)
		return;

	hooks =
	  g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free_irc_hook);
}

void
g_free_irc_hook (gpointer hook)
{
	free_irc_hook ((irc_hook*)hook);
}

void
free_irc_hook (irc_hook* hook)
{
	while (hook != NULL) {
		irc_hook *tmp = hook;
		free (hook->command);
        free (hook->entry);
		hook = hook->next;
		free (tmp);
	}
}

irc_hook*
create_irc_hook (const char* command,
                 void (*f) (irc_server*, IrciumMessage*))
{
	irc_hook* hook = malloc (sizeof (irc_hook));
	hook->command = strdup (command);
	hook->entry = f;
	hook->next = NULL;

	return hook;
}

void
add_hook (const char* command,
          void (*f) (irc_server*, IrciumMessage*))
{
	irc_hook* hook = create_irc_hook (command, f);
	irc_hook* head = get_hooks_private (command);
	if (head == NULL) {
		g_hash_table_insert (hooks, hook->command, hook);
	} else {
		while (head->next != NULL)
			head = head->next;
		head->next = hook;
	}
}

static irc_hook*
get_hooks_private (const char* command)
{
	return (irc_hook*)g_hash_table_lookup (hooks, command);
}

const irc_hook*
get_hooks (const char* command)
{
	return get_hooks_private (command);
}

void
exec_hooks (irc_server* s, const char* command, IrciumMessage* msg)
{
	const irc_hook* hook;
	for (hook = get_hooks (command); hook != NULL; hook = hook->next)
        // FIXME this entry isn't free'd
		hook->entry (s, msg);
}
