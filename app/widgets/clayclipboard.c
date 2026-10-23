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

#include "libclaymath/claymath.h"

#include "widgets-types.h"

#include "core/clay.h"
#include "core/claybuffer.h"
#include "core/claycurve.h"
#include "core/clayimage.h"
#include "core/claypickable.h"

#include "clayclipboard.h"
#include "claypixbuf.h"
#include "clayselectiondata.h"

#include "clay-intl.h"


#define CLAY_CLIPBOARD_KEY "clay-clipboard"


typedef struct _ClayClipboard ClayClipboard;

struct _ClayClipboard
{
  GSList         *pixbuf_formats;

  GtkTargetEntry *image_target_entries;
  gint            n_image_target_entries;

  GtkTargetEntry *buffer_target_entries;
  gint            n_buffer_target_entries;

  GtkTargetEntry *svg_target_entries;
  gint            n_svg_target_entries;

  GtkTargetEntry *curve_target_entries;
  gint            n_curve_target_entries;

  ClayImage      *image;
  ClayBuffer     *buffer;
  gchar          *svg;
  ClayCurve      *curve;

#ifdef G_OS_WIN32
  gboolean        storing;
  guint           max_info;
#endif
};


static ClayClipboard * clay_clipboard_get        (Clay             *clay);

static ClayClipboard * clay_clipboard_new        (gboolean          verbose);
static void            clay_clipboard_free       (ClayClipboard    *clay_clip);

static void      clay_clipboard_clear            (ClayClipboard    *clay_clip);

static GdkAtom * clay_clipboard_wait_for_targets (Clay             *clay,
                                                  gint             *n_targets);
static GdkAtom   clay_clipboard_wait_for_image   (Clay             *clay);
static GdkAtom   clay_clipboard_wait_for_buffer  (Clay             *clay);
static GdkAtom   clay_clipboard_wait_for_svg     (Clay             *clay);
static GdkAtom   clay_clipboard_wait_for_curve   (Clay             *clay);

static void      clay_clipboard_send_image       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Clay             *clay);
static void      clay_clipboard_send_buffer      (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Clay             *clay);
static void      clay_clipboard_send_svg         (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Clay             *clay);
static void      clay_clipboard_send_curve       (GtkClipboard     *clipboard,
                                                  GtkSelectionData *data,
                                                  guint             info,
                                                  Clay             *clay);


/*  public functions  */

void
clay_clipboard_init (Clay *clay)
{
  ClayClipboard *clay_clip;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_clip = clay_clipboard_get (clay);

  g_return_if_fail (clay_clip == NULL);

  clay_clip = clay_clipboard_new (clay->be_verbose);

#ifdef G_OS_WIN32
  clay_clip->storing  = FALSE;
  clay_clip->max_info = 1;
#endif

  g_object_set_data_full (G_OBJECT (clay), CLAY_CLIPBOARD_KEY,
                          clay_clip, (GDestroyNotify) clay_clipboard_free);
}

void
clay_clipboard_exit (Clay *clay)
{
  GtkClipboard *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) == G_OBJECT (clay))
    {
#ifdef G_OS_WIN32
      ClayClipboard *clay_clip = clay_clipboard_get (clay);

      clay_clip->storing = TRUE;
#endif
      gtk_clipboard_store (clipboard);
    }

  if (clipboard)
    /* If we don't clear the clipboard, it keeps a reference on the object
     * owner (i.e. Clay object probably) which fails to finalize.
     */
    gtk_clipboard_clear (clipboard);

  g_object_set_data (G_OBJECT (clay), CLAY_CLIPBOARD_KEY, NULL);
}

/**
 * clay_clipboard_has_image:
 * @clay: pointer to #Clay
 *
 * Tests if there's an image in the clipboard. If the global image cut
 * buffer of @clay is empty, this function returns %NULL.
 *
 * Returns: %TRUE if there's an image in the clipboard, %FALSE otherwise
 **/
