#include <err.h>    // err for panics
#include <errno.h>  // errno
#include <unistd.h> // read, write

#include <stdio.h>  // puts
#include <stdlib.h> // malloc

#include "config/config.h"
#include "irc/irc.h"
#include "log/log.h"

#include "b64/b64.h"
#include "irc-parser/ircium-message.h"

int
main (int argc, char** argv)
{
	bool is_secure;
	char *is_secure_str = getenv ("CIRC_SSL");
	if (is_secure_str != NULL && strncmp (is_secure_str, "true", 4) == 0) {
		is_secure = true;
	} else {
		is_secure = false;
	}

	irc_server s = { getenv ("CIRC_SERVER_NAME"),
		             getenv ("CIRC_SERVER_HOST"),
		             getenv ("CIRC_SERVER_PORT"),
		             NULL,
		             is_secure };

	char* nick = getenv ("CIRC_NICK");
	char* ident = getenv ("CIRC_IDENT");
	char* realname = getenv ("CIRC_REALNAME");
	char* sasl_enabled = getenv ("CIRC_SASL_ENABLED");




	log_info ("setting up connection\n");
	int ret = irc_server_connect (&s);
	if (ret == -1) {
		err (1, "Error Connecting");
	}
	log_info ("connection setup\n");

	log_info ("sending nick/user info\n");

    /* Sends NICK */
	GPtrArray* nick_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (nick_params, g_strdup (nick));
	IrciumMessage* nick_cmd =
	  ircium_message_new (NULL, NULL, "NICK", nick_params);
    if (irc_write_message (&s, nick_cmd) == -1) {
        perror ("client: nick_cmd");
        exit (EXIT_FAILURE);
    }

	g_free (g_ptr_array_free (nick_params, TRUE));

    /* Sends USER */
	GPtrArray* user_params = g_ptr_array_new_full (4, g_free);
	g_ptr_array_add (user_params, g_strdup (ident));
	g_ptr_array_add (user_params, g_strdup ("0"));
	g_ptr_array_add (user_params, g_strdup ("*"));
	g_ptr_array_add (user_params, g_strdup (realname));
	IrciumMessage* user_cmd =
	  ircium_message_new (NULL, NULL, "USER", user_params);
    if (irc_write_message (&s, user_cmd) == -1) {
        perror ("client: user_cmd");
        exit (EXIT_FAILURE);
    }

	g_free (g_ptr_array_free (user_params, TRUE));

	log_info ("sent nick/user info\n");

	/* If we want to do sasl auth we need to request
	 * the sasl capability and initialize the plain auth process
	 * after this the init event loop takes over doing the actual auth
     */
	if (sasl_enabled && strncmp(sasl_enabled, "true", 4) == 0) {
		log_info ("Doing SASL Auth\n");
		GPtrArray* cap_params = g_ptr_array_new_full (2, g_free);
		g_ptr_array_add (cap_params, g_strdup ("REQ"));
		g_ptr_array_add (cap_params, g_strdup ("sasl"));
		IrciumMessage* cap_cmd =
		  ircium_message_new (NULL, NULL, "CAP", cap_params);
		int ret = irc_write_message (&s, cap_cmd);
		g_free (g_ptr_array_free (cap_params, TRUE));

		GPtrArray* auth_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (auth_params, g_strdup ("PLAIN"));
		IrciumMessage* auth_cmd =
		  ircium_message_new (NULL, NULL, "AUTHENTICATE", auth_params);
		ret |= irc_write_message (&s, auth_cmd);
		g_free (g_ptr_array_free (auth_params, TRUE));
	}

	/* Init Event Loop handels auth via sasl and breaks on
	 * receiving either a MODE or WELCOME message.
	 */
    irc_do_event_loop (&s);

	return 0;
}
