#ifndef SCHEME_H
#define SCHEME_H

#include "ircium-parser/ircium-message.h"
#include <chibi/eval.h>

typedef struct mod_context
{
	const irc_server* serv;
	const IrciumMessage* msg;
} mod_context;

typedef struct module
{
	int id;
	char* path;
	sexp scheme_ctx;
	mod_context mod_ctx;
	pthread_mutex_t mtx;
	struct module* next;
} module;

void
scheme_init (void);
module*
scheme_get_module_from_id (int id);
void
scheme_add_irc_hook (const char* command, sexp func, module* mod);
void
scheme_add_command_hook (const char* command, sexp func, module* mod);
void
scmapi_define_foreign_functions (sexp ctx);

#endif
