/* ircium-message-tag.h
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

#pragma once

#include <glib-object.h>
#include <glib.h>

G_BEGIN_DECLS

#define IRCIUM_TYPE_MESSAGE_TAG (ircium_message_tag_get_type())

G_DECLARE_FINAL_TYPE (IrciumMessageTag,
		      ircium_message_tag,
		      IRCIUM, MESSAGE_TAG,
		      GObject)

IrciumMessageTag *ircium_message_tag_new (void);
IrciumMessageTag *ircium_message_tag_new_with_name (gchar *name);
IrciumMessageTag *ircium_message_tag_new_with_name_and_value (gchar *name,
                                                              gchar *value);

const gchar *ircium_message_tag_get_name (IrciumMessageTag *tag);
void ircium_message_tag_set_name (IrciumMessageTag *tag, gchar *name);
const gchar *ircium_message_tag_get_value (IrciumMessageTag *tag);
void ircium_message_tag_set_value (IrciumMessageTag *tag, gchar *value);
gboolean ircium_message_tag_has_value (IrciumMessageTag *tag);

gchar *ircium_message_tag_escape_string (const gchar *str);
gchar *ircium_message_tag_unescape_string (const gchar *str);

G_END_DECLS