gboolean
clay_clipboard_has_image (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      if (clay_clipboard_wait_for_image (clay) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  clay_clip = clay_clipboard_get (clay);

  return (clay_clip->image != NULL);
}

/**
 * clay_clipboard_has_buffer:
 * @clay: pointer to #Clay
 *
 * Tests if there's image data in the clipboard. If the global cut
 * buffer of @clay is empty, this function checks if there's image
 * data in %GDK_SELECTION_CLIPBOARD. This is done in a main-loop
 * similar to gtk_clipboard_wait_is_text_available(). The same caveats
 * apply here.
 *
 * Returns: %TRUE if there's image data in the clipboard, %FALSE otherwise
 **/
gboolean
clay_clipboard_has_buffer (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      if (clay_clipboard_wait_for_buffer (clay) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  clay_clip = clay_clipboard_get (clay);

  return (clay_clip->buffer != NULL);
}

/**
 * clay_clipboard_has_svg:
 * @clay: pointer to #Clay
 *
 * Tests if there's SVG data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's SVG data in the clipboard, %FALSE otherwise
 **/
gboolean
clay_clipboard_has_svg (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      if (clay_clipboard_wait_for_svg (clay) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  clay_clip = clay_clipboard_get (clay);

  return (clay_clip->svg != NULL);
}

/**
 * clay_clipboard_has_curve:
 * @clay: pointer to #Clay
 *
 * Tests if there's curve data in %GDK_SELECTION_CLIPBOARD.
 * This is done in a main-loop similar to
 * gtk_clipboard_wait_is_text_available(). The same caveats apply here.
 *
 * Returns: %TRUE if there's curve data in the clipboard, %FALSE otherwise
 **/
gboolean
clay_clipboard_has_curve (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      if (clay_clipboard_wait_for_curve (clay) != GDK_NONE)
        {
          return TRUE;
        }

      return FALSE;
    }

  clay_clip = clay_clipboard_get (clay);

  return (clay_clip->curve != NULL);
}

/**
 * clay_clipboard_get_object:
 * @clay: pointer to #Clay
 *
 * Retrieves either an image or a buffer from the global image cut
 * buffer of @clay.
 *
 * The returned #ClayObject needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #ClayObject or %NULL if there's no
 *               image or buffer in the clipboard
 **/
ClayObject *
clay_clipboard_get_object (Clay *clay)
{
  ClayObject *object;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);

  object = CLAY_OBJECT (clay_clipboard_get_image (clay));

  if (! object)
    object = CLAY_OBJECT (clay_clipboard_get_buffer (clay));

  return object;
}

/**
 * clay_clipboard_get_image:
 * @clay: pointer to #Clay
 *
 * Retrieves an image from the global image cut buffer of @clay.
 *
 * The returned #ClayImage needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #ClayImage or %NULL if there's no
 *               image in the clipboard
 **/
ClayImage *
clay_clipboard_get_image (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;
  ClayImage     *image = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      GdkAtom atom = clay_clipboard_wait_for_image (clay);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          clay_set_busy (clay);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              image = clay_selection_data_get_xcf (data, clay);

              gtk_selection_data_free (data);
            }

          clay_unset_busy (clay);
        }

      return image;
    }

  clay_clip = clay_clipboard_get (clay);

  if (! image && clay_clip->image)
    image = g_object_ref (clay_clip->image);

  return image;
}

/**
 * clay_clipboard_get_buffer:
 * @clay: pointer to #Clay
 *
 * Retrieves either image data from %GDK_SELECTION_CLIPBOARD or from
 * the global cut buffer of @clay.
 *
 * The returned #ClayBuffer needs to be unref'ed when it's no longer
 * needed.
 *
 * Returns: (nullable): a reference to a #ClayBuffer or %NULL if there's no
 *               image data
 **/
ClayBuffer *
clay_clipboard_get_buffer (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;
  ClayBuffer    *buffer = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      GdkAtom atom = clay_clipboard_wait_for_buffer (clay);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          clay_set_busy (clay);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              GdkPixbuf *pixbuf = gtk_selection_data_get_pixbuf (data);

              gtk_selection_data_free (data);

              if (pixbuf)
                {
                  buffer = clay_buffer_new_from_pixbuf (pixbuf, _("Clipboard"),
                                                        0, 0);
                  g_object_unref (pixbuf);
                }
            }

          if (! buffer)
            {
              /* on macOS, GDK may fail to on some mimes (like image/png).
                 Let it find any other usable target as last resort. See: #16768 */
              GdkPixbuf *pixbuf = gtk_clipboard_wait_for_image (clipboard);

              if (pixbuf)
                {
                  buffer = clay_buffer_new_from_pixbuf (pixbuf, _("Clipboard"),
                                                        0, 0);
                  g_object_unref (pixbuf);
                }
            }

          clay_unset_busy (clay);
        }

      return buffer;
    }

  clay_clip = clay_clipboard_get (clay);

  if (! buffer && clay_clip->buffer)
    buffer = g_object_ref (clay_clip->buffer);

  return buffer;
}

