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

#include <stdlib.h>
#include <string.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"

#include "core-types.h"

#include "path/clayvectorlayer.h"

#include "text/claytextlayer.h"

#include "clay.h"
#include "clay-parasites.h"
#include "claychannel.h"
#include "claycontainer.h"
#include "clayidtable.h"
#include "clayimage.h"
#include "clayimage-undo.h"
#include "clayimage-undo-push.h"
#include "clayitem.h"
#include "clayitem-preview.h"
#include "clayitemtree.h"
#include "claylinklayer.h"
#include "claylist.h"
#include "clayparasitelist.h"
#include "clayprogress.h"
#include "claysavable.h"
#include "claysavable-load.h"
#include "claystrokeoptions.h"

#include "paint/claypaintoptions.h"

#include "clay-intl.h"


enum
{
  REMOVED,
  VISIBILITY_CHANGED,
  COLOR_TAG_CHANGED,
  LOCK_CONTENT_CHANGED,
  LOCK_POSITION_CHANGED,
  LOCK_VISIBILITY_CHANGED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_IMAGE,
  PROP_ID,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_OFFSET_X,
  PROP_OFFSET_Y,
  PROP_VISIBLE,
  PROP_COLOR_TAG,
  PROP_LOCK_CONTENT,
  PROP_LOCK_POSITION,
  PROP_LOCK_VISIBILITY,
  N_PROPS
};


typedef struct _ClayItemPrivate ClayItemPrivate;

struct _ClayItemPrivate
{
  gint              ID;                          /*  provides a unique ID        */
  guint32           tattoo;                      /*  provides a permanent ID     */

  ClayImage        *image;                       /*  item owner                  */

  ClayParasiteList *parasites;                   /*  Plug-in parasite data       */

  gint              width, height;               /*  size in pixels              */
  gint              offset_x, offset_y;          /*  pixel offset in image       */

  guint             visible                : 1;  /*  item visibility             */
  guint             bind_visible_to_active : 1;  /*  visibility bound to active  */

  guint             lock_content           : 1;  /*  content editability         */
  guint             lock_position          : 1;  /*  content movability          */
  guint             lock_visibility        : 1;  /*  automatic visibility change */

  guint             removed                : 1;  /*  removed from the image?     */

  ClayColorTag      color_tag;                   /*  color tag                   */

  GList            *offset_nodes;                /*  offset nodes to manage      */
};

#define GET_PRIVATE(item) ((ClayItemPrivate *) clay_item_get_instance_private ((ClayItem *) (item)))


/*  local function prototypes  */

static void       clay_item_savable_iface_init      (ClaySavableInterface      *iface);

static void       clay_item_constructed             (GObject                   *object);
static void       clay_item_finalize                (GObject                   *object);
static void       clay_item_set_property            (GObject                   *object,
                                                     guint                      property_id,
                                                     const GValue              *value,
                                                     GParamSpec                *pspec);
static void       clay_item_get_property            (GObject                   *object,
                                                     guint                      property_id,
                                                     GValue                    *value,
                                                     GParamSpec                *pspec);

static gint64     clay_item_get_memsize             (ClayObject                *object,
                                                     gint64                    *gui_size);

static void       clay_item_save                    (ClaySavable               *savable,
                                                     ClaySaveState             *state);
static gboolean   clay_item_load_enter              (ClayLoadState             *state,
                                                     const gchar              **attribute_names,
                                                     const gchar              **attribute_values,
                                                     gpointer                   user_data,
                                                     GError                   **error);
static gboolean   clay_item_load_exit               (ClayLoadState             *state,
                                                     const gchar               *text,
                                                     gsize                      len,
                                                     gpointer                   user_data,
                                                     GError                   **error);

static gboolean   clay_item_real_is_content_locked  (ClayItem                  *item,
                                                     ClayItem                 **locked_item,
                                                     gboolean                   check_children);
static gboolean   clay_item_real_is_position_locked (ClayItem                  *item,
                                                     ClayItem                 **locked_item,
                                                     gboolean                   check_children);
static gboolean clay_item_real_is_visibility_locked (ClayItem                  *item,
                                                     ClayItem                 **locked_item);
static gboolean   clay_item_real_bounds             (ClayItem                  *item,
                                                     gdouble                   *x,
                                                     gdouble                   *y,
                                                     gdouble                   *width,
                                                     gdouble                   *height);
static ClayItem * clay_item_real_duplicate          (ClayItem                  *item,
                                                     GType                      new_type);
static void       clay_item_real_convert            (ClayItem                  *item,
                                                     ClayImage                 *dest_image,
                                                     GType                      old_type);
static gboolean   clay_item_real_rename             (ClayItem                  *item,
                                                     const gchar               *new_name,
                                                     const gchar               *undo_desc,
                                                     GError                   **error);
static void       clay_item_real_start_transform    (ClayItem                  *item,
                                                     gboolean                   push_undo);
static void       clay_item_real_end_transform      (ClayItem                  *item,
                                                     gboolean                   push_undo);
static void       clay_item_real_translate          (ClayItem                  *item,
                                                     gdouble                    offset_x,
                                                     gdouble                    offset_y,
                                                     gboolean                   push_undo);
static void       clay_item_real_scale              (ClayItem                  *item,
                                                     gint                       new_width,
                                                     gint                       new_height,
                                                     gint                       new_offset_x,
                                                     gint                       new_offset_y,
                                                     ClayInterpolationType      interpolation,
                                                     ClayProgress              *progress);
static void       clay_item_real_resize             (ClayItem                  *item,
                                                     ClayContext               *context,
                                                     ClayFillType               fill_type,
                                                     gint                       new_width,
                                                     gint                       new_height,
                                                     gint                       offset_x,
                                                     gint                       offset_y);
static ClayTransformResize
                  clay_item_real_get_clip           (ClayItem                  *item,
                                                     ClayTransformResize        clip_result);



G_DEFINE_TYPE_WITH_CODE (ClayItem, clay_item, CLAY_TYPE_FILTER,
                         G_ADD_PRIVATE (ClayItem)
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_SAVABLE,
                                                clay_item_savable_iface_init))

#define parent_class clay_item_parent_class

static guint clay_item_signals[LAST_SIGNAL] = { 0 };
static GParamSpec *clay_item_props[N_PROPS] = { NULL, };


