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
	if (get_config_value (CONFIG_KEY_STRING[2]) == "true") {
		is_secure = true;
	} else {
		is_secure = false;
	}

	irc_server s = { "Snoonet",
		             get_config_value (CONFIG_KEY_STRING[0]),
		             get_config_value (CONFIG_KEY_STRING[1]),
		             NULL,
		             is_secure };

	char* nick = get_config_value (CONFIG_KEY_STRING[3]);
	char* ident = get_config_value (CONFIG_KEY_STRING[4]);
	char* realname = get_config_value (CONFIG_KEY_STRING[5]);
	char* sasl_enabled = get_config_value (CONFIG_KEY_STRING[6]);
	char* auth_user = get_config_value (CONFIG_KEY_STRING[7]);
	char* auth_pass = get_config_value (CONFIG_KEY_STRING[8]);

	log_info ("setting up connection\n");
	int ret = irc_server_connect (&s);
	if (ret == -1) {
		err (1, "Error Connecting");
	}
	log_info ("connection setup\n");


	log_info ("sending nick/user info\n");

	GPtrArray* nick_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (nick_params, g_strdup (nick));
	IrciumMessage* nick_cmd =
	  ircium_message_new (NULL, NULL, "NICK", nick_params);
	ret |= irc_write_message (&s, nick_cmd);

	GPtrArray* user_params = g_ptr_array_new_full (4, g_free);
	g_ptr_array_add (user_params, g_strdup (ident));
	g_ptr_array_add (user_params, g_strdup ("0"));
	g_ptr_array_add (user_params, g_strdup ("*"));
	g_ptr_array_add (user_params, g_strdup (realname));
	IrciumMessage* user_cmd =
	  ircium_message_new (NULL, NULL, "USER", user_params);
	ret |= irc_write_message (&s, user_cmd);

	if (ret == -1 || errno) {
		err (1, "Error Setting Nick");
	}
	log_info ("sent nick/user info\n");

	if (strcmp (sasl_enabled, "true") == 0) {
		log_info ("Doing SASL Auth\n");
		GPtrArray* cap_params = g_ptr_array_new_full (2, g_free);
		g_ptr_array_add (cap_params, g_strdup ("REQ"));
		g_ptr_array_add (cap_params, g_strdup ("sasl"));
		IrciumMessage* cap_cmd =
		  ircium_message_new (NULL, NULL, "CAP", cap_params);
		ret |= irc_write_message (&s, cap_cmd);

		GPtrArray* auth_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (auth_params, g_strdup ("PLAIN"));
		IrciumMessage* auth_cmd =
		  ircium_message_new (NULL, NULL, "AUTHENTICATE", auth_params);
		ret |= irc_write_message (&s, auth_cmd);
	}

	irc_do_init_event_loop (&s);

	char* channel = get_config_value (CONFIG_KEY_STRING[9]);
	log_info ("channel: %s \n", channel);

	GPtrArray* join_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (join_params, channel);
	IrciumMessage* join_cmd =
	  ircium_message_new (NULL, NULL, "JOIN", join_params);

	ret |= irc_write_message (&s, join_cmd);
	if (ret == -1) {
		err (1, "Error during SASL Auth");
	}

	log_info ("entering main loop\n");
	irc_do_event_loop (&s);

	return 0;
}
