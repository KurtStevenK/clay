/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypdbprocedure.c
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#include "clay.h"

#include "claypdb-private.h"
#include "claypdb_pdb.h"
#include "claypdbprocedure.h"


enum
{
  PROP_0,
  PROP_PDB,
  N_PROPS
};


struct _ClayPDBProcedure
{
  ClayProcedure  parent_instance;

  ClayPDB       *pdb;
};


static void       clay_pdb_procedure_constructed   (GObject              *object);
static void       clay_pdb_procedure_finalize      (GObject              *object);
static void       clay_pdb_procedure_set_property  (GObject              *object,
                                                    guint                 property_id,
                                                    const GValue         *value,
                                                    GParamSpec           *pspec);
static void       clay_pdb_procedure_get_property  (GObject              *object,
                                                    guint                 property_id,
                                                    GValue               *value,
                                                    GParamSpec           *pspec);

static void       clay_pdb_procedure_install       (ClayProcedure        *procedure);
static void       clay_pdb_procedure_uninstall     (ClayProcedure        *procedure);
static ClayValueArray *
                  clay_pdb_procedure_run           (ClayProcedure        *procedure,
                                                    const ClayValueArray *args);


G_DEFINE_TYPE (ClayPDBProcedure, _clay_pdb_procedure, CLAY_TYPE_PROCEDURE)

#define parent_class _clay_pdb_procedure_parent_class

static GParamSpec *props[N_PROPS] = { NULL, };


static void
_clay_pdb_procedure_class_init (ClayPDBProcedureClass *klass)
{
  GObjectClass       *object_class    = G_OBJECT_CLASS (klass);
  ClayProcedureClass *procedure_class = CLAY_PROCEDURE_CLASS (klass);

  object_class->constructed  = clay_pdb_procedure_constructed;
  object_class->finalize     = clay_pdb_procedure_finalize;
  object_class->set_property = clay_pdb_procedure_set_property;
  object_class->get_property = clay_pdb_procedure_get_property;

  procedure_class->install   = clay_pdb_procedure_install;
  procedure_class->uninstall = clay_pdb_procedure_uninstall;
  procedure_class->run       = clay_pdb_procedure_run;

  props[PROP_PDB] =
    g_param_spec_object ("pdb",
                         "PDB",
                         "The ClayPDB of this plug-in process",
                         CLAY_TYPE_PDB,
                         CLAY_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, N_PROPS, props);
}

static void
_clay_pdb_procedure_init (ClayPDBProcedure *procedure)
{
}

static void
clay_pdb_procedure_constructed (GObject *object)
{
  ClayPDBProcedure *procedure = CLAY_PDB_PROCEDURE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  g_assert (CLAY_IS_PDB (procedure->pdb));
}

static void
clay_pdb_procedure_finalize (GObject *object)
{
  ClayPDBProcedure *procedure = CLAY_PDB_PROCEDURE (object);

  g_clear_object (&procedure->pdb);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_pdb_procedure_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  ClayPDBProcedure *procedure = CLAY_PDB_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_set_object (&procedure->pdb, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_procedure_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  ClayPDBProcedure *procedure = CLAY_PDB_PROCEDURE (object);

  switch (property_id)
    {
    case PROP_PDB:
      g_value_set_object (value, procedure->pdb);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pdb_procedure_install (ClayProcedure *procedure)
{
  g_warning ("Cannot install a ClayPDBProcedure");
}

static void
clay_pdb_procedure_uninstall (ClayProcedure *procedure)
{
  g_warning ("Cannot uninstall a ClayPDBProcedure");
}

static ClayValueArray *
clay_pdb_procedure_run (ClayProcedure        *procedure,
                        const ClayValueArray *args)
{
  ClayPDBProcedure *pdb_procedure = CLAY_PDB_PROCEDURE (procedure);

  return _clay_pdb_run_procedure_array (pdb_procedure->pdb,
                                        clay_procedure_get_name (procedure),
                                        (ClayValueArray *) args);
}


/*  public functions  */

ClayProcedure  *
_clay_pdb_procedure_new (ClayPDB     *pdb,
                         const gchar *name)
{
  ClayProcedure   *procedure;
  gchar           *blurb;
  gchar           *help;
  gchar           *help_id;
  gchar           *authors;
  gchar           *copyright;
  gchar           *date;
  ClayPDBProcType  type;
  gint             n_args;
  gint             n_return_vals;
  gint             i;

  g_return_val_if_fail (CLAY_IS_PDB (pdb), NULL);
  g_return_val_if_fail (clay_is_canonical_identifier (name), NULL);

  _clay_pdb_get_proc_info (name, &type, &n_args, &n_return_vals);

  procedure = g_object_new (CLAY_TYPE_PDB_PROCEDURE,
                            "plug-in",        _clay_pdb_get_plug_in (pdb),
                            "name",           name,
                            "procedure-type", type,
                            "pdb",            pdb,
                            NULL);

  _clay_pdb_get_proc_documentation (name,      &blurb, &help, &help_id);
  clay_procedure_set_documentation (procedure,  blurb,  help,  help_id);
  g_free (blurb);
  g_free (help);
  g_free (help_id);

  _clay_pdb_get_proc_attribution (name,      &authors, &copyright, &date);
  clay_procedure_set_attribution (procedure,  authors,  copyright,  date);
  g_free (authors);
  g_free (copyright);
  g_free (date);

  for (i = 0; i < n_args; i++)
    {
      GParamSpec *pspec = _clay_pdb_get_proc_argument (name, i);

      _clay_procedure_add_argument (procedure, pspec);
    }

  for (i = 0; i < n_return_vals; i++)
    {
      GParamSpec *pspec = _clay_pdb_get_proc_return_value (name, i);

      _clay_procedure_add_return_value (procedure, pspec);
    }

  if (type != CLAY_PDB_PROC_TYPE_INTERNAL)
    {
      gchar  *string;
      gchar **menu_paths;
      gchar **path;

      string = _clay_pdb_get_proc_image_types (name);
      if (string)
        {
          clay_procedure_set_image_types (procedure, string);
          g_free (string);
        }

      string = _clay_pdb_get_proc_menu_label (name);
      if (string)
        {
          clay_procedure_set_menu_label (procedure, string);
          g_free (string);
        }

      menu_paths = _clay_pdb_get_proc_menu_paths (name);
      for (path = menu_paths; path && *path; path++)
        clay_procedure_add_menu_path (procedure, *path);
      g_strfreev (menu_paths);
    }

  return procedure;
}
