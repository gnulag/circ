#ifndef SCHEME_H
#define SCHEME_H

#include "ircium-parser/ircium-message.h"
#include <chibi/eval.h>

typedef struct mod_context
{
	const irc_server *serv;
	const IrciumMessage *msg;
} mod_context;

typedef struct scm_module
{
	int id;
	char *path;
	sexp scm_ctx;
	mod_context mod_ctx;
	pthread_mutex_t mtx;
	struct scm_module *next;
} scm_module;

void
scm_init (void);
scm_module *
scm_get_module_from_id (int id);
void
scm_add_irc_hook (const char *command, sexp func, scm_module *mod);
void
scm_add_command_hook (const char *command, sexp func, scm_module *mod);
void
scmapi_define_foreign_functions (sexp ctx);

#endif
