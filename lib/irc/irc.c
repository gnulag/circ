#include <ev.h>            // Event loop
#include <gnutls/gnutls.h> // TLS support
#include <netdb.h>         // getaddrinfo
#include <sys/socket.h>    // Socket handling

#include <err.h> // err for panics
#include <errno.h>
#include <pthread.h> // pthread_mutex_*
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h> // malloc, free
#include <string.h>
#include <unistd.h> // close

#include <glib.h>

#include "irc.h"

#include "b64/b64.h"
#include "config/config.h"
#include "irc-parser/ircium-message.h"

#define IRC_MESSAGE_SIZE 8192 // IRCv3 message size + 1 for '\0'

typedef struct
{
	const irc_server* server;
	gnutls_session_t tls_session;
	int socket;
	ev_io ev_watcher;
	pthread_mutex_t ev_read_mtx;
	pthread_mutex_t ev_write_mtx;
} irc_connection;

static void
irc_init_loop_callback (EV_P_ ev_io* w, int re);
static void
irc_loop_callback (EV_P_ ev_io* w, int re);
int
irc_create_socket (const irc_server*);
int
setup_irc_connection (const irc_server*, int);
void
encrypt_irc_connection (irc_connection*);
irc_connection*
create_irc_connection (const irc_server*, int);
int
make_irc_connection_entry (irc_connection*);
irc_connection*
get_irc_server_connection (const irc_server*);
irc_connection*
get_irc_connection_from_watcher (const ev_io* w);
bool
server_connected (const irc_server* s);
bool
connections_cap_reached (void);

/* conns simply holds the connections we use, it should be replaced later */
#define MAX_CONNECTIONS 10
static irc_connection* conns[MAX_CONNECTIONS + 1];

/*
 * Attempts to connect to server s,
 * return 1 if there's an error, 0 if it succeeded
 */
