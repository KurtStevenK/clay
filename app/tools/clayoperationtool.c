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
#include "libclaycolor/claycolor.h"
#include "libclayconfig/clayconfig.h"
#include "libclaywidgets/claywidgets.h"

#include "tools-types.h"

#include "gegl/clay-gegl-utils.h"

#include "core/claychannel.h"
#include "core/claycontainer.h"
#include "core/claycontext.h"
#include "core/claydrawable.h"
#include "core/clayerror.h"
#include "core/clayimage.h"
#include "core/claylist.h"
#include "core/claypickable.h"
#include "core/claysettings.h"

#include "widgets/claybuffersourcebox.h"
#include "widgets/clayhelp-ids.h"
#include "widgets/claypickablebutton.h"

#include "propgui/claypropgui.h"

#include "display/claydisplay.h"
#include "display/claytoolgui.h"

#include "clayfilteroptions.h"
#include "clayoperationtool.h"

#include "clay-intl.h"


typedef struct _AuxInput AuxInput;

struct _AuxInput
{
  ClayOperationTool *tool;
  gchar             *pad;
  GeglNode          *node;
  GtkWidget         *box;
};


/*  local function prototypes  */

static void        clay_operation_tool_finalize        (GObject           *object);

static gboolean    clay_operation_tool_initialize      (ClayTool          *tool,
                                                        ClayDisplay       *display,
                                                        GError           **error);
static void        clay_operation_tool_control         (ClayTool          *tool,
                                                        ClayToolAction     action,
                                                        ClayDisplay       *display);

static gchar     * clay_operation_tool_get_operation   (ClayFilterTool    *filter_tool,
                                                        gchar            **description);
static void        clay_operation_tool_dialog          (ClayFilterTool    *filter_tool);
static void        clay_operation_tool_reset           (ClayFilterTool    *filter_tool);
static void        clay_operation_tool_set_config      (ClayFilterTool    *filter_tool,
                                                        ClayConfig        *config);
static void        clay_operation_tool_region_changed  (ClayFilterTool    *filter_tool);
static void        clay_operation_tool_color_picked    (ClayFilterTool    *filter_tool,
                                                        gpointer           identifier,
                                                        gdouble            x,
                                                        gdouble            y,
                                                        const Babl        *sample_format,
                                                        GeglColor         *color);

static void        clay_operation_tool_halt            (ClayOperationTool *op_tool);
static void        clay_operation_tool_commit          (ClayOperationTool *op_tool);

static void        clay_operation_tool_sync_op         (ClayOperationTool *op_tool,
                                                        gboolean           sync_colors);
static void        clay_operation_tool_create_gui      (ClayOperationTool *tool);
static void        clay_operation_tool_add_gui         (ClayOperationTool *tool);

static AuxInput *  clay_operation_tool_aux_input_new   (ClayOperationTool *tool,
                                                        GeglNode          *operation,
                                                        const gchar       *input_pad,
                                                        const gchar       *label,
                                                        const gchar       *description);
static void        clay_operation_tool_aux_input_detach(AuxInput          *input);
static void        clay_operation_tool_aux_input_clear (AuxInput          *input);
static void        clay_operation_tool_aux_input_free  (AuxInput          *input);

static void        clay_operation_tool_unlink_chains   (ClayOperationTool *op_tool);
static void        clay_operation_tool_relink_chains   (ClayOperationTool *op_tool);


G_DEFINE_TYPE (ClayOperationTool, clay_operation_tool,
               CLAY_TYPE_FILTER_TOOL)

#define parent_class clay_operation_tool_parent_class


void
clay_operation_tool_register (ClayToolRegisterCallback  callback,
                              gpointer                  data)
{
  (* callback) (CLAY_TYPE_OPERATION_TOOL,
                CLAY_TYPE_FILTER_OPTIONS,
                clay_color_options_gui,
                CLAY_CONTEXT_PROP_MASK_FOREGROUND |
                CLAY_CONTEXT_PROP_MASK_BACKGROUND,
                "clay-operation-tool",
                _("GEGL Operation"),
                _("Operation Tool: Use an arbitrary GEGL operation"),
                NULL, NULL,
                NULL, CLAY_HELP_TOOL_GEGL,
                CLAY_ICON_GEGL,
                data);
}

