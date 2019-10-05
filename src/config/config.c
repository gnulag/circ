#include "config.h"
#include "cJSON/cJSON.h"
#include "irc/irc.h"
#include "log/log.h"
#include "utlist/list.h"
#include <err.h> // err for panics
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool
cjson_parse_bool (const cJSON *json, char *field, bool defaultv)
{
	cJSON *value = cJSON_GetObjectItemCaseSensitive (json, field);
	if (defaultv) {
		if (cJSON_IsFalse (value))
			return false;
		return true;
	} else {
		if (cJSON_IsTrue (value))
			return true;
		return false;
	}
}

static char *
cjson_parse_string (const cJSON *json, char *field, char *defaultv)
{
	cJSON *value = cJSON_GetObjectItemCaseSensitive (json, field);
	if (cJSON_IsString (value) && value->valuestring != NULL) {
		strdup (value->valuestring);
	} else {
		strdup (defaultv);
	}
}

static struct config_t *config;

struct config_t *
get_config ()
{
	return config;
}

void
free_config ()
{
	free (config->server->name);
	free (config->server->host);
	free (config->server->port);

	free (config->server->user->nickname);
	free (config->server->user->ident);
	free (config->server->user->realname);
	free (config->server->user->sasl_user);
	free (config->server->user->sasl_pass);
	free (config->server->user);

	/* now delete each element, use the safe iterator */
	struct irc_channel *l, *tmp;
	LL_FOREACH_SAFE (config->server->channels, l, tmp) {
		LL_DELETE (config->server->channels, l);
		free (l);
	}
}

int
parse_config (const char *config_file_path)
{
	/*
	 * Load the config file form config_file_path
	 */
	FILE *infile;
	char *raw_config_file;
	long numbytes;

	infile = fopen (config_file_path, "r");

	if (infile == NULL)
		err (1, "Config File not Found");

	fseek (infile, 0L, SEEK_END);
	numbytes = ftell (infile);

	fseek (infile, 0L, SEEK_SET);

	raw_config_file = (char *)calloc (numbytes, sizeof (char));

	if (raw_config_file == NULL)
		err (1, "Config File Read Failed");

	fread (raw_config_file, sizeof (char), numbytes, infile);
	fclose (infile);

	cJSON *json = cJSON_Parse (raw_config_file);
	if (json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr ();
		if (error_ptr != NULL) {
			fprintf (stderr, "Error before: %s\n", error_ptr);
			exit (EXIT_FAILURE);
		}
	}

	/*
	 * Parse the Config File
	 */

	config = malloc (sizeof (config_t));

	config->debug = cjson_parse_bool (json, "debug", false);

	config->cmd_prefix = cjson_parse_string (json, "cmd_prefix", "%");
	config->db_path = cjson_parse_string (json, "db_path", "db.sqlite3");
	config->scheme_mod_dir = cjson_parse_string (json, "scheme_mod_dir", "scheme_mods/");

	/* Parse Servers section */
	cJSON *server = cJSON_GetObjectItemCaseSensitive (json, "server");
	if (cJSON_IsObject (server)) {
		config->server = malloc (sizeof (irc_server));

		config->server->name = cjson_parse_string (server, "name", "snoonet");
		config->server->host = cjson_parse_string (server, "host", "irc.snoonet.org");
		config->server->port = cjson_parse_string (server, "port", "6667");
		config->server->secure = cjson_parse_bool (server, "secure", false);

		/* Add user data to server */
		cJSON *user = cJSON_GetObjectItemCaseSensitive (server, "user");
		if (cJSON_IsObject (user)) {
			config->server->user = malloc (sizeof (irc_user));

			config->server->user->nickname = cjson_parse_string (user, "nickname", "circ");
			config->server->user->ident = cjson_parse_string (user, "ident", "circ");
			config->server->user->realname = cjson_parse_string (user, "realname", "circ");
			config->server->user->sasl_enabled = cjson_parse_bool (user, "sasl_enabled", false);
			config->server->user->sasl_user = cjson_parse_string (user, "sasl_user", "circ");
			config->server->user->sasl_pass = cjson_parse_string (user, "sasl_pass", "circ");
		}

		/* Iter channels and add to the server */
		cJSON *channel = NULL;
		config->server->channels = NULL;
		cJSON *channels = cJSON_GetObjectItemCaseSensitive (server, "channels");
		cJSON_ArrayForEach (channel, channels)
		{
			if (cJSON_IsString (channel)) {
				struct irc_channel *item;
				item = (irc_channel *)malloc (sizeof *item);
				strncpy (
				  item->channel, channel->valuestring, sizeof (item->channel));
				LL_APPEND (config->server->channels, item);
			} else
				err (1, "config: channel is not a string");
		}
	} else {
		err (1, "config: server is not an object");
	}

	/* Parse Modules section */
	int iter = 0;
	cJSON *module = NULL;
	cJSON *modules = cJSON_GetObjectItemCaseSensitive (json, "modules");
	int modules_size = cJSON_GetArraySize (modules);
	config->modules = malloc (modules_size * sizeof (char *));
	cJSON_ArrayForEach (module, modules)
	{
		if (cJSON_IsObject (module)) {
			struct module_t *tmp_module = malloc (sizeof (module_t));

			cJSON *name = cJSON_GetObjectItemCaseSensitive (module, "name");
			if (cJSON_IsString (name) && (name->valuestring != NULL)) {
				tmp_module->name = strdup (name->valuestring);
			} else
				err (1, "config: module: name is not a string");

			int matcher_iter = 0;
			cJSON *matcher = NULL;
			cJSON *matchers =
			  cJSON_GetObjectItemCaseSensitive (module, "matchers");
			int matchers_size = cJSON_GetArraySize (matchers);
			tmp_module->matchers = malloc (modules_size * sizeof (module_t *));
			cJSON_ArrayForEach (matcher, matchers)
			{
				if (cJSON_IsString (matcher) && matcher->valuestring != NULL) {
					tmp_module->matchers[matcher_iter] =
					  strdup (matcher->valuestring);
					matcher_iter++;
				} else
					err (1, "config: module: matchers is not a string");
			}

			cJSON *cmd = cJSON_GetObjectItemCaseSensitive (module, "cmd");
			if (cJSON_IsString (cmd) && (cmd->valuestring != NULL)) {
				tmp_module->cmd = strdup (cmd->valuestring);
			} else
				err (1, "config: module: cmd is not a string");

			cJSON *cwd = cJSON_GetObjectItemCaseSensitive (module, "cwd");
			if (cJSON_IsString (cwd) && (cwd->valuestring != NULL)) {
				tmp_module->cwd = strdup (cwd->valuestring);
			} else
				err (1, "config: module: cwd is not a string");

			config->modules[iter] = malloc (sizeof (*tmp_module));
			memcpy (config->modules[iter], tmp_module, sizeof (*tmp_module));
			free (tmp_module);
			iter++;
		}
	}

	free (raw_config_file);
	cJSON_Delete (json);
	return 0;
}
