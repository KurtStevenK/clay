/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayEarlyRc: pre-parsing of clayrc suitable for use during early
 * initialization, when the clay singleton is not constructed yet
 *
 * Copyright (C) 2017  Jehan <jehan@gimp.org>
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

#include "core/core-enums.h"


#define CLAY_TYPE_EARLY_RC            (clay_early_rc_get_type ())
#define CLAY_EARLY_RC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_EARLY_RC, ClayEarlyRc))
#define CLAY_EARLY_RC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_EARLY_RC, ClayEarlyRcClass))
#define CLAY_IS_EARLY_RC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_EARLY_RC))
#define CLAY_IS_EARLY_RC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_EARLY_RC))


typedef struct _ClayEarlyRcClass ClayEarlyRcClass;

struct _ClayEarlyRc
{
  GObject        parent_instance;

  GFile         *user_clayrc;
  GFile         *system_clayrc;
  gboolean       verbose;

  gchar         *language;

#ifdef G_OS_WIN32
  ClayWin32PointerInputAPI win32_pointer_input_api;
#endif
};

struct _ClayEarlyRcClass
{
  GObjectClass   parent_class;
};


GType          clay_early_rc_get_type     (void);

ClayEarlyRc  * clay_early_rc_new          (GFile      *system_clayrc,
                                           GFile      *user_clayrc,
                                           gboolean    verbose);
gchar        * clay_early_rc_get_language (ClayEarlyRc *rc);

#ifdef G_OS_WIN32
ClayWin32PointerInputAPI clay_early_rc_get_win32_pointer_input_api (ClayEarlyRc *rc);
#endif
