#ifndef IRC_HOOKS_H
#define IRC_HOOKS_H

#include "ircium-parser/ircium-message.h"
#include "irc.h"

typedef struct irc_hook
{
	char* command;
	void (*entry) (irc_server*, IrciumMessage* msg);
	struct irc_hook* next;
} irc_hook;

void
init_hooks (void);
void
add_hook (const char* command, void (*f) (irc_server*, IrciumMessage*));
const irc_hook*
get_hooks (const char* command);
void
exec_hooks (irc_server* s, const char* command, IrciumMessage* msg);

#endif /* IRC_HOOKS_H */
