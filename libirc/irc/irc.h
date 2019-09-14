#ifndef IRC_H
#define IRC_H

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>

#include "irc/parser.h"
#include "irc/serializer.h"

typedef struct irc_user
{
	char *nickname;
	char *ident;
	char *realname;

	bool sasl_enabled;
	char *sasl_user;
	char *sasl_pass;
} irc_user;

typedef struct irc_channel
{
	char channel[1024];
	struct irc_channel *next;
} irc_channel;

typedef struct irc_server
{
	char *name;
	char *host;
	char *port;
	bool secure;
	struct irc_user *user;
	struct irc_channel *channels;
} irc_server;

void
register_core_hooks (void);

int
irc_server_connect (const irc_server *);
void
irc_do_event_loop (const irc_server *);
void
irc_do_init_event_loop (const irc_server *);
int
irc_read_message (const irc_server *, char *);
void
irc_push_message (const irc_server *s, irc_msg *message);
void
irc_push_string (const irc_server *s, const char *str);
const irc_server *
irc_get_server_from_name (const char *name);
const char *
irc_get_server_name (const irc_server *);

void
quit_irc_connection (const irc_server *s);
void
free_irc_server_connection (const irc_server *s);

void
irc_init_channels (const irc_server *);

/* Functions to handle message parsing */
void
start_message (void *user_data);
void
start_tags (void *user_data);
void
on_tag (const uint8_t *name, size_t name_len, const uint8_t *esc_value, size_t esc_value_len, void *user_data);
void
end_tags (void *user_data);

void
on_prefix (const uint8_t *prefix, size_t prefix_len, void *user_data);

void
on_command (const uint8_t *command, size_t command_len, void *user_data);

void
start_params (void *user_data);
void
on_param (const uint8_t *param, size_t param_len, void *user_data);
void
end_params (void *user_data);

void
end_message (void *user_data);

void
on_error (ircmsg_parser_err_code error, void *user_data);

#endif /* IRC_H */