static void
clay_operation_tool_class_init (ClayOperationToolClass *klass)
{
  GObjectClass        *object_class      = G_OBJECT_CLASS (klass);
  ClayToolClass       *tool_class        = CLAY_TOOL_CLASS (klass);
  ClayFilterToolClass *filter_tool_class = CLAY_FILTER_TOOL_CLASS (klass);

  object_class->finalize            = clay_operation_tool_finalize;

  tool_class->initialize            = clay_operation_tool_initialize;
  tool_class->control               = clay_operation_tool_control;

  filter_tool_class->get_operation  = clay_operation_tool_get_operation;
  filter_tool_class->dialog         = clay_operation_tool_dialog;
  filter_tool_class->reset          = clay_operation_tool_reset;
  filter_tool_class->set_config     = clay_operation_tool_set_config;
  filter_tool_class->region_changed = clay_operation_tool_region_changed;
  filter_tool_class->color_picked   = clay_operation_tool_color_picked;
}

static void
clay_operation_tool_init (ClayOperationTool *op_tool)
{
}

static void
clay_operation_tool_finalize (GObject *object)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (object);

  g_clear_pointer (&op_tool->operation,   g_free);
  g_clear_pointer (&op_tool->description, g_free);

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) clay_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gboolean
clay_operation_tool_initialize (ClayTool     *tool,
                                ClayDisplay  *display,
                                GError      **error)
{
  if (CLAY_TOOL_CLASS (parent_class)->initialize (tool, display, error))
    {
      ClayFilterTool    *filter_tool = CLAY_FILTER_TOOL (tool);
      ClayOperationTool *op_tool     = CLAY_OPERATION_TOOL (tool);

      if (filter_tool->config)
        {
          GtkWidget *options_gui;

          clay_operation_tool_sync_op (op_tool, TRUE);
          options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
          if (! options_gui)
            {
              clay_operation_tool_create_gui (op_tool);
              clay_operation_tool_add_gui (op_tool);
            }
          else
            {
              g_object_unref (options_gui);
            }
        }

      return TRUE;
    }

  return FALSE;
}

static void
clay_operation_tool_control (ClayTool       *tool,
                             ClayToolAction  action,
                             ClayDisplay    *display)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (tool);

  switch (action)
    {
    case CLAY_TOOL_ACTION_PAUSE:
    case CLAY_TOOL_ACTION_RESUME:
      break;

    case CLAY_TOOL_ACTION_HALT:
      clay_operation_tool_halt (op_tool);
      break;

    case CLAY_TOOL_ACTION_COMMIT:
      clay_operation_tool_commit (op_tool);
      break;
    }

  CLAY_TOOL_CLASS (parent_class)->control (tool, action, display);
}

static gchar *
clay_operation_tool_get_operation (ClayFilterTool  *filter_tool,
                                   gchar          **description)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (filter_tool);

  *description = g_strdup (op_tool->description);

  return g_strdup (op_tool->operation);
}

static void
clay_operation_tool_dialog (ClayFilterTool *filter_tool)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (filter_tool);
  GtkWidget         *main_vbox;
  GtkWidget         *options_sw;
  GtkWidget         *options_gui;
  GtkWidget         *options_box;

  main_vbox = clay_filter_tool_dialog_get_vbox (filter_tool);

  /*  The options scrolled window  */
  options_sw = gtk_scrolled_window_new (NULL, NULL);
  g_weak_ref_set (&op_tool->options_sw_ref, options_sw);
  gtk_scrolled_window_set_overlay_scrolling (GTK_SCROLLED_WINDOW (options_sw),
                                             FALSE);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (options_sw),
                                       GTK_SHADOW_NONE);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (options_sw),
                                  GTK_POLICY_NEVER, GTK_POLICY_NEVER);
  gtk_box_pack_start (GTK_BOX (main_vbox), options_sw,
                      TRUE, TRUE, 0);
  gtk_widget_set_visible (options_sw, TRUE);

  /*  The options vbox  */
  options_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  g_weak_ref_set (&op_tool->options_box_ref, options_box);
  gtk_container_add (GTK_CONTAINER (options_sw), options_box);
  gtk_widget_set_visible (options_box, TRUE);

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  if (options_gui)
    {
      clay_operation_tool_add_gui (op_tool);
      g_object_unref (options_gui);
    }
}

static void
clay_operation_tool_reset (ClayFilterTool *filter_tool)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (filter_tool);

  clay_operation_tool_unlink_chains (op_tool);

  CLAY_FILTER_TOOL_CLASS (parent_class)->reset (filter_tool);

  if (filter_tool->config && CLAY_TOOL (op_tool)->drawables)
    clay_operation_tool_sync_op (op_tool, TRUE);

  clay_operation_tool_relink_chains (op_tool);
}

