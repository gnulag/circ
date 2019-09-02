#include <stdarg.h>
#include <stdio.h>

#include "log/log.h"
#include "irc/parser.h"
#include "irc/message.h"

irc_msg *
irc_msg_new (char *prefix, char *command, int params_length, char *params[])
{
	irc_msg *msg = alloc_msg ();
	msg->tags = allocate_tags ();
	msg->params = allocate_params ();
	msg->prefix = prefix;
	msg->command = command;

	for (int i = 0; i < params_length; i++) {
		append_param (params[i], msg->params);
	}

	msg->params->len = params_length;

	return msg;
};
