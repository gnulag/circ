#include "irc-parser/ircium-message.h"
#include "irc.h"

typedef struct irc_hook
{
	char* command;
	void (*entry) (const irc_server*, const IrciumMessage* msg);
	struct irc_hook* next;
} irc_hook;

void
init_hooks (void);
void
add_hook (const char* command, void (*f) (const irc_server*, const IrciumMessage*));
const irc_hook*
get_hooks (const char* command);
void
exec_hooks (const irc_server* s, char* command, const IrciumMessage* msg);