static void
clay_item_class_init (ClayItemClass *klass)
{
  GObjectClass      *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass   *clay_object_class = CLAY_OBJECT_CLASS (klass);
  ClayViewableClass *viewable_class    = CLAY_VIEWABLE_CLASS (klass);

  clay_item_signals[REMOVED] =
    g_signal_new ("removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, removed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_item_signals[VISIBILITY_CHANGED] =
    g_signal_new ("visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_item_signals[COLOR_TAG_CHANGED] =
    g_signal_new ("color-tag-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, color_tag_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_item_signals[LOCK_CONTENT_CHANGED] =
    g_signal_new ("lock-content-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, lock_content_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_item_signals[LOCK_POSITION_CHANGED] =
    g_signal_new ("lock-position-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, lock_position_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  clay_item_signals[LOCK_VISIBILITY_CHANGED] =
    g_signal_new ("lock-visibility-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (ClayItemClass, lock_visibility_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  object_class->constructed        = clay_item_constructed;
  object_class->finalize           = clay_item_finalize;
  object_class->set_property       = clay_item_set_property;
  object_class->get_property       = clay_item_get_property;

  clay_object_class->get_memsize   = clay_item_get_memsize;

  viewable_class->name_editable    = TRUE;
  viewable_class->get_preview_size = clay_item_get_preview_size;
  viewable_class->get_popup_size   = clay_item_get_popup_size;

  klass->removed                   = NULL;
  klass->visibility_changed        = NULL;
  klass->color_tag_changed         = NULL;
  klass->lock_content_changed      = NULL;
  klass->lock_position_changed     = NULL;
  klass->lock_visibility_changed   = NULL;

  klass->unset_removed             = NULL;
  klass->is_attached               = NULL;
  klass->is_content_locked         = clay_item_real_is_content_locked;
  klass->is_position_locked        = clay_item_real_is_position_locked;
  klass->is_visibility_locked      = clay_item_real_is_visibility_locked;
  klass->get_tree                  = NULL;
  klass->bounds                    = clay_item_real_bounds;
  klass->duplicate                 = clay_item_real_duplicate;
  klass->convert                   = clay_item_real_convert;
  klass->rename                    = clay_item_real_rename;
  klass->start_move                = NULL;
  klass->end_move                  = NULL;
  klass->start_transform           = clay_item_real_start_transform;
  klass->end_transform             = clay_item_real_end_transform;
  klass->translate                 = clay_item_real_translate;
  klass->scale                     = clay_item_real_scale;
  klass->resize                    = clay_item_real_resize;
  klass->flip                      = NULL;
  klass->rotate                    = NULL;
  klass->transform                 = NULL;
  klass->get_clip                  = clay_item_real_get_clip;
  klass->fill                      = NULL;
  klass->stroke                    = NULL;
  klass->to_selection              = NULL;

  klass->rename_desc               = NULL;
  klass->translate_desc            = NULL;
  klass->scale_desc                = NULL;
  klass->resize_desc               = NULL;
  klass->flip_desc                 = NULL;
  klass->rotate_desc               = NULL;
  klass->transform_desc            = NULL;
  klass->fill_desc                 = NULL;
  klass->stroke_desc               = NULL;

  clay_item_props[PROP_IMAGE] = g_param_spec_object ("image", NULL, NULL,
                                                     CLAY_TYPE_IMAGE,
                                                     CLAY_PARAM_READWRITE    |
                                                     G_PARAM_EXPLICIT_NOTIFY |
                                                     G_PARAM_CONSTRUCT);
  clay_item_props[PROP_ID] = g_param_spec_int ("id", NULL, NULL,
                                               0, G_MAXINT, 0,
                                               CLAY_PARAM_READABLE);

  clay_item_props[PROP_WIDTH] = g_param_spec_int ("width", NULL, NULL,
                                                  1, CLAY_MAX_IMAGE_SIZE, 1,
                                                  CLAY_PARAM_READABLE);

  clay_item_props[PROP_HEIGHT] = g_param_spec_int ("height", NULL, NULL,
                                                   1, CLAY_MAX_IMAGE_SIZE, 1,
                                                   CLAY_PARAM_READABLE);

  clay_item_props[PROP_OFFSET_X] = g_param_spec_int ("offset-x", NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE, 0,
                                                     CLAY_PARAM_READABLE);

  clay_item_props[PROP_OFFSET_Y] = g_param_spec_int ("offset-y", NULL, NULL,
                                                     -CLAY_MAX_IMAGE_SIZE,
                                                     CLAY_MAX_IMAGE_SIZE, 0,
                                                     CLAY_PARAM_READABLE);

  clay_item_props[PROP_VISIBLE] = g_param_spec_boolean ("visible", NULL, NULL,
                                                        TRUE,
                                                        CLAY_PARAM_READABLE);

  clay_item_props[PROP_COLOR_TAG] = g_param_spec_enum ("color-tag", NULL, NULL,
                                                       CLAY_TYPE_COLOR_TAG,
                                                       CLAY_COLOR_TAG_NONE,
                                                       CLAY_PARAM_READABLE);

  clay_item_props[PROP_LOCK_CONTENT] = g_param_spec_boolean ("lock-content",
                                                             NULL, NULL,
                                                             FALSE,
                                                             CLAY_PARAM_READABLE);

  clay_item_props[PROP_LOCK_POSITION] = g_param_spec_boolean ("lock-position",
                                                              NULL, NULL,
                                                              FALSE,
                                                              CLAY_PARAM_READABLE);

  clay_item_props[PROP_LOCK_VISIBILITY] = g_param_spec_boolean ("lock-visibility",
                                                                NULL, NULL,
                                                                FALSE,
                                                                CLAY_PARAM_READABLE);

  g_object_class_install_properties (object_class, N_PROPS, clay_item_props);
}

static void
clay_item_savable_iface_init (ClaySavableInterface *iface)
{
  iface->tag        = NULL;
  iface->save       = clay_item_save;
  iface->load_enter = clay_item_load_enter;
  iface->load_exit  = clay_item_load_exit;
}

static void
clay_item_init (ClayItem *item)
{
  ClayItemPrivate *private = GET_PRIVATE (item);

  g_object_force_floating (G_OBJECT (item));

  private->ID                     = 0;
  private->parasites              = clay_parasite_list_new ();
  private->visible                = TRUE;
  private->bind_visible_to_active = TRUE;
}

static void
clay_item_constructed (GObject *object)
{
  ClayItemPrivate *private = GET_PRIVATE (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_IMAGE (private->image));
  clay_assert (private->ID != 0);
}

static void
clay_item_finalize (GObject *object)
{
  ClayItemPrivate *private = GET_PRIVATE (object);

  if (private->offset_nodes)
    {
      g_list_free_full (private->offset_nodes,
                        (GDestroyNotify) g_object_unref);
      private->offset_nodes = NULL;
    }

  if (private->image && private->image->clay)
    {
      clay_id_table_remove (private->image->clay->item_table, private->ID);
      private->image = NULL;
    }

  g_clear_object (&private->parasites);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
clay_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  ClayItem *item = CLAY_ITEM (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      clay_item_set_image (item, g_value_get_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_item_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  ClayItemPrivate *private = GET_PRIVATE (object);

  switch (property_id)
    {
    case PROP_IMAGE:
      g_value_set_object (value, private->image);
      break;
    case PROP_ID:
      g_value_set_int (value, private->ID);
      break;
    case PROP_WIDTH:
      g_value_set_int (value, private->width);
      break;
    case PROP_HEIGHT:
      g_value_set_int (value, private->height);
      break;
    case PROP_OFFSET_X:
      g_value_set_int (value, private->offset_x);
      break;
    case PROP_OFFSET_Y:
      g_value_set_int (value, private->offset_y);
      break;
    case PROP_VISIBLE:
      g_value_set_boolean (value, private->visible);
      break;
    case PROP_COLOR_TAG:
      g_value_set_enum (value, private->color_tag);
      break;
    case PROP_LOCK_CONTENT:
      g_value_set_boolean (value, private->lock_content);
      break;
    case PROP_LOCK_POSITION:
      g_value_set_boolean (value, private->lock_position);
      break;
    case PROP_LOCK_VISIBILITY:
      g_value_set_boolean (value, private->lock_visibility);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gint64
clay_item_get_memsize (ClayObject *object,
                       gint64     *gui_size)
{
  ClayItemPrivate *private = GET_PRIVATE (object);
  gint64           memsize = 0;

  memsize += clay_object_get_memsize (CLAY_OBJECT (private->parasites),
                                      gui_size);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

static void
clay_item_save (ClaySavable   *savable,
                ClaySaveState *state)
{
  ClayItem         *item  = CLAY_ITEM (savable);
  ClayImage        *image = clay_item_get_image (item);
  ClayParasiteList *parasites;

  clay_savable_print_element (state,
                              "name", "%s", clay_object_get_name (CLAY_OBJECT (item)),
                              NULL);

  if (g_list_find (clay_image_get_selected_items (image,
                                                  G_TYPE_FROM_INSTANCE (item)),
                   item))
    clay_savable_print_element (state, "selected", NULL, NULL, NULL);

  clay_savable_print_element (state, "visible", "%b",
                              clay_item_get_visible (item), NULL);
  clay_savable_print_element (state, "tattoo", "%u",
                              (guint) clay_item_get_tattoo (item), NULL);
  clay_savable_print_element (state, "color-tag", "%[ClayColorTag]",
                              clay_item_get_color_tag (item),
                              NULL);

  if (clay_item_get_lock_content (item))
    clay_savable_print_element (state, "lock-content", NULL, NULL, NULL);
  if (clay_item_get_lock_position (item))
    clay_savable_print_element (state, "lock-position", NULL, NULL, NULL);
  if (clay_item_get_lock_visibility (item))
    clay_savable_print_element (state, "lock-visibility", NULL, NULL, NULL);

  parasites = clay_item_get_parasites (item);
  if (clay_parasite_list_length (parasites) > 0)
    clay_savable_save (CLAY_SAVABLE (parasites), state);
}

static gboolean
clay_item_load_enter (ClayLoadState  *state,
                      const gchar   **attribute_names,
                      const gchar   **attribute_values,
                      gpointer        user_data,
                      GError        **error)
{
  clay_savable_load_add_simple_handler (state, "name", G_TYPE_STRING,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "selected", G_TYPE_NONE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "visible", G_TYPE_BOOLEAN,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "tattoo", G_TYPE_UINT,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "color-tag", CLAY_TYPE_COLOR_TAG,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "lock-content", G_TYPE_NONE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "lock-position", G_TYPE_NONE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);
  clay_savable_load_add_simple_handler (state, "lock-visibility", G_TYPE_NONE,
                                        NULL, NULL, NULL, FALSE, FALSE, NULL);

  /* TODO: add parasite loading. */

  return TRUE;
}

static gboolean
clay_item_load_exit (ClayLoadState  *state,
                     const gchar    *text,
                     gsize           len,
                     gpointer        user_data,
                     GError        **error)
{
  ClayItem     *item;
  const gchar  *name            = NULL;
  gboolean      selected        = FALSE;
  gboolean      visible         = FALSE;
  guint         tattoo          = 0;
  ClayColorTag  tag             = CLAY_COLOR_TAG_NONE;
  gboolean      lock_content    = FALSE;
  gboolean      lock_position   = FALSE;
  gboolean      lock_visibility = FALSE;

  g_return_val_if_fail (CLAY_IS_ITEM (clay_savable_load_peek_active_object (state)), FALSE);
  item = CLAY_ITEM (clay_savable_load_peek_active_object (state));

  if (! clay_savable_load_get_values (state,
                                      "name", &name,
                                      NULL))
    {
      g_set_error (error, CLAY_WLBR_ERROR, CLAY_WLBR_ERROR_FORMAT,
                   "%s: failed to load a layer because of missing name.",
                   G_STRFUNC);

      g_object_unref (item);
      return TRUE;
    }

  clay_object_set_name (CLAY_OBJECT (item), name);

  clay_savable_load_get_values (state,
                                "selected",        &selected,
                                "visible",         &visible,
                                "tattoo",          &tattoo,
                                "color-tag",       &tag,
                                "lock-content",    &lock_content,
                                "lock-position",   &lock_position,
                                "lock-visibility", &lock_visibility,
                                NULL);

  /* TODO: handle "selected". */

  clay_item_set_visible (item, visible, FALSE);
  if (tattoo > 0)
    clay_item_set_tattoo (item, tattoo);
  clay_item_set_color_tag (item, tag, FALSE);
  clay_item_set_lock_content (item, lock_content, FALSE);
  clay_item_set_lock_position (item, lock_position, FALSE);
  clay_item_set_lock_visibility (item, lock_visibility, FALSE);

  return TRUE;
}

static gboolean
clay_item_real_is_content_locked (ClayItem  *item,
                                  ClayItem **locked_item,
                                  gboolean   check_children)
{
  ClayItem *parent = clay_item_get_parent (item);

  if (GET_PRIVATE (item)->lock_content)
    {
      if (locked_item)
        *locked_item = item;
    }
  else if (parent &&
           CLAY_ITEM_GET_CLASS (item)->is_content_locked (parent, locked_item, FALSE))
    {
      return TRUE;
    }

  return GET_PRIVATE (item)->lock_content;
}

static gboolean
clay_item_real_is_position_locked (ClayItem  *item,
                                   ClayItem **locked_item,
                                   gboolean   check_children)
{
  ClayItem *parent = clay_item_get_parent (item);

  if (GET_PRIVATE (item)->lock_position)
    {
      if (locked_item)
        *locked_item = item;
    }
  else if (parent &&
           CLAY_ITEM_GET_CLASS (item)->is_position_locked (parent, locked_item, FALSE))
    {
      return TRUE;
    }

  return GET_PRIVATE (item)->lock_position;
}

static gboolean
clay_item_real_is_visibility_locked (ClayItem  *item,
                                     ClayItem **locked_item)
{
  /* Unlike other locks, the visibility lock does not propagate from
   * parents or children.
   */

  if (GET_PRIVATE (item)->lock_visibility && locked_item)
    *locked_item = item;

  return GET_PRIVATE (item)->lock_visibility;
}

static gboolean
clay_item_real_bounds (ClayItem *item,
                       gdouble  *x,
                       gdouble  *y,
                       gdouble  *width,
                       gdouble  *height)
{
  ClayItemPrivate *private = GET_PRIVATE (item);

  *x      = 0;
  *y      = 0;
  *width  = private->width;
  *height = private->height;

  return TRUE;
}

static ClayItem *
clay_item_real_duplicate (ClayItem *item,
                          GType     new_type)
{
  ClayItemPrivate *private;
  ClayItem        *new_item;
  gchar           *new_name;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (CLAY_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_ITEM), NULL);

  /*  formulate the new name  */
  {
    const gchar *name;
    gint         len;

    name = clay_object_get_name (item);

    g_return_val_if_fail (name != NULL, NULL);

    len = strlen (_("copy"));

    if ((strlen (name) >= len &&
         strcmp (&name[strlen (name) - len], _("copy")) == 0) ||
        g_regex_match_simple ("#([0-9]+)\\s*$", name, 0, 0))
      {
        /* don't have redundant "copy"s */
        new_name = g_strdup (name);
      }
    else
      {
        new_name = g_strdup_printf (_("%s copy"), name);
      }
  }

  new_item = clay_item_new (new_type,
                            clay_item_get_image (item), new_name,
                            private->offset_x, private->offset_y,
                            clay_item_get_width  (item),
                            clay_item_get_height (item));

  g_free (new_name);

  clay_viewable_set_expanded (CLAY_VIEWABLE (new_item),
                              clay_viewable_get_expanded (CLAY_VIEWABLE (item)));

  g_object_unref (GET_PRIVATE (new_item)->parasites);
  GET_PRIVATE (new_item)->parasites = clay_parasite_list_copy (private->parasites);

  clay_item_set_visible   (new_item, clay_item_get_visible   (item), FALSE);
  clay_item_set_color_tag (new_item, clay_item_get_color_tag (item), FALSE);

  if (clay_item_can_lock_content (new_item))
    clay_item_set_lock_content (new_item, clay_item_get_lock_content (item),
                                FALSE);

  if (clay_item_can_lock_position (new_item))
    clay_item_set_lock_position (new_item, clay_item_get_lock_position (item),
                                 FALSE);

  if (clay_item_can_lock_visibility (new_item))
    clay_item_set_lock_visibility (new_item, clay_item_get_lock_visibility (item),
                                   FALSE);

  return new_item;
}

static void
clay_item_real_convert (ClayItem  *item,
                        ClayImage *dest_image,
                        GType      old_type)
{
  clay_item_set_image (item, dest_image);
}

static gboolean
clay_item_real_rename (ClayItem     *item,
                       const gchar  *new_name,
                       const gchar  *undo_desc,
                       GError      **error)
{
  if (clay_item_is_attached (item))
    clay_item_tree_rename_item (clay_item_get_tree (item), item,
                                new_name, TRUE, undo_desc);
  else
    clay_object_set_name (CLAY_OBJECT (item), new_name);

  return TRUE;
}

static void
clay_item_real_translate (ClayItem *item,
                          gdouble   offset_x,
                          gdouble   offset_y,
                          gboolean  push_undo)
{
  ClayItemPrivate *private = GET_PRIVATE (item);

  clay_item_set_offset (item,
                        private->offset_x + SIGNED_ROUND (offset_x),
                        private->offset_y + SIGNED_ROUND (offset_y));
}

static void
clay_item_real_start_transform (ClayItem *item,
                                gboolean  push_undo)
{
  clay_item_start_move (item, push_undo);
}

static void
clay_item_real_end_transform (ClayItem *item,
                              gboolean  push_undo)
{
  clay_item_end_move (item, push_undo);
}

static void
clay_item_real_scale (ClayItem              *item,
                      gint                   new_width,
                      gint                   new_height,
                      gint                   new_offset_x,
                      gint                   new_offset_y,
                      ClayInterpolationType  interpolation,
                      ClayProgress          *progress)
{
  ClayItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_WIDTH]);
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_HEIGHT]);
    }

  clay_item_set_offset (item, new_offset_x, new_offset_y);
}

static void
clay_item_real_resize (ClayItem     *item,
                       ClayContext  *context,
                       ClayFillType  fill_type,
                       gint          new_width,
                       gint          new_height,
                       gint          offset_x,
                       gint          offset_y)
{
  ClayItemPrivate *private = GET_PRIVATE (item);

  if (private->width != new_width)
    {
      private->width = new_width;
      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_WIDTH]);
    }

  if (private->height != new_height)
    {
      private->height = new_height;
      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_HEIGHT]);
    }

  clay_item_set_offset (item,
                        private->offset_x - offset_x,
                        private->offset_y - offset_y);
}

static ClayTransformResize
clay_item_real_get_clip (ClayItem            *item,
                         ClayTransformResize  clip_result)
{
  if (clay_item_get_lock_position (item))
    return CLAY_TRANSFORM_RESIZE_CLIP;
  else
    return clip_result;
}


/*  public functions  */

/**
 * clay_item_new:
 * @type:     The new item's type.
 * @image:    The new item's #ClayImage.
 * @name:     The name to assign the item.
 * @offset_x: The X offset to assign the item.
 * @offset_y: The Y offset to assign the item.
 * @width:    The width to assign the item.
 * @height:   The height to assign the item.
 *
 * Returns: The newly created item.
 */
ClayItem *
clay_item_new (GType        type,
               ClayImage   *image,
               const gchar *name,
               gint         offset_x,
               gint         offset_y,
               gint         width,
               gint         height)
{
  ClayItem        *item;
  ClayItemPrivate *private;

  g_return_val_if_fail (g_type_is_a (type, CLAY_TYPE_ITEM), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (image), NULL);
  g_return_val_if_fail (width > 0 && height > 0, NULL);

  item = g_object_new (type,
                       "image", image,
                       NULL);

  private = GET_PRIVATE (item);

  private->width  = width;
  private->height = height;
  clay_item_set_offset (item, offset_x, offset_y);

  if (name && strlen (name))
    clay_object_set_name (CLAY_OBJECT (item), name);
  else
    clay_object_set_static_name (CLAY_OBJECT (item),
                                 CLAY_VIEWABLE_GET_CLASS (item)->default_name);

  return item;
}

/**
 * clay_item_remove:
 * @item: the #ClayItem to remove.
 *
 * This function sets the 'removed' flag on @item to %TRUE, and emits
 * a 'removed' signal on the item.
 */
void
clay_item_removed (ClayItem *item)
{
  ClayContainer *children;

  g_return_if_fail (CLAY_IS_ITEM (item));

  GET_PRIVATE (item)->removed = TRUE;

  children = clay_viewable_get_children (CLAY_VIEWABLE (item));

  if (children)
    clay_container_foreach (children, (GFunc) clay_item_removed, NULL);

  g_signal_emit (item, clay_item_signals[REMOVED], 0);
}

/**
 * clay_item_is_removed:
 * @item: the #ClayItem to check.
 *
 * Returns: %TRUE if the 'removed' flag is set for @item, %FALSE otherwise.
 */
gboolean
clay_item_is_removed (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->removed;
}

/**
 * clay_item_unset_removed:
 * @item: a #ClayItem which was on the undo stack
 *
 * Unsets an item's "removed" state. This function is called when an
 * item was on the undo stack and is added back to its parent
 * container during and undo or redo. It must never be called from
 * anywhere else.
 **/
void
clay_item_unset_removed (ClayItem *item)
{
  ClayContainer *children;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_removed (item));

  GET_PRIVATE (item)->removed = FALSE;

  children = clay_viewable_get_children (CLAY_VIEWABLE (item));

  if (children)
    clay_container_foreach (children, (GFunc) clay_item_unset_removed, NULL);

  if (CLAY_ITEM_GET_CLASS (item)->unset_removed)
    CLAY_ITEM_GET_CLASS (item)->unset_removed (item);
}