static void
clay_operation_tool_set_config (ClayFilterTool *filter_tool,
                                ClayConfig     *config)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (filter_tool);

  clay_operation_tool_unlink_chains (op_tool);

  CLAY_FILTER_TOOL_CLASS (parent_class)->set_config (filter_tool, config);

  if (filter_tool->config && CLAY_TOOL (op_tool)->drawables)
    clay_operation_tool_sync_op (op_tool, FALSE);

  clay_operation_tool_relink_chains (op_tool);
}

static void
clay_operation_tool_region_changed (ClayFilterTool *filter_tool)
{
  ClayOperationTool *op_tool = CLAY_OPERATION_TOOL (filter_tool);

  /* when the region changes, do we want the operation's on-canvas
   * controller to move to a new position, or the operation to
   * change its properties to match the on-canvas controller?
   *
   * decided to leave the on-canvas controller where it is and
   * pretend it has changed, so the operation is updated
   * accordingly...
   */
  if (filter_tool->widget)
    g_signal_emit_by_name (filter_tool->widget, "changed");

  clay_operation_tool_sync_op (op_tool, FALSE);
}

static void
clay_operation_tool_color_picked (ClayFilterTool  *filter_tool,
                                  gpointer         identifier,
                                  gdouble          x,
                                  gdouble          y,
                                  const Babl      *sample_format,
                                  GeglColor       *color)
{
  gchar **pspecs = g_strsplit (identifier, ":", 2);

  if (pspecs[1])
    {
      GObjectClass  *object_class = G_OBJECT_GET_CLASS (filter_tool->config);
      GParamSpec    *pspec_x;
      GParamSpec    *pspec_y;
      gint           off_x, off_y;
      GeglRectangle  area;

      clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

      x -= off_x + area.x;
      y -= off_y + area.y;

      pspec_x = g_object_class_find_property (object_class, pspecs[0]);
      pspec_y = g_object_class_find_property (object_class, pspecs[1]);

      if (pspec_x && pspec_y &&
          G_PARAM_SPEC_TYPE (pspec_x) == G_PARAM_SPEC_TYPE (pspec_y))
        {
          GValue value_x = G_VALUE_INIT;
          GValue value_y = G_VALUE_INIT;

          g_value_init (&value_x, G_PARAM_SPEC_VALUE_TYPE (pspec_x));
          g_value_init (&value_y, G_PARAM_SPEC_VALUE_TYPE (pspec_y));

#define HAS_KEY(p,k,v) clay_gegl_param_spec_has_key (p, k, v)

          if (HAS_KEY (pspec_x, "unit", "relative-coordinate") &&
              HAS_KEY (pspec_y, "unit", "relative-coordinate"))
            {
              x /= (gdouble) area.width;
              y /= (gdouble) area.height;
            }

          if (G_IS_PARAM_SPEC_INT (pspec_x))
            {
              g_value_set_int (&value_x, x);
              g_value_set_int (&value_y, y);

              g_param_value_validate (pspec_x, &value_x);
              g_param_value_validate (pspec_y, &value_y);

              g_object_set (filter_tool->config,
                            pspecs[0], g_value_get_int (&value_x),
                            pspecs[1], g_value_get_int (&value_y),
                            NULL);
            }
          else if (G_IS_PARAM_SPEC_DOUBLE (pspec_x))
            {
              g_value_set_double (&value_x, x);
              g_value_set_double (&value_y, y);

              g_param_value_validate (pspec_x, &value_x);
              g_param_value_validate (pspec_y, &value_y);

              g_object_set (filter_tool->config,
                            pspecs[0], g_value_get_double (&value_x),
                            pspecs[1], g_value_get_double (&value_y),
                            NULL);
            }
          else
            {
              g_warning ("%s: unhandled param spec of type %s",
                         G_STRFUNC, G_PARAM_SPEC_TYPE_NAME (pspec_x));
            }

          g_value_unset (&value_x);
          g_value_unset (&value_y);
        }
    }
  else
    {
      g_object_set (filter_tool->config,
                    pspecs[0], color,
                    NULL);
    }

  g_strfreev (pspecs);
}

static void
clay_operation_tool_halt (ClayOperationTool *op_tool)
{
  /*  don't reset op_tool->operation and op_tool->description so the
   *  tool can be properly restarted by clicking on an image
   */

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) clay_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;
}

