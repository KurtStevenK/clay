/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbprogress.h
 * Copyright (C) 2004 Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_PDB_PROGRESS            (clay_pdb_progress_get_type ())
#define CLAY_PDB_PROGRESS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PDB_PROGRESS, ClayPdbProgress))
#define CLAY_PDB_PROGRESS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PDB_PROGRESS, ClayPdbProgressClass))
#define CLAY_IS_PDB_PROGRESS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PDB_PROGRESS))
#define CLAY_IS_PDB_PROGRESS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PDB_PROGRESS))
#define CLAY_PDB_PROGRESS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PDB_PROGRESS, ClayPdbProgressClass))


typedef struct _ClayPdbProgressClass  ClayPdbProgressClass;

struct _ClayPdbProgress
{
  GObject      object;

  gboolean     active;
  gdouble      value;

  ClayPDB     *pdb;
  ClayContext *context;
  gchar       *callback_name;
  gboolean     callback_busy;
};

struct _ClayPdbProgressClass
{
  GObjectClass  parent_class;

  GList        *progresses;
};


GType             clay_pdb_progress_get_type        (void);

ClayPdbProgress * clay_pdb_progress_get_by_callback (ClayPdbProgressClass *klass,
                                                     const gchar          *callback_name);