/**
 * clay_item_is_attached:
 * @item: The #ClayItem to check.
 *
 * Returns: %TRUE if the item is attached to an image, %FALSE otherwise.
 */
gboolean
clay_item_is_attached (ClayItem *item)
{
  ClayImage *image;
  ClayItem  *parent;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  image = clay_item_get_image (item);
  if (image != NULL && clay_image_is_hidden_item (image, item))
    return TRUE;

  parent = clay_item_get_parent (item);

  if (parent)
    return clay_item_is_attached (parent);

  return CLAY_ITEM_GET_CLASS (item)->is_attached (item);
}

ClayItem *
clay_item_get_parent (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  return CLAY_ITEM (clay_viewable_get_parent (CLAY_VIEWABLE (item)));
}

ClayItemTree *
clay_item_get_tree (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  if (CLAY_ITEM_GET_CLASS (item)->get_tree)
    return CLAY_ITEM_GET_CLASS (item)->get_tree (item);

  return NULL;
}

ClayContainer *
clay_item_get_container (ClayItem *item)
{
  ClayItem     *parent;
  ClayItemTree *tree;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  parent = clay_item_get_parent (item);

  if (parent)
    return clay_viewable_get_children (CLAY_VIEWABLE (parent));

  tree = clay_item_get_tree (item);

  if (tree)
    return tree->container;

  return NULL;
}

