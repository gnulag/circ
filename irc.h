#include <stdbool.h>

typedef struct {
	char *name;		// Server name
	char *host;		// Server host
	char *port;		// Connection port 
	char **channels;	// Joined channels (or to join)
	bool use_TLS;		// Whether to use TLS
} irc_server;

int irc_server_connect(const irc_server *);
int irc_read_bytes(const irc_server *, char *, size_t);
int irc_write_bytes(const irc_server *, char *, size_t);