/**
 * clay_clipboard_get_svg:
 * @clay: pointer to #Clay
 * @svg_length: returns the size of the SVG stream in bytes
 *
 * Retrieves SVG data from %GDK_SELECTION_CLIPBOARD or from the global
 * SVG buffer of @clay.
 *
 * The returned data needs to be freed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #ClayBuffer or %NULL if there's no
 *               image data
 **/
gchar *
clay_clipboard_get_svg (Clay  *clay,
                        gsize *svg_length)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;
  gchar         *svg = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (svg_length != NULL, NULL);

  *svg_length = 0;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      GdkAtom atom = clay_clipboard_wait_for_svg (clay);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          clay_set_busy (clay);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              const guchar *stream;

              stream = clay_selection_data_get_stream (data, svg_length);

              if (stream)
                svg = g_memdup2 (stream, *svg_length);

              gtk_selection_data_free (data);
            }

          clay_unset_busy (clay);
        }

      return svg;
    }

  clay_clip = clay_clipboard_get (clay);

  if (! svg && clay_clip->svg)
    {
      svg = g_strdup (clay_clip->svg);
      *svg_length = strlen (svg);
    }

  return svg;
}

/**
 * clay_clipboard_get_curve:
 * @clay: pointer to #Clay
 *
 * Retrieves curve data from %GDK_SELECTION_CLIPBOARD or from the global
 * curve buffer of @clay.
 *
 * The returned curve needs to be unref'ed when it's no longer needed.
 *
 * Returns: (nullable): a reference to a #ClayCurve or %NULL if there's no
 *               curve data
 **/
ClayCurve *
clay_clipboard_get_curve (Clay *clay)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;
  ClayCurve     *curve = NULL;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard &&
      gtk_clipboard_get_owner (clipboard) != G_OBJECT (clay))
    {
      GdkAtom atom = clay_clipboard_wait_for_curve (clay);

      if (atom != GDK_NONE)
        {
          GtkSelectionData *data;

          clay_set_busy (clay);

          data = gtk_clipboard_wait_for_contents (clipboard, atom);

          if (data)
            {
              curve = clay_selection_data_get_curve (data);

              gtk_selection_data_free (data);
            }

          clay_unset_busy (clay);
        }

      return curve;
    }

  clay_clip = clay_clipboard_get (clay);

  if (! curve && clay_clip->curve)
    curve = g_object_ref (clay_clip->curve);

  return curve;
}

/**
 * clay_clipboard_set_image:
 * @clay:  pointer to #Clay
 * @image: (nullable): a #ClayImage, or %NULL.
 *
 * Offers the image in %GDK_SELECTION_CLIPBOARD.
 **/
void
clay_clipboard_set_image (Clay      *clay,
                          ClayImage *image)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (image == NULL || CLAY_IS_IMAGE (image));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  clay_clip = clay_clipboard_get (clay);

  clay_clipboard_clear (clay_clip);

  if (image)
    {
      clay_clip->image = g_object_ref (image);

      gtk_clipboard_set_with_owner (clipboard,
                                    clay_clip->image_target_entries,
                                    clay_clip->n_image_target_entries,
                                    (GtkClipboardGetFunc) clay_clipboard_send_image,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (clay));

      /*  mark the first two entries (image/x-xcf and image/png) as
       *  suitable for storing
       */
      gtk_clipboard_set_can_store (clipboard,
                                   clay_clip->image_target_entries,
                                   MIN (2, clay_clip->n_image_target_entries));
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (clay))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * clay_clipboard_set_buffer:
 * @clay:   pointer to #Clay
 * @buffer: (nullable): a #ClayBuffer, or %NULL.
 *
 * Offers the buffer in %GDK_SELECTION_CLIPBOARD.
 **/