GList *
clay_item_get_container_iter (ClayItem *item)
{
  ClayContainer *container;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  container = clay_item_get_container (item);

  if (container)
    return CLAY_LIST (container)->queue->head;

  return NULL;
}

gint
clay_item_get_index (ClayItem *item)
{
  ClayContainer *container;

  g_return_val_if_fail (CLAY_IS_ITEM (item), -1);

  container = clay_item_get_container (item);

  if (container)
    return clay_container_get_child_index (container, CLAY_OBJECT (item));

  return -1;
}

GList *
clay_item_get_path (ClayItem *item)
{
  ClayContainer *container;
  GList         *path = NULL;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (clay_item_is_attached (item), NULL);

  container = clay_item_get_container (item);

  while (container)
    {
      guint32 index = clay_container_get_child_index (container,
                                                      CLAY_OBJECT (item));

      path = g_list_prepend (path, GUINT_TO_POINTER (index));

      item = clay_item_get_parent (item);

      if (item)
        container = clay_item_get_container (item);
      else
        container = NULL;
    }

  return path;
}

gboolean
clay_item_bounds (ClayItem *item,
                  gint     *x,
                  gint     *y,
                  gint     *width,
                  gint     *height)
{
  gdouble  tmp_x, tmp_y, tmp_width, tmp_height;
  gboolean retval;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  retval = CLAY_ITEM_GET_CLASS (item)->bounds (item,
                                               &tmp_x, &tmp_y,
                                               &tmp_width, &tmp_height);

  if (x)      *x      = floor (tmp_x);
  if (y)      *y      = floor (tmp_y);
  if (width)  *width  = ceil (tmp_x + tmp_width)  - floor (tmp_x);
  if (height) *height = ceil (tmp_y + tmp_height) - floor (tmp_y);

  return retval;
}

gboolean
clay_item_bounds_f (ClayItem *item,
                    gdouble  *x,
                    gdouble  *y,
                    gdouble  *width,
                    gdouble  *height)
{
  gdouble  tmp_x, tmp_y, tmp_width, tmp_height;
  gboolean retval;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  retval = CLAY_ITEM_GET_CLASS (item)->bounds (item,
                                               &tmp_x, &tmp_y,
                                               &tmp_width, &tmp_height);

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

/**
 * clay_item_duplicate:
 * @item:     The #ClayItem to duplicate.
 * @new_type: The type to make the new item.
 *
 * Returns: the newly created item.
 */
ClayItem *
clay_item_duplicate (ClayItem *item,
                     GType     new_type)
{
  ClayItemPrivate *private;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  g_return_val_if_fail (CLAY_IS_IMAGE (private->image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_ITEM), NULL);

  return CLAY_ITEM_GET_CLASS (item)->duplicate (item, new_type);
}

/**
 * clay_item_convert:
 * @item:       The #ClayItem to convert.
 * @dest_image: The #ClayImage in which to place the converted item.
 * @new_type:   The type to convert the item to.
 *
 * Returns: the new item that results from the conversion.
 */
ClayItem *
clay_item_convert (ClayItem  *item,
                   ClayImage *dest_image,
                   GType      new_type)
{
  ClayItem *new_item;
  GType     old_type;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (GET_PRIVATE (item)->image), NULL);
  g_return_val_if_fail (CLAY_IS_IMAGE (dest_image), NULL);
  g_return_val_if_fail (g_type_is_a (new_type, CLAY_TYPE_ITEM), NULL);

  old_type = G_TYPE_FROM_INSTANCE (item);

  new_item = clay_item_duplicate (item, new_type);

  if (new_item)
    CLAY_ITEM_GET_CLASS (new_item)->convert (new_item, dest_image, old_type);

  return new_item;
}

/**
 * clay_item_rename:
 * @item:     The #ClayItem to rename.
 * @new_name: The new name to give the item.
 * @error:    Return location for error message.
 *
 * This function assigns a new name to the item, if the desired name is
 * different from the name it already has, and pushes an entry onto the
 * undo stack for the item's image.  If @new_name is NULL or empty, the
 * default name for the item's class is used.  If the name is changed,
 * the ClayObject::name-changed signal is emitted for the item.
 *
 * Returns: %TRUE if the @item could be renamed, %FALSE otherwise.
 */
gboolean
clay_item_rename (ClayItem     *item,
                  const gchar  *new_name,
                  GError      **error)
{
  ClayItemClass *item_class;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = CLAY_ITEM_GET_CLASS (item);

  if (! new_name || ! *new_name)
    new_name = CLAY_VIEWABLE_GET_CLASS (item)->default_name;

  if (strcmp (new_name, clay_object_get_name (item)))
    return item_class->rename (item, new_name, item_class->rename_desc, error);

  return TRUE;
}

/**
 * clay_item_get_width:
 * @item: The #ClayItem to check.
 *
 * Returns: The width of the item.
 */
gint
clay_item_get_width (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->width;
}

/**
 * clay_item_get_height:
 * @item: The #ClayItem to check.
 *
 * Returns: The height of the item.
 */
gint
clay_item_get_height (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->height;
}

void
clay_item_set_size (ClayItem *item,
                    gint      width,
                    gint      height)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (private->width  != width ||
      private->height != height)
    {
      g_object_freeze_notify (G_OBJECT (item));

      if (private->width != width)
        {
          private->width = width;
          g_object_notify_by_pspec (G_OBJECT (item),
                                    clay_item_props[PROP_WIDTH]);
        }

      if (private->height != height)
        {
          private->height = height;
          g_object_notify_by_pspec (G_OBJECT (item),
                                    clay_item_props[PROP_HEIGHT]);
        }

      g_object_thaw_notify (G_OBJECT (item));

      clay_viewable_size_changed (CLAY_VIEWABLE (item));
    }
}

/**
 * clay_item_get_offset:
 * @item:     The #ClayItem to check.
 * @offset_x: (out) (optional): Return location for the item's X offset.
 * @offset_y: (out) (optional): Return location for the item's Y offset.
 *
 * Reveals the X and Y offsets of the item.
 */
void
clay_item_get_offset (ClayItem *item,
                      gint     *offset_x,
                      gint     *offset_y)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));

  private = GET_PRIVATE (item);

  if (offset_x) *offset_x = private->offset_x;
  if (offset_y) *offset_y = private->offset_y;
}

void
clay_item_set_offset (ClayItem *item,
                      gint      offset_x,
                      gint      offset_y)
{
  ClayItemPrivate *private;
  GList           *list;

  g_return_if_fail (CLAY_IS_ITEM (item));

  private = GET_PRIVATE (item);

  g_object_freeze_notify (G_OBJECT (item));

  if (private->offset_x != offset_x)
    {
      private->offset_x = offset_x;
      g_object_notify_by_pspec (G_OBJECT (item),
                                clay_item_props[PROP_OFFSET_X]);
    }

  if (private->offset_y != offset_y)
    {
      private->offset_y = offset_y;
      g_object_notify_by_pspec (G_OBJECT (item),
                                clay_item_props[PROP_OFFSET_Y]);
    }

  for (list = private->offset_nodes; list; list = g_list_next (list))
    {
      GeglNode *node = list->data;

      gegl_node_set (node,
                     "x", (gdouble) private->offset_x,
                     "y", (gdouble) private->offset_y,
                     NULL);
    }

  g_object_thaw_notify (G_OBJECT (item));
}

gint
clay_item_get_offset_x (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_x;
}

gint
clay_item_get_offset_y (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->offset_y;
}

void
clay_item_start_move (ClayItem *item,
                      gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  if (CLAY_ITEM_GET_CLASS (item)->start_move)
    CLAY_ITEM_GET_CLASS (item)->start_move (item, push_undo);
}

void
clay_item_end_move (ClayItem *item,
                    gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  if (CLAY_ITEM_GET_CLASS (item)->end_move)
    CLAY_ITEM_GET_CLASS (item)->end_move (item, push_undo);
}

void
clay_item_start_transform (ClayItem *item,
                           gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  if (CLAY_ITEM_GET_CLASS (item)->start_transform)
    CLAY_ITEM_GET_CLASS (item)->start_transform (item, push_undo);
}

void
clay_item_end_transform (ClayItem *item,
                         gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  if (CLAY_ITEM_GET_CLASS (item)->end_transform)
    CLAY_ITEM_GET_CLASS (item)->end_transform (item, push_undo);
}

/**
 * clay_item_translate:
 * @item:      The #ClayItem to move.
 * @offset_x:  Increment to the X offset of the item.
 * @offset_y:  Increment to the Y offset of the item.
 * @push_undo: If %TRUE, create an entry in the image's undo stack
 *             for this action.
 *
 * Adds the specified increments to the X and Y offsets for the item.
 */
