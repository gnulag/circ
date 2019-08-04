#include "irc-parser/ircium-message.h"
#include "irc.h"

typedef struct irc_hook
{
	char* command;
	void (*entry) (ServerType*, IrciumMessage* msg);
	struct irc_hook* next;
} irc_hook;

void
init_hooks (void);
void
add_hook (const char* command, void (*f) (ServerType*, IrciumMessage*));
const irc_hook*
get_hooks (const char* command);
void
exec_hooks (ServerType* s, const char* command, IrciumMessage* msg);
