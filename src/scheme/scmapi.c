#include <string.h>

#include "../config/config.h"
#include "scheme.h"

static scm_module *
get_module (sexp ctx)
{
	sexp id_sym = sexp_intern (ctx, "circ-module-id", -1);
	sexp id_obj = sexp_eval (ctx, id_sym, sexp_context_env (ctx));
	int id = sexp_unbox_fixnum (id_obj);
	return scm_get_module_from_id (id);
}

sexp
scmapi_register_hook (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_FALSE;

	const char *cmd_c = sexp_string_data (cmd);
	scm_add_irc_hook (cmd_c, func, mod);
	return SEXP_TRUE;
}

sexp
scmapi_register_command (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_FALSE;

	const char *cmd_c = sexp_string_data (cmd);
	scm_add_command_hook (cmd_c, func, mod);
	return SEXP_TRUE;
}

sexp
scmapi_register_match (sexp ctx, sexp self, sexp n, sexp regex, sexp func)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_FALSE;

	const char *regex_c = sexp_string_data (regex);
	scm_add_regex_hook (regex_c, func, mod);
	return SEXP_TRUE;
}

sexp
scmapi_send_raw (sexp ctx, sexp self, sexp n, sexp raw)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	if (!sexp_stringp (raw)) {
		printf ("scmapi_send_raw: %s: The argument is not a string\n",
			mod->path);
		return SEXP_NULL;
	}

	const irc_server *s = mod->mod_ctx.serv;
	const char *raw_c = sexp_string_data (raw);
	irc_push_string (s, raw_c);

	return SEXP_NULL;
}

sexp
scmapi_get_bot_nick (sexp ctx, sexp self, sexp n)
{
	config_t *config = get_config ();
	return sexp_c_string (ctx, config->server->user->nickname, -1);
}

sexp
scmapi_get_cmd_prefix (sexp ctx, sexp self, sexp n)
{
	config_t *config = get_config ();
	return sexp_c_string (ctx, config->cmd_prefix, -1);
}

sexp
scmapi_get_db_path (sexp ctx, sexp self, sexp n)
{
	config_t *config = get_config ();
	return sexp_c_string (ctx, config->db_path, -1);
}

sexp
scmapi_get_server_name (sexp ctx, sexp self, sexp n)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const irc_server *s = mod->mod_ctx.serv;
	return sexp_c_string (ctx, s->name, -1);
}

sexp
scmapi_get_message_source (sexp ctx, sexp self, sexp n)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const irc_msg *msg = mod->mod_ctx.msg;
	return sexp_c_string (ctx, msg->prefix, -1);
}

sexp
scmapi_get_message_command (sexp ctx, sexp self, sexp n)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const irc_msg *msg = mod->mod_ctx.msg;
	return sexp_c_string (ctx, msg->command, -1);
}

/* sexp */
/* scmapi_get_message_tags (sexp ctx, sexp self, sexp n) */
/* { */
/* scm_module *mod = get_module (ctx); */
/* if (mod == NULL) */
/* return SEXP_NULL; */

/* const irc_msg *msg = mod->mod_ctx.msg; */
/* return array_to_scheme_list (ctx, *msg->tags, 0); */
/* } */

static sexp
msg_params_to_scheme_list (sexp ctx, struct irc_msg_params *arr, int index)
{
	if (arr->len == index)
		return SEXP_NULL;
	else
		return sexp_cons (ctx,
				  sexp_c_string (ctx, arr->params[index], -1),
				  msg_params_to_scheme_list (ctx, arr, index + 1));
}

sexp
scmapi_get_message_params (sexp ctx, sexp self, sexp n)
{
	scm_module *mod = get_module (ctx);
	if (mod == NULL)
		return SEXP_NULL;

	const irc_msg *msg = mod->mod_ctx.msg;
	return msg_params_to_scheme_list (ctx, msg->params, 0);
}

void
scmapi_define_foreign_functions (sexp ctx)
{
	sexp env = sexp_context_env (ctx);

	/* Entry registrations */
	sexp_define_foreign (ctx, env, "register-hook", 2, scmapi_register_hook);
	sexp_define_foreign (ctx, env, "register-command", 2, scmapi_register_command);
	sexp_define_foreign (ctx, env, "register-match", 2, scmapi_register_match);

	/* Server interactions */
	sexp_define_foreign (ctx, env, "send-raw", 1, scmapi_send_raw);

	/* IRC config information */
	sexp_define_foreign (ctx, env, "get-bot-nick", 0, scmapi_get_bot_nick);
	sexp_define_foreign (ctx, env, "get-cmd-prefix", 0, scmapi_get_cmd_prefix);
	sexp_define_foreign (ctx, env, "get-db-path", 0, scmapi_get_db_path);

	/* Server information */
	sexp_define_foreign (
	  ctx, env, "get-server-name", 0, scmapi_get_server_name);

	/* message information */
	sexp_define_foreign (ctx, env, "get-message-source", 0, scmapi_get_message_source);
	sexp_define_foreign (ctx, env, "get-message-command", 0, scmapi_get_message_command);
	sexp_define_foreign (ctx, env, "get-message-params", 0, scmapi_get_message_params);
	/* sexp_define_foreign (ctx, env, "get-message-tags", 0, scmapi_get_message_tags); */
}
