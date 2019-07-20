/* ircium-message.c
 *
 * Copyright 2019 Jani Juhani Sinervo <jani@sinervo.fi>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "ircium-message.h"

struct _IrciumMessage
{
	GObject parent_instance;

	GPtrArray *tags;
	gchar *source;
	gchar *command;
	GPtrArray *params;
};

G_DEFINE_TYPE (IrciumMessage, ircium_message, G_TYPE_OBJECT)

static IrciumMessage *
ircium_message_new_real (void)
{
	return g_object_new (IRCIUM_TYPE_MESSAGE, NULL);
}

IrciumMessage *
ircium_message_new (GPtrArray *tags,
                    gchar     *source,
                    gchar     *cmd,
                    GPtrArray *params)
{
	IrciumMessage *ret = ircium_message_new_real ();
	ret->tags = tags != NULL ? g_ptr_array_ref (tags) : NULL;
	ret->source = g_strdup (source);
	ret->command = g_strdup (cmd);
	ret->params = params != NULL ? g_ptr_array_ref (params) : NULL;

	return ret;
}

static void
ircium_message_finalize (GObject *object)
{
	IrciumMessage *self = (IrciumMessage *)object;

	g_clear_pointer (&self->tags, g_ptr_array_unref);
	g_clear_pointer (&self->source, g_free);
	g_clear_pointer (&self->command, g_free);
	g_clear_pointer (&self->params, g_ptr_array_unref);

	G_OBJECT_CLASS (ircium_message_parent_class)->finalize (object);
}

static void
ircium_message_class_init (IrciumMessageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ircium_message_finalize;
}

static void
ircium_message_init (IrciumMessage *self)
{
}

static gboolean
legal_tag_char (gchar c) {
	return c != ' ' && c != '\r' && c != '\0' && c != '\n';
}

gchar *
get_string_between (const guint8 *start,
                    const guint8 *end)
{
	ptrdiff_t diff = end - start;
	gchar *ret = g_malloc0 (diff + 1);

	memcpy (ret, start, diff);

	return ret;
}

static IrciumMessageTag *
new_tag_named (const guint8 *head,
               const guint8 *iter)
{
	gchar *tag_name = get_string_between (head, iter);
	IrciumMessageTag *tag = ircium_message_tag_new_with_name (tag_name);
	g_free (tag_name);
	return tag;
}

static gchar *
tag_val (const guint8 *head,
	 const guint8 *iter)
{
	gchar *temp = get_string_between (head, iter);
	gchar *ret = ircium_message_tag_unescape_string (temp);
	g_free (temp);
	return ret;
}

IrciumMessage *
ircium_message_parse (GByteArray *bytes,
                      gboolean    has_tag_cap)
{
	IrciumMessage *msg = NULL;

	GPtrArray *tags = NULL;
	const guint8 *iter = bytes->data;
	if (has_tag_cap) {
		// If we understand tags, like if we have
		// enabled IRCv3.3 message-tags, or explicitly
		// enable something like `server-time` while
		// having IRCv3.2 compatibility, we want to parse them.
		//
		// Notably, there might be no tags in the message,
		// but if we don't understand them at the moment, there
		// is no point in even trying to parse them.
		const gchar test_char = *iter;
		if (test_char == '@') {
			// We have tags! Hooray! Now, let's scan our bytes
			// so we can look at this stuff a bit more.
			tags = g_ptr_array_new_with_free_func (g_object_unref);
			gboolean is_tag_name = TRUE;
			const guint8 *head = ++iter;
			IrciumMessageTag *tag = NULL;
			for (gchar c = *iter; legal_tag_char (c); c = *++iter) {
				if (c == ';') {
					// We have a tag seperator.
					if (is_tag_name) {
						tag = new_tag_named (head,
								     iter);
						g_ptr_array_add (tags, tag);
					} else {
						gchar *val = tag_val (head,
								      iter);
						ircium_message_tag_set_value
							(tag, val);
						g_free (val);
					}
					is_tag_name = TRUE;
					head = iter + 1;
					continue;
				}
				if (c == '=') {
					// We have a separator
					// between our tag and its value.
					if (!is_tag_name) {
						// This is probably part of the
						// tag's value. Equals doesn't
						// get escaped. Continue as
						// usual.
						continue;
					}
					// Now we have our tag for real.
					tag = new_tag_named (head, iter);
					g_ptr_array_add (tags, tag);
					is_tag_name = FALSE;
					head = iter + 1;
					continue;
				}
			}
			// The last tag name or the last tag's value
			// is a bit special. After it is a space that
			// seperates the tags from the rest of the message,
			// and it thus has to be handled specially.
			//
			// According to the condition of the for-loop above,
			// we might have also gotten some other character.
			// If we've hit something but space, the message is
			// malformed.
			if (*iter != ' ') {
				goto clean_tags;
			}
			if (is_tag_name) {
				tag = new_tag_named (head,
						     iter);
				g_ptr_array_add (tags, tag);
			} else {
				gchar *val = tag_val (head,
						      iter);
				ircium_message_tag_set_value
					(tag, val);
				g_free (val);
			}
			while (*++iter == ' ');
		}
	} else if (*iter == '@') {
		// We have a tag (probably) but we don't have the capability
		// to handle it. This is not a good state.
		goto ret;
	}

	// Now we'll look for source, if such exists in this message.
	gchar *source = NULL;
	if (*iter == ':') {
		const guint8 *head = ++iter;
		for (; *iter != ' '; ++iter);
		// We now have our source.
		source = get_string_between (head, iter);

		while (*++iter == ' ');
	}

	// Now we're going to be parsing our command. This may either be
	// a numeric or a command.
	gchar *command = NULL;
	{
		const guint8 *head = iter;
		while (*++iter != ' ');
		command = get_string_between (head, iter);

		while (*++iter == ' ');
	}

	GPtrArray *params = g_ptr_array_new_with_free_func (g_free);
	{
		const guint8 *head = iter;
		gboolean is_trailing_param = FALSE;
		for (;*iter != '\r' && *(iter + 1) != '\n'; ++iter) {
			if (*iter == ':' && !is_trailing_param &&
			    *(iter - 1) == ' ') {
				is_trailing_param = TRUE;
				head = iter + 1;
				continue;
			}
			if (*iter == ' ' && !is_trailing_param) {
				// The following might happen if
				// we have multiple space characters one
				// after another.
				if (head == iter) {
					head = iter + 1;
					continue;
				}
				// We have something to actually make into
				// a parameter.
				gchar *param = get_string_between (head, iter);
				g_ptr_array_add (params, param);
				head = iter + 1;
			}
		}
		// Final parameter (may or may not be trailing) or no parameters
		// I don't know if this is the best condition for this, but
		// I came up with this.
		if (head == iter && !is_trailing_param) {
			g_clear_pointer (&params, g_ptr_array_unref);
		} else {
			gchar *param = get_string_between (head, iter);
			g_ptr_array_add (params, param);
		}
	}

	// If we've gotten this far, our message should be good to go.
	msg = ircium_message_new_real ();
	msg->tags = tags != NULL ? g_ptr_array_ref (tags) : NULL;
	msg->source = g_strdup (source);
	msg->command = g_strdup (command);
	msg->params = params != NULL ? g_ptr_array_ref (params) : NULL;

	g_clear_pointer (&params, g_ptr_array_unref);
	g_clear_pointer (&command, g_free);
	g_clear_pointer (&source, g_free);
clean_tags:
	g_clear_pointer (&tags, g_ptr_array_unref);
ret:
	return msg;
}

const GPtrArray *
ircium_message_get_tags (IrciumMessage *msg)
{
	return msg->tags;
}

const gchar *
ircium_message_get_source (IrciumMessage *msg)
{
	return msg->source;
}

const gchar *
ircium_message_get_command (IrciumMessage *msg)
{
	return msg->command;
}

const GPtrArray *
ircium_message_get_params (IrciumMessage *msg)
{
	return msg->params;
}

GBytes *
ircium_message_serialize (IrciumMessage *msg)
{
	g_return_val_if_fail (IRCIUM_IS_MESSAGE (msg), NULL);

	// First, we serialize the tags if we have any.
	gchar *serialized_tags = NULL;
	if (msg->tags != NULL) {
		serialized_tags = g_strdup ("@");
		gchar delimiter = ';';
		for (gsize i = 0; i < msg->tags->len; ++i) {
			IrciumMessageTag *tag = msg->tags->pdata[i];
			if (i == msg->tags->len - 1) delimiter = ' ';
			gchar *old_str = serialized_tags;
			const gchar *name = ircium_message_tag_get_name (tag);
			if (ircium_message_tag_has_value (tag)) {
				const gchar *val =
					ircium_message_tag_get_value (tag);
				gchar *escaped_val =
					ircium_message_tag_escape_string (val);
				serialized_tags =
					g_strdup_printf ("%s%s=%s%c",
							 serialized_tags,
							 name,
							 escaped_val,
							 delimiter);
				g_free (escaped_val);
			} else {
				serialized_tags =
					g_strdup_printf ("%s%s%c",
							 serialized_tags,
							 name,
							 delimiter);
			}
			g_free (old_str);
		}
	}

	// Now, we'll serialize the source, if we have any.
	gchar *serialized_source = NULL;
	if (msg->source != NULL) {
		serialized_source = g_strdup_printf (":%s ", msg->source);
	}

	// Now the command.
	gchar *serialized_command = g_strdup (msg->command);

	// And the parameters if we have any.
	gchar *serialized_params = NULL;
	if (msg->params != NULL) {
		serialized_params = g_strdup (" ");
		gboolean is_trailing = FALSE;
		for (gsize i = 0; i < msg->params->len; ++i) {
			gchar *param = msg->params->pdata[i];
			if (i == msg->params->len - 1) is_trailing = TRUE;
			gchar *old_str = serialized_params;
			gchar *tmp = serialized_params == NULL ?
				"" : serialized_params;
			if (is_trailing) {
				serialized_params =
					g_strdup_printf ("%s:%s",
							 tmp,
							 param);
			} else {
				serialized_params =
					g_strdup_printf ("%s%s ",
							 tmp,
							 param);
			}
			g_free (old_str);
		}
	}

	gchar *serialized_msg =
		g_strdup_printf ("%s%s%s%s\r\n",
				 serialized_tags ? serialized_tags : "",
				 serialized_source ? serialized_source : "",
				 serialized_command,
				 serialized_params ? serialized_params : "");

	// We really don't care about the NUL byte anyhow,
	// because this will be sent across the wire.
	gsize len = strlen (serialized_msg);

	GBytes *bytes = g_bytes_new_take (serialized_msg, len);

	g_clear_pointer (&serialized_tags, g_free);
	g_clear_pointer (&serialized_source, g_free);
	g_clear_pointer (&serialized_command, g_free);
	g_clear_pointer (&serialized_params, g_free);

	return bytes;
}
