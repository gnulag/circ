#include <sys/socket.h>		// Socket handling
#include <ev.h>			// Event loop
#include <gnutls/gnutls.h>	// TLS support
#include <netdb.h>		// getaddrinfo

#include <errno.h>
#include <stdlib.h>		// malloc, free
#include <string.h>
#include <unistd.h>		// close

#include "irc.h"


typedef struct {
	const irc_server *server;
	gnutls_session_t tls_session;
	int socket;
} irc_connection;

int irc_create_socket (const irc_server *);
int setup_irc_connection (const irc_server *, int);
void encrypt_irc_connection (irc_connection *);
irc_connection *create_irc_connection (const irc_server *, int);
int make_irc_connection_entry (irc_connection *);
irc_connection *get_irc_connection (const irc_server *);
bool server_connected (const irc_server *s);
bool connections_cap_reached (void);

/* conns simply holds the connections we use, it should be replaced later */
#define MAX_CONNECTIONS 10
static irc_connection *conns[MAX_CONNECTIONS + 1];

/*
 * Attempts to connect to server s,
 * return 1 if there's an error, 0 if it succeeded
 */
int irc_server_connect (const irc_server *s) {
	/*
	 * For now, don't attempt to connect if we're already connected
	 * to this server or if we have too many connections
	 */
	if (server_connected (s) || connections_cap_reached ())
		return 1;

	int sock = irc_create_socket (s);
	if (sock == -1)
		return 1;

	setup_irc_connection (s, sock);
	return 0;
}

/* Read nbytes from the irc_server s's connection */
int irc_read_bytes (const irc_server *s, char *buf, size_t nbytes) {
	if (buf == NULL)
		return -1;

	int ret;
	irc_connection *c = get_irc_connection (s);
	if (c == NULL)
		return -1;

	if (s->use_TLS)
		ret = gnutls_record_recv (c->tls_session, buf, nbytes);
	else
		ret = read (c->socket, buf, nbytes);

	return ret;
}

/* Write nbytes to the irc_server s's connection */
int irc_write_bytes (const irc_server *s, char *buf, size_t nbytes) {
	if (buf == NULL)
		return -1;

	int ret;
	irc_connection *c = get_irc_connection (s);
	if (c == NULL)
		return -1;

	if (s->use_TLS)
		ret = gnutls_record_send (c->tls_session, buf, nbytes);
	else
		ret = write (c->socket, buf, nbytes);

	return ret;
}

/* Creates a socket to connect to the irc_server s and returns it */
int irc_create_socket (const irc_server *s) {
	int ret, sock = -1;
	struct addrinfo *ai = NULL, *ai_head;

	ret = getaddrinfo (s->host, s->port, NULL, &ai);
	/* ai is a linked list, save the head to free it */
	ai_head = ai;
	if (ret)
		return -1;

	/* Try the address info until we get a valid socket */
	while (sock == -1 && (ai = ai->ai_next) != NULL) {
		sock = socket (ai->ai_family,
			ai->ai_socktype,
			ai->ai_protocol);

		/* We have a valid socket. Setup the connection */
		ret = connect(sock, ai->ai_addr, ai->ai_addrlen);
		if (ret == -1 || errno) {
			/*
			 * Connection failed, close
			 * the socket and keep looking
			 */
			close (sock);
			sock = -1;
			errno = 0;
			continue;
		}
	}

	freeaddrinfo (ai_head);
	return sock;
}

/* setup an IRC connection to server s, return whether it succeeded */
int setup_irc_connection (const irc_server *s, int sock) {
	irc_connection *c = create_irc_connection (s, sock);
	if (c == NULL)
		return 1;

	make_irc_connection_entry (c);
	if (s->use_TLS)
		encrypt_irc_connection (c);

	return 0;
}

/* Encrypt the irc_connection c with GnuTLS */
void encrypt_irc_connection (irc_connection *c) {
	int ret;
	gnutls_certificate_credentials_t creds;

	/* Initialize the credentails */
	gnutls_certificate_allocate_credentials (&creds);
	gnutls_certificate_set_x509_system_trust (creds);

	/* Initialize the session */
	gnutls_init (&c->tls_session, GNUTLS_CLIENT | GNUTLS_NONBLOCK);
	gnutls_set_default_priority (c->tls_session);

	/* Set credentials information */
	gnutls_credentials_set (c->tls_session, GNUTLS_CRD_CERTIFICATE, creds);
	gnutls_server_name_set (c->tls_session,
		GNUTLS_NAME_DNS,
		c->server->host,
		strlen (c->server->host));

	/* Link the socket to GnuTLS */
	gnutls_transport_set_int (c->tls_session, c->socket);
	do {
		/* Perform the handshake or die trying */
		ret = gnutls_handshake (c->tls_session);
	} while (ret == GNUTLS_E_INTERRUPTED || ret == GNUTLS_E_AGAIN);
}

/* Create an irc_connection for irc_server s */
irc_connection *create_irc_connection (const irc_server *s, int sock) {
	irc_connection *c = malloc (sizeof (irc_connection));
	if (c == NULL)
		return NULL;

	c->server = s;
	c->socket = sock;
	return c;
}

/* Store the irc_connection *c in the conns array */
int make_irc_connection_entry (irc_connection *c) {
	int i;
	for (i = 0; i < MAX_CONNECTIONS; i++) {
		if (conns[i] == NULL) {
			conns[i] = c;
			return 0;
		}
	}

	return -1;
}

/*
 * Return an irc_connection to irc_server s if there's one,
 * return NULL if there's none
 */
irc_connection *get_irc_connection (const irc_server *s) {
	int i;
	for (i = 0; conns[i] != NULL; i++) {
		if (s == conns[i]->server)
			return conns[i];
	}

	return NULL;
}

/* Returns whether the server is connected */
bool server_connected (const irc_server *s) {
	return get_irc_connection (s) != NULL;
}

/* Returns whether the connections cap is reached */
bool connections_cap_reached () {
	return conns[MAX_CONNECTIONS - 1] != NULL;
}
