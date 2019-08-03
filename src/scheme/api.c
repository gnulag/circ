#include <string.h>
#include "irc/irc.h"
#include "scheme.h"

static module*
get_module (sexp ctx)
{
	sexp idobj = sexp_eval_string(ctx, "circ-module-id", -1, sexp_context_env (ctx));
	int id = sexp_unbox_fixnum(idobj);
	return get_module_from_id (id);
}

sexp
sexp_register_hook (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	module* mod = get_module (ctx);
	const char* cmd_c = sexp_string_data (cmd);
	add_irc_hook (cmd_c, func, mod);
	return SEXP_NULL;
}

sexp
sexp_register_command (sexp ctx, sexp self, sexp n, sexp cmd, sexp func)
{
	module* mod = get_module (ctx);
	const char* cmd_c = sexp_string_data (cmd);
	add_command_hook (cmd_c, func, mod);
	return SEXP_NULL;
}

sexp
sexp_send_message (sexp ctx, sexp self, sexp servname, sexp target, sexp text)
{
	const char* servname_c = sexp_string_data (servname);
	const char* target_c = sexp_string_data (target);
	const char* text_c = sexp_string_data (text);
	const irc_server *s = irc_get_server_from_name (servname_c);
	char buf[4096];
	snprintf (buf, 4096, "PRIVMSG %s :%s", target_c, text_c);
	irc_write_bytes (s, buf, strlen(buf));
}

void
define_foreign_functions (sexp ctx)
{
	sexp env = sexp_context_env (ctx);
	sexp_define_foreign (ctx, env, "register-hook", 2, sexp_register_hook);
	sexp_define_foreign (ctx, env, "register-command", 2, sexp_register_command);
	sexp_define_foreign (ctx, env, "send-message", 3, sexp_send_message);
}
