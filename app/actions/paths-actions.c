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

#include "libclaywidgets/claywidgets.h"

#include "actions-types.h"

#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/clayimage.h"

#include "widgets/clayactiongroup.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claywidgets-utils.h"

#include "actions.h"
#include "items-actions.h"
#include "paths-actions.h"
#include "paths-commands.h"

#include "clay-intl.h"


static const ClayActionEntry paths_actions[] =
{
  { "paths-edit", CLAY_ICON_TOOL_PATH,
    NC_("paths-action", "Edit Pa_th"), NULL, { NULL },
    NC_("paths-action", "Edit the active path"),
    paths_edit_cmd_callback,
    CLAY_HELP_TOOL_PATH },

  { "paths-edit-attributes", CLAY_ICON_EDIT,
    NC_("paths-action", "_Edit Path Attributes..."), NULL, { NULL },
    NC_("paths-action", "Edit path attributes"),
    paths_edit_attributes_cmd_callback,
    CLAY_HELP_PATH_EDIT },

  { "paths-new", CLAY_ICON_DOCUMENT_NEW,
    NC_("paths-action", "_New Path..."), NULL, { NULL },
    NC_("paths-action", "Create a new path..."),
    paths_new_cmd_callback,
    CLAY_HELP_PATH_NEW },

  { "paths-new-last-values", CLAY_ICON_DOCUMENT_NEW,
    NC_("paths-action", "_New Path with last values"), NULL, { NULL },
    NC_("paths-action", "Create a new path with last used values"),
    paths_new_last_vals_cmd_callback,
    CLAY_HELP_PATH_NEW },

  { "paths-duplicate", CLAY_ICON_OBJECT_DUPLICATE,
    NC_("paths-action", "D_uplicate Paths"), NULL, { NULL },
    NC_("paths-action", "Duplicate these paths"),
    paths_duplicate_cmd_callback,
    CLAY_HELP_PATH_DUPLICATE },

  { "paths-delete", CLAY_ICON_EDIT_DELETE,
    NC_("paths-action", "_Delete Paths"), NULL, { NULL },
    NC_("paths-action", "Delete the selected paths"),
    paths_delete_cmd_callback,
    CLAY_HELP_PATH_DELETE },

  { "paths-merge-visible", NULL,
    NC_("paths-action", "Merge _Visible Paths"), NULL, { NULL }, NULL,
    paths_merge_visible_cmd_callback,
    CLAY_HELP_PATH_MERGE_VISIBLE },

  { "paths-raise", CLAY_ICON_GO_UP,
    NC_("paths-action", "_Raise Paths"), NULL, { NULL },
    NC_("paths-action", "Raise the selected paths"),
    paths_raise_cmd_callback,
    CLAY_HELP_PATH_RAISE },

  { "paths-raise-to-top", CLAY_ICON_GO_TOP,
    NC_("paths-action", "Raise Paths to _Top"), NULL, { NULL },
    NC_("paths-action", "Raise the selected paths to the top"),
    paths_raise_to_top_cmd_callback,
    CLAY_HELP_PATH_RAISE_TO_TOP },

  { "paths-lower", CLAY_ICON_GO_DOWN,
    NC_("paths-action", "_Lower Paths"), NULL, { NULL },
    NC_("paths-action", "Lower the selected paths"),
    paths_lower_cmd_callback,
    CLAY_HELP_PATH_LOWER },

  { "paths-lower-to-bottom", CLAY_ICON_GO_BOTTOM,
    NC_("paths-action", "Lower Paths to _Bottom"), NULL, { NULL },
    NC_("paths-action", "Lower the selected paths to the bottom"),
    paths_lower_to_bottom_cmd_callback,
    CLAY_HELP_PATH_LOWER_TO_BOTTOM },

  { "paths-fill", CLAY_ICON_TOOL_BUCKET_FILL,
    NC_("paths-action", "Fill Pat_hs..."), NULL, { NULL },
    NC_("paths-action", "Fill the paths"),
    paths_fill_cmd_callback,
    CLAY_HELP_PATH_FILL },

  { "paths-fill-last-values", CLAY_ICON_TOOL_BUCKET_FILL,
    NC_("paths-action", "Fill Paths"), NULL, { NULL },
    NC_("paths-action", "Fill the paths with last values"),
    paths_fill_last_vals_cmd_callback,
    CLAY_HELP_PATH_FILL },

  { "paths-stroke", CLAY_ICON_PATH_STROKE,
    NC_("paths-action", "Stro_ke Paths..."), NULL, { NULL },
    NC_("paths-action", "Paint along the paths"),
    paths_stroke_cmd_callback,
    CLAY_HELP_PATH_STROKE },

  { "paths-stroke-last-values", CLAY_ICON_PATH_STROKE,
    NC_("paths-action", "Stro_ke Paths"), NULL, { NULL },
    NC_("paths-action", "Paint along the paths with last values"),
    paths_stroke_last_vals_cmd_callback,
    CLAY_HELP_PATH_STROKE },

  { "paths-copy", CLAY_ICON_EDIT_COPY,
    NC_("paths-action", "Co_py Paths"), NULL, { NULL }, NULL,
    paths_copy_cmd_callback,
    CLAY_HELP_PATH_COPY },

  { "paths-paste", CLAY_ICON_EDIT_PASTE,
    NC_("paths-action", "Paste Pat_h"), NULL, { NULL }, NULL,
    paths_paste_cmd_callback,
    CLAY_HELP_PATH_PASTE },

  { "paths-import", CLAY_ICON_DOCUMENT_OPEN,
    NC_("paths-action", "I_mport Path..."), NULL, { NULL }, NULL,
    paths_import_cmd_callback,
    CLAY_HELP_PATH_IMPORT },

  { "paths-export-selected", CLAY_ICON_DOCUMENT_SAVE_AS,
    NC_("paths-action", "E_xport Selected Paths..."), NULL, { NULL }, NULL,
    paths_export_selected_cmd_callback,
    CLAY_HELP_PATH_EXPORT },

  { "paths-export-all", CLAY_ICON_DOCUMENT_SAVE,
    NC_("paths-action", "Export _All Paths..."), NULL, { NULL }, NULL,
    paths_export_all_cmd_callback,
    CLAY_HELP_PATH_EXPORT },

  { "paths-to-vector-layer", NULL,
    NC_("paths-action", "Path to Vector Layer"), NULL, { NULL }, NULL,
    path_to_vector_layer_cmd_callback,
    CLAY_HELP_PATH_TO_VECTOR_LAYER },
};