void
clay_clipboard_set_buffer (Clay       *clay,
                           ClayBuffer *buffer)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (buffer == NULL || CLAY_IS_BUFFER (buffer));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  clay_clip = clay_clipboard_get (clay);

  clay_clipboard_clear (clay_clip);

  if (buffer)
    {
      clay_clip->buffer = g_object_ref (buffer);

      gtk_clipboard_set_with_owner (clipboard,
                                    clay_clip->buffer_target_entries,
                                    clay_clip->n_buffer_target_entries,
                                    (GtkClipboardGetFunc) clay_clipboard_send_buffer,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (clay));

      /*  mark the first entry (image/png) as suitable for storing  */
      if (clay_clip->n_buffer_target_entries > 0)
        gtk_clipboard_set_can_store (clipboard,
                                     clay_clip->buffer_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (clay))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * clay_clipboard_set_svg:
 * @clay: pointer to #Clay
 * @svg: (nullable): a string containing the SVG data, or %NULL
 *
 * Offers SVG data in %GDK_SELECTION_CLIPBOARD.
 **/
void
clay_clipboard_set_svg (Clay        *clay,
                        const gchar *svg)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  clay_clip = clay_clipboard_get (clay);

  clay_clipboard_clear (clay_clip);

  if (svg)
    {
      clay_clip->svg = g_strdup (svg);

      gtk_clipboard_set_with_owner (clipboard,
                                    clay_clip->svg_target_entries,
                                    clay_clip->n_svg_target_entries,
                                    (GtkClipboardGetFunc) clay_clipboard_send_svg,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (clay));

      /*  mark the first entry (image/svg) as suitable for storing  */
      gtk_clipboard_set_can_store (clipboard,
                                   clay_clip->svg_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (clay))
    {
      gtk_clipboard_clear (clipboard);
    }
}

/**
 * clay_clipboard_set_text:
 * @clay: pointer to #Clay
 * @text: a %NULL-terminated string in UTF-8 encoding
 *
 * Offers @text in %GDK_SELECTION_CLIPBOARD and %GDK_SELECTION_PRIMARY.
 **/
void
clay_clipboard_set_text (Clay        *clay,
                         const gchar *text)
{
  GtkClipboard *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (text != NULL);

  clay_clipboard_clear (clay_clipboard_get (clay));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (clipboard)
    gtk_clipboard_set_text (clipboard, text, -1);

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_PRIMARY);
  if (clipboard)
    gtk_clipboard_set_text (clipboard, text, -1);
}

/**
 * clay_clipboard_set_curve:
 * @clay: pointer to #Clay
 * @curve: (nullable): a #ClayCurve, or %NULL
 *
 * Offers curve data in %GDK_SELECTION_CLIPBOARD.
 **/
void
clay_clipboard_set_curve (Clay      *clay,
                          ClayCurve *curve)
{
  ClayClipboard *clay_clip;
  GtkClipboard  *clipboard;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (curve == NULL || CLAY_IS_CURVE (curve));

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);
  if (! clipboard)
    return;

  clay_clip = clay_clipboard_get (clay);

  clay_clipboard_clear (clay_clip);

  if (curve)
    {
      clay_clip->curve = g_object_ref (curve);

      gtk_clipboard_set_with_owner (clipboard,
                                    clay_clip->curve_target_entries,
                                    clay_clip->n_curve_target_entries,
                                    (GtkClipboardGetFunc) clay_clipboard_send_curve,
                                    (GtkClipboardClearFunc) NULL,
                                    G_OBJECT (clay));

      gtk_clipboard_set_can_store (clipboard,
                                   clay_clip->curve_target_entries, 1);
    }
  else if (gtk_clipboard_get_owner (clipboard) == G_OBJECT (clay))
    {
      gtk_clipboard_clear (clipboard);
    }
}


/*  private functions  */

static ClayClipboard *
clay_clipboard_get (Clay *clay)
{
  return g_object_get_data (G_OBJECT (clay), CLAY_CLIPBOARD_KEY);
}

