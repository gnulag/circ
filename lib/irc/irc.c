#include <ev.h>            // Event loop
#include <gnutls/gnutls.h> // TLS support
#include <netdb.h>         // getaddrinfo
#include <sys/socket.h>    // Socket handling

#include <err.h> // err for panics
#include <errno.h>
#include <fcntl.h>
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
#include "log/log.h"

#define IRC_MESSAGE_SIZE 8192 // IRCv3 message size + 1 for '\0'

typedef struct message_queue
{
	char* message;
	struct message_queue *next;
} message_queue;

typedef struct
{
	const irc_server* server;
	gnutls_session_t tls_session;
	int socket;
	ev_io watcher;
	ev_timer timer;
	pthread_mutex_t queue_mtx;
	message_queue *queue;
	ev_io ev_init_watcher;
} irc_connection;

int
setnonblock (int fd);
int
verify_socket (int sock);
static void
irc_init_loop_callback (EV_P_ ev_io* w, int re);
static void
irc_loop_read_callback (EV_P_ ev_io* w, int re);
static void
irc_timeout_callback(EV_P_ ev_timer* w, int re);
static void
irc_process_message_queue (irc_connection* conn);
static void
handle_message (irc_connection* conn, const char *message);
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

// Simply adds O_NONBLOCK to the file descriptor of choice
int
setnonblock (int fd)
{
	int flags;

	flags = fcntl (fd, F_GETFL);
	flags |= O_NONBLOCK;
	return fcntl (fd, F_SETFL, flags);
}