void
clay_item_translate (ClayItem *item,
                     gdouble   offset_x,
                     gdouble   offset_y,
                     gboolean  push_undo)
{
  ClayItemClass *item_class;
  ClayImage     *image;

  g_return_if_fail (CLAY_IS_ITEM (item));

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_DISPLACE,
                                 item_class->translate_desc);

  clay_item_start_transform (item, push_undo);

  item_class->translate (item, offset_x, offset_y, push_undo);

  clay_item_end_transform (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

/**
 * clay_item_check_scaling:
 * @item:       Item to check
 * @new_width:  proposed width of item, in pixels
 * @new_height: proposed height of item, in pixels
 *
 * Scales item dimensions, then snaps them to pixel centers
 *
 * Returns: %FALSE if any dimension reduces to zero as a result
 *          of this; otherwise, returns %TRUE.
 **/
gboolean
clay_item_check_scaling (ClayItem *item,
                         gint      new_width,
                         gint      new_height)
{
  ClayItemPrivate *private;
  ClayImage       *image;
  gdouble          img_scale_w;
  gdouble          img_scale_h;
  gint             new_item_offset_x;
  gint             new_item_offset_y;
  gint             new_item_width;
  gint             new_item_height;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);
  image   = clay_item_get_image (item);

  img_scale_w       = ((gdouble) new_width /
                       (gdouble) clay_image_get_width (image));
  img_scale_h       = ((gdouble) new_height /
                       (gdouble) clay_image_get_height (image));
  new_item_offset_x = SIGNED_ROUND (img_scale_w * private->offset_x);
  new_item_offset_y = SIGNED_ROUND (img_scale_h * private->offset_y);
  new_item_width    = SIGNED_ROUND (img_scale_w * (private->offset_x +
                                                   clay_item_get_width (item))) -
                      new_item_offset_x;
  new_item_height   = SIGNED_ROUND (img_scale_h * (private->offset_y +
                                                   clay_item_get_height (item))) -
                      new_item_offset_y;

  return (new_item_width > 0 && new_item_height > 0);
}

void
clay_item_scale (ClayItem              *item,
                 gint                   new_width,
                 gint                   new_height,
                 gint                   new_offset_x,
                 gint                   new_offset_y,
                 ClayInterpolationType  interpolation,
                 ClayProgress          *progress)
{
  ClayItemClass *item_class;
  ClayImage     *image;
  gboolean       push_undo;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  push_undo = clay_item_is_attached (item);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_SCALE,
                                 item_class->scale_desc);

  clay_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->scale (item, new_width, new_height, new_offset_x, new_offset_y,
                     interpolation, progress);

  g_object_thaw_notify (G_OBJECT (item));

  clay_item_end_transform (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

/**
 * clay_item_scale_by_factors:
 * @item:     Item to be transformed by explicit width and height factors.
 * @w_factor: scale factor to apply to width and horizontal offset
 * @h_factor: scale factor to apply to height and vertical offset
 * @interpolation:
 * @progress:
 *
 * Scales item dimensions and offsets by uniform width and
 * height factors.
 *
 * Use clay_item_scale_by_factors() in circumstances when the same
 * width and height scaling factors are to be uniformly applied to a
 * set of items. In this context, the item's dimensions and offsets
 * from the sides of the containing image all change by these
 * predetermined factors. By fiat, the fixed point of the transform is
 * the upper left hand corner of the image. Returns %FALSE if a
 * requested scale factor is zero or if a scaling zero's out a item
 * dimension; returns %TRUE otherwise.
 *
 * Use clay_item_scale() in circumstances where new item width
 * and height dimensions are predetermined instead.
 *
 * Side effects: Undo set created for item. Old item imagery
 *               scaled & painted to new item tiles.
 *
 * Returns: %TRUE, if the scaled item has positive dimensions
 *          %FALSE if the scaled item has at least one zero dimension
 **/
gboolean
clay_item_scale_by_factors (ClayItem              *item,
                            gdouble                w_factor,
                            gdouble                h_factor,
                            ClayInterpolationType  interpolation,
                            ClayProgress          *progress)
{
  return clay_item_scale_by_factors_with_origin (item,
                                                 w_factor, h_factor,
                                                 0, 0, 0, 0,
                                                 interpolation, progress);
}

/**
 * clay_item_scale_by_factors:
 * @item:         Item to be transformed by explicit width and height factors.
 * @w_factor:     scale factor to apply to width and horizontal offset
 * @h_factor:     scale factor to apply to height and vertical offset
 * @origin_x:     x-coordinate of the transformation input origin
 * @origin_y:     y-coordinate of the transformation input origin
 * @new_origin_x: x-coordinate of the transformation output origin
 * @new_origin_y: y-coordinate of the transformation output origin
 * @interpolation:
 * @progress:
 *
 * Same as clay_item_scale_by_factors(), but with the option to specify
 * custom input and output points of origin for the transformation.
 *
 * Returns: %TRUE, if the scaled item has positive dimensions
 *          %FALSE if the scaled item has at least one zero dimension
 **/
gboolean
clay_item_scale_by_factors_with_origin (ClayItem              *item,
                                        gdouble                w_factor,
                                        gdouble                h_factor,
                                        gint                   origin_x,
                                        gint                   origin_y,
                                        gint                   new_origin_x,
                                        gint                   new_origin_y,
                                        ClayInterpolationType  interpolation,
                                        ClayProgress          *progress)
{
  ClayItemPrivate *private;
  ClayContainer   *children;
  gint             new_width, new_height;
  gint             new_offset_x, new_offset_y;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);

  private = GET_PRIVATE (item);

  if (w_factor <= 0.0 || h_factor <= 0.0)
    {
      g_warning ("%s: requested width or height scale is non-positive",
                 G_STRFUNC);
      return FALSE;
    }

  children = clay_viewable_get_children (CLAY_VIEWABLE (item));

  /* avoid discarding empty layer groups */
  if (children && clay_container_is_empty (children))
    return TRUE;

  new_offset_x = SIGNED_ROUND (w_factor * (private->offset_x - origin_x));
  new_offset_y = SIGNED_ROUND (h_factor * (private->offset_y - origin_y));
  new_width    = SIGNED_ROUND (w_factor * (private->offset_x - origin_x +
                                           clay_item_get_width (item))) -
                 new_offset_x;
  new_height   = SIGNED_ROUND (h_factor * (private->offset_y - origin_y +
                                           clay_item_get_height (item))) -
                 new_offset_y;

  new_offset_x += new_origin_x;
  new_offset_y += new_origin_y;

  if (new_width > 0 && new_height > 0)
    {
      clay_item_scale (item,
                       new_width, new_height,
                       new_offset_x, new_offset_y,
                       interpolation, progress);
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_item_scale_by_origin:
 * @item:         The item to be transformed by width & height scale factors
 * @new_width:    The width that item will acquire
 * @new_height:   The height that the item will acquire
 * @interpolation:
 * @progress:
 * @local_origin: sets fixed point of the scaling transform. See below.
 *
 * Sets item dimensions to new_width and
 * new_height. Derives vertical and horizontal scaling
 * transforms from new width and height. If local_origin is
 * %TRUE, the fixed point of the scaling transform coincides
 * with the item's center point.  Otherwise, the fixed
 * point is taken to be [-ClayItem::offset_x, -ClayItem::->offset_y].
 *
 * Since this function derives scale factors from new and
 * current item dimensions, these factors will vary from
 * item to item because of aliasing artifacts; factor
 * variations among items can be quite large where item
 * dimensions approach pixel dimensions. Use
 * clay_item_scale_by_factors() where constant scales are to
 * be uniformly applied to a number of items.
 *
 * Side effects: undo set created for item.
 *               Old item imagery scaled
 *               & painted to new item tiles
 **/
void
clay_item_scale_by_origin (ClayItem              *item,
                           gint                   new_width,
                           gint                   new_height,
                           ClayInterpolationType  interpolation,
                           ClayProgress          *progress,
                           gboolean               local_origin)
{
  ClayItemPrivate *private;
  gint             new_offset_x, new_offset_y;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  private = GET_PRIVATE (item);

  if (new_width == 0 || new_height == 0)
    {
      g_warning ("%s: requested width or height equals zero", G_STRFUNC);
      return;
    }

  if (local_origin)
    {
      new_offset_x = (private->offset_x +
                      ((clay_item_get_width  (item) - new_width)  / 2.0));
      new_offset_y = (private->offset_y +
                      ((clay_item_get_height (item) - new_height) / 2.0));
    }
  else
    {
      new_offset_x = (gint) (((gdouble) new_width *
                              (gdouble) private->offset_x /
                              (gdouble) clay_item_get_width (item)));

      new_offset_y = (gint) (((gdouble) new_height *
                              (gdouble) private->offset_y /
                              (gdouble) clay_item_get_height (item)));
    }

  clay_item_scale (item,
                   new_width, new_height,
                   new_offset_x, new_offset_y,
                   interpolation, progress);
}

void
clay_item_resize (ClayItem     *item,
                  ClayContext  *context,
                  ClayFillType  fill_type,
                  gint          new_width,
                  gint          new_height,
                  gint          offset_x,
                  gint          offset_y)
{
  ClayItemClass *item_class;
  ClayImage     *image;
  gboolean       push_undo;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  if (new_width < 1 || new_height < 1)
    return;

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  push_undo = clay_item_is_attached (item);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_ITEM_RESIZE,
                                 item_class->resize_desc);

  /* note that we call clay_item_start_move(), and not
   * clay_item_start_transform().  whether or not a resize operation should be
   * considered a transform operation, or a move operation, depends on the
   * intended use of these functions by subclasses.  atm, we only use
   * clay_item_{start,end}_transform() to suspend mask resizing in group
   * layers, which should not happen when reisizing a group, hence the call to
   * clay_item_start_move().
   *
   * see the comment in clay_group_layer_resize() for more information.
   */
  clay_item_start_move (item, push_undo);

  if (push_undo && CLAY_IS_DRAWABLE (item))
    clay_drawable_enable_resize_undo (CLAY_DRAWABLE (item));

  g_object_freeze_notify (G_OBJECT (item));

  item_class->resize (item, context, fill_type,
                      new_width, new_height, offset_x, offset_y);

  g_object_thaw_notify (G_OBJECT (item));

  clay_item_end_move (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

void
clay_item_flip (ClayItem            *item,
                ClayContext         *context,
                ClayOrientationType  flip_type,
                gdouble              axis,
                gboolean             clip_result)
{
  ClayItemClass *item_class;
  ClayImage     *image;
  gboolean       push_undo;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_attached (item));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  push_undo = clay_item_is_attached (item);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TRANSFORM,
                                 item_class->flip_desc);

  clay_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->flip (item, context, flip_type, axis, clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  clay_item_end_transform (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

void
clay_item_rotate (ClayItem         *item,
                  ClayContext      *context,
                  ClayRotationType  rotate_type,
                  gdouble           center_x,
                  gdouble           center_y,
                  gboolean          clip_result)
{
  ClayItemClass *item_class;
  ClayImage     *image;
  gboolean       push_undo;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_attached (item));
  g_return_if_fail (CLAY_IS_CONTEXT (context));

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  push_undo = clay_item_is_attached (item);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TRANSFORM,
                                 item_class->rotate_desc);

  clay_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->rotate (item, context, rotate_type, center_x, center_y,
                      clip_result);

  g_object_thaw_notify (G_OBJECT (item));

  clay_item_end_transform (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

void
clay_item_transform (ClayItem               *item,
                     ClayContext            *context,
                     const ClayMatrix3      *matrix,
                     ClayTransformDirection  direction,
                     ClayInterpolationType   interpolation,
                     ClayTransformResize     clip_result,
                     ClayProgress           *progress)
{
  ClayItemClass *item_class;
  ClayImage     *image;
  gboolean       push_undo;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_attached (item));
  g_return_if_fail (CLAY_IS_CONTEXT (context));
  g_return_if_fail (matrix != NULL);
  g_return_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress));

  item_class = CLAY_ITEM_GET_CLASS (item);
  image = clay_item_get_image (item);

  push_undo = clay_item_is_attached (item);

  if (push_undo)
    clay_image_undo_group_start (image, CLAY_UNDO_GROUP_TRANSFORM,
                                 item_class->transform_desc);

  clay_item_start_transform (item, push_undo);

  g_object_freeze_notify (G_OBJECT (item));

  item_class->transform (item, context, matrix, direction, interpolation,
                         clip_result, progress, TRUE);

  g_object_thaw_notify (G_OBJECT (item));

  clay_item_end_transform (item, push_undo);

  if (push_undo)
    clay_image_undo_group_end (image);
}

ClayTransformResize
clay_item_get_clip (ClayItem            *item,
                    ClayTransformResize  clip_result)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), CLAY_TRANSFORM_RESIZE_ADJUST);

  return CLAY_ITEM_GET_CLASS (item)->get_clip (item, clip_result);
}

