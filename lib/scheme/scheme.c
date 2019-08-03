#include <fts.h>
#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../irc/hooks.h"
#include "scheme.h"

#define MAX_COMMAND_SIZE 4096

typedef struct module
{
	char* path;
	sexp scheme_ctx;
	pthread_mutex_t mtx;
	struct module* next;
} module;

typedef struct mod_entry
{
	module* mod;
	sexp func;
	struct mod_entry* next;
} mod_entry;

static module* module_list;

/*
 * Executes hooks to IRC commands
 * The IRC command is used as key
 * A linked list of modules is used as value
 * IRC command -> mod_entry*
 */
static GHashTable* irc_hooks;

sexp
garr_to_scheme_list (sexp ctx, const GPtrArray* arr, int index);
static void
scheme_run_module_entry (const mod_entry* me,
                         const irc_server* s,
                         const IrciumMessage* msg);
static void
to_scheme (const irc_server* s, const IrciumMessage* msg);
static void
scheme_load_modules (char* dir);
static module*
scheme_create_module (char* path);

static void
scheme_entry (const irc_server* s, const IrciumMessage* msg)
{
	const char* cmd = ircium_message_get_command (msg);
	const mod_entry* me = g_hash_table_lookup (irc_hooks, cmd);
	if (me == NULL)
		return;
	while ((me = me->next)) {
		scheme_run_module_entry (me, s, msg);
	}
}

sexp
garr_to_scheme_list (sexp ctx, const GPtrArray* arr, int index)
{
	sexp_gc_var1 (list);
	if (index < arr->len)
		list = sexp_cons (ctx,
		                  g_ptr_array_index (arr, index),
		                  garr_to_scheme_list (ctx, arr, index + 1));
	else
		return SEXP_NULL;
}

static void
scheme_run_module_entry (const mod_entry* me,
                         const irc_server* s,
                         const IrciumMessage* msg)
{
	int i;
	sexp servname, source, command, params, tags;
	sexp servname_sym, source_sym, command_sym, params_sym, tags_sym;

	sexp ctx = me->mod->scheme_ctx;

	/* Put important information in the module's namespace */
	servname = sexp_c_string (ctx, irc_get_server_name (s), -1);
	servname_sym = sexp_intern (ctx, "server-name", -1);
	source = sexp_c_string (ctx, ircium_message_get_source (msg), -1);
	source_sym = sexp_intern (ctx, "message-source", -1);
	command = sexp_c_string (ctx, ircium_message_get_command (msg), -1);
	command_sym = sexp_intern (ctx, "message-command", -1);
	params = garr_to_scheme_list (ctx, ircium_message_get_params (msg), 0);
	params_sym = sexp_intern (ctx, "message-params", -1);
	tags = garr_to_scheme_list (ctx, ircium_message_get_tags (msg), 0);
	tags_sym = sexp_intern (ctx, "message-tags", -1);

	pthread_mutex_lock (&me->mod->mtx);

#define DEF_SCHEME_VAR(sym, val)                                               \
	sexp_env_define (ctx, sexp_context_env (ctx), sym, val);
	DEF_SCHEME_VAR (servname_sym, servname);
	DEF_SCHEME_VAR (source_sym, source);
	DEF_SCHEME_VAR (command_sym, command);
	DEF_SCHEME_VAR (params_sym, params);
	DEF_SCHEME_VAR (tags_sym, tags);

	sexp_eval (ctx, me->func, sexp_context_env (ctx));

	pthread_mutex_unlock (&me->mod->mtx);
}

void
scheme_init ()
{
	char* mods_dir = getenv ("SCHEME_MOD_DIR");
	if (mods_dir == NULL)
		mods_dir = "scheme_mods/";

	if (irc_hooks == NULL)
		irc_hooks = g_hash_table_new (g_str_hash, g_str_equal);

	scheme_load_modules (mods_dir);
	add_hook ("*", scheme_entry);
}

static void
scheme_load_modules (char* dir)
{
	char* paths[] = { dir, NULL };
	FTS* f = fts_open (paths, FTS_LOGICAL | FTS_NOSTAT, NULL);
	FTSENT* fe;

	module* mod = module_list;

	while ((fe = fts_read (f))) {
		if (strlen (fe->fts_name) > 4 &&
		    (strncmp (fe->fts_name + (fe->fts_namelen - 4), ".scm", 4) == 0 ||
		     strncmp (fe->fts_name + (fe->fts_namelen - 2), ".ss", 2) == 0)) {
			mod->next = scheme_create_module (fe->fts_path);
			mod = mod->next;
		}
	}
}

static module*
scheme_create_module (char* path)
{
	module* mod = malloc (sizeof (module));
	mod->path = strdup (path);
	pthread_mutex_init (&mod->mtx, NULL);

	mod->scheme_ctx = sexp_make_eval_context (NULL, NULL, NULL, 0, 0);
	sexp* ctx = &mod->scheme_ctx;
	sexp_load_standard_env (*ctx, NULL, SEXP_SEVEN);
	sexp_load_standard_ports (*ctx, NULL, stdin, stdout, stderr, 1);

	sexp_gc_var1 (obj);
	sexp_gc_preserve1 (*ctx, obj);
	obj = sexp_c_string (*ctx, "lib/scheme/circ.ss", -1);
	sexp_load (*ctx, obj, NULL);
	obj = sexp_c_string (*ctx, path, -1);
	sexp_load (*ctx, obj, NULL);

	return mod;
}
