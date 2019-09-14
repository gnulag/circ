/*
 * Central Place to define the structure of the irc messages
 */
#ifndef IRC_MSG_H
#define IRC_MSG_H

#include <stdio.h>

typedef struct irc_msg_tag
{
	char *name;
	char *value;
} irc_msg_tag;

typedef struct irc_msg_tags
{
	int len;
	struct irc_msg_tag **tags;
} irc_msg_tags;

typedef struct irc_msg_params
{
	int len;
	char **params;
} irc_msg_params;

typedef struct irc_msg
{
	struct irc_msg_tags *tags;
	char *prefix;
	char *command;
	struct irc_msg_params *params;
} irc_msg;

irc_msg *
irc_msg_new (char *prefix, char *command, int params_length, char *params[]);

#endif
