#include "config.h"
#include "log/log.h"
#include "cJSON/cJSON.h"
#include <err.h> // err for panics
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
parse_config (const char* config_file_path, struct ConfigType* config)
{
	/*
     * Load the config file form config_file_path 
     */
    FILE* infile;
	char* raw_config_file;
	long numbytes;

	infile = fopen (config_file_path, "r");

	if (infile == NULL)
		err (1, "Config File not Found");

	fseek (infile, 0L, SEEK_END);
	numbytes = ftell (infile);

	fseek (infile, 0L, SEEK_SET);

	raw_config_file = (char*)calloc (numbytes, sizeof (char));

	if (raw_config_file == NULL)
		err (1, "Config File Read Failed");

	fread (raw_config_file, sizeof (char), numbytes, infile);
	fclose (infile);

	cJSON* json = cJSON_Parse (raw_config_file);
	if (json == NULL) {
		const char* error_ptr = cJSON_GetErrorPtr ();
		if (error_ptr != NULL) {
			fprintf (stderr, "Error before: %s\n", error_ptr);
			exit (EXIT_FAILURE);
		}
	}

    /*
     * Parse the Config File
     */

    /* Parse Servers section */
	cJSON* server = cJSON_GetObjectItemCaseSensitive (json, "server");
    if (cJSON_IsObject (server)) {
        config->server = malloc (sizeof(ServerType));

    	cJSON* name = cJSON_GetObjectItemCaseSensitive (server, "name");
    	if (cJSON_IsString (name) && (name->valuestring != NULL)) {
    		config->server->name =
    		  malloc (sizeof (name->valuestring));
    		strcpy (config->server->name, name->valuestring);
    	} else
    		err (1, "config: name is not a string");
    
    	cJSON* host = cJSON_GetObjectItemCaseSensitive (server, "host");
    	if (cJSON_IsString (host) && (host->valuestring != NULL)) {
    		config->server->host =
    		  malloc (sizeof (host->valuestring));
    		strcpy (config->server->host, host->valuestring);
    	} else
    		err (1, "config: host is not a string");
    
    	cJSON* port = cJSON_GetObjectItemCaseSensitive (server, "port");
    	if (cJSON_IsString (port) && (port->valuestring != NULL)) {
    		config->server->port =
    		  malloc (sizeof (port->valuestring));
    		strcpy (config->server->port, port->valuestring);
    	} else
    		err (1, "config: port is not a string");
    
    	cJSON* secure = cJSON_GetObjectItemCaseSensitive (server, "secure");
    	if (cJSON_IsTrue (secure))
    		config->server->secure = true;
    	else if (cJSON_IsFalse (secure))
    		config->server->secure = false;
    	else
    		err (1, "config: secure is not a bool");
    
        /* Iter channels and add to the server */
    	int channel_iter = 0;
    	cJSON* channel = NULL;
    	cJSON* channels =
    	  cJSON_GetObjectItemCaseSensitive (json, "channels");
    	cJSON_ArrayForEach (channel, channels)
    	{
    		if (cJSON_IsString (channel)) {
                config->server->channels[channel_iter] = malloc (strlen (channel->valuestring));
                strcpy (config->server->channels[channel_iter], channel->valuestring);
    			channel_iter++;
    		}
    	}
    
        /* Add user data to server */
    	cJSON* user = cJSON_GetObjectItemCaseSensitive (server, "user");
    	if (cJSON_IsObject (user)) {
            struct UserType *tmp_user;
            tmp_user = malloc (sizeof(UserType));
    
    		cJSON* nickname =
    		  cJSON_GetObjectItemCaseSensitive (user, "nickname");
    		if (cJSON_IsString (nickname) &&
    		    (nickname->valuestring != NULL)) {
    			tmp_user->nickname =
    			  malloc (sizeof (nickname->valuestring));
    			strcpy (tmp_user->nickname,
    			        nickname->valuestring);
    		} else
    			err (1, "config: nickname is not a string");
    
    		cJSON* ident = cJSON_GetObjectItemCaseSensitive (user, "ident");
    		if (cJSON_IsString (ident) && (ident->valuestring != NULL)) {
    			tmp_user->ident =
    			  malloc (sizeof (ident->valuestring));
    			strcpy (tmp_user->ident, ident->valuestring);
    		} else
    			err (1, "config: ident is not a string");
    
    		cJSON* realname =
    		  cJSON_GetObjectItemCaseSensitive (user, "realname");
    		if (cJSON_IsString (realname) &&
    		    (realname->valuestring != NULL)) {
    			tmp_user->realname =
    			  malloc (sizeof (realname->valuestring));
    			strcpy (tmp_user->realname,
    			        realname->valuestring);
    		} else
    			err (1, "config: realname is not a string");
    
    		cJSON* sasl_enabled =
    		  cJSON_GetObjectItemCaseSensitive (user, "sasl_enabled");
    		if (cJSON_IsTrue (sasl_enabled))
    			tmp_user->sasl_enabled = true;
    		else if (cJSON_IsFalse (sasl_enabled))
    			tmp_user->sasl_enabled = false;
    		else
    			err (1, "config: sasl_enabled is not a bool");
    
    		cJSON* sasl_user =
    		  cJSON_GetObjectItemCaseSensitive (user, "sasl_user");
    		if (cJSON_IsString (sasl_user) &&
    		    (sasl_user->valuestring != NULL)) {
    			tmp_user->sasl_user =
    			  malloc (sizeof (sasl_user->valuestring));
    			strcpy (tmp_user->sasl_user,
    			        sasl_user->valuestring);
    		} else
    			err (1, "config: sasl_user is not a string");
    
    		cJSON* sasl_pass =
    		  cJSON_GetObjectItemCaseSensitive (user, "sasl_pass");
    		if (cJSON_IsString (sasl_pass) &&
    		    (sasl_pass->valuestring != NULL)) {
    			tmp_user->sasl_pass =
    			  malloc (sizeof (sasl_pass->valuestring));
    			strcpy (tmp_user->sasl_pass,
    			        sasl_pass->valuestring);
    		} else
    			err (1, "config: sasl_pass is not a string");

            config->server->user = malloc (sizeof(tmp_user));
            memcpy (config->server->user, tmp_user, sizeof(tmp_user));
            free (tmp_user);
    	}
    } else {
        err (1, "config: server is not an object");
    }
    
    /* Parse Modules section */
    int iter = 0;
    cJSON* module = NULL;
    cJSON* modules = cJSON_GetObjectItemCaseSensitive (json, "modules");
    cJSON_ArrayForEach (module, modules)
    {
        if (cJSON_IsObject (module)) {
            struct ModuleType *tmp_module = malloc (sizeof(ModuleType));

            cJSON* name =
              cJSON_GetObjectItemCaseSensitive (module, "name");
            if (cJSON_IsString (name) &&
                (name->valuestring != NULL)) {
                tmp_module->name = malloc (sizeof (name->valuestring));
                strcpy (tmp_module->name, name->valuestring);
            } else
                err (1, "config: module: name is not a string");

            int matcher_iter = 0;
            cJSON* matcher = NULL;
            cJSON* matchers =
              cJSON_GetObjectItemCaseSensitive (module, "matchers");
            cJSON_ArrayForEach (matcher, matchers) {
                if (cJSON_IsString (matcher) && matcher->valuestring != NULL) {
                    tmp_module->matchers[matcher_iter] = malloc (strlen (matcher->valuestring));
                    strcpy (tmp_module->matchers[matcher_iter], matcher->valuestring);
                    matcher_iter++;
                } else
                    err (1, "config: module: matchers is not a string");
            }

            cJSON* cmd =
              cJSON_GetObjectItemCaseSensitive (module, "cmd");
            if (cJSON_IsString (cmd) &&
                (cmd->valuestring != NULL)) {
                tmp_module->cmd = malloc (sizeof (cmd->valuestring));
                strcpy (tmp_module->cmd, cmd->valuestring);
            } else
                err (1, "config: module: cmd is not a string");

            cJSON* cwd =
              cJSON_GetObjectItemCaseSensitive (module, "cwd");
            if (cJSON_IsString (cwd) &&
                (cwd->valuestring != NULL)) {
                tmp_module->cwd = malloc (sizeof (cwd->valuestring));
                strcpy (tmp_module->cwd, cwd->valuestring);
            } else
                err (1, "config: module: cwd is not a string");
            
            config->modules[iter] = malloc (sizeof(tmp_module));
            memcpy (config->modules[iter], tmp_module, sizeof(tmp_module));
            free (tmp_module);
            iter++;
        }
    }

	free (raw_config_file);
	cJSON_Delete (json);
	return 0;
}

char*
get_config_value (const char* key)
{
	return getenv (key);
}

int
get_config_length ()
{
	return sizeof (CONFIG_KEY_STRING) / sizeof (CONFIG_KEY_STRING[0]);
}

void
print_config ()
{
	int config_length = get_config_length ();

	for (int i = 0; i < config_length; i++) {
		log_info ("%s: %s\n",
		          CONFIG_KEY_STRING[i],
		          get_config_value (CONFIG_KEY_STRING[i]));
	}
}
