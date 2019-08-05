#ifndef IRC_H
#define IRC_H

#include "ircium-parser/ircium-message.h"
#include <glib.h>
#include <stdbool.h>

typedef struct irc_user
{
	char* nickname;
	char* ident;
	char* realname;

	bool sasl_enabled;
	char* sasl_user;
	char* sasl_pass;
} irc_user;

typedef struct irc_channel
{
	char channel[1024];
	struct irc_channel* next;
} irc_channel;

typedef struct irc_server
{
	char* name;
	char* host;
	char* port;
	bool secure;
	struct irc_user* user;
	struct irc_channel* channels;
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
irc_write_bytes (const irc_server* s, const char* buf, size_t nbytes);
const irc_server*
irc_get_server_from_name (const char* name);
const char*
irc_get_server_name (const irc_server*);

void
quit_irc_connection (const irc_server* s);
void
free_irc_server_connection (const irc_server* s);

void
irc_init_channels (const irc_server*);

#endif /* IRC_H */
