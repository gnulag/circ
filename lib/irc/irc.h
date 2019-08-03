#include "irc-parser/ircium-message.h"
#include <glib.h>
#include <stdbool.h>

typedef struct
{
	char* name;      // Server name
	char* host;      // Server host
	char* port;      // Connection port
	char** channels; // Joined channels (or to join)
	bool use_TLS;    // Whether to use TLS
} irc_server;

void
register_core_hooks (void);

int
irc_server_connect (const irc_server*);
void
irc_do_event_loop (const irc_server*);
void
irc_do_init_event_loop (const irc_server*);
int
irc_read_message (const irc_server*, char*);
int
irc_read_bytes (const irc_server*, char*, size_t);
int
irc_write_message (const irc_server* s, const IrciumMessage* message);
int
irc_write_bytes (const irc_server*, guint8*, size_t);

void
irc_init_channels (const irc_server*);
