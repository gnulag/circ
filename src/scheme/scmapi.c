#include "irc/irc.h"
#include "scheme.h"
#include <string.h>

static module*
get_module (sexp ctx)
{
	sexp idobj =
	  sexp_eval_string (ctx, "circ-module-id", -1, sexp_context_env (ctx));
	int id = sexp_unbox_fixnum (idobj);
	return scheme_get_module_from_id (id);
}

sexp
scmapi_register_hook (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	module* mod = get_module (ctx);
	const char* cmd_c = sexp_string_data (cmd);
	scheme_add_irc_hook (cmd_c, func, mod);
	return SEXP_NULL;
}

sexp
scmapi_register_command (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	module* mod = get_module (ctx);
	const char* cmd_c = sexp_string_data (cmd);
	scheme_add_command_hook (cmd_c, func, mod);
	return SEXP_NULL;
}

sexp
scmapi_reply (sexp ctx, sexp self, sexp text)
{
	module* mod = get_module (ctx);

	if (mod == NULL)
		return SEXP_NULL;

	if (!sexp_stringp(text)) {
		printf("scmapi_reply: %s: The argument is not a string\n", mod->path);
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

void
scmapi_define_foreign_functions (sexp ctx)
{
	sexp env = sexp_context_env (ctx);
	sexp_define_foreign (ctx, env, "register-hook", 2, scmapi_register_hook);
	sexp_define_foreign (
	  ctx, env, "register-command", 2, scmapi_register_command);
	sexp_define_foreign (ctx, env, "reply", 1, scmapi_reply);
}