int
irc_server_connect (const irc_server* s)
{
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

/* Start an I/O event loop for reading server s. */
void
irc_do_init_event_loop (const irc_server* s)
{
	irc_connection* conn = get_irc_server_connection (s);

	struct ev_loop* loop = EV_DEFAULT;

	pthread_mutex_init (&conn->ev_read_mtx, NULL);
	pthread_mutex_init (&conn->ev_write_mtx, NULL);
	ev_io_init (
	  &conn->ev_watcher, irc_init_loop_callback, conn->socket, EV_READ);
	ev_io_start (loop, &conn->ev_watcher);

	ev_run (loop, 0);
}

static void
irc_init_loop_callback (EV_P_ ev_io* w, int re)
{
	irc_connection* conn = get_irc_connection_from_watcher (w);
	//	int i;
	//	char **messages;
	char buf[IRC_MESSAGE_SIZE];
	memset (buf, 0, sizeof (buf));

	pthread_mutex_lock (&conn->ev_read_mtx);
	irc_read_message (conn->server, buf);
	pthread_mutex_unlock (&conn->ev_read_mtx);

	GByteArray* gbuf;
	gbuf = g_byte_array_new ();
	gbuf = g_byte_array_append (gbuf, buf, sizeof (buf));
	IrciumMessage* parsed_message = ircium_message_parse (gbuf, false);

	const char* msg_command = ircium_message_get_command (parsed_message);
	const char* msg_source = ircium_message_get_source (parsed_message);
	const GPtrArray* msg_params = ircium_message_get_params (parsed_message);
	const GPtrArray* msg_tags;

	puts (buf);

	char* sasl_enabled = get_config_value (CONFIG_KEY_STRING[6]);
	if (strcmp (sasl_enabled, "true") != 0) {
		ev_break;
	} else if (strcmp (sasl_enabled, "true") == 0 &&
	           strcmp (msg_command, "AUTHENTICATE") == 0 &&
	           strcmp (msg_params->pdata[0], "+") == 0) {

		char* auth_user = get_config_value (CONFIG_KEY_STRING[7]);
		char* auth_pass = get_config_value (CONFIG_KEY_STRING[8]);

		puts ("Doing SASL Auth");

		gchar* auth_string;
		char nullchar = '\0';
		auth_string = g_strdup_printf (
		  "%s%c%s%c%s", auth_user, nullchar, auth_user, nullchar, auth_pass);

		char* auth_string_encoded =
		  b64_encode (auth_string, sizeof (auth_user) + sizeof (auth_pass) + 2);

		GPtrArray* pass_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (pass_params, auth_string_encoded);
		IrciumMessage* pass_cmd =
		  ircium_message_new (NULL, NULL, "AUTHENTICATE", pass_params);
		int ret = irc_write_message (conn->server, pass_cmd);
		if (ret == -1) {
			err (1, "Error during SASL Auth");
		}

	} else if (strcmp (sasl_enabled, "true") == 0 &&
	           strcmp (msg_command, "900") == 0) {
		ev_break;
	} else if (strcmp (sasl_enabled, "true") == 0 &&
	           strcmp (msg_command, "904") == 0) {
		err (1, "Error during SASL Auth");
	}
}

/* Start an I/O event loop for reading server s. */
void
irc_do_event_loop (const irc_server* s)
{
	irc_connection* conn = get_irc_server_connection (s);

	struct ev_loop* loop = EV_DEFAULT;

	pthread_mutex_init (&conn->ev_read_mtx, NULL);
	pthread_mutex_init (&conn->ev_write_mtx, NULL);
	ev_io_init (&conn->ev_watcher, irc_loop_callback, conn->socket, EV_READ);
	ev_io_start (loop, &conn->ev_watcher);

	ev_run (loop, 0);
}

/* irc_loop_callback handles a single IRC message synchronously */
static void
irc_loop_callback (EV_P_ ev_io* w, int re)
{
	irc_connection* conn = get_irc_connection_from_watcher (w);
	//	int i;
	//	char **messages;
	char buf[IRC_MESSAGE_SIZE];
	memset (buf, 0, sizeof (buf));

	pthread_mutex_lock (&conn->ev_read_mtx);
	irc_read_message (conn->server, buf);
	pthread_mutex_unlock (&conn->ev_read_mtx);

	GByteArray* gbuf;
	gbuf = g_byte_array_new ();
	gbuf = g_byte_array_append (gbuf, buf, sizeof (buf));
	IrciumMessage* parsed_message = ircium_message_parse (gbuf, false);

	const char* msg_command = ircium_message_get_command (parsed_message);
	const char* msg_source = ircium_message_get_source (parsed_message);
	const GPtrArray* msg_params = ircium_message_get_params (parsed_message);
	const GPtrArray* msg_tags;

	puts (buf);

	if (strstr (msg_command, "PING") != NULL) {
		IrciumMessage* pong_cmd =
		  ircium_message_new (NULL, NULL, "PONG", msg_params);
		pthread_mutex_lock (&conn->ev_read_mtx);
		int ret = irc_write_message (conn->server, pong_cmd);
		pthread_mutex_unlock (&conn->ev_read_mtx);

		if (ret == -1 || errno) {
			err (1, "Error Responding to PING with PONG");
		}
	}

	// to_modules should return a list of responses for each matching module
	//	messages = to_modules(buf);
	//	pthread_mutex_lock (&conn->ev_write_mtx);
	//	for (i = 0; messages[i] != NULL; i++)
	//		irc_write_bytes(ev_serv, messages[i]);
	//	pthread_mutex_unlock (&conn->ev_write_mtx);
}

/* irc_read_message reads an IRC message to a buffer */
int
irc_read_message (const irc_server* s, char buf[IRC_MESSAGE_SIZE])
{
	int n = 1;
	int i = 0;

	for (i = 0; buf[i - 2] != '\r' && buf[i] != '\n'; i++) {
		n = irc_read_bytes (s, buf + i, 1);
	}

	return i;
}

/* Read nbytes from the irc_server s's connection */
int
irc_read_bytes (const irc_server* s, char* buf, size_t nbytes)
{
	if (buf == NULL)
		return -1;

	int ret;
	irc_connection* c = get_irc_server_connection (s);
	if (c == NULL)
		return -1;

	if (s->use_TLS)
		ret = gnutls_record_recv (c->tls_session, buf, nbytes);
	else
		ret = read (c->socket, buf, nbytes);

	return ret;
}

int
irc_write_message (const irc_server* s, IrciumMessage* message)
{
	GBytes* bytes = ircium_message_serialize (message);

	gsize len = 0;
	guint8* data = g_bytes_unref_to_data (bytes, &len);

	puts (data);

	size_t size = len;
	int ret = irc_write_bytes (s, data, size);

	return ret;
}

/* Write nbytes to the irc_server s's connection */
int
irc_write_bytes (const irc_server* s, guint8* buf, size_t nbytes)
{
	if (buf == NULL)
		return -1;

	int ret;
	irc_connection* c = get_irc_server_connection (s);
	if (c == NULL) {
		return -1;
	}

	if (s->use_TLS) {
		ret = gnutls_record_send (c->tls_session, buf, nbytes);
	} else {
		ret = write (c->socket, buf, nbytes);
	}

	return ret;
}

/* Creates a socket to connect to the irc_server s and returns it */
int
irc_create_socket (const irc_server* s)
{
	int ret, sock = -1;
	struct addrinfo *ai = NULL, *ai_head;

	ret = getaddrinfo (s->host, s->port, NULL, &ai);
	/* ai is a linked list, save the head to free it */
	ai_head = ai;
	if (ret)
		return -1;

	/* Try the address info until we get a valid socket */
	while (sock == -1 && (ai = ai->ai_next) != NULL) {
		sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);

		/* We have a valid socket. Setup the connection */
		ret = connect (sock, ai->ai_addr, ai->ai_addrlen);
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
int
setup_irc_connection (const irc_server* s, int sock)
{
	irc_connection* c = create_irc_connection (s, sock);
	if (c == NULL)
		return 1;

	make_irc_connection_entry (c);
	if (s->use_TLS)
		encrypt_irc_connection (c);

	return 0;
}

/* Encrypt the irc_connection c with GnuTLS */
void
encrypt_irc_connection (irc_connection* c)
{
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
irc_connection*
create_irc_connection (const irc_server* s, int sock)
{
	irc_connection* c = malloc (sizeof (irc_connection));
	if (c == NULL)
		return NULL;

	c->server = s;
	c->socket = sock;
	return c;
}

/* Store the irc_connection *c in the conns array */
int
make_irc_connection_entry (irc_connection* c)
{
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
irc_connection*
get_irc_server_connection (const irc_server* s)
{
	int i;
	for (i = 0; conns[i] != NULL; i++) {
		if (s == conns[i]->server)
			return conns[i];
	}

	return NULL;
}

/* Return the irc_connection for the server watched by the given watcher */
irc_connection*
get_irc_connection_from_watcher (const ev_io* w)
{
	int i;
	for (i = 0; conns[i] != NULL; i++) {
		if (w == &conns[i]->ev_watcher)
			return conns[i];
	}

	return NULL;
}

/* Returns whether the server is connected */
bool
server_connected (const irc_server* s)
{
	return get_irc_server_connection (s) != NULL;
}

/* Returns whether the connections cap is reached */
bool
connections_cap_reached ()
{
	return conns[MAX_CONNECTIONS - 1] != NULL;
}
