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

#include <string.h>

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "operations/clay-operation-config.h"
#include "operations/clayoperationsettings.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"
#include "core/clay-filter-history.h"
#include "core/clayimage.h"
#include "core/clayitem.h"
#include "core/clayprogress.h"

#include "display/claydisplay.h"

#include "widgets/clayaction.h"

#include "actions.h"
#include "filters-commands.h"
#include "claygeglprocedure.h"
#include "procedure-commands.h"

#include "clay-intl.h"


/*  local function prototypes  */

static gchar * filters_parse_operation (Clay          *clay,
                                        const gchar   *operation_str,
                                        const gchar   *icon_name,
                                        ClayObject   **settings);

/*  public functions  */

void
filters_apply_cmd_callback (ClayAction *action,
                            GVariant   *value,
                            gpointer    data)
{
  ClayImage     *image;
  GList         *drawables;
  gchar         *operation;
  ClayObject    *settings;
  ClayProcedure *procedure;
  GVariant      *variant;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) != 1)
    {
      /* We only support running filters on single drawable for now. */
      g_list_free (drawables);
      return;
    }

  operation = filters_parse_operation (image->clay,
                                       g_variant_get_string (value, NULL),
                                       clay_action_get_icon_name (action),
                                       &settings);

  procedure = clay_gegl_procedure_new (image->clay,
                                       NULL,
                                       CLAY_RUN_NONINTERACTIVE, settings,
                                       operation,
                                       clay_action_get_name (action),
                                       clay_action_get_label (action),
                                       clay_action_get_tooltip (action),
                                       clay_action_get_icon_name (action),
                                       clay_action_get_help_id (action));

  g_free (operation);

  if (settings)
    g_object_unref (settings);

  clay_filter_history_add (image->clay, procedure);

  variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
  g_variant_take_ref (variant);
  filters_history_cmd_callback (NULL, variant, data);

  g_variant_unref (variant);
  g_object_unref (procedure);
  g_list_free (drawables);
}

void
filters_apply_interactive_cmd_callback (ClayAction *action,
                                        GVariant   *value,
                                        gpointer    data)
{
  ClayImage     *image;
  GList         *drawables;
  ClayProcedure *procedure;
  GVariant      *variant;

  return_if_no_drawables (image, drawables, data);

  if (g_list_length (drawables) != 1)
    {
      /* We only support running filters on single drawable for now. */
      g_list_free (drawables);
      return;
    }

  procedure = clay_gegl_procedure_new (image->clay,
                                       NULL,
                                       CLAY_RUN_INTERACTIVE, NULL,
                                       g_variant_get_string (value, NULL),
                                       clay_action_get_name (action),
                                       clay_action_get_label (action),
                                       clay_action_get_tooltip (action),
                                       clay_action_get_icon_name (action),
                                       clay_action_get_help_id (action));

  clay_filter_history_add (image->clay, procedure);

  variant = g_variant_new_uint64 (GPOINTER_TO_SIZE (procedure));
  g_variant_take_ref (variant);
  filters_history_cmd_callback (NULL, variant, data);

  g_variant_unref (variant);
  g_object_unref (procedure);
  g_list_free (drawables);
}

void
filters_repeat_cmd_callback (ClayAction *action,
                             GVariant   *value,
                             gpointer    data)
{
  Clay          *clay;
  ClayDisplay   *display;
  ClayProcedure *procedure;
  ClayRunMode    run_mode;

  return_if_no_clay (clay, data);
  return_if_no_display (display, data);

  run_mode = (ClayRunMode) g_variant_get_int32 (value);

  procedure = clay_filter_history_nth (clay, 0);

  if (procedure)
    filters_run_procedure (clay, display, procedure, run_mode);
}

void
filters_history_cmd_callback (ClayAction *action,
                              GVariant   *value,
                              gpointer    data)
{
  Clay          *clay;
  ClayDisplay   *display;
  ClayProcedure *procedure;
  gsize          hack;
  return_if_no_clay (clay, data);
  return_if_no_display (display, data);

  hack = g_variant_get_uint64 (value);

  procedure = GSIZE_TO_POINTER (hack);

  filters_run_procedure (clay, display, procedure, CLAY_RUN_INTERACTIVE);
}


