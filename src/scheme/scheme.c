#include <fts.h>
#include <glib.h>
#include <pthread.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>

#include "config/config.h"
#include "irc/hooks.h"
#include "log/log.h"
#include "scheme.h"

#define MAX_COMMAND_SIZE 4096

typedef struct mod_entry
{
	scm_module *mod;
	sexp func;
	struct mod_entry *next;
} mod_entry;

typedef struct regex_hook
{
	scm_module *mod;
	sexp func;
	regex_t *regex;
	struct regex_hook *next;
} regex_hook;

static unsigned int mod_ids = 0;
static scm_module *module_list;

/*
 * Executes hooks to chat commands
 * The command name without prefix is used as key
 * A linked list of modules entries is used as value
 * chat command -> mod_entry*
 */
static GHashTable *command_hooks;
/*
 * Executes hooks to IRC commands
 * The IRC command is used as key
 * A linked list of module entries is used as value
 * IRC command -> mod_entry*
 */
static GHashTable *irc_hooks;
/*
 * A linked list of regex hooks
 * All of the element of this list are tried
 * against every PRIVMSG that comes in
 */
static regex_hook *regex_hooks;

static void
scm_exec_irc_hooks (const irc_server *s, const irc_msg *msg);
static void
scm_exec_command_hooks (const irc_server *s, const irc_msg *msg);
static void
scm_exec_regex_hooks (const irc_server *s, const irc_msg *msg);
static void
scm_run_module (scm_module *mod,
		sexp func,
		const irc_server *s,
		const irc_msg *msg);
static void
scm_load_modules (char *dir);
static scm_module *
scm_create_module (char *path);
static void
scm_register_module (scm_module *mod);

static void
scm_entry (const irc_server *s, const irc_msg *msg)
{
	scm_exec_irc_hooks (s, msg);
	if (strcmp (msg->command, "PRIVMSG") == 0) {
		scm_exec_command_hooks (s, msg);
		scm_exec_regex_hooks (s, msg);
	}
}

void
scm_add_irc_hook (const char *command, sexp func, scm_module *mod)
{
	mod_entry *me = malloc (sizeof (mod_entry));
	me->mod = mod;
	me->func = func;
	me->next = NULL;
	g_hash_table_insert (irc_hooks, strdup (command), me);
}

static void
scm_exec_irc_hooks (const irc_server *s, const irc_msg *msg)
{
	mod_entry *me = g_hash_table_lookup (irc_hooks, msg->command);
	if (me == NULL)
		return;

	do {
		scm_run_module (me->mod, me->func, s, msg);
	} while ((me = me->next));
}

void
scm_add_command_hook (const char *command, sexp func, scm_module *mod)
{
	mod_entry *me = malloc (sizeof (mod_entry));
	me->mod = mod;
	me->func = func;
	me->next = NULL;
	g_hash_table_insert (command_hooks, strdup (command), me);
}

static void
scm_exec_command_hooks (const irc_server *s, const irc_msg *msg)
{
	config_t *config = get_config ();

	if (strcmp (msg->command, "PRIVMSG") != 0)
		return;

	const char *text = msg->params->params[1];
	size_t text_len = strlen (text);
	size_t cmd_prefix_len = strlen (config->cmd_prefix);

	if (text_len < cmd_prefix_len ||
	    strncmp (text, config->cmd_prefix, cmd_prefix_len) != 0)
		return;

	int i = cmd_prefix_len, j = 0;
	char cmd[MAX_COMMAND_SIZE];
	while (text[i] != ' ' && text[i] != '\0')
		cmd[j++] = text[i++];
	cmd[j] = '\0';

	mod_entry *me = g_hash_table_lookup (command_hooks, cmd);
	if (me == NULL)
		return;

	do {
		scm_run_module (me->mod, me->func, s, msg);
	} while ((me = me->next));
}

void
scm_add_regex_hook (const char *rx_str, sexp func, scm_module *mod)
{
	regex_t *rx = malloc (sizeof (regex_t));
	char errbuf[4096];
	int ret = regcomp (rx, rx_str, REG_NOSUB | REG_EXTENDED);
	if (ret) {
		regerror (ret, rx, errbuf, sizeof (errbuf));
		log_info (errbuf);
		return;
	}

	regex_hook *rx_hook = malloc (sizeof (regex_hook));
	rx_hook->mod = mod;
	rx_hook->func = func;
	rx_hook->regex = rx;
	rx_hook->next = NULL;

	regex_hook *hooks;
	if (regex_hooks == NULL)
		regex_hooks = rx_hook;
	else {
		for (hooks = regex_hooks; hooks->next != NULL; hooks = hooks->next)
			;
		hooks->next = rx_hook;
	}
}

