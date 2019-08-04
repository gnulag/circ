#include "irc-parser/ircium-message.h"
#include <glib.h>
#include <stdbool.h>
#include "config/config.h"

void
register_core_hooks (void);

int
irc_server_connect (void);
void
irc_do_event_loop (ServerType*);
void
irc_do_init_event_loop (ServerType*);
int
irc_read_message (ServerType*, char*);
int
irc_read_bytes (ServerType*, char*, size_t);
int
irc_write_message (ServerType* s, IrciumMessage* message);
int
irc_write_bytes (ServerType* s, guint8* buf, size_t nbytes);

void
quit_irc_connection (ServerType* s);
void
free_irc_server_connection (ServerType* s);

void
irc_init_channels (ServerType*);
