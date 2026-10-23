/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayplugindef.c
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "plug-in-types.h"

#include "core/clay-memsize.h"

#include "clayplugindef.h"
#include "claypluginprocedure.h"


static void     clay_plug_in_def_finalize    (GObject    *object);

static gint64   clay_plug_in_def_get_memsize (ClayObject *object,
                                              gint64     *gui_size);


G_DEFINE_TYPE (ClayPlugInDef, clay_plug_in_def, CLAY_TYPE_OBJECT)

#define parent_class clay_plug_in_def_parent_class


static void
clay_plug_in_def_class_init (ClayPlugInDefClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_plug_in_def_finalize;

  clay_object_class->get_memsize = clay_plug_in_def_get_memsize;
}

static void
clay_plug_in_def_init (ClayPlugInDef *def)
{
}

static void
clay_plug_in_def_finalize (GObject *object)
{
  ClayPlugInDef *plug_in_def = CLAY_PLUG_IN_DEF (object);

  g_object_unref (plug_in_def->file);
  g_free (plug_in_def->help_domain_name);
  g_free (plug_in_def->help_domain_uri);

  g_slist_free_full (plug_in_def->procedures, (GDestroyNotify) g_object_unref);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_plug_in_def_get_memsize (ClayObject *object,
                              gint64     *gui_size)
{
  ClayPlugInDef *plug_in_def = CLAY_PLUG_IN_DEF (object);
  gint64         memsize     = 0;

  memsize += clay_g_object_get_memsize (G_OBJECT (plug_in_def->file));
  memsize += clay_string_get_memsize (plug_in_def->help_domain_name);
  memsize += clay_string_get_memsize (plug_in_def->help_domain_uri);

  memsize += clay_g_slist_get_memsize (plug_in_def->procedures, 0);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}


/*  public functions  */

ClayPlugInDef *
clay_plug_in_def_new (GFile *file,
                      GFile *root_folder)
{
  ClayPlugInDef *plug_in_def;

  g_return_val_if_fail (G_IS_FILE (file), NULL);

  plug_in_def = g_object_new (CLAY_TYPE_PLUG_IN_DEF, NULL);

  plug_in_def->file        = g_object_ref (file);
  plug_in_def->root_folder = g_object_ref (root_folder);

  return plug_in_def;
}

void
clay_plug_in_def_add_procedure (ClayPlugInDef       *plug_in_def,
                                ClayPlugInProcedure *proc)
{
  ClayPlugInProcedure *overridden;

  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  overridden = clay_plug_in_procedure_find (plug_in_def->procedures,
                                            clay_object_get_name (proc));

  if (overridden)
    clay_plug_in_def_remove_procedure (plug_in_def, overridden);

  proc->mtime = plug_in_def->mtime;

  clay_plug_in_procedure_set_help_domain (proc,
                                          plug_in_def->help_domain_name);

  plug_in_def->procedures = g_slist_append (plug_in_def->procedures,
                                            g_object_ref (proc));
}

void
clay_plug_in_def_remove_procedure (ClayPlugInDef       *plug_in_def,
                                   ClayPlugInProcedure *proc)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));
  g_return_if_fail (CLAY_IS_PLUG_IN_PROCEDURE (proc));

  plug_in_def->procedures = g_slist_remove (plug_in_def->procedures, proc);
  g_object_unref (proc);
}

void
clay_plug_in_def_set_help_domain (ClayPlugInDef *plug_in_def,
                                  const gchar   *domain_name,
                                  const gchar   *domain_uri)
{
  GSList *list;

  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));

  if (plug_in_def->help_domain_name)
    g_free (plug_in_def->help_domain_name);
  plug_in_def->help_domain_name = g_strdup (domain_name);

  if (plug_in_def->help_domain_uri)
    g_free (plug_in_def->help_domain_uri);
  plug_in_def->help_domain_uri = g_strdup (domain_uri);

  for (list = plug_in_def->procedures; list; list = g_slist_next (list))
    {
      ClayPlugInProcedure *procedure = list->data;

      clay_plug_in_procedure_set_help_domain (procedure,
                                              plug_in_def->help_domain_name);
    }
}

void
clay_plug_in_def_set_mtime (ClayPlugInDef *plug_in_def,
                            gint64         mtime)
{
  GSList *list;

  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->mtime = mtime;

  for (list = plug_in_def->procedures; list; list = g_slist_next (list))
    {
      ClayPlugInProcedure *proc = list->data;

      proc->mtime = plug_in_def->mtime;
    }
}

void
clay_plug_in_def_set_needs_query (ClayPlugInDef *plug_in_def,
                                  gboolean       needs_query)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->needs_query = needs_query ? TRUE : FALSE;
}

void
clay_plug_in_def_set_has_init (ClayPlugInDef *plug_in_def,
                               gboolean       has_init)
{
  g_return_if_fail (CLAY_IS_PLUG_IN_DEF (plug_in_def));

  plug_in_def->has_init = has_init ? TRUE : FALSE;
}