static ClayClipboard *
clay_clipboard_new (gboolean verbose)
{
  ClayClipboard *clay_clip = g_slice_new0 (ClayClipboard);
  GSList        *list;

  clay_clip->pixbuf_formats = clay_pixbuf_get_formats ();

  for (list = clay_clip->pixbuf_formats; list; list = g_slist_next (list))
    {
      GdkPixbufFormat *format = list->data;

      if (gdk_pixbuf_format_is_writable (format))
        {
          gchar **mime_types;
          gchar **type;

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            clay_clip->n_buffer_target_entries++;

          g_strfreev (mime_types);
        }
    }

  /*  the image_target_entries have the XCF target, and all pixbuf
   *  targets that are also in buffer_target_entries
   */
  clay_clip->n_image_target_entries = clay_clip->n_buffer_target_entries + 1;
  clay_clip->image_target_entries   = g_new0 (GtkTargetEntry,
                                              clay_clip->n_image_target_entries);

  clay_clip->image_target_entries[0].target = g_strdup ("image/x-xcf");
  clay_clip->image_target_entries[0].flags  = 0;
  clay_clip->image_target_entries[0].info   = 0;

  if (clay_clip->n_buffer_target_entries > 0)
    {
      gint i = 0;

      clay_clip->buffer_target_entries = g_new0 (GtkTargetEntry,
                                                 clay_clip->n_buffer_target_entries);

      for (list = clay_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat *format = list->data;

          if (gdk_pixbuf_format_is_writable (format))
            {
              gchar  *format_name;
              gchar **mime_types;
              gchar **type;

              format_name = gdk_pixbuf_format_get_name (format);
              mime_types  = gdk_pixbuf_format_get_mime_types (format);

              for (type = mime_types; *type; type++)
                {
                  const gchar *mime_type = *type;

                  if (verbose)
                    g_printerr ("clipboard: writable pixbuf format: %s\n",
                                mime_type);

                  clay_clip->image_target_entries[i + 1].target = g_strdup (mime_type);
                  clay_clip->image_target_entries[i + 1].flags  = 0;
                  clay_clip->image_target_entries[i + 1].info   = i + 1;

                  clay_clip->buffer_target_entries[i].target = g_strdup (mime_type);
                  clay_clip->buffer_target_entries[i].flags  = 0;
                  clay_clip->buffer_target_entries[i].info   = i;

                  i++;
                }

              g_strfreev (mime_types);
              g_free (format_name);
            }
        }
    }

  clay_clip->n_svg_target_entries = 2;
  clay_clip->svg_target_entries   = g_new0 (GtkTargetEntry, 2);

  clay_clip->svg_target_entries[0].target = g_strdup ("image/svg");
  clay_clip->svg_target_entries[0].flags  = 0;
  clay_clip->svg_target_entries[0].info   = 0;

  clay_clip->svg_target_entries[1].target = g_strdup ("image/svg+xml");
  clay_clip->svg_target_entries[1].flags  = 0;
  clay_clip->svg_target_entries[1].info   = 1;

  clay_clip->n_curve_target_entries = 1;
  clay_clip->curve_target_entries   = g_new0 (GtkTargetEntry, 1);

  clay_clip->curve_target_entries[0].target = g_strdup ("application/x-clay-curve");
  clay_clip->curve_target_entries[0].flags  = 0;
  clay_clip->curve_target_entries[0].info   = 0;

  return clay_clip;
}

static void
clay_clipboard_free (ClayClipboard *clay_clip)
{
  gint i;

  clay_clipboard_clear (clay_clip);

  g_slist_free (clay_clip->pixbuf_formats);

  for (i = 0; i < clay_clip->n_image_target_entries; i++)
    g_free ((gchar *) clay_clip->image_target_entries[i].target);

  g_free (clay_clip->image_target_entries);

  for (i = 0; i < clay_clip->n_buffer_target_entries; i++)
    g_free ((gchar *) clay_clip->buffer_target_entries[i].target);

  g_free (clay_clip->buffer_target_entries);

  for (i = 0; i < clay_clip->n_svg_target_entries; i++)
    g_free ((gchar *) clay_clip->svg_target_entries[i].target);

  g_free (clay_clip->svg_target_entries);

  for (i = 0; i < clay_clip->n_curve_target_entries; i++)
    g_free ((gchar *) clay_clip->curve_target_entries[i].target);

  g_free (clay_clip->curve_target_entries);

  g_slice_free (ClayClipboard, clay_clip);
}

