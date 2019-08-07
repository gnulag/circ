#include <err.h>
#include <glib.h>
#include <stdio.h>

#include "b64/b64.h"
#include "config/config.h"
#include "hooks.h"
#include "irc/irc.h"
#include "log/log.h"
#include "utlist/list.h"

static void
register_preinit_hook (const irc_server *s, const IrciumMessage *msg)
{
	struct config_t *config = get_config ();

	log_debug ("Registering client...\n");
	/* Sends NICK */
	GPtrArray *nick_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (nick_params, g_strdup (config->server->user->nickname));
	const IrciumMessage *nick_cmd =
	  ircium_message_new (NULL, NULL, "NICK", nick_params);
	irc_push_message (s, nick_cmd);
	g_object_unref ((gpointer)nick_cmd);

	/* Sends USER */
	GPtrArray *user_params = g_ptr_array_new_full (4, g_free);
	g_ptr_array_add (user_params, g_strdup (config->server->user->ident));
	g_ptr_array_add (user_params, g_strdup ("0"));
	g_ptr_array_add (user_params, g_strdup ("*"));
	g_ptr_array_add (user_params, g_strdup (config->server->user->realname));
	const IrciumMessage *user_cmd =
	  ircium_message_new (NULL, NULL, "USER", user_params);
	irc_push_message (s, user_cmd);
	g_object_unref ((gpointer)user_cmd);
}

static void
sasl_preinit_hook (const irc_server *s, const IrciumMessage *msg)
{
	log_info ("Doing SASL Auth\n");
	GPtrArray *cap_params = g_ptr_array_new_full (2, g_free);
	g_ptr_array_add (cap_params, g_strdup ("REQ"));
	g_ptr_array_add (cap_params, g_strdup ("sasl"));
	const IrciumMessage *cap_cmd =
	  ircium_message_new (NULL, NULL, "CAP", cap_params);
	irc_push_message (s, cap_cmd);
	g_object_unref ((gpointer)cap_cmd);

	GPtrArray *auth_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (auth_params, g_strdup ("PLAIN"));
	const IrciumMessage *auth_cmd =
	  ircium_message_new (NULL, NULL, "AUTHENTICATE", auth_params);
	irc_push_message (s, auth_cmd);
	g_object_unref ((gpointer)auth_cmd);
}

static void
sasl_auth_hook (const irc_server *s, const IrciumMessage *msg)
{
	/* When we receive "AUTHENTICATE +" we can send our user data
	 */
	struct config_t *config = get_config ();
	char *auth_user = config->server->user->sasl_user;
	char *auth_pass = config->server->user->sasl_pass;

	log_info ("Doing SASL Auth\n");

	gchar *auth_string;
	char nullchar = '\0';
	auth_string = g_strdup_printf (
	  "%s%c%s%c%s", auth_user, nullchar, auth_user, nullchar, auth_pass);

	char *auth_string_encoded =
	  b64_encode (auth_string, strlen (auth_user) * 2 + strlen (auth_pass) + 2);

	GPtrArray *pass_params = g_ptr_array_new_full (1, g_free);

	g_ptr_array_add (pass_params, auth_string_encoded);
	const IrciumMessage *pass_cmd =
	  ircium_message_new (NULL, NULL, "AUTHENTICATE", pass_params);

	irc_push_message (s, pass_cmd);
	g_object_unref ((gpointer)pass_cmd);

	g_free (auth_string);
}

static void
sasl_cap_hook (const irc_server *s, const IrciumMessage *msg)
{
	/* Once we receive the 903 command we know the auth was successful.
	 * to proceed we need to end the CAP phase
	 */
	GPtrArray *pass_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (pass_params, "END");
	const IrciumMessage *pass_cmd =
	  ircium_message_new (NULL, NULL, "CAP", pass_params);

	irc_push_message (s, pass_cmd);
	g_object_unref ((gpointer)pass_cmd);
}

static void
sasl_error_hook (const irc_server *s, const IrciumMessage *msg)
{
	err (1, "Error during SASL Auth");
}

static void
channel_join_hook (const irc_server *s, const IrciumMessage *msg)
{
	/* If we encounter either the first MODE message or a WELCOME (001)
	 * message we know we are auth'ed and can exit the init loop.
	 * Before that we JOIN the Channels
	 */
	struct config_t *config = get_config ();

	log_debug ("Joining Channels: \n");

	struct irc_channel *l;
	LL_FOREACH (config->server->channels, l) {
		GPtrArray *join_params = g_ptr_array_new_full (1, g_free);
		g_ptr_array_add (join_params, l->channel);
		IrciumMessage *join_cmd =
		  ircium_message_new (NULL, NULL, "JOIN", join_params);

		irc_push_message (s, join_cmd);
		g_object_unref ((gpointer)join_cmd);
	}
}

static void
ping_hook (const irc_server *s, const IrciumMessage *msg)
{
	const GPtrArray *msg_params = ircium_message_get_params (msg);

	/* Responds to PING request with the correct PONG so we don't get timeouted
	 */
	GPtrArray *msg_params_nonconst = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (msg_params_nonconst, msg_params->pdata[0]);

	const IrciumMessage *pong_cmd =
	  ircium_message_new (NULL, NULL, "PONG", msg_params_nonconst);

	irc_push_message (s, pong_cmd);
	g_object_unref ((gpointer)pong_cmd);
}

static void
invite_hook (const irc_server *s, const IrciumMessage *msg)
{
	const GPtrArray *ircium_params = ircium_message_get_params (msg);
	GPtrArray *msg_params = g_ptr_array_new_full (ircium_params->len, g_free);
	g_ptr_array_add (msg_params, ircium_params->pdata[1]);

	const IrciumMessage *join_cmd =
	  ircium_message_new (NULL, NULL, "JOIN", msg_params);
	irc_push_message (s, join_cmd);
	g_object_unref ((gpointer)join_cmd);
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
}
