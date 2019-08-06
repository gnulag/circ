/* ircium-message-tag.c
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

#include "ircium-message-tag.h"
#include <stdio.h>

struct _IrciumMessageTag
{
	GObject parent_instance;

	gchar *name;
	gchar *value;
};

G_DEFINE_TYPE (IrciumMessageTag, ircium_message_tag, G_TYPE_OBJECT)

enum
{
	PROP_0,

	PROP_NAME,
	PROP_VALUE,

	N_PROPS
};

static GParamSpec *properties[N_PROPS];

IrciumMessageTag *
ircium_message_tag_new (void)
{
	return g_object_new (IRCIUM_TYPE_MESSAGE_TAG, NULL);
}

IrciumMessageTag *
ircium_message_tag_new_with_name (gchar *name)
{
	return g_object_new (IRCIUM_TYPE_MESSAGE_TAG, "name", name, NULL);
}

IrciumMessageTag *
ircium_message_tag_new_with_name_and_value (gchar *name, gchar *value)
{
	return g_object_new (
	  IRCIUM_TYPE_MESSAGE_TAG, "name", name, "value", value, NULL);
}

static void
ircium_message_tag_finalize (GObject *object)
{
	IrciumMessageTag *self = (IrciumMessageTag *)object;

	g_clear_pointer (&self->name, g_free);
	g_clear_pointer (&self->value, g_free);

	G_OBJECT_CLASS (ircium_message_tag_parent_class)->finalize (object);
}

static void
ircium_message_tag_get_property (GObject *object,
				 guint prop_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	IrciumMessageTag *self = IRCIUM_MESSAGE_TAG (object);

	switch (prop_id) {
		case PROP_NAME:
			g_value_set_string (value, self->name);
			break;
		case PROP_VALUE:
			g_value_set_string (value, self->value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
ircium_message_tag_set_property (GObject *object,
				 guint prop_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	IrciumMessageTag *self = IRCIUM_MESSAGE_TAG (object);

	switch (prop_id) {
		case PROP_NAME:
			self->name = g_value_dup_string (value);
			break;
		case PROP_VALUE:
			self->value = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
ircium_message_tag_class_init (IrciumMessageTagClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = ircium_message_tag_finalize;
	object_class->get_property = ircium_message_tag_get_property;
	object_class->set_property = ircium_message_tag_set_property;

	properties[PROP_NAME] =
	  g_param_spec_string ("name",
			       "name",
			       "The name of this tag",
			       NULL,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT);
	properties[PROP_VALUE] =
	  g_param_spec_string ("value",
			       "value",
			       "The value of this tag, if one exists",
			       NULL,
			       G_PARAM_READWRITE | G_PARAM_CONSTRUCT);

	g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
ircium_message_tag_init (IrciumMessageTag *self)
{}

const gchar *
ircium_message_tag_get_name (IrciumMessageTag *tag)
{
	g_return_val_if_fail (IRCIUM_IS_MESSAGE_TAG (tag), NULL);
	return tag->name;
}

void
ircium_message_tag_set_name (IrciumMessageTag *tag, gchar *name)
{
	g_return_if_fail (IRCIUM_IS_MESSAGE_TAG (tag));

	g_clear_pointer (&tag->name, g_free);
	tag->name = g_strdup (name);
}

const gchar *
ircium_message_tag_get_value (IrciumMessageTag *tag)
{
	g_return_val_if_fail (IRCIUM_IS_MESSAGE_TAG (tag), NULL);
	return tag->value;
}

void
ircium_message_tag_set_value (IrciumMessageTag *tag, gchar *value)
{
	g_return_if_fail (IRCIUM_IS_MESSAGE_TAG (tag));

	g_clear_pointer (&tag->value, g_free);
	tag->value = g_strdup (value);
}

gboolean
ircium_message_tag_has_value (IrciumMessageTag *tag)
{
	g_return_val_if_fail (IRCIUM_IS_MESSAGE_TAG (tag), FALSE);
	return tag->value != NULL;
}

gchar *
ircium_message_tag_escape_string (const gchar *str)
{
	gsize final_str_len = 0;
	for (const gchar *iter = str; (*iter) != '\0'; ++iter) {
		switch (*iter) {
			case ';':
				final_str_len += 2;
				break;
			case ' ':
				final_str_len += 2;
				break;
			case '\\':
				final_str_len += 2;
				break;
			case '\r':
				final_str_len += 2;
				break;
			case '\n':
				final_str_len += 2;
				break;
			default:
				final_str_len += 1;
				break;
		}
	}
	gchar *ret = g_malloc0 (final_str_len + 1);
	gsize pos = 0;
	for (const gchar *iter = str; *iter != '\0'; ++iter) {
		switch (*iter) {
			case ';':
				ret[pos] = '\\';
				ret[++pos] = ':';
				break;
			case ' ':
				ret[pos] = '\\';
				ret[++pos] = 's';
				break;
			case '\\':
				ret[pos] = '\\';
				ret[++pos] = '\\';
				break;
			case '\r':
				ret[pos] = '\\';
				ret[++pos] = 'r';
				break;
			case '\n':
				ret[pos] = '\\';
				ret[++pos] = 'n';
				break;
			default:
				ret[pos] = *iter;
				break;
		}
		++pos;
	}

	return ret;
}

gchar *
ircium_message_tag_unescape_string (const gchar *str)
{
	gsize final_str_len = 0;
	for (const gchar *iter = str; *iter != '\0';) {
		if (*iter == '\\') {
			switch (*++iter) {
				case '\0':
					continue;
				case ':':
					++final_str_len;
					break;
				case 's':
					++final_str_len;
					break;
				case '\\':
					++final_str_len;
					break;
				case 'r':
					++final_str_len;
					break;
				case 'n':
					++final_str_len;
					break;
				default:
					break;
			}
			++iter;
			continue;
		}
		++final_str_len;
		++iter;
	}

	gchar *ret = g_malloc0 (final_str_len + 1);

	gsize pos = 0;
	for (const gchar *iter = str; *iter != '\0';) {
		if (*iter == '\\') {
			switch (*++iter) {
				case '\0':
					continue;
				case ':':
					ret[pos++] = ';';
					++iter;
					break;
				case 's':
					ret[pos++] = ' ';
					++iter;
					break;
				case '\\':
					ret[pos++] = '\\';
					++iter;
					break;
				case 'r':
					ret[pos++] = '\r';
					++iter;
					break;
				case 'n':
					ret[pos++] = '\n';
					++iter;
					break;
				default:
					break;
			}
			continue;
		}
		ret[pos++] = *iter;
		++iter;
	}

	return ret;
}
