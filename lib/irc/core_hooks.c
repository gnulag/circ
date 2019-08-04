#include <err.h>
#include <glib.h>
#include <stdio.h>

#include "b64/b64.h"
#include "log/log.h"
#include "hooks.h"
#include "util/list.h"

static void
register_preinit_hook (ServerType* s, IrciumMessage* msg)
{
    struct ConfigType *config = get_config();

	log_debug ("Registering client...\n");
	/* Sends NICK */
	GPtrArray* nick_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (nick_params, g_strdup (config->server->user->nickname));
	IrciumMessage* nick_cmd =
	  ircium_message_new (NULL, NULL, "NICK", nick_params);
	if (irc_write_message (s, nick_cmd) == -1) {
		perror ("client: nick_cmd");
		exit (EXIT_FAILURE);
	}

	/* Sends USER */
	GPtrArray* user_params = g_ptr_array_new_full (4, g_free);
	g_ptr_array_add (user_params, g_strdup (config->server->user->ident));
	g_ptr_array_add (user_params, g_strdup ("0"));
	g_ptr_array_add (user_params, g_strdup ("*"));
	g_ptr_array_add (user_params, g_strdup (config->server->user->realname));
	IrciumMessage* user_cmd =
	  ircium_message_new (NULL, NULL, "USER", user_params);
	if (irc_write_message (s, user_cmd) == -1) {
		perror ("client: user_cmd");
		exit (EXIT_FAILURE);
	}
}

static void
sasl_preinit_hook (ServerType* s, IrciumMessage* msg)
{
	log_info ("Doing SASL Auth\n");
	GPtrArray* cap_params = g_ptr_array_new_full (2, g_free);
	g_ptr_array_add (cap_params, g_strdup ("REQ"));
	g_ptr_array_add (cap_params, g_strdup ("sasl"));
	IrciumMessage* cap_cmd = ircium_message_new (NULL, NULL, "CAP", cap_params);
	irc_write_message (s, cap_cmd);
	g_free (g_ptr_array_free (cap_params, TRUE));

	GPtrArray* auth_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (auth_params, g_strdup ("PLAIN"));
	IrciumMessage* auth_cmd =
	  ircium_message_new (NULL, NULL, "AUTHENTICATE", auth_params);
	irc_write_message (s, auth_cmd);
}

static void
sasl_auth_hook (ServerType* s, IrciumMessage* msg)
{
	/* When we receive "AUTHENTICATE +" we can send our user data
	 */
    struct ConfigType *config = get_config();
	char* auth_user = config->server->user->sasl_user;
	char* auth_pass = config->server->user->sasl_pass;

	log_info ("Doing SASL Auth\n");

	gchar* auth_string;
	char nullchar = '\0';
	auth_string = g_strdup_printf (
	  "%s%c%s%c%s", auth_user, nullchar, auth_user, nullchar, auth_pass);

	char* auth_string_encoded =
	  b64_encode (auth_string, strlen (auth_user) * 2 + strlen (auth_pass) + 2);

	GPtrArray* pass_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (pass_params, auth_string_encoded);
	IrciumMessage* pass_cmd =
	  ircium_message_new (NULL, NULL, "AUTHENTICATE", pass_params);

	int ret = irc_write_message (s, pass_cmd);

    free (auth_string_encoded);
	g_free (auth_string);
	if (ret == -1) {
		err (1, "Error during SASL Auth");
	}
}

static void
sasl_cap_hook (ServerType* s, IrciumMessage* msg)
{
	/* Once we receive the 903 command we know the auth was successful.
	 * to proceed we need to end the CAP phase
	 */
	GPtrArray* pass_params = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (pass_params, "END");
	IrciumMessage* pass_cmd =
	  ircium_message_new (NULL, NULL, "CAP", pass_params);

	if (irc_write_message (s, pass_cmd) == -1) {
		perror ("client: user_cmd");
		exit (EXIT_FAILURE);
	}
}

static void
sasl_error_hook (ServerType* s, IrciumMessage* msg)
{
	err (1, "Error during SASL Auth");
}

static void
channel_join_hook (ServerType* s, IrciumMessage* msg)
{
	/* If we encounter either the first MODE message or a WELCOME (001)
	 * message we know we are auth'ed and can exit the init loop.
	 * Before that we JOIN the Channels
	 */
    struct ConfigType *config = get_config();

    log_debug ("Joining Channels: \n");

    struct ChannelType *l;
    LL_FOREACH (config->server->channels, l) {
        GPtrArray* join_params = g_ptr_array_new_full (1, g_free);
        g_ptr_array_add (join_params, l->channel);
        IrciumMessage* join_cmd =
          ircium_message_new (NULL, NULL, "JOIN", join_params);
        if (irc_write_message (s, join_cmd) == -1) {
            perror ("client: user_cmd");
            exit (EXIT_FAILURE);
        }
    }
}

static void
ping_hook (ServerType* s, IrciumMessage* msg)
{
	const GPtrArray* msg_params = ircium_message_get_params (msg);

	/* Responds to PING request with the correct PONG so we don't get timeouted
	 */
	GPtrArray* msg_params_nonconst = g_ptr_array_new_full (1, g_free);
	g_ptr_array_add (msg_params_nonconst, msg_params->pdata[0]);

	IrciumMessage* pong_cmd =
	  ircium_message_new (NULL, NULL, "PONG", msg_params_nonconst);
	irc_write_message (s, pong_cmd);
}

static void
invite_hook (ServerType* s, IrciumMessage* msg)
{
	const GPtrArray* ircium_params = ircium_message_get_params (msg);
	GPtrArray* msg_params = g_ptr_array_new_full (ircium_params->len, g_free);
	g_ptr_array_add (msg_params, ircium_params->pdata[1]);

	IrciumMessage* join_cmd =
	  ircium_message_new (NULL, NULL, "JOIN", msg_params);
	irc_write_message (s, join_cmd);
}

void
register_core_hooks ()
{
    struct ConfigType *config = get_config();

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