static void
clay_operation_tool_commit (ClayOperationTool *op_tool)
{
  /*  remove the aux input boxes from the dialog, so they don't get
   *  destroyed when the parent class runs its commit()
   */

  g_list_foreach (op_tool->aux_inputs,
                  (GFunc) clay_operation_tool_aux_input_detach, NULL);
}

static void
clay_operation_tool_sync_op (ClayOperationTool *op_tool,
                             gboolean           sync_colors)
{
  ClayFilterTool   *filter_tool = CLAY_FILTER_TOOL (op_tool);
  ClayToolOptions  *options     = CLAY_TOOL_GET_OPTIONS (op_tool);
  GParamSpec      **pspecs;
  guint             n_pspecs;
  gint              off_x, off_y;
  GeglRectangle     area;
  gint              i;

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  pspecs = g_object_class_list_properties (G_OBJECT_GET_CLASS (filter_tool->config),
                                           &n_pspecs);

  for (i = 0; i < n_pspecs; i++)
    {
      GParamSpec *pspec = pspecs[i];

#define HAS_KEY(p,k,v) clay_gegl_param_spec_has_key (p, k, v)

      if (HAS_KEY (pspec, "role", "output-extent"))
        {
          if (HAS_KEY (pspec, "unit", "pixel-coordinate") &&
              HAS_KEY (pspec, "axis", "x"))
            {
              g_object_set (filter_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-coordinate") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (filter_tool->config, pspec->name, 0, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "x"))
            {
              g_object_set (filter_tool->config, pspec->name, area.width, NULL);
            }
          else if (HAS_KEY (pspec, "unit", "pixel-distance") &&
                   HAS_KEY (pspec, "axis", "y"))
            {
              g_object_set (filter_tool->config, pspec->name, area.height, NULL);
            }
        }
      else if (sync_colors)
        {
          GeglColor *color = NULL;

          if (HAS_KEY (pspec, "role", "color-primary"))
            color = clay_context_get_foreground (CLAY_CONTEXT (options));
          else if (sync_colors && HAS_KEY (pspec, "role", "color-secondary"))
            color = clay_context_get_background (CLAY_CONTEXT (options));

          if (color != NULL)
            g_object_set (filter_tool->config, pspec->name, color, NULL);
        }
    }

  g_free (pspecs);
}

static void
clay_operation_tool_create_gui (ClayOperationTool *op_tool)
{
  ClayFilterTool  *filter_tool = CLAY_FILTER_TOOL (op_tool);
  GtkWidget       *options_gui;
  gint             off_x, off_y;
  GeglRectangle    area;
  gchar          **input_pads;

  clay_filter_tool_get_drawable_area (filter_tool, &off_x, &off_y, &area);

  options_gui =
    clay_prop_gui_new (G_OBJECT (filter_tool->config),
                       G_TYPE_FROM_INSTANCE (filter_tool->config), 0,
                       &area,
                       CLAY_CONTEXT (CLAY_TOOL_GET_OPTIONS (op_tool)),
                       (ClayCreatePickerFunc) clay_filter_tool_add_color_picker,
                       (ClayCreateControllerFunc) clay_filter_tool_add_controller,
                       filter_tool);
  g_weak_ref_set (&op_tool->options_gui_ref, options_gui);

  input_pads = gegl_node_list_input_pads (filter_tool->operation);

  if (input_pads)
    {
      gint i;

      for (i = 0; input_pads[i]; i++)
        {
          AuxInput *input;
          GRegex   *regex;
          gchar    *label;

          if (! strcmp (input_pads[i], "input"))
            continue;

          regex = g_regex_new ("^aux(\\d*)$", 0, 0, NULL);

          g_return_if_fail (regex != NULL);

          label = g_strdup (gegl_node_get_pad_label (filter_tool->operation, input_pads[i]));

          if (label == NULL)
            label = g_regex_replace (regex,
                                     input_pads[i], -1, 0,
                                     /* Translators: don't translate "Aux" */
                                     _("Aux\\1 Input"),
                                     0, NULL);

          input = clay_operation_tool_aux_input_new (op_tool,
                                                     filter_tool->operation,
                                                     input_pads[i], label,
                                                     gegl_node_get_pad_description (filter_tool->operation,
                                                                                    input_pads[i]));

          op_tool->aux_inputs = g_list_prepend (op_tool->aux_inputs, input);

          g_free (label);

          g_regex_unref (regex);
        }

      g_strfreev (input_pads);
    }
}

static void
clay_operation_tool_add_gui (ClayOperationTool *op_tool)
{
  GtkSizeGroup   *size_group  = NULL;
  GtkWidget      *options_gui;
  GtkWidget      *options_box;
  GtkWidget      *options_sw;
  GtkWidget      *shell;
  GdkRectangle    workarea;
  GtkRequisition  minimum;
  GList          *list;
  gboolean        scrolling;

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  options_box = g_weak_ref_get (&op_tool->options_box_ref);
  options_sw  = g_weak_ref_get (&op_tool->options_sw_ref);
  g_return_if_fail (options_gui && options_box && options_sw);

  for (list = op_tool->aux_inputs; list; list = g_list_next (list))
    {
      AuxInput  *input = list->data;
      GtkWidget *toggle;

      if (! size_group)
        size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

      toggle =
        clay_buffer_source_box_get_toggle (CLAY_BUFFER_SOURCE_BOX (input->box));

      gtk_size_group_add_widget (size_group, toggle);

      gtk_box_pack_start (GTK_BOX (options_box), input->box,
                          FALSE, FALSE, 0);
      gtk_widget_set_visible (input->box, TRUE);
    }

  if (size_group)
    g_object_unref (size_group);

  gtk_box_pack_start (GTK_BOX (options_box), options_gui, TRUE, TRUE, 0);
  gtk_widget_set_visible (options_gui, TRUE);

  shell = GTK_WIDGET (clay_display_get_shell (CLAY_TOOL (op_tool)->display));
  gdk_monitor_get_workarea (clay_widget_get_monitor (shell), &workarea);
  gtk_widget_get_preferred_size (options_box, &minimum, NULL);

  scrolling = minimum.height > workarea.height / 2;

  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (options_sw),
                                  GTK_POLICY_NEVER,
                                  scrolling ?
                                  GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER);

  if (scrolling)
    gtk_widget_set_size_request (options_sw, -1, workarea.height / 2);
  else
    gtk_widget_set_size_request (options_sw, -1, -1);

  g_object_unref (options_gui);
  g_object_unref (options_box);
  g_object_unref (options_sw);
}


/*  aux input utility functions  */

static void
clay_operation_tool_aux_input_notify (ClayBufferSourceBox *box,
                                      const GParamSpec    *pspec,
                                      AuxInput            *input)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (input->tool);

  /* emit "notify" so ClayFilterTool will update its preview
   *
   * FIXME: this is a bad hack that should go away once ClayImageMap
   * and ClayFilterTool are refactored to be more filter-ish.
   */
  if (filter_tool->config)
    g_signal_emit_by_name (filter_tool->config,
                           "notify", NULL);
}

