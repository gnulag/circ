// Copyright (c) 2019 Jani Juhani Sinervo
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
//  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include "serializer.h"
#include "log/log.h"
#include <string.h>

size_t
serializer_tag_count (void *user_data);
void
serializer_on_tag (size_t tag_idx,
		   size_t *const tag_len,
		   const uint8_t **tag,
		   size_t *const val_len,
		   const uint8_t **val,
		   void *user_data);
bool
serializer_on_prefix (size_t *const prefix_len,
		      const uint8_t **prefix,
		      void *user_data);
void
serializer_on_command (size_t *const command_len,
		       const uint8_t **command,
		       void *user_data);
size_t
serializer_param_count (void *user_data);
void
serializer_on_param (size_t param_idx,
		     size_t *const param_len,
		     const uint8_t **param,
		     void *user_data);

static size_t
ptrarr_len (const void **ptrarr)
{
	size_t count = 0;
	for (const void **iter = ptrarr; *iter != NULL; ++iter) {
		++count;
	}
	return count;
}

ircmsg_serializer_callbacks serializer_cbs = {
	.tag_count = serializer_tag_count,
	.on_tag = serializer_on_tag,
	.on_prefix = serializer_on_prefix,
	.on_command = serializer_on_command,
	.param_count = serializer_param_count,
	.on_param = serializer_on_param,
};

size_t
serializer_tag_count (void *user_data)
{
	struct irc_msg *msg = user_data;
	return msg->tags != NULL ? ptrarr_len ((const void **)msg->tags) : 0;
}

void
serializer_on_tag (size_t tag_idx,
		   size_t *const tag_len,
		   const uint8_t **tag,
		   size_t *const val_len,
		   const uint8_t **val,
		   void *user_data)
{
	struct irc_msg *msg = user_data;
	struct irc_msg_tag *t = msg->tags->tags[tag_idx];

	*tag_len = strlen (t->name);
	*tag = (uint8_t *)t->name;

	if (t->value != NULL) {
		*val_len = strlen (t->value);
		*val = (uint8_t *)t->value;
	} else {
		*val_len = 0;
		*val = NULL;
	}
}

bool
serializer_on_prefix (size_t *const prefix_len,
		      const uint8_t **prefix,
		      void *user_data)
{
	struct irc_msg *msg = user_data;
	if (msg->prefix == NULL)
		return false;
	*prefix_len = strlen (msg->prefix);
	*prefix = (uint8_t *)msg->prefix;
	return true;
}

void
serializer_on_command (size_t *const command_len,
		       const uint8_t **command,
		       void *user_data)
{
	struct irc_msg *msg = user_data;
	*command_len = strlen (msg->command);
	*command = (uint8_t *)msg->command;
}

size_t
serializer_param_count (void *user_data)
{
	struct irc_msg *msg = user_data;
	return msg->params->len;
}

void
serializer_on_param (size_t param_idx,
		     size_t *const param_len,
		     const uint8_t **param,
		     void *user_data)
{
	struct irc_msg *msg = user_data;
	char *p = msg->params->params[param_idx];
	*param_len = strlen (p);
	*param = (uint8_t *)p;
}