static void
clay_clipboard_clear (ClayClipboard *clay_clip)
{
  g_clear_object (&clay_clip->image);
  g_clear_object (&clay_clip->buffer);
  g_clear_pointer (&clay_clip->svg, g_free);
  g_clear_object (&clay_clip->curve);
}

static GdkAtom *
clay_clipboard_wait_for_targets (Clay *clay,
                                 gint *n_targets)
{
  GtkClipboard *clipboard;

  clipboard = gtk_clipboard_get_for_display (gdk_display_get_default (),
                                             GDK_SELECTION_CLIPBOARD);

  if (clipboard)
    {
      GtkSelectionData *data;
      GdkAtom           atom = gdk_atom_intern_static_string ("TARGETS");

      data = gtk_clipboard_wait_for_contents (clipboard, atom);

      if (data)
        {
          GdkAtom  *targets;
          gboolean  success;

          success = gtk_selection_data_get_targets (data, &targets, n_targets);

          gtk_selection_data_free (data);

          if (success)
            {
              if (clay->be_verbose)
                {
                  gint i;

                  for (i = 0; i < *n_targets; i++)
                    g_printerr ("clipboard: offered type: %s\n",
                                gdk_atom_name (targets[i]));

                  g_printerr ("\n");
                }

              return targets;
            }
        }
    }

  return NULL;
}