static AuxInput *
clay_operation_tool_aux_input_new (ClayOperationTool *op_tool,
                                   GeglNode          *operation,
                                   const gchar       *input_pad,
                                   const gchar       *label,
                                   const gchar       *description)
{
  AuxInput    *input = g_slice_new (AuxInput);
  ClayContext *context;

  input->tool = op_tool;
  input->pad  = g_strdup (input_pad);
  input->node = gegl_node_new_child (NULL,
                                     "operation", "gegl:buffer-source",
                                     NULL);

  gegl_node_connect (input->node, "output",
                     operation,   input_pad);

  context = CLAY_CONTEXT (CLAY_TOOL_GET_OPTIONS (op_tool));

  input->box = clay_buffer_source_box_new (context, input->node, label);
  if (description)
    gtk_widget_set_tooltip_text (input->box, description);

  /* make AuxInput owner of the box */
  g_object_ref_sink (input->box);

  g_signal_connect (input->box, "notify::pickable",
                    G_CALLBACK (clay_operation_tool_aux_input_notify),
                    input);
  g_signal_connect (input->box, "notify::enabled",
                    G_CALLBACK (clay_operation_tool_aux_input_notify),
                    input);

  return input;
}

static void
clay_operation_tool_aux_input_detach (AuxInput *input)
{
  GtkWidget *parent = gtk_widget_get_parent (input->box);

  if (parent)
    gtk_container_remove (GTK_CONTAINER (parent), input->box);
}

static void
clay_operation_tool_aux_input_clear (AuxInput *input)
{
  clay_operation_tool_aux_input_detach (input);

  g_object_set (input->box,
                "pickable", NULL,
                NULL);
}

static void
clay_operation_tool_aux_input_free (AuxInput *input)
{
  clay_operation_tool_aux_input_clear (input);

  g_free (input->pad);
  g_object_unref (input->node);
  g_object_unref (input->box);

  g_slice_free (AuxInput, input);
}