static void
scm_exec_regex_hooks (const irc_server *s, const IrciumMessage *msg)
{
	regex_hook *hooks;
	const GPtrArray *params = ircium_message_get_params (msg);
	char *text = params->pdata[1];
	for (hooks = regex_hooks; hooks != NULL; hooks = hooks->next)
		if (regexec (hooks->regex, text, 0, NULL, 0) == 0)
			scm_run_module (hooks->mod, hooks->func, s, msg);
}

static void
scm_run_module (scm_module *mod,
		sexp func,
		const irc_server *s,
		const irc_msg *msg)
{
	pthread_mutex_lock (&mod->mtx);
	sexp ctx = mod->scm_ctx;
	mod->mod_ctx.serv = s;
	mod->mod_ctx.msg = msg;

	sexp id_obj = sexp_make_integer (ctx, mod->id);
	sexp id_sym = sexp_intern (ctx, "circ-module-id", -1);
	sexp_env_define (ctx, sexp_context_env (ctx), id_sym, id_obj);

	sexp res = sexp_apply (ctx, func, SEXP_NULL);
	if (sexp_exceptionp (res))
		sexp_print_exception (ctx, res, sexp_current_error_port (ctx));

	pthread_mutex_unlock (&mod->mtx);
}

void
scm_init ()
{
	config_t *config = get_config ();

	if (irc_hooks == NULL)
		irc_hooks = g_hash_table_new (g_str_hash, g_str_equal);
	if (command_hooks == NULL)
		command_hooks = g_hash_table_new (g_str_hash, g_str_equal);

	scm_load_modules (config->scheme_mod_dir);
	add_hook ("*", scm_entry);
}

static void
scm_load_modules (char *dir)
{
	char *paths[] = { dir, NULL };
	FTS *f = fts_open (paths, FTS_LOGICAL | FTS_NOSTAT, NULL);
	FTSENT *fe;

	log_info ("-----\nLoading Modules:\n");
	while ((fe = fts_read (f)))
		if (strlen (fe->fts_name) > 4 &&
		    (strncmp (fe->fts_name + (fe->fts_namelen - 4), ".scm", 4) == 0 ||
		     strncmp (fe->fts_name + (fe->fts_namelen - 2), ".ss", 2) == 0)) {
			log_info ("%s\n", fe->fts_name);
			scm_create_module (fe->fts_path);
		}
	log_info ("-----\n");

	fts_close (f);
}

static scm_module *
scm_create_module (char *path)
{
	scm_module *mod = malloc (sizeof (scm_module));
	mod->id = ++mod_ids;
	mod->path = strdup (path);
	mod->next = NULL;
	pthread_mutex_init (&mod->mtx, NULL);

	pthread_mutex_lock (&mod->mtx);

	scm_register_module (mod);

	mod->scm_ctx = sexp_make_eval_context (NULL, NULL, NULL, 0, 0);
	sexp ctx = mod->scm_ctx;
	sexp_load_standard_env (ctx, NULL, SEXP_SEVEN);
	sexp_load_standard_ports (ctx, NULL, stdin, stdout, stderr, 1);

	sexp id_obj = sexp_make_integer (ctx, mod->id);
	sexp id_sym = sexp_intern (ctx, "circ-module-id", -1);
	sexp_env_define (ctx, sexp_context_env (ctx), id_sym, id_obj);

	scmapi_define_foreign_functions (ctx);

	sexp obj = sexp_c_string (ctx, path, -1);
	sexp res = sexp_load (ctx, obj, NULL);
	if (sexp_exceptionp (res))
		sexp_print_exception (ctx, res, sexp_current_error_port (ctx));

	pthread_mutex_unlock (&mod->mtx);

	return mod;
}

static void
scm_register_module (scm_module *mod)
{
	if (mod == NULL)
		return;

	scm_module *modlist = module_list;
	if (modlist == NULL) {
		module_list = mod;
		return;
	}

	while (modlist->next != NULL)
		modlist = modlist->next;
	modlist->next = mod;
}

scm_module *
scm_get_module_from_id (int id)
{
	scm_module *mod = module_list;
	if (mod == NULL)
		return NULL;

	do {
		if (mod->id == id)
			return mod;
	} while ((mod = mod->next));

	return NULL;
}