static GdkAtom
clay_clipboard_wait_for_image (Clay *clay)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = clay_clipboard_wait_for_targets (clay, &n_targets);

  if (targets)
    {
      GdkAtom image_atom = gdk_atom_intern_static_string ("image/x-xcf");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == image_atom)
            {
              result = image_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

static GdkAtom
clay_clipboard_wait_for_buffer (Clay *clay)
{
  ClayClipboard *clay_clip = clay_clipboard_get (clay);
  GdkAtom       *targets;
  gint           n_targets;
  GdkAtom        result    = GDK_NONE;

  targets = clay_clipboard_wait_for_targets (clay, &n_targets);

  if (targets)
    {
      GSList *list;

      for (list = clay_clip->pixbuf_formats; list; list = g_slist_next (list))
        {
          GdkPixbufFormat  *format = list->data;
          gchar           **mime_types;
          gchar           **type;

          if (clay->be_verbose)
            g_printerr ("clipboard: checking pixbuf format '%s'\n",
                        gdk_pixbuf_format_get_name (format));

          mime_types = gdk_pixbuf_format_get_mime_types (format);

          for (type = mime_types; *type; type++)
            {
              gchar   *mime_type = *type;
              GdkAtom  atom      = gdk_atom_intern (mime_type, FALSE);
              gint     i;

              if (clay->be_verbose)
                g_printerr ("  - checking mime type '%s'\n", mime_type);

              for (i = 0; i < n_targets; i++)
                {
                  if (targets[i] == atom)
                    {
                      result = atom;
                      break;
                    }
                }

              if (result != GDK_NONE)
                break;
            }

          g_strfreev (mime_types);

          if (result != GDK_NONE)
            break;
        }

      g_free (targets);
    }

  return result;
}

static GdkAtom
clay_clipboard_wait_for_svg (Clay *clay)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = clay_clipboard_wait_for_targets (clay, &n_targets);

  if (targets)
    {
      GdkAtom svg_atom     = gdk_atom_intern_static_string ("image/svg");
      GdkAtom svg_xml_atom = gdk_atom_intern_static_string ("image/svg+xml");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == svg_atom)
            {
              result = svg_atom;
              break;
            }
          else if (targets[i] == svg_xml_atom)
            {
              result = svg_xml_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

static GdkAtom
clay_clipboard_wait_for_curve (Clay *clay)
{
  GdkAtom *targets;
  gint     n_targets;
  GdkAtom  result = GDK_NONE;

  targets = clay_clipboard_wait_for_targets (clay, &n_targets);

  if (targets)
    {
      GdkAtom curve_atom = gdk_atom_intern_static_string ("application/x-clay-curve");
      gint    i;

      for (i = 0; i < n_targets; i++)
        {
          if (targets[i] == curve_atom)
            {
              result = curve_atom;
              break;
            }
        }

      g_free (targets);
    }

  return result;
}

/* gdk_pixbuf_format_is_save_option_supported() was added in gdk-pixbuf 2.36
 * This version check as well as the IGNORE_DEPRECATIONS below can be removed
 * if gdk_pixbuf_minver in meson.build gets bumped
 */
#if GDK_PIXBUF_CHECK_VERSION(2, 36, 0)
static gboolean
clay_clipboard_saver_supports_dpi (const gchar *format_name)
{
  GSList   *formats;
  GSList   *l;
  gboolean  supported = FALSE;

  formats = gdk_pixbuf_get_formats ();
  for (l = formats; l; l = l->next)
    {
      GdkPixbufFormat *format = l->data;

      if (strcmp (gdk_pixbuf_format_get_name (format), format_name) == 0)
        {
          G_GNUC_BEGIN_IGNORE_DEPRECATIONS
          supported = gdk_pixbuf_format_is_save_option_supported (format, "x-dpi") &&
                      gdk_pixbuf_format_is_save_option_supported (format, "y-dpi");
          G_GNUC_END_IGNORE_DEPRECATIONS
          break;
        }
    }

  g_slist_free (formats);
  return supported;
}
#endif

static void
clay_clipboard_send_pixbuf (GtkSelectionData *data,
                            GdkPixbuf        *pixbuf,
                            gdouble           res_x,
                            gdouble           res_y,
                            gboolean          verbose)
{
  GdkAtom      target      = gtk_selection_data_get_target (data);
  const gchar *format      = NULL;
  const gchar *compression = NULL;
  gchar        x_dpi_str[16];
  gchar        y_dpi_str[16];

  g_snprintf (x_dpi_str, sizeof (x_dpi_str), "%d", ROUND (res_x));
  g_snprintf (y_dpi_str, sizeof (y_dpi_str), "%d", ROUND (res_y));

  gdk_pixbuf_set_option (pixbuf, "x-dpi", x_dpi_str);
  gdk_pixbuf_set_option (pixbuf, "y-dpi", y_dpi_str);

  /* gtk_selection_data_set_pixbuf does not pass DPI information to gdk-pixbuf
   * or considers DPI options set via gdk_pixbuf_set_option. To support pasting
   * PNGs to other apps that support reading DPI information from PNG images
   * we need to rely on lower-level functions that allow setting these props.
   * On macOS, other apps read images from the pasteboard as TIFF instead.
   */
  if (target == gdk_atom_intern_static_string ("image/png"))
    {
      format      = "png";
      compression = "2";
    }
  else if (target == gdk_atom_intern_static_string ("image/tiff"))
    {
      format      = "tiff";
      compression = "1"; /* COMPRESSION_NONE, libtiff's default */
    }

  if (format)
    {
      gchar    *buffer      = NULL;
      gsize     buffer_size = 0;
      gboolean  saved;

#if GDK_PIXBUF_CHECK_VERSION(2, 36, 0)
      if (clay_clipboard_saver_supports_dpi (format))
        {
          if (verbose)
            g_printerr ("clipboard: setting x-dpi=%s y-dpi=%s on %s clipboard data\n",
                        x_dpi_str, y_dpi_str, format);

          saved = gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size,
                                             format, NULL,
                                             "compression", compression,
                                             "x-dpi", x_dpi_str,
                                             "y-dpi", y_dpi_str,
                                             NULL);
        }
      else
#endif
        {
          saved = gdk_pixbuf_save_to_buffer (pixbuf, &buffer, &buffer_size,
                                             format, NULL,
                                             "compression", compression,
                                             NULL);
        }
      if (saved)
        {
          gtk_selection_data_set (data, target,
                                  8, (guchar *) buffer, buffer_size);
        }

      g_free (buffer);
    }
  else
    {
      gtk_selection_data_set_pixbuf (data, pixbuf);
    }
}

static void
clay_clipboard_send_image (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Clay             *clay)
{
  ClayClipboard *clay_clip = clay_clipboard_get (clay);

  if (clay_clip == NULL)
    return;

  clay_set_busy (clay);

  if (info == 0)
    {
      if (clay->be_verbose)
        g_printerr ("clipboard: sending image data as '%s'\n",
                    clay_clip->image_target_entries[info].target);

      clay_selection_data_set_xcf (data, clay_clip->image);
    }
  else
    {
      GdkPixbuf *pixbuf;

#ifdef G_OS_WIN32
      /* On Windows gtk_clipboard_store ignores the number of items we have
       * set to be stored, instead storing all. Since this can be a long list,
       * shutting down Clay on Windows can take considerable time when you
       * have a larger selection on the clipboard. So we manually stop
       * handling every target larger than 1 (png) and even stop handling
       * png after the first time, since it often gets called multiple times
       * for png.
       */
      if (clay_clip->storing)
        {
          if (info > clay_clip->max_info)
            {
              clay_unset_busy (clay);
              return;
            }
          else
            {
              clay_clip->max_info = 0;
            }
        }
#endif
      clay_pickable_flush (CLAY_PICKABLE (clay_clip->image));

      pixbuf = clay_viewable_get_pixbuf (CLAY_VIEWABLE (clay_clip->image),
                                         clay_get_user_context (clay),
                                         clay_image_get_width (clay_clip->image),
                                         clay_image_get_height (clay_clip->image),
                                         1, NULL);

      if (pixbuf)
        {
          gdouble res_x;
          gdouble res_y;

          clay_image_get_resolution (clay_clip->image, &res_x, &res_y);

          if (clay->be_verbose)
            g_printerr ("clipboard: sending image data as '%s'\n",
                        clay_clip->image_target_entries[info].target);

          clay_clipboard_send_pixbuf (data, pixbuf, res_x, res_y,
                                      clay->be_verbose);
        }
      else
        {
          g_warning ("%s: clay_viewable_get_pixbuf() failed", G_STRFUNC);
        }
    }

  clay_unset_busy (clay);
}

static void
clay_clipboard_send_buffer (GtkClipboard     *clipboard,
                            GtkSelectionData *data,
                            guint             info,
                            Clay             *clay)
{
  ClayClipboard *clay_clip = clay_clipboard_get (clay);
  GdkPixbuf     *pixbuf;

  if (clay_clip == NULL)
    return;

  clay_set_busy (clay);

  pixbuf = clay_viewable_get_pixbuf (CLAY_VIEWABLE (clay_clip->buffer),
                                     clay_get_user_context (clay),
                                     clay_buffer_get_width (clay_clip->buffer),
                                     clay_buffer_get_height (clay_clip->buffer),
                                     1, NULL);

  if (pixbuf)
    {
      gdouble res_x;
      gdouble res_y;

      clay_buffer_get_resolution (clay_clip->buffer, &res_x, &res_y);

      if (clay->be_verbose)
        g_printerr ("clipboard: sending pixbuf data as '%s'\n",
                    clay_clip->buffer_target_entries[info].target);

      clay_clipboard_send_pixbuf (data, pixbuf, res_x, res_y,
                                  clay->be_verbose);
    }
  else
    {
      g_warning ("%s: clay_viewable_get_pixbuf() failed", G_STRFUNC);
    }

  clay_unset_busy (clay);
}

static void
clay_clipboard_send_svg (GtkClipboard     *clipboard,
                         GtkSelectionData *data,
                         guint             info,
                         Clay             *clay)
{
  ClayClipboard *clay_clip = clay_clipboard_get (clay);

  if (clay_clip == NULL)
    return;

  clay_set_busy (clay);

  if (clay_clip->svg)
    {
      if (clay->be_verbose)
        g_printerr ("clipboard: sending SVG data as '%s'\n",
                    clay_clip->svg_target_entries[info].target);

      clay_selection_data_set_stream (data,
                                      (const guchar *) clay_clip->svg,
                                      strlen (clay_clip->svg));
    }

  clay_unset_busy (clay);
}

static void
clay_clipboard_send_curve (GtkClipboard     *clipboard,
                           GtkSelectionData *data,
                           guint             info,
                           Clay             *clay)
{
  ClayClipboard *clay_clip = clay_clipboard_get (clay);

  if (clay_clip == NULL)
    return;

  clay_set_busy (clay);

  if (clay_clip->curve)
    {
      if (clay->be_verbose)
        g_printerr ("clipboard: sending curve data as '%s'\n",
                    clay_clip->curve_target_entries[info].target);

      clay_selection_data_set_curve (data, clay_clip->curve);
    }

  clay_unset_busy (clay);
}
