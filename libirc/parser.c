#include <string.h>
#include <stdlib.h>

#include "irc/parser.h"
#include "log/log.h"

static size_t
ptrarr_len (const void **ptrarr)
{
	size_t count = 0;
	for (const void **iter = ptrarr; *iter != NULL; ++iter) {
		++count;
	}
	return count;
}

static void
ptrarr_resize (void ***ptrarr, size_t new_size)
{
	*ptrarr = realloc (*ptrarr, (new_size + 1) * sizeof (**ptrarr));
	(*ptrarr)[new_size] = NULL;
}

static char *
alloc_strlen (const char *str, size_t len)
{
	char *ret = calloc (len + 1, sizeof (*ret));
	if (str != NULL)
		memcpy (ret, str, len);
	return ret;
}

void
parse_start_message (void *user_data)
{
	struct irc_msg *msg = user_data;
	msg = alloc_msg ();
}

void
parse_start_tags (void *user_data)
{
	struct irc_msg *msg = user_data;
	msg->tags = allocate_tags ();
}

void
parse_on_tag (const uint8_t *name, size_t name_len, const uint8_t *esc_value, size_t esc_value_len, void *user_data)
{
	struct irc_msg *msg = user_data;

	// If the tag name is unique (as in hasn't been seen before)
	// allocate a new tag.
	bool is_duplicate = false;

	struct irc_msg_tag *tag = NULL;
	char *current_name = alloc_strlen ((const char *)name, name_len);
	for (size_t i = 0; i < ptrarr_len ((const void **)msg->tags); ++i) {
		tag = msg->tags->tags[i];
		if (strcmp (tag->name, current_name) == 0) {
			free (current_name);
			is_duplicate = true;
			break;
		}
	}
	if (!is_duplicate) {
		tag = calloc (1, sizeof (*tag));
		tag->name = current_name;
	}

	size_t value_len = ircmsg_tag_value_unescaped_size (esc_value, esc_value_len);
	if (is_duplicate) {
		free (tag->value);
	}
	tag->value = value_len > 0 ? calloc (value_len + 1, sizeof (*tag->value)) : NULL;

	ircmsg_tag_value_unescape (esc_value, esc_value_len, (uint8_t *)tag->value, value_len);

	if (!is_duplicate)
		append_tag (tag, msg->tags);
}

void
parse_end_tags (void *user_data)
{
	(void)user_data;
}

void
parse_on_prefix (const uint8_t *prefix, size_t prefix_len, void *user_data)
{
	struct irc_msg *msg = user_data;

	msg->prefix = alloc_strlen ((const char *)prefix, prefix_len);
}

void
parse_on_command (const uint8_t *command, size_t command_len, void *user_data)
{
	struct irc_msg *msg = user_data;

	msg->command = alloc_strlen ((const char *)command, command_len);
}

void
parse_start_params (void *user_data)
{
	struct irc_msg *msg = user_data;
	msg->params = allocate_params ();
}

void
parse_on_param (const uint8_t *param, size_t param_len, void *user_data)
{
	struct irc_msg *msg = user_data;
	char *captured_param = alloc_strlen ((const char *)param, param_len);

	append_param (captured_param, msg->params);
}

void
parse_end_params (void *user_data)
{
	(void)user_data;
}

void
parse_end_message (void *user_data)
{
	(void)user_data;
}

void
parse_on_error (ircmsg_parser_err_code error, void *user_data)
{
	struct irc_msg *msg = user_data;
	free_msg (msg);
	msg = NULL;
}

const ircmsg_parser_callbacks parse_cbs = {
	.start_message = parse_start_message,

	.start_tags = parse_start_tags,
	.on_tag = parse_on_tag,
	.end_tags = parse_end_tags,

	.on_prefix = parse_on_prefix,

	.on_command = parse_on_command,

	.start_params = parse_start_params,
	.on_param = parse_on_param,
	.end_params = parse_end_params,

	.end_message = parse_end_message,

	.on_error = parse_on_error,
};

struct irc_msg_tags *
allocate_tags (void)
{
	struct irc_msg_tags *ret = calloc (1, sizeof (*ret));
	ret->len = 0;
	return ret;
}

struct irc_msg_params *
allocate_params (void)
{
	struct irc_msg_params *ret = calloc (1, sizeof (*ret));
	ret->len = 0;
	return ret;
}

void
append_tag (struct irc_msg_tag *tag, struct irc_msg_tags *tags)
{
	size_t new_len = tags->len + 1;
	puts ("a");
	ptrarr_resize ((void ***)&tags->tags, new_len);
	tags->tags[tags->len] = tag;
	tags->len = new_len;
}

void
append_param (char *param, struct irc_msg_params *params)
{
	size_t new_len = params->len + 1;
	ptrarr_resize ((void ***)&params->params, new_len);
	params->params[params->len] = param;
	params->len = new_len;
}

void
free_tags (struct irc_msg_tags *tags)
{
	if (tags != NULL) {
		if (tags->tags != NULL) {
			for (size_t i = 0; i < tags->len; ++i) {
				free (tags->tags[i]->name);
				free (tags->tags[i]->value);
				free (tags->tags[i]);
			}
		}
		free (tags);
	}
}

void
free_params (struct irc_msg_params *params)
{
	if (params != NULL) {
		if (params->params != NULL) {
			for (size_t i = 0; i < params->len; ++i) {
				free (params->params[i]);
			}
		}
		free (params);
	}
}

struct irc_msg *
alloc_msg (void)
{
	struct irc_msg *ret = calloc (1, sizeof (*ret));
	return ret;
}

void
free_msg (struct irc_msg *msg)
{
	if (msg == NULL)
		return;
	free_tags (msg->tags);
	free_params (msg->params);
	if (msg->prefix != NULL) {
		free (msg->prefix);
	}
	if (msg->command != NULL) {
		free (msg->command);
	}
	free (msg);
}