/* Check if the socket is already Read- or Writeable */
int
verify_socket (int sock)
{
	struct timeval tv;
	fd_set readfds, writefds;
	int error;

	FD_SET (sock, &readfds);
	FD_SET (sock, &writefds);

	puts ("check write\n");
	tv.tv_sec = 10;
	tv.tv_usec = 500000;
	int rv = select (sock + 1, &readfds, &writefds, NULL, &tv);

	if (rv == -1) {
		perror ("client: connect failed");
		exit (EXIT_FAILURE);
	} else if (rv == 0) {
		perror ("client: connect timeout");
		exit (EXIT_FAILURE);
	}

	if (FD_ISSET (sock, &writefds) || FD_ISSET (sock, &readfds)) {
		int len = sizeof (error);
		if (getsockopt (sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
			perror ("client: socket not writeable");
			exit (EXIT_FAILURE);
		}
	} else {
		perror ("client: select error: sock not set");
		exit (EXIT_FAILURE);
	}
}

/*
 * Attempts to connect to server s,
 * return -1 if there's an error, 0 if it succeeded
 */
int
irc_server_connect (const irc_server* s)
{
	/*
	 * For now, don't attempt to connect if we're already connected
	 * to this server or if we have too many connections
	 */
	if (server_connected (s)) {
		log_info ("Server already connected");
		return -1;
	}

	if (connections_cap_reached ()) {
		log_info ("To Many Connections");
		return -1;
	}

	int sock = irc_create_socket (s);
	if (sock == -1) {
		err (1, "failed creating socket");
	}

	int ret = setup_irc_connection (s, sock);
	if (ret == 1) {
		err (1, "failed creating connection");
	} else if (ret != 0) {
		perror ("client: could not connect");
		exit (EXIT_FAILURE);
	}

	return 0;
}

static void
irc_init_loop_callback (EV_P_ ev_io* w, int re)
{
	irc_connection* conn = get_irc_connection_from_watcher (w);
	
    char buf[IRC_MESSAGE_SIZE];
	memset (buf, 0, sizeof (buf));

	irc_read_message (conn->server, buf);

	log_debug ("init loop: %s\n", buf);

	GByteArray* gbuf;
	gbuf = g_byte_array_new ();
	gbuf = g_byte_array_append (gbuf, (guint8* )buf, sizeof (buf));
	IrciumMessage* parsed_message = ircium_message_parse (gbuf, false);

	g_byte_array_free (gbuf, TRUE);

	const char* msg_command = ircium_message_get_command (parsed_message);
	const char* msg_source = ircium_message_get_source (parsed_message);
	const GPtrArray* msg_params = ircium_message_get_params (parsed_message);
	const GPtrArray* msg_tags;

	/* Responds to PING request with the correct PONG so we don't get timeouted
	 */
	if (strstr (msg_command, "PING") != NULL) {
		GPtrArray* msg_params_nonconst = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (msg_params_nonconst, msg_params->pdata[0]);

		IrciumMessage* pong_cmd =
		  ircium_message_new (NULL, NULL, "PONG", msg_params_nonconst);
		int ret = irc_write_message (conn->server, pong_cmd);

		g_free (g_ptr_array_free (msg_params_nonconst, TRUE));

		if (ret == -1) {
			err (1, "Error Responding to PING with PONG");
		}
	}

	char* sasl_enabled_str = getenv ("CIRC_SASL_ENABLED");
	bool sasl_enabled =
	  sasl_enabled_str && strncmp (sasl_enabled_str, "true", 4);
	if (sasl_enabled == 0 && strcmp (msg_command, "AUTHENTICATE") == 0 &&
	    strcmp (msg_params->pdata[0], "+") == 0) {
		/* When we receive "AUTHENTICATE +" we can send our user data
		 */
		char* auth_user = getenv ("CIRC_AUTH_USER");
		char* auth_pass = getenv ("CIRC_AUTH_PASS");

		log_info ("Doing SASL Auth\n");

		gchar* auth_string;
		char nullchar = '\0';
		auth_string = g_strdup_printf (
		  "%s%c%s%c%s", auth_user, nullchar, auth_user, nullchar, auth_pass);

		char* auth_string_encoded = b64_encode (
		  auth_string, strlen (auth_user) * 2 + strlen (auth_pass) + 2);

		GPtrArray* pass_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (pass_params, auth_string_encoded);
		IrciumMessage* pass_cmd =
		  ircium_message_new (NULL, NULL, "AUTHENTICATE", pass_params);

		int ret = irc_write_message (conn->server, pass_cmd);

		g_free (g_ptr_array_free (pass_params, TRUE));
		g_free (auth_string);

		if (ret == -1) {
			err (1, "Error during SASL Auth");
		}

	} else if (sasl_enabled == 0 && (strcmp (msg_command, "900") == 0 ||
	                                 strcmp (msg_command, "903") == 0)) {
		/* Once we receive the 903 command we know the auth was successful.
		 * to proceed we need to end the CAP phase
		 */
		GPtrArray* pass_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (pass_params, "END");
		IrciumMessage* pass_cmd =
		  ircium_message_new (NULL, NULL, "CAP", pass_params);

		int ret = irc_write_message (conn->server, pass_cmd);

		if (ret == -1) {
			err (1, "Error during SASL Auth");
		}
	} else if (sasl_enabled == 0 && strcmp (msg_command, "904") == 0) {
		err (1, "Error during SASL Auth");
	} else if (strcmp (msg_command, "MODE") == 0 ||
	           strcmp (msg_command, "001") == 0) {
		/* If we encounter either the first MODE message or a WELCOME (001)
		 * message we know we are auth'ed and can exit the init loop.
		 * Before that we JOIN the Channels
		 */
		char* channel = getenv ("CIRC_CHANNEL");
		log_info ("channel: %s \n", channel);

		GPtrArray* join_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (join_params, channel);
		IrciumMessage* join_cmd =
		  ircium_message_new (NULL, NULL, "JOIN", join_params);
		int ret = irc_write_message (conn->server, join_cmd);

		if (ret == -1) {
			err (1, "Error while Joining Channels");
		}

		ev_io_stop (EV_A_ w);
		ev_io_init (&conn->watcher, irc_loop_read_callback, conn->socket, EV_READ);
		ev_io_start (EV_A_ & conn->watcher);
	}
}

/* Start an I/O event loop for reading server s. */
void
irc_do_event_loop (const irc_server* s)
{
	irc_connection* conn = get_irc_server_connection (s);
	ev_timer timeout_watcher;

	struct ev_loop* loop = EV_DEFAULT;

	/* We First start the loop with the init callback to perform
	 * our initial setup like SASL and JOINing our channels.
	 * After that is finished the init loop stops the watcher
	 * and starts it again with the main callback.
	 */
	ev_io_init (&conn->watcher, irc_init_loop_callback, conn->socket, EV_READ);
	ev_io_start (loop, &conn->watcher);

	ev_timer_init (&timeout_watcher, irc_timeout_callback, 6, 0);
	ev_timer_start (loop, &timeout_watcher);

	while (true) {
		ev_run (loop, EVRUN_ONCE);
		irc_process_message_queue (conn);
	}
}

/* irc_loop_read_callback handles a single IRC message synchronously */
static void
irc_loop_read_callback (EV_P_ ev_io* w, int re)
{
	irc_connection* conn = get_irc_connection_from_watcher (w);
	char* buf = malloc(IRC_MESSAGE_SIZE);
	memset (buf, 0, IRC_MESSAGE_SIZE);
	irc_read_message (conn->server, buf);
	log_debug ("main loop: %s\n", buf);

	pthread_mutex_lock(&conn->queue_mtx);
	message_queue* mq = conn->queue;
	if (mq == NULL) {
		conn->queue = malloc(sizeof(message_queue));
		conn->queue->next = NULL;
	} else {
		while (mq->next != NULL)
			mq = mq->next;

		mq->next = malloc(sizeof(conn->queue));
		mq->next->next = NULL;
	}
	pthread_mutex_unlock(&conn->queue_mtx);

	ev_break (EV_A_ EVBREAK_ALL);
}

static void
irc_timeout_callback(EV_P_ ev_timer* w, int re)
{
	ev_break (EV_A_ EVBREAK_ONE);
}

static void
irc_process_message_queue (irc_connection* conn)
{
	message_queue* next;
	while (conn->queue != NULL) {
		handle_message(conn, conn->queue->message);
		next = conn->queue->next;
		pthread_mutex_lock(&conn->queue_mtx);
		// free(conn->queue->message);
		free(conn->queue);
		conn->queue = next;
		pthread_mutex_unlock(&conn->queue_mtx);
	}
}

static void
handle_message (irc_connection* conn, const char *message)
{
	GByteArray* gbuf;
	gbuf = g_byte_array_new ();
	gbuf = g_byte_array_append (gbuf, (guint8* )message, strlen (message));
	IrciumMessage* parsed_message = ircium_message_parse (gbuf, false);

	g_byte_array_free (gbuf, TRUE);

	const char* msg_command = ircium_message_get_command (parsed_message);
	const char* msg_source = ircium_message_get_source (parsed_message);
	const GPtrArray* msg_params = ircium_message_get_params (parsed_message);
	const GPtrArray* msg_tags;

	/* Responds to PING request with the correct PONG so we don't get timeouted
	 */
	if (strstr (msg_command, "PING") != NULL) {
		GPtrArray* msg_params_nonconst = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (msg_params_nonconst, msg_params->pdata[0]);

		IrciumMessage* pong_cmd =
		  ircium_message_new (NULL, NULL, "PONG", msg_params_nonconst);
		int ret = irc_write_message (conn->server, pong_cmd);

		g_free (g_ptr_array_free (msg_params_nonconst, TRUE));

		if (ret == -1) {
			err (1, "Error Responding to PING with PONG");
		}
	}

	// to_modules should return a list of responses for each matching module
	//	messages = to_modules(message);
	//	for (i = 0; messages[i] != NULL; i++)
	//		irc_write_bytes(ev_serv, messages[i]);
}

/* irc_read_message reads an IRC message to a buffer */
int
irc_read_message (const irc_server* s, char buf[IRC_MESSAGE_SIZE])
{
	int n = 1;
	int i = 0;

	irc_connection* c = get_irc_server_connection (s);
	if (c == NULL)
		return -1;

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
		ret = recv (c->socket, buf, nbytes, 0);

	return ret;
}

/* Serialize an IrciumMessage and send it to the server */
int
irc_write_message (const irc_server* s, IrciumMessage* message)
{
	GBytes* bytes = ircium_message_serialize (message);

	gsize len = 0;
	guint8* data = g_bytes_unref_to_data (bytes, &len);

	log_debug ("sending command: %s\n", data);

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
		puts ("emptry connection");
		return -1;
	}

	if (s->use_TLS) {
		ret = gnutls_record_send (c->tls_session, buf, nbytes);
	} else {
		ret = send (c->socket, buf, nbytes, 0);
	}

	return ret;
}

