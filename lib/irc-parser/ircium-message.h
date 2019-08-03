/* ircium-message.h
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

#include "ircium-message-tag.h"

G_BEGIN_DECLS

#define IRCIUM_TYPE_MESSAGE (ircium_message_get_type ())

G_DECLARE_FINAL_TYPE (IrciumMessage, ircium_message, IRCIUM, MESSAGE, GObject)

IrciumMessage*
ircium_message_parse (const GByteArray* bytes, const gboolean has_tag_cap);

IrciumMessage*
ircium_message_new (GPtrArray* tags,
                    const gchar* source,
                    const gchar* cmd,
                    GPtrArray* params);

const GPtrArray*
ircium_message_get_tags (const IrciumMessage* msg);
const gchar*
ircium_message_get_source (const IrciumMessage* msg);
const gchar*
ircium_message_get_command (const IrciumMessage* msg);
const GPtrArray*
ircium_message_get_params (const IrciumMessage* msg);

GBytes*
ircium_message_serialize (const IrciumMessage* msg);

gchar*
get_string_between (const guint8* start, const guint8* end);

G_END_DECLS