gboolean
clay_item_fill (ClayItem        *item,
                GList           *drawables,
                ClayFillOptions *fill_options,
                gboolean         push_undo,
                ClayProgress    *progress,
                GError         **error)
{
  ClayItemClass *item_class;
  GList         *iter;
  gboolean       locked = FALSE;
  gboolean       retval = FALSE;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (clay_item_is_attached (item), FALSE);
  g_return_val_if_fail (CLAY_IS_FILL_OPTIONS (fill_options), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = CLAY_ITEM_GET_CLASS (item);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (iter->data)), FALSE);
    }

  if (item_class->fill)
    {
      ClayImage *image = clay_item_get_image (item);

      if (push_undo)
        clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT,
                                     item_class->fill_desc);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL))
            {
              locked = TRUE;
              continue;
            }

          retval = item_class->fill (item, iter->data, fill_options,
                                     push_undo, progress, error);
          if (! retval)
            break;
        }

      if (locked)
        clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                              _("A selected layer's pixels are locked."));

      if (push_undo)
        clay_image_undo_group_end (image);
    }

  return retval;
}

gboolean
clay_item_stroke (ClayItem          *item,
                  GList             *drawables,
                  ClayContext       *context,
                  ClayStrokeOptions *stroke_options,
                  ClayPaintOptions  *paint_options,
                  gboolean           push_undo,
                  ClayProgress      *progress,
                  GError           **error)
{
  ClayItemClass *item_class;
  GList         *iter;
  gboolean       locked = FALSE;
  gboolean       retval = FALSE;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (clay_item_is_attached (item), FALSE);
  g_return_val_if_fail (CLAY_IS_CONTEXT (context), FALSE);
  g_return_val_if_fail (CLAY_IS_STROKE_OPTIONS (stroke_options), FALSE);
  g_return_val_if_fail (paint_options == NULL ||
                        CLAY_IS_PAINT_OPTIONS (paint_options), FALSE);
  g_return_val_if_fail (progress == NULL || CLAY_IS_PROGRESS (progress), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  item_class = CLAY_ITEM_GET_CLASS (item);

  for (iter = drawables; iter; iter = iter->next)
    {
      g_return_val_if_fail (CLAY_IS_DRAWABLE (iter->data), FALSE);
      g_return_val_if_fail (clay_item_is_attached (CLAY_ITEM (iter->data)), FALSE);
    }

  if (item_class->stroke)
    {
      ClayImage *image = clay_item_get_image (item);

      clay_stroke_options_prepare (stroke_options, context, paint_options);

      if (push_undo)
        clay_image_undo_group_start (image, CLAY_UNDO_GROUP_PAINT,
                                     item_class->stroke_desc);

      for (iter = drawables; iter; iter = iter->next)
        {
          if (clay_item_is_content_locked (CLAY_ITEM (iter->data), NULL))
            {
              locked = TRUE;
              continue;
            }

          retval = item_class->stroke (item, iter->data, stroke_options, push_undo,
                                       progress, error);
          if (! retval)
            break;
        }

      if (locked)
        clay_message_literal (image->clay, NULL, CLAY_MESSAGE_WARNING,
                              _("A selected layer's pixels are locked."));

      if (push_undo)
        clay_image_undo_group_end (image);

      clay_stroke_options_finish (stroke_options);
    }

  return retval;
}

void
clay_item_to_selection (ClayItem       *item,
                        ClayChannelOps  op,
                        gboolean        antialias,
                        gboolean        feather,
                        gdouble         feather_radius_x,
                        gdouble         feather_radius_y)
{
  ClayItemClass *item_class;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_is_attached (item));

  item_class = CLAY_ITEM_GET_CLASS (item);

  if (item_class->to_selection)
    item_class->to_selection (item, op, antialias,
                              feather, feather_radius_x, feather_radius_y);
}

void
clay_item_add_offset_node (ClayItem *item,
                           GeglNode *node)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (GEGL_IS_NODE (node));

  private = GET_PRIVATE (item);

  g_return_if_fail (g_list_find (private->offset_nodes, node) == NULL);

  gegl_node_set (node,
                 "x", (gdouble) private->offset_x,
                 "y", (gdouble) private->offset_y,
                 NULL);

  private->offset_nodes = g_list_append (private->offset_nodes,
                                         g_object_ref (node));
}

void
clay_item_remove_offset_node (ClayItem *item,
                              GeglNode *node)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (GEGL_IS_NODE (node));

  private = GET_PRIVATE (item);

  g_return_if_fail (g_list_find (private->offset_nodes, node) != NULL);

  private->offset_nodes = g_list_remove (private->offset_nodes, node);
  g_object_unref (node);
}

gint
clay_item_get_id (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), -1);

  return GET_PRIVATE (item)->ID;
}

ClayItem *
clay_item_get_by_id (Clay *clay,
                     gint  item_id)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (clay->item_table == NULL)
    return NULL;

  return (ClayItem *) clay_id_table_lookup (clay->item_table, item_id);
}

/* In current code, the ID is assumed to be set optionally, after
 * construction, when we try to re-assign previous IDs to loaded items.
 *
 * Do not use this function in code other than for loading code. Items
 * are not expected to change ID in normal usage.
 *
 * The @id attribute must have been reserved.
 */
void
clay_item_set_id (ClayItem *item,
                  gint      id)
{
  ClayItemPrivate *private;
  ClayImage       *image;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (id >= CLAY_ID_TABLE_START_ID && id <= CLAY_ID_TABLE_END_ID);

  private = GET_PRIVATE (item);
  image   = private->image;

  g_return_if_fail (image != NULL && private->ID != 0);

  clay_id_table_remove (image->clay->item_table, private->ID);
  clay_id_table_insert_reserved (image->clay->item_table, id, item);
  private->ID = id;

  g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_ID]);
}

ClayTattoo
clay_item_get_tattoo (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), 0);

  return GET_PRIVATE (item)->tattoo;
}

void
clay_item_set_tattoo (ClayItem   *item,
                      ClayTattoo  tattoo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  GET_PRIVATE (item)->tattoo = tattoo;
}

ClayImage *
clay_item_get_image (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->image;
}

void
clay_item_set_image (ClayItem  *item,
                     ClayImage *image)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (! clay_item_is_attached (item));
  g_return_if_fail (! clay_item_is_removed (item));
  g_return_if_fail (CLAY_IS_IMAGE (image));

  private = GET_PRIVATE (item);

  if (image == private->image)
    return;

  g_object_freeze_notify (G_OBJECT (item));

  if (private->ID == 0)
    {
      private->ID = clay_id_table_insert (image->clay->item_table, item);

      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_ID]);
    }

  if (private->tattoo == 0 || private->image != image)
    {
      private->tattoo = clay_image_get_new_tattoo (image);
    }

  private->image = image;
  g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_IMAGE]);

  g_object_thaw_notify (G_OBJECT (item));
}

