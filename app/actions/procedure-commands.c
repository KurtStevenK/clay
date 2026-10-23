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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "actions-types.h"

#include "operations/clayoperationsettings.h"

#include "core/clay.h"
#include "core/clayimage.h"
#include "core/claydrawable.h"
#include "core/clayparamspecs.h"
#include "core/clayprogress.h"

#include "pdb/clayprocedure.h"

#include "display/claydisplay.h"

#include "procedure-commands.h"


static inline gboolean
CLAY_IS_PARAM_SPEC_RUN_MODE (GParamSpec *pspec)
{
  return (G_IS_PARAM_SPEC_ENUM (pspec) &&
          pspec->value_type == CLAY_TYPE_RUN_MODE);
}

ClayValueArray *
procedure_commands_get_run_mode_arg (ClayProcedure *procedure)
{
  ClayValueArray *args;
  gint            n_args = 0;

  args = clay_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (clay_value_array_index (args, n_args),
                        CLAY_RUN_INTERACTIVE);
      n_args++;
    }

  if (n_args > 0)
    clay_value_array_truncate (args, n_args);

  return args;
}

ClayValueArray *
procedure_commands_get_data_args (ClayProcedure *procedure,
                                  ClayObject    *object)
{
  ClayValueArray *args;
  gint            n_args = 0;

  args = clay_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (clay_value_array_index (args, n_args),
                        CLAY_RUN_INTERACTIVE);
      n_args++;
    }

  if (clay_value_array_length (args) > n_args &&
      G_IS_PARAM_SPEC_STRING (procedure->args[n_args]))
    {
      if (object)
        {
          g_value_set_string (clay_value_array_index (args, n_args),
                              clay_object_get_name (object));
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active data object for the plug-in!");
          clay_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args > 0)
    clay_value_array_truncate (args, n_args);

  return args;
}

ClayValueArray *
procedure_commands_get_image_args (ClayProcedure   *procedure,
                                   ClayImage       *image)
{
  ClayValueArray *args;
  gint            n_args = 0;

  args = clay_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (clay_value_array_index (args, n_args),
                        CLAY_RUN_INTERACTIVE);
      n_args++;
    }

  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (clay_value_array_index (args, n_args), image);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active image for the plug-in!");
          clay_value_array_unref (args);
          return NULL;
        }
    }

  if (n_args)
    clay_value_array_truncate (args, n_args);

  return args;
}

ClayValueArray *
procedure_commands_get_items_args (ClayProcedure *procedure,
                                   ClayImage     *image,
                                   GList         *items_list)
{
  ClayValueArray *args;
  gint            n_args = 0;

  args = clay_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (clay_value_array_index (args, n_args),
                        CLAY_RUN_INTERACTIVE);
      n_args++;
    }

  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      if (image)
        {
          g_value_set_object (clay_value_array_index (args, n_args), image);
          n_args++;

          if (clay_value_array_length (args) > n_args &&
              CLAY_IS_PARAM_SPEC_ITEM (procedure->args[n_args]))
            {
              if (items_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single item are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (clay_value_array_index (args, n_args),
                                      items_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected items for the plug-in!");
                  clay_value_array_unref (args);
                  return NULL;
                }
            }
          else if (CLAY_IS_PARAM_SPEC_CORE_OBJECT_ARRAY (procedure->args[n_args]))
            {
              ClayItem **items   = NULL;
              gint       n_items;
              GList     *iter;
              gint       i;


              n_items = g_list_length (items_list);
              items   = g_new0 (ClayItem *, n_items + 1);
              for (iter = items_list, i = 0; iter; iter = iter->next, i++)
                items[i] = iter->data;

              g_value_set_boxed (clay_value_array_index (args, n_args++), (GObject **) items);

              g_free (items);
            }
        }
    }

  if (n_args)
    clay_value_array_truncate (args, n_args);

  return args;
}

