/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claypatternclipboard.c
 * Copyright (C) 2006 Michael Natterer <mitch@gimp.org>
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

#include "core-types.h"

#include "clay.h"
#include "claybuffer.h"
#include "claychannel.h"
#include "claypatternclipboard.h"
#include "clayimage.h"
#include "claypickable.h"
#include "clayselection.h"
#include "claytempbuf.h"

#include "clay-intl.h"


#if defined(ARCH_ARM64) || defined(ARCH_X86_64) || defined(ARCH_PPC64)
#define PATTERN_MAX_SIZE 8192
#else
#define PATTERN_MAX_SIZE 1024
#endif

enum
{
  PROP_0,
  PROP_CLAY
};


/*  local function prototypes  */

static void       clay_pattern_clipboard_constructed  (GObject      *object);
static void       clay_pattern_clipboard_set_property (GObject      *object,
                                                       guint         property_id,
                                                       const GValue *value,
                                                       GParamSpec   *pspec);
static void       clay_pattern_clipboard_get_property (GObject      *object,
                                                       guint         property_id,
                                                       GValue       *value,
                                                       GParamSpec   *pspec);
static ClayData * clay_pattern_clipboard_duplicate    (ClayData     *data);

static void       clay_pattern_clipboard_changed      (Clay         *clay,
                                                       ClayPattern  *pattern);


G_DEFINE_TYPE (ClayPatternClipboard, clay_pattern_clipboard, CLAY_TYPE_PATTERN)

#define parent_class clay_pattern_clipboard_parent_class


static void
clay_pattern_clipboard_class_init (ClayPatternClipboardClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS (klass);
  ClayDataClass *data_class   = CLAY_DATA_CLASS (klass);

  object_class->constructed  = clay_pattern_clipboard_constructed;
  object_class->set_property = clay_pattern_clipboard_set_property;
  object_class->get_property = clay_pattern_clipboard_get_property;

  data_class->duplicate      = clay_pattern_clipboard_duplicate;

  g_object_class_install_property (object_class, PROP_CLAY,
                                   g_param_spec_object ("clay", NULL, NULL,
                                                        CLAY_TYPE_CLAY,
                                                        CLAY_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_pattern_clipboard_init (ClayPatternClipboard *pattern)
{
}

static void
clay_pattern_clipboard_constructed (GObject *object)
{
  ClayPatternClipboard *pattern = CLAY_PATTERN_CLIPBOARD (object);

  G_OBJECT_CLASS (parent_class)->constructed (object);

  clay_assert (CLAY_IS_CLAY (pattern->clay));

  g_signal_connect_object (pattern->clay, "clipboard-changed",
                           G_CALLBACK (clay_pattern_clipboard_changed),
                           pattern, 0);

  clay_pattern_clipboard_changed (pattern->clay, CLAY_PATTERN (pattern));
}

static void
clay_pattern_clipboard_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  ClayPatternClipboard *pattern = CLAY_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_CLAY:
      pattern->clay = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
clay_pattern_clipboard_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  ClayPatternClipboard *pattern = CLAY_PATTERN_CLIPBOARD (object);

  switch (property_id)
    {
    case PROP_CLAY:
      g_value_set_object (value, pattern->clay);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static ClayData *
clay_pattern_clipboard_duplicate (ClayData *data)
{
  ClayData *new = g_object_new (CLAY_TYPE_PATTERN, NULL);

  clay_data_copy (new, data);

  return new;
}

ClayData *
clay_pattern_clipboard_new (Clay *clay)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  return g_object_new (CLAY_TYPE_PATTERN_CLIPBOARD,
                       "name", _("Clipboard Image"),
                       "clay", clay,
                       NULL);
}


/*  private functions  */

static void
clay_pattern_clipboard_changed (Clay        *clay,
                                ClayPattern *pattern)
{
  ClayObject *paste;
  GeglBuffer *buffer       = NULL;
  gboolean    unref_buffer = FALSE;

  g_clear_pointer (&pattern->mask, clay_temp_buf_unref);

  paste = clay_get_clipboard_object (clay);

  if (CLAY_IS_IMAGE (paste))
    {
      ClayContext *context = clay_get_user_context (clay);

      clay_pickable_flush (CLAY_PICKABLE (paste));
      if (context)
        {
          ClayChannel *mask = clay_image_get_mask (CLAY_IMAGE (paste));

          if (! clay_channel_is_empty (mask))
            {
              GList *pickables;
              gint   offset_x;
              gint   offset_y;

              pickables = g_list_prepend (NULL, CLAY_IMAGE (paste));
              buffer = clay_selection_extract (CLAY_SELECTION (mask),
                                               pickables, context,
                                               FALSE, FALSE, FALSE,
                                               &offset_x, &offset_y,
                                               NULL);
              g_list_free (pickables);

              unref_buffer = TRUE;
            }
          else
            {
              buffer = clay_pickable_get_buffer (CLAY_PICKABLE (paste));
            }
        }
      else
        {
          buffer = clay_pickable_get_buffer (CLAY_PICKABLE (paste));
        }
    }
  else if (CLAY_IS_BUFFER (paste))
    {
      buffer = clay_buffer_get_buffer (CLAY_BUFFER (paste));
    }

  if (buffer)
    {
      gint width  = MIN (gegl_buffer_get_width  (buffer), PATTERN_MAX_SIZE);
      gint height = MIN (gegl_buffer_get_height (buffer), PATTERN_MAX_SIZE);

      pattern->mask = clay_temp_buf_new (width, height,
                                         gegl_buffer_get_format (buffer));

      gegl_buffer_get (buffer,
                       GEGL_RECTANGLE (0, 0, width, height), 1.0,
                       NULL,
                       clay_temp_buf_get_data (pattern->mask),
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (unref_buffer)
        g_object_unref (buffer);
    }
  else
    {
      pattern->mask = clay_temp_buf_new (16, 16, babl_format ("R'G'B' u8"));
      memset (clay_temp_buf_get_data (pattern->mask), 255, 16 * 16 * 3);
    }

  clay_data_dirty (CLAY_DATA (pattern));
}
