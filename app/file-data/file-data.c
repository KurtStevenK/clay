/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include "libclaybase/claybase.h"

#include "core/core-types.h"

#include "core/clay.h"
#include "core/claydrawable.h"
#include "core/clayparamspecs.h"

#include "plug-in/claypluginmanager.h"
#include "plug-in/claypluginprocedure.h"

#include "file-data.h"
#include "file-data-gbr.h"
#include "file-data-gex.h"
#include "file-data-gih.h"
#include "file-data-pat.h"

#include "clay-intl.h"


void
file_data_init (Clay *clay)
{
  ClayPlugInProcedure *proc;
  GFile               *file;
  ClayProcedure       *procedure;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  /*  file-gbr-load  */
  file = g_file_new_for_path ("file-gbr-load");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_gbr_load_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay brush"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-brush",
                                   strlen ("clay-brush") + 1,
                                   NULL);
  clay_plug_in_procedure_set_image_types (proc, NULL);
  clay_plug_in_procedure_set_file_proc (proc, "gbr, gbp", NULL, "",
                                        "20, string, Clay");
  clay_plug_in_procedure_set_mime_types (proc, "image/clay-x-gbr");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "file-gbr-load");
  clay_procedure_set_static_help (procedure,
                                  "Loads Clay brushes",
                                  "Loads Clay brushes (1 or 4 bpp "
                                  "and old .gpb format)",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Jens Lautenbacher, "
                                         "Sven Neumann, Michael Natterer",
                                         "Tim Newsome, Jens Lautenbacher, "
                                         "Sven Neumann, Michael Natterer",
                                         "1995-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));

  clay_procedure_add_return_value (procedure,
                                   clay_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gbr-export-internal  */
  file = g_file_new_for_path ("file-gbr-export-internal");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_gbr_save_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay brush"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-brush",
                                   strlen ("clay-brush") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  clay_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  clay_plug_in_procedure_set_file_proc (proc, "gbr", NULL, "", NULL);
  clay_plug_in_procedure_set_mime_types (proc, "image/x-clay-gbr");
  clay_plug_in_procedure_set_handles_remote (proc);
#endif

  clay_object_set_static_name (CLAY_OBJECT (procedure),
                               "file-gbr-export-internal");
  clay_procedure_set_static_help (procedure,
                                  "Exports Clay brush file (.GBR)",
                                  "Exports Clay brush file (.GBR)",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1995-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_core_object_array ("drawables",
                                                                  "Drawables",
                                                                  "Selected drawables",
                                                                  CLAY_TYPE_DRAWABLE,
                                                                  CLAY_PARAM_READWRITE | CLAY_PARAM_NO_VALIDATE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "Spacing of the brush",
                                                 1, 1000, 10,
                                                 CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "brush",
                                                       FALSE, FALSE, TRUE,
                                                       "Clay Brush",
                                                       CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gih-load  */
  file = g_file_new_for_path ("file-gih-load");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_gih_load_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay brush (animated)"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-brush",
                                   strlen ("clay-brush") + 1,
                                   NULL);
  clay_plug_in_procedure_set_image_types (proc, NULL);
  clay_plug_in_procedure_set_file_proc (proc, "gih", NULL, "", "");
  clay_plug_in_procedure_set_mime_types (proc, "image/clay-x-gih");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "file-gih-load");
  clay_procedure_set_static_help (procedure,
                                  "Loads Clay animated brushes",
                                  "This procedure loads a Clay brush "
                                  "pipe as an image.",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tor Lillqvist, Michael Natterer",
                                         "Tor Lillqvist, Michael Natterer",
                                         "1999-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));

  clay_procedure_add_return_value (procedure,
                                   clay_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gih-export-internal  */
  file = g_file_new_for_path ("file-gih-export-internal");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_gih_save_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay brush (animated)"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-brush",
                                   strlen ("clay-brush") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  clay_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  clay_plug_in_procedure_set_file_proc (proc, "gih", NULL, "", NULL);
  clay_plug_in_procedure_set_mime_types (proc, "image/x-clay-gih");
  clay_plug_in_procedure_set_handles_remote (proc);
#endif

  clay_object_set_static_name (CLAY_OBJECT (procedure),
                               "file-gih-export-internal");
  clay_procedure_set_static_help (procedure,
                                  "Exports Clay animated brush file (.gih)",
                                  "Exports Clay animated brush file (.gih)",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tor Lillqvist, Michael Natterer",
                                         "Tor Lillqvist, Michael Natterer",
                                         "1999-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_core_object_array ("drawables",
                                                                  "drawables",
                                                                  "Drawables to save",
                                                                  CLAY_TYPE_DRAWABLE,
                                                                  CLAY_PARAM_READWRITE | CLAY_PARAM_NO_VALIDATE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_int ("spacing",
                                                 "spacing",
                                                 "Spacing of the brush",
                                                 1, 1000, 10,
                                                 CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "brush",
                                                       FALSE, FALSE, TRUE,
                                                       "Clay Brush",
                                                       CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_string ("params",
                                                       "params",
                                                       "The pipe's parameters",
                                                       FALSE, FALSE, TRUE,
                                                       NULL,
                                                       CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-pat-load  */
  file = g_file_new_for_path ("file-pat-load");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_pat_load_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay pattern"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-pattern",
                                   strlen ("clay-pattern") + 1,
                                   NULL);
  clay_plug_in_procedure_set_image_types (proc, NULL);
  clay_plug_in_procedure_set_file_proc (proc, "pat", NULL, "",
                                        "20,string,GPAT");
  clay_plug_in_procedure_set_mime_types (proc, "image/clay-x-pat");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "file-pat-load");
  clay_procedure_set_static_help (procedure,
                                  "Loads Clay patterns",
                                  "Loads Clay patterns",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1997-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));
  clay_procedure_add_return_value (procedure,
                                   clay_param_spec_image ("image",
                                                          "Image",
                                                          "Output image",
                                                          FALSE,
                                                          CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-pat-export-internal  */
  file = g_file_new_for_path ("file-pat-export-internal");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_pat_save_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay pattern"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-pattern",
                                   strlen ("clay-pattern") + 1,
                                   NULL);

#if 0
  /* do not register as file procedure */
  clay_plug_in_procedure_set_image_types (proc, "RGB*, GRAY*, INDEXED*");
  clay_plug_in_procedure_set_file_proc (proc, "pat", NULL, "", NULL);
  clay_plug_in_procedure_set_mime_types (proc, "image/x-clay-pat");
  clay_plug_in_procedure_set_handles_remote (proc);
#endif

  clay_object_set_static_name (CLAY_OBJECT (procedure),
                               "file-pat-export-internal");
  clay_procedure_set_static_help (procedure,
                                  "Exports Clay pattern file (.PAT)",
                                  "Exports Clay pattern file (.PAT)",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Tim Newsome, Michael Natterer",
                                         "Tim Newsome, Michael Natterer",
                                         "1995-2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_image ("image",
                                                      "Image",
                                                      "Input image",
                                                      FALSE,
                                                      CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_core_object_array ("drawables",
                                                                  "Drawables",
                                                                  "Selected drawables",
                                                                  CLAY_TYPE_DRAWABLE,
                                                                  CLAY_PARAM_READWRITE | CLAY_PARAM_NO_VALIDATE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to export",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               clay_param_spec_string ("name",
                                                       "name",
                                                       "The name of the "
                                                       "pattern",
                                                       FALSE, FALSE, TRUE,
                                                       "Clay Pattern",
                                                       CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);

  /*  file-gex-load  */
  file = g_file_new_for_path ("file-gex-load");
  procedure = clay_plug_in_procedure_new (CLAY_PDB_PROC_TYPE_INTERNAL, file, NULL);
  g_object_unref (file);

  procedure->marshal_func = file_gex_load_invoker;

  proc = CLAY_PLUG_IN_PROCEDURE (procedure);
  proc->menu_label = g_strdup (_("Clay extension"));
  clay_plug_in_procedure_set_icon (proc, CLAY_ICON_TYPE_ICON_NAME,
                                   (const guint8 *) "clay-plugin",
                                   strlen ("clay-plugin") + 1,
                                   NULL);
  clay_plug_in_procedure_set_file_proc (proc, "gex", NULL, "",
                                        "20, string, Clay");
  clay_plug_in_procedure_set_generic_file_proc (proc, TRUE);
  clay_plug_in_procedure_set_mime_types (proc, "image/clay-x-gex");
  clay_plug_in_procedure_set_handles_remote (proc);

  clay_object_set_static_name (CLAY_OBJECT (procedure), "file-gex-load");
  clay_procedure_set_static_help (procedure,
                                  "Loads Clay extension",
                                  "Loads Clay extension",
                                  NULL);
  clay_procedure_set_static_attribution (procedure,
                                         "Jehan", "Jehan", "2019");

  clay_procedure_add_argument (procedure,
                               clay_param_spec_enum ("run-mode",
                                                     "Dummy Param",
                                                     "Dummy parameter",
                                                     CLAY_TYPE_RUN_MODE,
                                                     CLAY_RUN_INTERACTIVE,
                                                     CLAY_PARAM_READWRITE));
  clay_procedure_add_argument (procedure,
                               g_param_spec_object ("file",
                                                    "File",
                                                    "The file to load",
                                                    G_TYPE_FILE,
                                                    CLAY_PARAM_READWRITE));

  clay_procedure_add_return_value (procedure,
                                   clay_param_spec_string ("extension-id",
                                                           "ID of installed extension",
                                                           "Identifier of the newly installed extension",
                                                           FALSE, TRUE, FALSE, NULL,
                                                           CLAY_PARAM_READWRITE));

  clay_plug_in_manager_add_procedure (clay->plug_in_manager, proc);
  g_object_unref (procedure);
}

void
file_data_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));
}
