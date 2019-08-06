#include <string.h>

#include "../config/config.h"
#include "irc/irc.h"
#include "scheme.h"

static scm_module*
get_module (sexp ctx)
{
	sexp id_sym = sexp_intern (ctx, "circ-module-id", -1);
	sexp id_obj = sexp_eval (ctx, id_sym, sexp_context_env (ctx));
	int id = sexp_unbox_fixnum (id_obj);
	return scm_get_module_from_id (id);
}

static sexp
GPtrArray_to_scheme_list (sexp ctx, const GPtrArray* arr, int index)
{
	if (arr->len == index)
		return SEXP_NULL;
	else
		return sexp_cons (ctx,
				  sexp_c_string (ctx, (char*)arr->pdata[index], -1),
				  GPtrArray_to_scheme_list (ctx, arr, index + 1));
}

sexp
scmapi_register_hook (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_FALSE;

	const char* cmd_c = sexp_string_data (cmd);
	scm_add_irc_hook (cmd_c, func, mod);
	return SEXP_TRUE;
}

sexp
scmapi_register_command (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_FALSE;

	const char* cmd_c = sexp_string_data (cmd);
	scm_add_command_hook (cmd_c, func, mod);
	return SEXP_TRUE;
}

sexp
scmapi_send_raw (sexp ctx, sexp self, sexp n, sexp raw)
{

	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	if (!sexp_stringp (raw)) {
		printf ("scmapi_send_raw: %s: The argument is not a string\n",
			mod->path);
		return SEXP_NULL;
	}

	const irc_server* s = mod->mod_ctx.serv;
	const char* raw_c = sexp_string_data (raw);
	irc_write_bytes (s, raw_c, strlen (raw_c));

	return SEXP_NULL;
}

sexp
scmapi_reply (sexp ctx, sexp self, sexp n, sexp text)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	if (!sexp_stringp (text)) {
		printf ("scmapi_reply: %s: The argument is not a string\n", mod->path);
		return SEXP_NULL;
	}

	const irc_server* s = mod->mod_ctx.serv;
	const char* text_c = sexp_string_data (text);
	const char* command = ircium_message_get_command (mod->mod_ctx.msg);
	const GPtrArray* params = ircium_message_get_params (mod->mod_ctx.msg);

	if (strcmp (command, "PRIVMSG") != 0)
		return SEXP_NULL;

	char buf[4096];
	snprintf (buf, 4096, "PRIVMSG %s :%s\r\n", params->pdata[0], text_c);
	irc_write_bytes (s, buf, strlen (buf));

	return SEXP_NULL;
}

sexp
scmapi_get_cmd_prefix (sexp ctx, sexp self, sexp n)
{
	config_t* config = get_config ();
	return sexp_c_string (ctx, config->cmd_prefix, -1);
}

sexp
scmapi_get_server_name (sexp ctx, sexp self, sexp n)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const irc_server* s = mod->mod_ctx.serv;
	return sexp_c_string (ctx, s->name, -1);
}

sexp
scmapi_get_message_source (sexp ctx, sexp self, sexp n)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const IrciumMessage* msg = mod->mod_ctx.msg;
	return sexp_c_string (ctx, (char*)ircium_message_get_source (msg), -1);
}

sexp
scmapi_get_message_command (sexp ctx, sexp self, sexp n)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const IrciumMessage* msg = mod->mod_ctx.msg;
	return sexp_c_string (ctx, (char*)ircium_message_get_command (msg), -1);
}

sexp
scmapi_get_message_tags (sexp ctx, sexp self, sexp n)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const IrciumMessage* msg = mod->mod_ctx.msg;
	return GPtrArray_to_scheme_list (ctx, ircium_message_get_tags (msg), 0);
}

sexp
scmapi_get_message_params (sexp ctx, sexp self, sexp n)
{
	scm_module* mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const IrciumMessage* msg = mod->mod_ctx.msg;
	return GPtrArray_to_scheme_list (ctx, ircium_message_get_params (msg), 0);
}

void
scmapi_define_foreign_functions (sexp ctx)
{
	sexp env = sexp_context_env (ctx);

	/* Entry registrations */
	sexp_define_foreign (ctx, env, "register-hook", 2, scmapi_register_hook);
	sexp_define_foreign (
	  ctx, env, "register-command", 2, scmapi_register_command);

	/* Server interactions */
	sexp_define_foreign (ctx, env, "reply", 1, scmapi_reply);
	sexp_define_foreign (ctx, env, "send-raw", 1, scmapi_send_raw);

	/* IRC config information */
	sexp_define_foreign (ctx, env, "get-cmd-prefix", 0, scmapi_get_cmd_prefix);

	/* Server information */
	sexp_define_foreign (
	  ctx, env, "get-server-name", 0, scmapi_get_server_name);

	/* message information */
	sexp_define_foreign (
	  ctx, env, "get-message-source", 0, scmapi_get_message_source);
	sexp_define_foreign (
	  ctx, env, "get-message-command", 0, scmapi_get_message_command);
	sexp_define_foreign (
	  ctx, env, "get-message-params", 0, scmapi_get_message_params);
	sexp_define_foreign (
	  ctx, env, "get-message-tags", 0, scmapi_get_message_tags);
}