/* Creates a socket to connect to the irc_server s and returns it */
int
irc_create_socket (const irc_server* s)
{
	int ret, sock = -1;
	struct addrinfo *ai, hints;

	memset (&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	ret = getaddrinfo (s->host, s->port, &hints, &ai);
	if (ret != 0) {
		perror ("client: address");
		exit (EXIT_FAILURE);
	}

	/* Try the address info until we get a valid socket */
	int conn = -1;
	while (conn == -1 && (ai = ai->ai_next) != NULL) {
		sock = socket (ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (sock == -1) {
            perror ("client: socket");
            exit (EXIT_FAILURE);
        }


		/* [> We have a valid socket. Setup the connection <] */
		conn = connect (sock, ai->ai_addr, ai->ai_addrlen);
		if (conn == -1) {
			close (sock);
			conn = -1;
		}
	}

	freeaddrinfo (ai);

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
	if (s->use_TLS) {
		log_debug ("Encrypting connection\n");
		encrypt_irc_connection (c);
	}

    int ret = setnonblock (c->socket);
    if (ret == -1) {
    	perror ("client: socket O_NONBLOCK");
    	exit (EXIT_FAILURE);
    }
	
    verify_socket (sock);

	return 0;
}

/* Encrypt the irc_connection c with GnuTLS */
void
encrypt_irc_connection (irc_connection* c)
{
	int ret;
	gnutls_certificate_credentials_t creds;

	/* Initialize the credentials */
	gnutls_certificate_allocate_credentials (&creds);

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
	c->queue = NULL;
	pthread_mutex_init(&c->queue_mtx, NULL);
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
		if (w == &conns[i]->watcher)
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