ClayValueArray *
procedure_commands_get_display_args (ClayProcedure *procedure,
                                     ClayDisplay   *display,
                                     ClayObject    *settings)
{
  ClayValueArray *args;
  gint            n_args = 0;

  args = clay_procedure_get_arguments (procedure);

  /* initialize the first argument  */
  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[n_args]))
    {
      g_value_set_enum (clay_value_array_index (args, n_args),
                        CLAY_RUN_INTERACTIVE);
      n_args++;
    }

  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_DISPLAY (procedure->args[n_args]))
    {
      if (display)
        {
          g_value_set_object (clay_value_array_index (args, n_args), display);
          n_args++;
        }
      else
        {
          g_warning ("Uh-oh, no active display for the plug-in!");
          clay_value_array_unref (args);
          return NULL;
        }
    }

  if (clay_value_array_length (args) > n_args &&
      CLAY_IS_PARAM_SPEC_IMAGE (procedure->args[n_args]))
    {
      ClayImage *image = display ? clay_display_get_image (display) : NULL;

      if (image)
        {
          GList *drawables_list = clay_image_get_selected_drawables (image);

          g_value_set_object (clay_value_array_index (args, n_args), image);
          n_args++;

          if (clay_value_array_length (args) > n_args &&
              CLAY_IS_PARAM_SPEC_DRAWABLE (procedure->args[n_args]))
            {
              if (drawables_list)
                {
                  g_printerr ("%s: plug-in procedures expecting a single drawable are deprecated!\n",
                              G_STRFUNC);
                  g_value_set_object (clay_value_array_index (args, n_args),
                                      drawables_list->data);
                  n_args++;
                }
              else
                {
                  g_warning ("Uh-oh, no selected drawables for the plug-in!");

                  clay_value_array_unref (args);
                  g_list_free (drawables_list);

                  return NULL;
                }
            }
          else if (clay_value_array_length (args) > n_args &&
                   CLAY_IS_PARAM_SPEC_CORE_OBJECT_ARRAY (procedure->args[n_args]))
            {
              ClayDrawable **drawables   = NULL;
              gint           n_drawables;
              GList         *iter;
              gint           i;

              n_drawables = g_list_length (drawables_list);

              drawables = g_new0 (ClayDrawable *, n_drawables + 1);
              for (iter = drawables_list, i = 0; iter; iter = iter->next, i++)
                drawables[i] = iter->data;

              g_value_set_boxed (clay_value_array_index (args, n_args++), (GObject **) drawables);

              g_free (drawables);
            }
          g_list_free (drawables_list);
        }
    }

  /* Some filters have a settings object (see filters_settings_actions
   * list), which we want to pass around, but we don't want to pass
   * other types of object data.
   */
  if (clay_value_array_length (args) > n_args &&
      g_type_is_a (G_PARAM_SPEC_VALUE_TYPE (procedure->args[n_args]),
                   CLAY_TYPE_OPERATION_SETTINGS))
    {
      g_value_set_object (clay_value_array_index (args, n_args), settings);
      n_args++;
    }

  if (n_args)
    clay_value_array_truncate (args, n_args);

  return args;
}

gboolean
procedure_commands_run_procedure (ClayProcedure  *procedure,
                                  Clay           *clay,
                                  ClayProgress   *progress,
                                  ClayValueArray *args)
{
  ClayValueArray *return_vals;
  GError         *error = NULL;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (clay_value_array_length (args) > 0 &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (clay_value_array_index (args, 0),
                      CLAY_RUN_NONINTERACTIVE);

  return_vals = clay_procedure_execute (procedure, clay,
                                        clay_get_user_context (clay),
                                        progress, args,
                                        &error);
  clay_value_array_unref (return_vals);

  if (error)
    {
      clay_message_literal (clay,
                            G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}

gboolean
procedure_commands_run_procedure_async (ClayProcedure  *procedure,
                                        Clay           *clay,
                                        ClayProgress   *progress,
                                        ClayRunMode     run_mode,
                                        ClayValueArray *args,
                                        ClayDisplay    *display)
{
  GError *error = NULL;

  g_return_val_if_fail (CLAY_IS_PROCEDURE (procedure), FALSE);
  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (display == NULL || CLAY_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (args != NULL, FALSE);

  if (clay_value_array_length (args) > 0 &&
      CLAY_IS_PARAM_SPEC_RUN_MODE (procedure->args[0]))
    g_value_set_enum (clay_value_array_index (args, 0),
                      run_mode);

  clay_procedure_execute_async (procedure, clay,
                                clay_get_user_context (clay),
                                progress, args,
                                display, &error);

  if (error)
    {
      clay_message_literal (clay,
                            G_OBJECT (progress), CLAY_MESSAGE_ERROR,
                            error->message);
      g_clear_error (&error);

      return FALSE;
    }

  return TRUE;
}