static void
clay_operation_tool_unlink_chains (ClayOperationTool *op_tool)
{
  GObject *options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  GList   *chains;

  g_return_if_fail (options_gui != NULL);

  chains = g_object_get_data (options_gui, "chains");

  while (chains)
    {
      ClayChainButton *chain = chains->data;
      gboolean         active;

      active = clay_chain_button_get_active (chain);

      g_object_set_data (G_OBJECT (chain), "was-active",
                         GINT_TO_POINTER (active));

      if (active)
        {
          clay_chain_button_set_active (chain, FALSE);
        }

      chains = chains->next;
    }

  g_object_unref (options_gui);
}

static void
clay_operation_tool_relink_chains (ClayOperationTool *op_tool)
{
  ClayFilterTool *filter_tool = CLAY_FILTER_TOOL (op_tool);
  GObject        *options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  GList          *chains;

  g_return_if_fail (options_gui != NULL);

  chains = g_object_get_data (options_gui, "chains");

  while (chains)
    {
      ClayChainButton *chain = chains->data;

      if (g_object_get_data (G_OBJECT (chain), "was-active"))
        {
          const gchar *name_x    = g_object_get_data (chains->data, "x-property");
          const gchar *name_y    = g_object_get_data (chains->data, "y-property");
          const gchar *names[2]  = { name_x, name_y };
          GValue       values[2] = { G_VALUE_INIT, G_VALUE_INIT };
          GValue       double_x  = G_VALUE_INIT;
          GValue       double_y  = G_VALUE_INIT;

          g_object_getv (filter_tool->config, 2, names, values);

          g_value_init (&double_x, G_TYPE_DOUBLE);
          g_value_init (&double_y, G_TYPE_DOUBLE);

          if (g_value_transform (&values[0], &double_x) &&
              g_value_transform (&values[1], &double_y) &&
              g_value_get_double (&double_x) ==
              g_value_get_double (&double_y))
            {
              clay_chain_button_set_active (chain, TRUE);
            }

          g_value_unset (&double_x);
          g_value_unset (&double_y);
          g_value_unset (&values[0]);
          g_value_unset (&values[1]);

          g_object_set_data (G_OBJECT (chain), "was-active", NULL);
        }

      chains = chains->next;
    }

  g_object_unref (options_gui);
}


/*  public functions  */

void
clay_operation_tool_set_operation (ClayOperationTool  *op_tool,
                                   ClayDrawableFilter *filter,
                                   const gchar        *operation,
                                   const gchar        *title,
                                   const gchar        *description,
                                   const gchar        *undo_desc,
                                   const gchar        *icon_name,
                                   const gchar        *help_id)
{
  ClayTool       *tool;
  ClayFilterTool *filter_tool;
  GtkWidget      *options_gui;

  g_return_if_fail (CLAY_IS_OPERATION_TOOL (op_tool));

  tool        = CLAY_TOOL (op_tool);
  filter_tool = CLAY_FILTER_TOOL (op_tool);

  g_free (op_tool->operation);
  g_free (op_tool->description);

  op_tool->operation   = g_strdup (operation);
  op_tool->description = g_strdup (description);

  clay_tool_set_label     (tool, title);
  clay_tool_set_undo_desc (tool, undo_desc);
  clay_tool_set_icon_name (tool, icon_name);
  clay_tool_set_help_id   (tool, help_id);

  g_list_free_full (op_tool->aux_inputs,
                    (GDestroyNotify) clay_operation_tool_aux_input_free);
  op_tool->aux_inputs = NULL;

  clay_filter_tool_set_widget (filter_tool, NULL);

  options_gui = g_weak_ref_get (&op_tool->options_gui_ref);
  if (options_gui)
    {
      clay_filter_tool_disable_color_picking (filter_tool);
      g_object_unref (options_gui);
      gtk_widget_destroy (options_gui);
    }

  if (! operation)
    return;

  clay_filter_tool_get_operation (filter_tool, filter);

  /* Update filter name for GeglOperation tool presets */
  if (filter_tool->filter && description)
    g_object_set (filter_tool->filter,
                  "name", description,
                  NULL);

  if (tool->drawables)
    clay_operation_tool_sync_op (op_tool, TRUE);

  if (filter_tool->config && tool->display)
    {
      GtkWidget *options_box;

      clay_operation_tool_create_gui (op_tool);

      options_box = g_weak_ref_get (&op_tool->options_box_ref);
      if (options_box)
        {
          clay_operation_tool_add_gui (op_tool);
          g_object_unref (options_box);
        }
    }
}
