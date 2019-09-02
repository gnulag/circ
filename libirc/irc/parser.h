#ifndef IRC_MSG_PARSER_H
#define IRC_MSG_PARSER_H

#include <stdbool.h>
#include "ircmsg/parser.h"
#include "irc/message.h"

struct irc_msg_tags *
allocate_tags (void);
struct irc_msg_params *
allocate_params (void);

void
append_tag (struct irc_msg_tag *tag, struct irc_msg_tags *tags);
void
append_param (char *param, struct irc_msg_params *params);

void
free_tags (struct irc_msg_tags *tags);
void
free_params (struct irc_msg_params *params);

struct irc_msg *
alloc_msg (void);
void
free_msg (struct irc_msg *msg);

extern const ircmsg_parser_callbacks parse_cbs;

#endif
