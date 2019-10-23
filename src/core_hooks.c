#include <err.h>
#include <stdio.h>

#include "b64/b64.h"
#include "config/config.h"
#include "hooks.h"
#include "irc/irc.h"
#include "log/log.h"
#include "utlist/list.h"
#include "irc/serializer.h"

static void
register_preinit_hook (const irc_server *s, const irc_msg *msg)
{
	struct config_t *config = get_config ();

	log_debug ("Registering client...\n");

	char *nick_params[] = { config->server->user->nickname };
	irc_msg *nick_msg =
	  irc_msg_new (NULL, "NICK", 1, nick_params);
	irc_push_message (s, nick_msg);

	char *user_params[] = { config->server->user->ident, "0", "*", config->server->user->realname };
	irc_msg *user_msg =
	  irc_msg_new (NULL, "USER", 4, user_params);
	irc_push_message (s, user_msg);
}

static void
sasl_preinit_hook (const irc_server *s, const irc_msg *msg)
{
	log_info ("Doing SASL Auth\n");

	char *cap_params[] = { "REQ", "sasl" };
	irc_msg *cap_msg =
	  irc_msg_new (NULL, "CAP", 2, cap_params);
	irc_push_message (s, cap_msg);

	char *auth_params[] = { "PLAIN" };
	irc_msg *auth_msg =
	  irc_msg_new (NULL, "AUTHENTICATE", 1, auth_params);
	irc_push_message (s, auth_msg);
}

static void
sasl_auth_hook (const irc_server *s, const irc_msg *msg)
{
	/* When we receive "AUTHENTICATE +" we can send our user data
	 */
	struct config_t *config = get_config ();
	char *auth_user = config->server->user->sasl_user;
	char *auth_pass = config->server->user->sasl_pass;

	log_info ("Doing SASL Auth\n");

	// TODO convert to non glib
	gchar *auth_string;
	char nullchar = '\0';
	auth_string = g_strdup_printf (
	  "%s%c%s%c%s", auth_user, nullchar, auth_user, nullchar, auth_pass);

	char *auth_string_encoded =
	  b64_encode (auth_string, strlen (auth_user) * 2 + strlen (auth_pass) + 2);

	char *auth_params[] = { auth_string_encoded };
	irc_msg *auth_msg =
	  irc_msg_new (NULL, "AUTHENTICATE", 1, auth_params);
	irc_push_message (s, auth_msg);

	g_free (auth_string);
}

static void
sasl_cap_hook (const irc_server *s, const irc_msg *msg)
{
	/* Once we receive the 903 command we know the auth was successful.
	 * to proceed we need to end the CAP phase
	 */
	char *cap_params[] = { "END" };
	irc_msg *cap_msg =
	  irc_msg_new (NULL, "CAP", 1, cap_params);
	irc_push_message (s, cap_msg);
}

static void
sasl_error_hook (const irc_server *s, const irc_msg *msg)
{
	log_error ("Error during SASL Auth\n");
	raise (SIGINT);
}

static void
channel_join_hook (const irc_server *s, const irc_msg *msg)
{
	/* If we encounter either the first MODE message or a WELCOME (001)
	 * message we know we are auth'ed and can exit the init loop.
	 * Before that we JOIN the Channels
	 */
	struct config_t *config = get_config ();

	log_debug ("Joining Channels: \n");

	struct irc_channel *l;
	LL_FOREACH (config->server->channels, l) {
		char *join_params[] = { l->channel };
		irc_msg *join_msg =
		  irc_msg_new (NULL, "JOIN", 1, join_params);
		irc_push_message (s, join_msg);
	}
}

static void
ping_hook (const irc_server *s, const irc_msg *msg)
{
	/* Responds to PING request with the correct PONG so we don't get timeouted
	 */
	char *pong_params[] = { msg->params->params[0] };
	irc_msg *pong_msg =
	  irc_msg_new (NULL, "PONG", 1, pong_params);
	irc_push_message (s, pong_msg);
}

static void
invite_hook (const irc_server *s, const irc_msg *msg)
{
	char *join_params[] = { msg->params->params[1] };
	irc_msg *join_msg =
	  irc_msg_new (NULL, "JOIN", 1, join_params);
	irc_push_message (s, join_msg);
}

static void
err_nickname_in_use_hook (const irc_server *s, const irc_msg *msg)
{
	log_error ("Nickname already in use\n");
	raise (SIGINT);
}

void
register_core_hooks ()
{
	struct config_t *config = get_config ();

	/* Handle SASL */
	if (config->server->user->sasl_enabled) {
		add_hook ("PREINIT", sasl_preinit_hook);
		add_hook ("AUTHENTICATE", sasl_auth_hook);
		add_hook ("900", sasl_cap_hook);
		add_hook ("903", sasl_cap_hook);
		add_hook ("904", sasl_error_hook);
	}

	add_hook ("PREINIT", register_preinit_hook);
	add_hook ("INVITE", invite_hook);
	add_hook ("PING", ping_hook);
	add_hook ("001", channel_join_hook);
	add_hook ("433", err_nickname_in_use_hook);
}
