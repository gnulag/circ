#include <err.h>    // err for panics
#include <errno.h>  // errno
#include <unistd.h> // read, write

#include <stdbool.h> // malloc
#include <stdio.h>   // puts
#include <stdlib.h>  // malloc

#include <glib.h>

#include "config/config.h"

#include "b64/b64.h"
#include "utlist/list.h"

#include "irc/hooks.h"
#include "log/log.h"

#include "scheme/scheme.h"

void
exitHandler (int sig)
{
	log_debug ("Exiting\n");
	config_t *config = get_config ();
	quit_irc_connection (config->server);
	free_config ();
}

int
main (int argc, char **argv)
{
	signal (SIGHUP, exitHandler);
	signal (SIGINT, exitHandler);
	signal (SIGQUIT, exitHandler);
	signal (SIGILL, exitHandler);
	signal (SIGTRAP, exitHandler);
	signal (SIGABRT, exitHandler);

	const char *config_file_path = "./config.json";
	parse_config (config_file_path);

	struct config_t *config = get_config ();

	log_debug (
	  "-----\nConnecting to server: %s\nHost: %s\nPort: %s\nSSL: %u\n-----\n",
	  config->server->name,
	  config->server->host,
	  config->server->port,
	  config->server->secure);

	log_debug ("-----\nNickname %s\nIdent: %s\nRealname: %s\n-----\n",
		   config->server->user->nickname,
		   config->server->user->ident,
		   config->server->user->realname);

	log_info ("-----\nCommand Prefix: %s\n-----\n", config->cmd_prefix);

	struct irc_channel *elt;
	LL_FOREACH (config->server->channels, elt) {
		log_debug ("%s\n", elt->channel);
	}

	init_hooks ();
	setenv ("CHIBI_MODULE_PATH", "chibi-scheme/lib:scheme_libs", 1);
	scm_init ();
	register_core_hooks ();

	log_info ("setting up connection\n");
	int ret = irc_server_connect (config->server);
	if (ret == -1) {
		err (1, "Error Connecting");
	}
	log_info ("connection setup\n");

	exec_hooks (config->server, "PREINIT", NULL);

	/* Init Event Loop handels auth via sasl and breaks on
	 * receiving either a MODE or WELCOME message.
	 */
	irc_do_event_loop (config->server);

	return 0;
}
