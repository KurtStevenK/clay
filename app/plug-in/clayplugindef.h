/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugindef.h
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

#include "core/clayobject.h"


#define CLAY_TYPE_PLUG_IN_DEF            (clay_plug_in_def_get_type ())
#define CLAY_PLUG_IN_DEF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PLUG_IN_DEF, ClayPlugInDef))
#define CLAY_PLUG_IN_DEF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PLUG_IN_DEF, ClayPlugInDefClass))
#define CLAY_IS_PLUG_IN_DEF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PLUG_IN_DEF))
#define CLAY_IS_PLUG_IN_DEF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PLUG_IN_DEF))


typedef struct _ClayPlugInDefClass ClayPlugInDefClass;

struct _ClayPlugInDef
{
  ClayObject  parent_instance;

  GFile      *file;
  GFile      *root_folder;
  GSList     *procedures;
  gchar      *help_domain_name;
  gchar      *help_domain_uri;
  gint64      mtime;
  gboolean    needs_query;  /* Does the plug-in need to be queried ?     */
  gboolean    has_init;     /* Does the plug-in need to be initialized ? */
};

struct _ClayPlugInDefClass
{
  ClayObjectClass  parent_class;
};


GType           clay_plug_in_def_get_type (void);

ClayPlugInDef * clay_plug_in_def_new      (GFile               *file,
                                           GFile               *root_folder);

void   clay_plug_in_def_add_procedure     (ClayPlugInDef       *plug_in_def,
                                           ClayPlugInProcedure *proc);
void   clay_plug_in_def_remove_procedure  (ClayPlugInDef       *plug_in_def,
                                           ClayPlugInProcedure *proc);

void   clay_plug_in_def_set_help_domain   (ClayPlugInDef       *plug_in_def,
                                           const gchar         *domain_name,
                                           const gchar         *domain_uri);

void   clay_plug_in_def_set_mtime         (ClayPlugInDef       *plug_in_def,
                                           gint64               mtime);
void   clay_plug_in_def_set_needs_query   (ClayPlugInDef       *plug_in_def,
                                           gboolean             needs_query);
void   clay_plug_in_def_set_has_init      (ClayPlugInDef       *plug_in_def,
                                           gboolean             has_init);