static const ClayToggleActionEntry paths_toggle_actions[] =
{
  { "paths-visible", CLAY_ICON_VISIBLE,
    NC_("paths-action", "Toggle Path _Visibility"), NULL, { NULL }, NULL,
    paths_visible_cmd_callback,
    FALSE,
    CLAY_HELP_PATH_VISIBLE },

  { "paths-lock-content", CLAY_ICON_LOCK_CONTENT,
    NC_("paths-action", "L_ock Strokes of Path"), NULL, { NULL }, NULL,
    paths_lock_content_cmd_callback,
    FALSE,
    CLAY_HELP_PATH_LOCK_STROKES },

  { "paths-lock-position", CLAY_ICON_LOCK_POSITION,
    NC_("paths-action", "L_ock Position of Path"), NULL, { NULL }, NULL,
    paths_lock_position_cmd_callback,
    FALSE,
    CLAY_HELP_PATH_LOCK_POSITION }
};

static const ClayEnumActionEntry paths_color_tag_actions[] =
{
  { "paths-color-tag-none", CLAY_ICON_EDIT_CLEAR,
    NC_("paths-action", "None"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Clear"),
    CLAY_COLOR_TAG_NONE, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-blue", NULL,
    NC_("paths-action", "Blue"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Blue"),
    CLAY_COLOR_TAG_BLUE, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-green", NULL,
    NC_("paths-action", "Green"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Green"),
    CLAY_COLOR_TAG_GREEN, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-yellow", NULL,
    NC_("paths-action", "Yellow"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Yellow"),
    CLAY_COLOR_TAG_YELLOW, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-orange", NULL,
    NC_("paths-action", "Orange"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Orange"),
    CLAY_COLOR_TAG_ORANGE, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-brown", NULL,
    NC_("paths-action", "Brown"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Brown"),
    CLAY_COLOR_TAG_BROWN, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-red", NULL,
    NC_("paths-action", "Red"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Red"),
    CLAY_COLOR_TAG_RED, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-violet", NULL,
    NC_("paths-action", "Violet"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Violet"),
    CLAY_COLOR_TAG_VIOLET, FALSE,
    CLAY_HELP_PATH_COLOR_TAG },

  { "paths-color-tag-gray", NULL,
    NC_("paths-action", "Gray"), NULL, { NULL },
    NC_("paths-action", "Path Color Tag: Set to Gray"),
    CLAY_COLOR_TAG_GRAY, FALSE,
    CLAY_HELP_PATH_COLOR_TAG }
};

static const ClayEnumActionEntry paths_to_selection_actions[] =
{
  { "paths-selection-replace", CLAY_ICON_SELECTION_REPLACE,
    NC_("paths-action", "Paths to Sele_ction"), NULL, { NULL },
    NC_("paths-action", "Path to selection"),
    CLAY_CHANNEL_OP_REPLACE, FALSE,
    CLAY_HELP_PATH_SELECTION_REPLACE },

  { "paths-selection-from-paths", CLAY_ICON_SELECTION_REPLACE,
    NC_("paths-action", "Selection Fr_om Paths"), NULL, { "<shift>V", NULL },
    NC_("paths-action", "Replace selection with paths"),
    CLAY_CHANNEL_OP_REPLACE, FALSE,
    CLAY_HELP_PATH_SELECTION_REPLACE },

  { "paths-selection-add", CLAY_ICON_SELECTION_ADD,
    NC_("paths-action", "_Add Paths to Selection"), NULL, { NULL },
    NC_("paths-action", "Add paths to selection"),
    CLAY_CHANNEL_OP_ADD, FALSE,
    CLAY_HELP_PATH_SELECTION_ADD },

  { "paths-selection-subtract", CLAY_ICON_SELECTION_SUBTRACT,
    NC_("paths-action", "_Subtract Paths from Selection"), NULL, { NULL },
    NC_("paths-action", "Subtract paths from selection"),
    CLAY_CHANNEL_OP_SUBTRACT, FALSE,
    CLAY_HELP_PATH_SELECTION_SUBTRACT },

  { "paths-selection-intersect", CLAY_ICON_SELECTION_INTERSECT,
    NC_("paths-action", "_Intersect Paths with Selection"), NULL, { NULL },
    NC_("paths-action", "Intersect paths with selection"),
    CLAY_CHANNEL_OP_INTERSECT, FALSE,
    CLAY_HELP_PATH_SELECTION_INTERSECT }
};

static const ClayEnumActionEntry paths_selection_to_paths_actions[] =
{
  { "paths-selection-to-path", CLAY_ICON_SELECTION_TO_PATH,
    NC_("paths-action", "Selecti_on to Path"),
    NC_("paths-action", "To _Path"), { NULL },
    NC_("paths-action", "Selection to path"),
    FALSE, FALSE,
    CLAY_HELP_SELECTION_TO_PATH },

  { "paths-selection-to-path-advanced", CLAY_ICON_SELECTION_TO_PATH,
    NC_("paths-action", "Selection to Path (_Advanced)"), NULL, { NULL },
    NC_("paths-action", "Advanced options"),
    TRUE, FALSE,
    CLAY_HELP_SELECTION_TO_PATH }
};

static const ClayEnumActionEntry paths_select_actions[] =
{
  { "paths-select-top", NULL,
    NC_("paths-action", "Select _Top Path"), NULL, { NULL },
    NC_("paths-action", "Select the topmost path"),
    CLAY_ACTION_SELECT_FIRST, FALSE,
    CLAY_HELP_PATH_TOP },

  { "paths-select-bottom", NULL,
    NC_("paths-action", "Select _Bottom Path"), NULL, { NULL },
    NC_("paths-action", "Select the bottommost path"),
    CLAY_ACTION_SELECT_LAST, FALSE,
    CLAY_HELP_PATH_BOTTOM },

  { "paths-select-previous", NULL,
    NC_("paths-action", "Select _Previous Path"), NULL, { NULL },
    NC_("paths-action", "Select the path above the current path"),
    CLAY_ACTION_SELECT_PREVIOUS, FALSE,
    CLAY_HELP_PATH_PREVIOUS },

  { "paths-select-next", NULL,
    NC_("paths-action", "Select _Next Path"), NULL, { NULL },
    NC_("paths-action", "Select the path below the current path"),
    CLAY_ACTION_SELECT_NEXT, FALSE,
    CLAY_HELP_PATH_NEXT }
};

void
paths_actions_setup (ClayActionGroup *group)
{
  clay_action_group_add_actions (group, "paths-action",
                                 paths_actions,
                                 G_N_ELEMENTS (paths_actions));

  clay_action_group_add_toggle_actions (group, "paths-action",
                                        paths_toggle_actions,
                                        G_N_ELEMENTS (paths_toggle_actions));

  clay_action_group_add_enum_actions (group, "paths-action",
                                      paths_color_tag_actions,
                                      G_N_ELEMENTS (paths_color_tag_actions),
                                      paths_color_tag_cmd_callback);

  clay_action_group_add_enum_actions (group, "paths-action",
                                      paths_to_selection_actions,
                                      G_N_ELEMENTS (paths_to_selection_actions),
                                      paths_to_selection_cmd_callback);

  clay_action_group_add_enum_actions (group, "paths-action",
                                      paths_selection_to_paths_actions,
                                      G_N_ELEMENTS (paths_selection_to_paths_actions),
                                      paths_selection_to_paths_cmd_callback);

  clay_action_group_add_enum_actions (group, "paths-action",
                                      paths_select_actions,
                                      G_N_ELEMENTS (paths_select_actions),
                                      paths_select_cmd_callback);

  items_actions_setup (group, "paths");
}

void
paths_actions_update (ClayActionGroup *group,
                      gpointer         data)
{
  ClayImage    *image            = action_data_get_image (data);
  GList        *selected_path    = NULL;
  gint          n_selected_paths = 0;

  gint          n_paths       = 0;
  gboolean      mask_empty    = TRUE;
  gboolean      dr_writable   = FALSE;
  gboolean      dr_children   = FALSE;

  gboolean      have_prev     = FALSE; /* At least 1 selected path has a previous sibling. */
  gboolean      have_next     = FALSE; /* At least 1 selected path has a next sibling.     */
  gboolean      first_selected      = FALSE; /* First channel is selected */
  gboolean      last_selected       = FALSE; /* Last channel is selected */

  if (image)
    {
      GList *drawables;
      GList *iter;

      n_paths    = clay_image_get_n_paths (image);
      mask_empty = clay_channel_is_empty (clay_image_get_mask (image));

      selected_path    = clay_image_get_selected_paths (image);
      n_selected_paths = g_list_length (selected_path);

      for (iter = selected_path; iter; iter = iter->next)
        {
          GList *paths_list;
          GList *iter2;

          paths_list = clay_item_get_container_iter (CLAY_ITEM (iter->data));
          iter2 = g_list_find (paths_list, iter->data);

          if (iter2)
            {
              if (clay_item_get_index (iter2->data) == 0)
                first_selected = TRUE;
              if (clay_item_get_index (iter2->data) == n_paths - 1)
                last_selected = TRUE;

              if (g_list_previous (iter2))
                have_prev = TRUE;

              if (g_list_next (iter2))
                have_next = TRUE;
            }
        }

      drawables = clay_image_get_selected_drawables (image);

      if (g_list_length (drawables) == 1)
        {
          dr_writable = ! clay_item_is_content_locked (CLAY_ITEM (drawables->data), NULL);

          if (clay_viewable_get_children (CLAY_VIEWABLE (drawables->data)))
            dr_children = TRUE;
        }

      g_list_free (drawables);
    }

#define SET_SENSITIVE(action,condition) \
        clay_action_group_set_action_sensitive (group, action, (condition) != 0, NULL)
#define SET_ACTIVE(action,condition) \
        clay_action_group_set_action_active (group, action, (condition) != 0)

  SET_SENSITIVE ("paths-edit",            n_selected_paths == 1);
  SET_SENSITIVE ("paths-edit-attributes", n_selected_paths == 1);

  SET_SENSITIVE ("paths-new",             image);
  SET_SENSITIVE ("paths-new-last-values", image);
  SET_SENSITIVE ("paths-duplicate",       n_selected_paths > 0);
  SET_SENSITIVE ("paths-delete",          n_selected_paths > 0);
  SET_SENSITIVE ("paths-merge-visible",   n_paths > 1);

  SET_SENSITIVE ("paths-raise",           n_selected_paths > 0 && have_prev && !first_selected);
  SET_SENSITIVE ("paths-raise-to-top",    n_selected_paths > 0 && have_prev);
  SET_SENSITIVE ("paths-lower",           n_selected_paths > 0 && have_next && !last_selected);
  SET_SENSITIVE ("paths-lower-to-bottom", n_selected_paths > 0 && have_next);

  SET_SENSITIVE ("paths-copy",            n_selected_paths > 0);
  SET_SENSITIVE ("paths-paste",           image);
  SET_SENSITIVE ("paths-import",          image);
  SET_SENSITIVE ("paths-export-selected", n_selected_paths > 0);
  SET_SENSITIVE ("paths-export-all",      n_paths > 0);

  SET_SENSITIVE ("paths-to-vector-layer", n_selected_paths == 1);

  SET_SENSITIVE ("paths-selection-to-path",          image && !mask_empty);
  SET_SENSITIVE ("paths-selection-to-path-advanced", image && !mask_empty);
  SET_SENSITIVE ("paths-fill",                       n_selected_paths > 0 &&
                                                       dr_writable &&
                                                       !dr_children);
  SET_SENSITIVE ("paths-fill-last-values",           n_selected_paths > 0 &&
                                                       dr_writable &&
                                                       !dr_children);
  SET_SENSITIVE ("paths-stroke",                     n_selected_paths > 0 &&
                                                       dr_writable &&
                                                       !dr_children);
  SET_SENSITIVE ("paths-stroke-last-values",         n_selected_paths > 0 &&
                                                       dr_writable &&
                                                       !dr_children);

  SET_SENSITIVE ("paths-selection-replace",      n_selected_paths > 0);
  SET_SENSITIVE ("paths-selection-from-paths",   n_selected_paths > 0);
  SET_SENSITIVE ("paths-selection-add",          n_selected_paths > 0);
  SET_SENSITIVE ("paths-selection-subtract",     n_selected_paths > 0);
  SET_SENSITIVE ("paths-selection-intersect",    n_selected_paths > 0);

  SET_SENSITIVE ("paths-select-top",       n_selected_paths > 0 && have_prev);
  SET_SENSITIVE ("paths-select-bottom",    n_selected_paths > 0 && have_next);
  SET_SENSITIVE ("paths-select-previous",  n_selected_paths > 0 && have_prev);
  SET_SENSITIVE ("paths-select-next",      n_selected_paths > 0 && have_next);

#undef SET_SENSITIVE
#undef SET_ACTIVE

  items_actions_update (group, "paths", selected_path);
}
