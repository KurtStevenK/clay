/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayRc
 * Copyright (C) 2001  Sven Neumann <sven@gimp.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "config/claypluginconfig.h"


#define CLAY_TYPE_RC            (clay_rc_get_type ())
#define CLAY_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_RC, ClayRc))
#define CLAY_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_RC, ClayRcClass))
#define CLAY_IS_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_RC))
#define CLAY_IS_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_RC))


typedef struct _ClayRcClass ClayRcClass;

struct _ClayRc
{
  ClayPluginConfig  parent_instance;

  GFile            *user_clayrc;
  GFile            *system_clayrc;
  gboolean          verbose;
  gboolean          autosave;
  guint             save_idle_id;
};

struct _ClayRcClass
{
  ClayPluginConfigClass  parent_class;
};


GType     clay_rc_get_type          (void);

ClayRc  * clay_rc_new               (GObject     *clay,
                                     GFile       *system_clayrc,
                                     GFile       *user_clayrc,
                                     gboolean     verbose);

void      clay_rc_load_system       (ClayRc      *rc);
void      clay_rc_load_user         (ClayRc      *rc);

void      clay_rc_set_autosave      (ClayRc      *rc,
                                     gboolean     autosave);
void      clay_rc_save              (ClayRc      *rc);

gchar   * clay_rc_query             (ClayRc      *rc,
                                     const gchar *key);

void      clay_rc_set_unknown_token (ClayRc      *rc,
                                     const gchar *token,
                                     const gchar *value);
