#ifndef IRC_H
#define IRC_H

#include "ircium-parser/ircium-message.h"
#include <glib.h>
#include <stdbool.h>

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
irc_server_connect (void);
void
irc_do_event_loop (irc_server*);
void
irc_do_init_event_loop (irc_server*);
int
irc_read_message (irc_server*, char*);
int
irc_read_bytes (irc_server*, char*, size_t);
int
irc_write_message (irc_server* s, IrciumMessage* message);
int
irc_write_bytes (irc_server* s, guint8* buf, size_t nbytes);

void
quit_irc_connection (irc_server* s);
void
free_irc_server_connection (irc_server* s);

void
irc_init_channels (irc_server*);

#endif /* IRC_H */