/*  private functions  */

static gchar *
filters_parse_operation (Clay         *clay,
                         const gchar  *operation_str,
                         const gchar  *icon_name,
                         ClayObject  **settings)
{
  const gchar *newline = strchr (operation_str, '\n');

  *settings = NULL;

  if (newline)
    {
      gchar       *operation;
      const gchar *serialized;

      operation  = g_strndup (operation_str, newline - operation_str);
      serialized = newline + 1;

      if (*serialized)
        {
          GError *error = NULL;

          *settings =
            g_object_new (clay_operation_config_get_type (clay, operation,
                                                          icon_name,
                                                          CLAY_TYPE_OPERATION_SETTINGS),
                          NULL);

          if (! clay_config_deserialize_string (CLAY_CONFIG (*settings),
                                                serialized, -1, NULL,
                                                &error))
            {
              g_warning ("filters_parse_operation: deserializing hardcoded "
                         "operation settings failed: %s",
                         error->message);
              g_clear_error (&error);

              g_object_unref (*settings);
              *settings = NULL;
            }
        }

      return operation;
    }

  return g_strdup (operation_str);
}

void
filters_run_procedure (Clay          *clay,
                       ClayDisplay   *display,
                       ClayProcedure *procedure,
                       ClayRunMode    run_mode)
{
  ClayObject     *settings = NULL;
  ClayValueArray *args;

  if (CLAY_IS_GEGL_PROCEDURE (procedure))
    {
      ClayGeglProcedure *gegl_procedure = CLAY_GEGL_PROCEDURE (procedure);

      if (gegl_procedure->default_run_mode == CLAY_RUN_NONINTERACTIVE)
        run_mode = CLAY_RUN_NONINTERACTIVE;

      settings = gegl_procedure->default_settings;
    }

  /* Prevent non-interactive or repeated filters from running on invisible
   * layers unless the user has allowed this option */
  if (run_mode == CLAY_RUN_NONINTERACTIVE)
    {
      ClayImage     *image  = clay_display_get_image (display);
      ClayGuiConfig *config = CLAY_GUI_CONFIG (display->clay->config);
      GList         *drawables;
      GList         *iter;

      drawables = clay_image_get_selected_drawables (image);
      for (iter = drawables; iter; iter = iter->next)
        {
          if (! clay_item_is_visible (CLAY_ITEM (iter->data)) &&
              ! config->edit_non_visible)
            {
              clay_message_literal (display->clay, G_OBJECT (display),
                                    CLAY_MESSAGE_ERROR,
                                    _("A selected layer is not visible."));
              g_list_free (drawables);
              return;
            }
        }
      g_list_free (drawables);
    }

  args = procedure_commands_get_display_args (procedure, display, settings);

  if (args)
    {
      gboolean success = FALSE;

      if (run_mode == CLAY_RUN_NONINTERACTIVE)
        {
          success =
            procedure_commands_run_procedure (procedure, clay,
                                              CLAY_PROGRESS (display),
                                              args);
        }
      else
        {
          success =
            procedure_commands_run_procedure_async (procedure, clay,
                                                    CLAY_PROGRESS (display),
                                                    run_mode, args,
                                                    display);
        }

      if (success &&
          (! CLAY_IS_GEGL_PROCEDURE (procedure) ||
           /* XXX ClayGeglProcedure made for filter-editing are
            * short-lived and should not be added to history. I'm not
            * sure it makes sense UX-wise anyway ("used" filters are for
            * first calls, not edits).
            * If ever we decide to change this UX logic, some code logic
            * changes are needed too. Cf. crash report #12576 and my
            * (Jehan's) technical comment in there..
            */
           ! clay_gegl_procedure_is_editing_filter (CLAY_GEGL_PROCEDURE (procedure))))
        clay_filter_history_add (clay, procedure);

      clay_value_array_unref (args);
    }
}