/**
 * clay_item_replace_item:
 * @item: a newly allocated #ClayItem
 * @replace: the #ClayItem to be replaced by @item
 *
 * This function shouly only be called right after @item has been
 * newly allocated.
 *
 * Replaces @replace by @item, as far as possible within the #ClayItem
 * class. The new @item takes over @replace's ID, tattoo, offset, size
 * etc. and all these properties are set to %NULL on @replace.
 *
 * This function *only* exists to allow subclasses to do evil hacks
 * like in XCF text layer loading. Don't ever use this function if you
 * are not sure.
 *
 * After this function returns, @replace has become completely
 * unusable, should only be used to steal everything it has (like its
 * drawable properties if it's a drawable), and then be destroyed.
 **/
void
clay_item_replace_item (ClayItem *item,
                        ClayItem *replace)
{
  ClayItemPrivate *private;
  gint             offset_x = 0;
  gint             offset_y = 0;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (! clay_item_is_attached (item));
  g_return_if_fail (! clay_item_is_removed (item));
  g_return_if_fail (CLAY_IS_ITEM (replace));

  private = GET_PRIVATE (item);

  clay_object_set_name (CLAY_OBJECT (item), clay_object_get_name (replace));

  if (private->ID)
    clay_id_table_remove (clay_item_get_image (item)->clay->item_table,
                          clay_item_get_id (item));

  private->ID = clay_item_get_id (replace);
  clay_id_table_replace (clay_item_get_image (item)->clay->item_table,
                         clay_item_get_id (item),
                         item);

  /* Set image before tattoo so that the explicitly set tattoo overrides
   * the one implicitly set when setting the image
   */
  clay_item_set_image (item, clay_item_get_image (replace));
  GET_PRIVATE (replace)->image  = NULL;

  clay_item_set_tattoo (item, clay_item_get_tattoo (replace));
  clay_item_set_tattoo (replace, 0);

  g_object_unref (private->parasites);
  private->parasites = GET_PRIVATE (replace)->parasites;
  GET_PRIVATE (replace)->parasites = NULL;

  clay_item_get_offset (replace, &offset_x, &offset_y);
  clay_item_set_offset (item, offset_x, offset_y);

  clay_item_set_size (item,
                      clay_item_get_width  (replace),
                      clay_item_get_height (replace));

  clay_item_set_visible       (item, clay_item_get_visible (replace), FALSE);
  clay_item_set_color_tag     (item, clay_item_get_color_tag (replace), FALSE);
  clay_item_set_lock_content  (item, clay_item_get_lock_content (replace), FALSE);
  clay_item_set_lock_position (item, clay_item_get_lock_position (replace), FALSE);
  clay_item_set_lock_visibility (item, clay_item_get_lock_visibility (replace), FALSE);
}

/**
 * clay_item_set_parasites:
 * @item: a #ClayItem
 * @parasites: a #ClayParasiteList
 *
 * Set an @item's #ClayParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when creating items from unusual
 * sources.
 **/
void
clay_item_set_parasites (ClayItem         *item,
                         ClayParasiteList *parasites)
{
  ClayItemPrivate *private;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (CLAY_IS_PARASITE_LIST (parasites));

  private = GET_PRIVATE (item);

  g_set_object (&private->parasites, parasites);
}

/**
 * clay_item_get_parasites:
 * @item: a #ClayItem
 *
 * Get an @item's #ClayParasiteList. It's usually never needed to
 * fiddle with an item's parasite list directly. This function exists
 * for special purposes only, like when saving an item to XCF.
 *
 * Returns: The @item's #ClayParasiteList.
 **/
ClayParasiteList *
clay_item_get_parasites (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  return GET_PRIVATE (item)->parasites;
}

gboolean
clay_item_parasite_validate (ClayItem            *item,
                             const ClayParasite  *parasite,
                             GError             **error)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (parasite != NULL, FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  return TRUE;
}

void
clay_item_parasite_attach (ClayItem           *item,
                           const ClayParasite *parasite,
                           gboolean            push_undo)
{
  ClayItemPrivate *private;
  ClayParasite     copy;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (parasite != NULL);

  private = GET_PRIVATE (item);

  /*  make a temporary copy of the ClayParasite struct because
   *  clay_parasite_shift_parent() changes it
   */
  copy = *parasite;

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      /*  only set the dirty bit manually if we can be saved and the new
       *  parasite differs from the current one and we aren't undoable
       */
      if (clay_parasite_is_undoable (&copy))
        {
          /* do a group in case we have attach_parent set */
          clay_image_undo_group_start (private->image,
                                       CLAY_UNDO_GROUP_PARASITE_ATTACH,
                                       C_("undo-type", "Attach Parasite"));

          clay_image_undo_push_item_parasite (private->image, NULL, item, &copy);
        }
      else if (clay_parasite_is_persistent (&copy) &&
               ! clay_parasite_compare (&copy,
                                        clay_item_parasite_find
                                        (item, clay_parasite_get_name (&copy))))
        {
          clay_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Attach Parasite to Item"));
        }
    }

  clay_parasite_list_add (private->parasites, &copy);

  if (clay_parasite_has_flag (&copy, CLAY_PARASITE_ATTACH_PARENT))
    {
      clay_parasite_shift_parent (&copy);
      clay_image_parasite_attach (private->image, &copy, TRUE);
    }
  else if (clay_parasite_has_flag (&copy, CLAY_PARASITE_ATTACH_GRANDPARENT))
    {
      clay_parasite_shift_parent (&copy);
      clay_parasite_shift_parent (&copy);
      clay_parasite_attach (private->image->clay, &copy);
    }

  if (clay_item_is_attached (item) &&
      clay_parasite_is_undoable (&copy))
    {
      clay_image_undo_group_end (private->image);
    }
}

void
clay_item_parasite_detach (ClayItem    *item,
                           const gchar *name,
                           gboolean     push_undo)
{
  ClayItemPrivate    *private;
  const ClayParasite *parasite;

  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (name != NULL);

  private = GET_PRIVATE (item);

  parasite = clay_parasite_list_find (private->parasites, name);

  if (! parasite)
    return;

  if (! clay_item_is_attached (item))
    push_undo = FALSE;

  if (push_undo)
    {
      if (clay_parasite_is_undoable (parasite))
        {
          clay_image_undo_push_item_parasite_remove (private->image,
                                                     C_("undo-type", "Remove Parasite from Item"),
                                                     item,
                                                     clay_parasite_get_name (parasite));
        }
      else if (clay_parasite_is_persistent (parasite))
        {
          clay_image_undo_push_cantundo (private->image,
                                         C_("undo-type", "Remove Parasite from Item"));
        }
    }

  clay_parasite_list_remove (private->parasites, name);
}

const ClayParasite *
clay_item_parasite_find (ClayItem    *item,
                         const gchar *name)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  return clay_parasite_list_find (GET_PRIVATE (item)->parasites, name);
}

static void
clay_item_parasite_list_foreach_func (gchar          *name,
                                      ClayParasite   *parasite,
                                      gchar        ***cur)
{
  *(*cur)++ = (gchar *) g_strdup (name);
}

gchar **
clay_item_parasite_list (ClayItem *item)
{
  ClayItemPrivate  *private;
  gint              count;
  gchar           **list;
  gchar           **cur;

  g_return_val_if_fail (CLAY_IS_ITEM (item), NULL);

  private = GET_PRIVATE (item);

  count = clay_parasite_list_length (private->parasites);

  cur = list = g_new0 (gchar *, count + 1);

  clay_parasite_list_foreach (private->parasites,
                              (GHFunc) clay_item_parasite_list_foreach_func,
                              &cur);

  return list;
}

gboolean
clay_item_set_visible (ClayItem *item,
                       gboolean  visible,
                       gboolean  push_undo)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  visible = visible ? TRUE : FALSE;

  if (clay_item_get_visible (item) != visible)
    {
      if (clay_item_is_visibility_locked (item, NULL))
        {
          return FALSE;
        }

      if (push_undo && clay_item_is_attached (item))
        {
          ClayImage *image = clay_item_get_image (item);

          if (image)
            clay_image_undo_push_item_visibility (image, NULL, item);
        }

      GET_PRIVATE (item)->visible = visible;

      if (GET_PRIVATE (item)->bind_visible_to_active)
        clay_filter_set_active (CLAY_FILTER (item), visible);

      g_signal_emit (item, clay_item_signals[VISIBILITY_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item), clay_item_props[PROP_VISIBLE]);
    }

  return TRUE;
}

gboolean
clay_item_get_visible (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->visible;
}

gboolean
clay_item_is_visible (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  if (clay_item_get_visible (item))
    {
      ClayItem *parent;

      parent = CLAY_ITEM (clay_viewable_get_parent (CLAY_VIEWABLE (item)));

      if (parent)
        return clay_item_is_visible (parent);

      return TRUE;
    }

  return FALSE;
}

void
clay_item_bind_visible_to_active (ClayItem *item,
                                  gboolean  bind)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  GET_PRIVATE (item)->bind_visible_to_active = bind;

  if (bind)
    clay_filter_set_active (CLAY_FILTER (item), clay_item_get_visible (item));
}

void
clay_item_set_color_tag (ClayItem     *item,
                         ClayColorTag  color_tag,
                         gboolean      push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));

  if (clay_item_get_color_tag (item) != color_tag)
    {
      if (push_undo && clay_item_is_attached (item))
        {
          ClayImage *image = clay_item_get_image (item);

          if (image)
            clay_image_undo_push_item_color_tag (image, NULL, item);
        }

      GET_PRIVATE (item)->color_tag = color_tag;

      g_signal_emit (item, clay_item_signals[COLOR_TAG_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                clay_item_props[PROP_COLOR_TAG]);
    }
}

ClayColorTag
clay_item_get_color_tag (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), CLAY_COLOR_TAG_NONE);

  return GET_PRIVATE (item)->color_tag;
}

