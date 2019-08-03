#include <err.h>    // err for panics
#include <errno.h>  // errno
#include <unistd.h> // read, write

#include <stdio.h>  // puts
#include <stdlib.h> // malloc
#include <stdbool.h> // malloc

#include "config/config.h"
#include "irc/hooks.h"
#include "log/log.h"

#include "b64/b64.h"
#include "irc-parser/ircium-message.h"
#include "util/list.h"

#include <glib.h>

int
main (int argc, char** argv)
{
    
    const char *config_file_path = "./config.json";
    parse_config (config_file_path);
    
    struct ConfigType* config = get_config();

    log_debug ("-----\nConnecting to server: %s\nHost: %s\nPort: %s\nSSL: %u\n-----\n",
            config->server->name,
            config->server->host,
            config->server->port,
            config->server->secure);

    log_debug ("-----\nNickname %s\nIdent: %s\nRealname: %s\n-----\n",
            config->server->user->nickname,
            config->server->user->ident,
            config->server->user->realname);
   
    struct ChannelType *elt;
    LL_FOREACH (config->server->channels, elt) {
        log_debug ("%s\n", elt->channel);
    }

	init_hooks ();
	register_core_hooks ();

	irc_server s = { config->server->name,
		             config->server->host,
		             config->server->port,
		             NULL,
		             config->server->secure };

	log_info ("setting up connection\n");
	int ret = irc_server_connect (&s);
	if (ret == -1) {
		err (1, "Error Connecting");
	}
	log_info ("connection setup\n");

	// Calling exec_hooks with a NULL message executes the PREINIT hooks
	exec_hooks (&s, NULL);

	/* Init Event Loop handels auth via sasl and breaks on
	 * receiving either a MODE or WELCOME message.
	 */
	irc_do_event_loop (&s);

	return 0;
}
