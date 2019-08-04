#include <fts.h>
#include <glib.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../irc/hooks.h"
#include "scheme.h"

#define MAX_COMMAND_SIZE 4096

typedef struct mod_entry
{
	module* mod;
	sexp func;
	struct mod_entry* next;
} mod_entry;

static unsigned int mod_ids = 0;
static module* module_list;

/*
 * Executes hooks to chat commands
 * The command name without prefix is used as key
 * A linked list of modules entries is used as value
 * chat command -> mod_entry*
 */
static GHashTable* command_hooks;
/*
 * Executes hooks to IRC commands
 * The IRC command is used as key
 * A linked list of module entries is used as value
 * IRC command -> mod_entry*
 */
static GHashTable* irc_hooks;

static char* cmd_prefix;

static void
scheme_exec_irc_hooks (const irc_server *s, const IrciumMessage *msg);
static void
scheme_exec_command_hooks (const irc_server *s, const IrciumMessage *msg);
static void
scheme_run_module_entry (mod_entry* me,
                         const irc_server* s,
                         const IrciumMessage* msg);
static void
to_scheme (const irc_server* s, const IrciumMessage* msg);
static void
scheme_load_modules (char* dir);
static module*
scheme_create_module (char* path);
static void
scheme_register_module (module* mod);

static void
scheme_entry (const irc_server* s, const IrciumMessage* msg)
{
	scheme_exec_irc_hooks(s, msg);
	scheme_exec_command_hooks(s, msg);
}

void
scheme_add_irc_hook (const char *command, sexp func, module *mod)
{
	mod_entry *me = malloc(sizeof(mod_entry));
	me->mod = mod;
	me->func = func;
	me->next = NULL;
	g_hash_table_insert(irc_hooks, strdup(command), me);
}

static void
scheme_exec_irc_hooks (const irc_server *s, const IrciumMessage *msg)
{
	const char* irc_cmd = ircium_message_get_command (msg);
	mod_entry* me = g_hash_table_lookup (irc_hooks, irc_cmd);
	if (me == NULL)
		return;

	do {
		scheme_run_module_entry (me, s, msg);
	} while ((me = me->next));
}

void
scheme_add_command_hook (const char *command, sexp func, module *mod)
{
	mod_entry *me = malloc(sizeof(mod_entry));
	me->mod = mod;
	me->func = func;
	me->next = NULL;
	g_hash_table_insert(command_hooks, strdup(command), me);
}

static void
scheme_exec_command_hooks (const irc_server *s, const IrciumMessage *msg)
{
	const char* irc_cmd = ircium_message_get_command (msg);
	if (strcmp(irc_cmd, "PRIVMSG") != 0)
		return;

	const GPtrArray *params = ircium_message_get_params (msg);
	const char* text = params->pdata[1];
	size_t text_len = strlen(text);
	size_t cmd_prefix_len = strlen(cmd_prefix);

	if (text_len < cmd_prefix_len ||
			strncmp(text, cmd_prefix, cmd_prefix_len) != 0)
		return;

	int i = cmd_prefix_len, j = 0;
	char cmd[MAX_COMMAND_SIZE];
	while (text[i] != ' ' && text[i] != '\0')
		cmd[j++] = text[i++];
	cmd[j] = '\0';

	mod_entry* me = g_hash_table_lookup (command_hooks, cmd);
	if (me == NULL)
		return;

	do {
		scheme_run_module_entry (me, s, msg);
	} while ((me = me->next));
}

static void
scheme_run_module_entry (mod_entry* me,
                         const irc_server* s,
                         const IrciumMessage* msg)
{
	puts("in scheme_run_module_entry");
	pthread_mutex_lock (&me->mod->mtx);
	me->mod->mod_ctx.serv = s;
	me->mod->mod_ctx.msg = msg;
	sexp_apply(me->mod->scheme_ctx, me->func, SEXP_NULL);
	pthread_mutex_unlock (&me->mod->mtx);
}

void
scheme_init ()
{
	char* mods_dir = getenv ("SCHEME_MOD_DIR");
	if (mods_dir == NULL)
		mods_dir = "scheme_mods/";

	cmd_prefix = getenv("CIRC_CMD_PREFIX");
	if (cmd_prefix == NULL)
		cmd_prefix = ".";

	if (irc_hooks == NULL)
		irc_hooks = g_hash_table_new (g_str_hash, g_str_equal);
	if (command_hooks == NULL)
		command_hooks = g_hash_table_new (g_str_hash, g_str_equal);

	scheme_load_modules (mods_dir);
	add_hook ("*", scheme_entry);
}

static void
scheme_load_modules (char* dir)
{
	char* paths[] = { dir, NULL };
	FTS* f = fts_open (paths, FTS_LOGICAL | FTS_NOSTAT, NULL);
	FTSENT *fe;
	
	while ((fe = fts_read (f)))
		if (strlen (fe->fts_name) > 4 &&
		    (strncmp (fe->fts_name + (fe->fts_namelen - 4), ".scm", 4) == 0 ||
		     strncmp (fe->fts_name + (fe->fts_namelen - 2), ".ss", 2) == 0))
				scheme_create_module (fe->fts_path);
}

static module*
scheme_create_module (char* path)
{
	module* mod = malloc (sizeof (module));
	mod->id = ++mod_ids;
	mod->path = strdup (path);
	pthread_mutex_init (&mod->mtx, NULL);

	scheme_register_module (mod);

	mod->scheme_ctx = sexp_make_eval_context (NULL, NULL, NULL, 0, 0);
	sexp ctx = mod->scheme_ctx;
	sexp_load_standard_env (ctx, NULL, SEXP_SEVEN);
	sexp_load_standard_ports (ctx, NULL, stdin, stdout, stderr, 1);

	sexp id = sexp_make_integer(ctx, mod->id);
	sexp id_sym = sexp_intern(ctx, "circ-module-id", -1);
	sexp_env_define(ctx, sexp_context_env (ctx), id_sym, id);

	scmapi_define_foreign_functions (ctx);

	sexp_gc_var1 (obj);
	sexp_gc_preserve1 (ctx, obj);
	obj = sexp_c_string (ctx, path, -1);
	sexp_load (ctx, obj, NULL);

	return mod;
}

static void
scheme_register_module (module* mod)
{
	module* modlist = module_list;
	if (modlist == NULL) {
		module_list = mod;
		return;
	}

	while (modlist->next != NULL)
		modlist = modlist->next;
	modlist->next = mod;
}

module*
scheme_get_module_from_id (int id)
{
	module* mod = module_list;
	if (mod == NULL)
		return NULL;

	do {
		if (mod->id == id)
			return mod;
	} while ((mod = mod->next));

	return NULL;
}