ClayColorTag
clay_item_get_merged_color_tag (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), CLAY_COLOR_TAG_NONE);

  if (clay_item_get_color_tag (item) == CLAY_COLOR_TAG_NONE)
    {
      ClayItem *parent;

      parent = CLAY_ITEM (clay_viewable_get_parent (CLAY_VIEWABLE (item)));

      if (parent)
        return clay_item_get_merged_color_tag (parent);
    }

  return clay_item_get_color_tag (item);
}

void
clay_item_set_lock_content (ClayItem *item,
                            gboolean  lock_content,
                            gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_can_lock_content (item));

  lock_content = lock_content ? TRUE : FALSE;

  if (clay_item_get_lock_content (item) != lock_content)
    {
      if (push_undo && clay_item_is_attached (item))
        {
          ClayImage *image = clay_item_get_image (item);

          clay_image_undo_push_item_lock_content (image, NULL, item);
        }

      GET_PRIVATE (item)->lock_content = lock_content;

      g_signal_emit (item, clay_item_signals[LOCK_CONTENT_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                clay_item_props[PROP_LOCK_CONTENT]);
    }
}

gboolean
clay_item_get_lock_content (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_content;
}

gboolean
clay_item_can_lock_content (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
clay_item_is_content_locked (ClayItem  *item,
                             ClayItem **locked_item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return CLAY_ITEM_GET_CLASS (item)->is_content_locked (item, locked_item, TRUE);
}

void
clay_item_set_lock_position (ClayItem *item,
                             gboolean  lock_position,
                             gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_can_lock_position (item));

  lock_position = lock_position ? TRUE : FALSE;

  if (clay_item_get_lock_position (item) != lock_position)
    {
      if (push_undo && clay_item_is_attached (item))
        {
          ClayImage *image = clay_item_get_image (item);

          clay_image_undo_push_item_lock_position (image, NULL, item);
        }

      GET_PRIVATE (item)->lock_position = lock_position;

      g_signal_emit (item, clay_item_signals[LOCK_POSITION_CHANGED], 0);

      g_object_notify_by_pspec (G_OBJECT (item),
                                clay_item_props[PROP_LOCK_POSITION]);
    }
}

gboolean
clay_item_get_lock_position (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_position;
}

gboolean
clay_item_can_lock_position (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
clay_item_is_position_locked (ClayItem  *item,
                              ClayItem **locked_item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return CLAY_ITEM_GET_CLASS (item)->is_position_locked (item, locked_item, TRUE);
}

void
clay_item_set_lock_visibility (ClayItem *item,
                               gboolean  lock_visibility,
                               gboolean  push_undo)
{
  g_return_if_fail (CLAY_IS_ITEM (item));
  g_return_if_fail (clay_item_can_lock_visibility (item));

  lock_visibility = lock_visibility ? TRUE : FALSE;

  if (clay_item_get_lock_visibility (item) != lock_visibility)
    {
      if (push_undo && clay_item_is_attached (item))
        {
          ClayImage *image = clay_item_get_image (item);

          clay_image_undo_push_item_lock_visibility (image, NULL, item);
        }

      GET_PRIVATE (item)->lock_visibility = lock_visibility;

      g_signal_emit (item, clay_item_signals[LOCK_VISIBILITY_CHANGED], 0);

      g_object_notify (G_OBJECT (item), "lock-visibility");
    }
}

gboolean
clay_item_get_lock_visibility (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return GET_PRIVATE (item)->lock_visibility;
}

gboolean
clay_item_can_lock_visibility (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return TRUE;
}

gboolean
clay_item_is_visibility_locked (ClayItem  *item,
                                ClayItem **locked_item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return CLAY_ITEM_GET_CLASS (item)->is_visibility_locked (item, locked_item);
}

gboolean
clay_item_mask_bounds (ClayItem *item,
                       gint     *x1,
                       gint     *y1,
                       gint     *x2,
                       gint     *y2)
{
  ClayImage   *image;
  ClayChannel *selection;
  gint         x, y, width, height;
  gboolean     retval;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);
  g_return_val_if_fail (clay_item_is_attached (item), FALSE);

  image     = clay_item_get_image (item);
  selection = clay_image_get_mask (image);

  /* check for is_empty() before intersecting so we ignore the
   * selection if it is suspended (like when stroking)
   */
  if (CLAY_ITEM (selection) != item       &&
      ! clay_channel_is_empty (selection) &&
      clay_item_bounds (CLAY_ITEM (selection), &x, &y, &width, &height))
    {
      gint off_x = 0;
      gint off_y = 0;
      gint x2, y2;

      clay_item_get_offset (item, &off_x, &off_y);

      x2 = x + width;
      y2 = y + height;

      x  = CLAMP (x  - off_x, 0, clay_item_get_width  (item));
      y  = CLAMP (y  - off_y, 0, clay_item_get_height (item));
      x2 = CLAMP (x2 - off_x, 0, clay_item_get_width  (item));
      y2 = CLAMP (y2 - off_y, 0, clay_item_get_height (item));

      width  = x2 - x;
      height = y2 - y;

      retval = TRUE;
    }
  else
    {
      x      = 0;
      y      = 0;
      width  = clay_item_get_width  (item);
      height = clay_item_get_height (item);

      retval = FALSE;
    }

  if (x1) *x1 = x;
  if (y1) *y1 = y;
  if (x2) *x2 = x + width;
  if (y2) *y2 = y + height;

  return retval;
}

/**
 * clay_item_mask_intersect:
 * @item:   a #ClayItem
 * @x: (out) (optional): return location for x
 * @y: (out) (optional): return location for y
 * @width: (out) (optional): return location for the width
 * @height: (out) (optional): return location for the height
 *
 * Intersect the area of the @item and its image's selection mask.
 * The computed area is the bounding box of the selection within the
 * item.
 */
gboolean
clay_item_mask_intersect (ClayItem *item,
                          gint     *x,
                          gint     *y,
                          gint     *width,
                          gint     *height)
{
  ClayImage   *image;
  ClayChannel *selection;
  gint         tmp_x, tmp_y;
  gint         tmp_width, tmp_height;
  gboolean     retval;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  image     = clay_item_get_image (item);
  selection = clay_image_get_mask (image);

  /* check for is_empty() before intersecting so we ignore the
   * selection if it is suspended (like when stroking)
   */
  if (CLAY_ITEM (selection) != item       &&
      ! clay_channel_is_empty (selection) &&
      clay_item_bounds (CLAY_ITEM (selection),
                        &tmp_x, &tmp_y, &tmp_width, &tmp_height))
    {
      gint off_x = 0;
      gint off_y = 0;

      clay_item_get_offset (item, &off_x, &off_y);

      retval = clay_rectangle_intersect (tmp_x - off_x, tmp_y - off_y,
                                         tmp_width, tmp_height,
                                         0, 0,
                                         clay_item_get_width  (item),
                                         clay_item_get_height (item),
                                         &tmp_x, &tmp_y,
                                         &tmp_width, &tmp_height);
    }
  else
    {
      tmp_x      = 0;
      tmp_y      = 0;
      tmp_width  = clay_item_get_width  (item);
      tmp_height = clay_item_get_height (item);

      retval = TRUE;
    }

  if (x)      *x      = tmp_x;
  if (y)      *y      = tmp_y;
  if (width)  *width  = tmp_width;
  if (height) *height = tmp_height;

  return retval;
}

gboolean
clay_item_is_in_set (ClayItem    *item,
                     ClayItemSet  set)
{
  ClayItemPrivate *private;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  private = GET_PRIVATE (item);

  switch (set)
    {
    case CLAY_ITEM_SET_NONE:
      return FALSE;

    case CLAY_ITEM_SET_ALL:
      return TRUE;

    case CLAY_ITEM_SET_IMAGE_SIZED:
      return (clay_item_get_width  (item) == clay_image_get_width  (private->image) &&
              clay_item_get_height (item) == clay_image_get_height (private->image));

    case CLAY_ITEM_SET_VISIBLE:
      return clay_item_get_visible (item);
    }

  return FALSE;
}

gboolean
clay_item_is_rasterizable (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return (clay_item_is_text_layer (item) ||
          clay_item_is_link_layer (item) ||
          clay_item_is_vector_layer (item));
}

gboolean
clay_item_is_rasterized (ClayItem *item)
{
  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  return ((CLAY_IS_TEXT_LAYER (item) && ! clay_item_is_text_layer (item)) ||
          (CLAY_IS_LINK_LAYER (item) && ! clay_item_is_link_layer (item)) ||
          (CLAY_IS_VECTOR_LAYER (item) && ! clay_item_is_vector_layer (item)));
}

/* Returns whether @item is writable. If it is not writable,
 * @read_only_item will be set to the first non-writable item found
 * which made @item non-writable (in some cases, it can be a child or
 * parent item).
 *
 * Non-writable items are:
 *
 * - Rasterizable items (which were not rasterized) or groups with a
 *   rasterizable child;
 * - Items whose content is locked (because itself, a child or parent is
 *   locked).
 */
gboolean
clay_item_is_writable (ClayItem  *item,
                       ClayItem **read_only_item)
{
  ClayContainer *container;

  g_return_val_if_fail (CLAY_IS_ITEM (item), FALSE);

  if (clay_item_is_content_locked (item, read_only_item))
    {
      return FALSE;
    }
  else if (clay_item_is_rasterizable (item))
    {
      if (read_only_item)
        *read_only_item = item;
      return FALSE;
    }
  else if ((container = clay_viewable_get_children (CLAY_VIEWABLE (item))))
    {
      gint n_children = clay_container_get_n_children (container);

      for (gint i = 0; i < n_children; i++)
        {
          ClayObject *child = clay_container_get_child_by_index (container, i);

          if (! clay_item_is_writable (CLAY_ITEM (child), read_only_item))
            return FALSE;
        }
    }

  return TRUE;
}
