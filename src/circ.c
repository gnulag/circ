#include <err.h>	// err for panics
#include <errno.h>	// errno
#include <unistd.h>	// read, write

#include <stdio.h>	// puts
#include <stdlib.h>	// malloc

#include "config/config.h"
#include "irc/irc.h"
#include "log/log.h"

#include "irc-parser/ircium-message.h"

int
main (int argc, char** argv)
{
    bool is_secure;
    if (get_config_value (CONFIG_KEY_STRING[2]) == "true") {
        is_secure = true;
    }
    else {
        is_secure = false; 
    }

    irc_server s = {
    	"Snoonet",
        get_config_value (CONFIG_KEY_STRING[0]),
        get_config_value (CONFIG_KEY_STRING[1]),
    	NULL,
    	is_secure
    };

    char *nick = get_config_value (CONFIG_KEY_STRING[3]);
    char *ident = get_config_value (CONFIG_KEY_STRING[4]);
    char *realname = get_config_value (CONFIG_KEY_STRING[5]);

	puts ("setting up connection");
	int ret = irc_server_connect (&s);
	puts ("connection setup");
	if (ret == -1) {
		err (1, "Error Connecting");
    }

	puts ("sending nick/user info");
	
    GPtrArray *nick_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (nick_params, g_strdup (nick));
    IrciumMessage *nick_cmd = ircium_message_new (NULL, NULL, "NICK", nick_params);

	GPtrArray *user_params = g_ptr_array_new_full (4, g_free);
	g_ptr_array_add (user_params, g_strdup (ident));
	g_ptr_array_add (user_params, g_strdup ("0"));
	g_ptr_array_add (user_params, g_strdup ("*"));
	g_ptr_array_add (user_params, g_strdup (realname));
    IrciumMessage *user_cmd = ircium_message_new (NULL, NULL, "USER", user_params);

    ret |= irc_write_message (&s, nick_cmd);
    ret |= irc_write_message (&s, user_cmd);
    
    if (ret == -1 || errno) {
        err (1, "Error Setting Nick");
    }

	puts ("sent nick/user info");
   
    /* char join_cmd[] = "JOIN #test\r\n"; */
    /* ret |= irc_write_bytes (&s, join_cmd, sizeof (join_cmd)); */
    /* if (ret == -1 || errno) { */
        /* err (1, "Error Joining channel"); */
    /* } */

	puts ("entering event loop");
    irc_do_event_loop(&s);

	return 0;
}
