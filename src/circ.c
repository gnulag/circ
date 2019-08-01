#include <err.h>    // err for panics
#include <errno.h>  // errno
#include <unistd.h> // read, write

#include <stdio.h>  // puts
#include <stdlib.h> // malloc

#include "config/config.h"
#include "irc/hooks.h"
#include "log/log.h"

#include "b64/b64.h"
#include "irc-parser/ircium-message.h"

#include <glib.h>


int
main (int argc, char** argv)
{
	bool is_secure;
	char* is_secure_str = getenv ("CIRC_SSL");
	if (is_secure_str != NULL && strncmp (is_secure_str, "true", 4) == 0) {
		is_secure = true;
	} else {
		is_secure = false;
	}

	init_hooks ();
	register_core_hooks ();
    
    /* config = { */
        /* .server_name = "default", */
        /* .server_host = "localhost", */
        /* .server_port = "6667", */
        /* .server_ssl = false, */
        /* .nickname = "circ", */
        /* .ident = "circ", */
        /* .realname = "https://github.com/gnulag/circ", */
        /* .sasl_enabled = false, */
        /* .sasl_user = "", */
        /* .sasl_pass = "", */
        /* .channels[0] = "#gnulag" */
    /* }; */

    struct ConfigType* config;
    config = malloc (sizeof(ConfigType*));
    const char *config_file_path = "./config.json";
    parse_config (config_file_path, config);

    log_debug ("-----\nConnecting to server: %s\nHost: %s\nPort: %s\nSSL: %u\n-----\n",
            config->server->name,
            config->server->host,
            config->server->port,
            config->server->secure);

	irc_server s = { config->server->name,
		             config->server->host,
		             config->server->port,
		             NULL,
		             config->server->secure };

	char* sasl_enabled = getenv ("CIRC_SASL_ENABLED");

	log_info ("setting up connection\n");
	int ret = irc_server_connect (&s);
	if (ret == -1 || errno) {
		err (1, "Error Connecting");
	}
	log_info ("connection setup\n");

	// Calling exec_hooks with a NULL message executes the PREINIT hooks
	exec_hooks (&s, "PREINIT", NULL);

	/* Init Event Loop handels auth via sasl and breaks on
	 * receiving either a MODE or WELCOME message.
	 */
	irc_do_event_loop (&s);

	return 0;
}
