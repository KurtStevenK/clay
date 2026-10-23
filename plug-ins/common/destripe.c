/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * Destripe filter
 *
 * Copyright 1997 Marc Lehmann, heavily modified from a filter by
 * Michael Sweet.
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
 *
 */

#include "config.h"

#include <string.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"


#define PLUG_IN_PROC    "plug-in-destripe"
#define PLUG_IN_BINARY  "destripe"
#define PLUG_IN_ROLE    "clay-destripe"
#define PLUG_IN_VERSION "0.2"
#define MAX_AVG         100


typedef struct _Destripe      Destripe;
typedef struct _DestripeClass DestripeClass;

struct _Destripe
{
  ClayPlugIn parent_instance;
};

struct _DestripeClass
{
  ClayPlugInClass parent_class;
};


#define DESTRIPE_TYPE  (destripe_get_type ())
#define DESTRIPE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), DESTRIPE_TYPE, Destripe))

GType                   destripe_get_type         (void);

static GList          * destripe_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * destripe_create_procedure (ClayPlugIn           *plug_in,
                                                   const gchar          *name);

static ClayValueArray * destripe_run              (ClayProcedure        *procedure,
                                                   ClayRunMode           run_mode,
                                                   ClayImage            *image,
                                                   ClayDrawable        **drawables,
                                                   ClayProcedureConfig  *config,
                                                   gpointer              run_data);

static void             destripe                  (GObject              *config,
                                                   ClayDrawable         *drawable,
                                                   ClayPreview          *preview);
static void             destripe_preview          (GtkWidget            *widget,
                                                   GObject              *config);

static gboolean         destripe_dialog           (ClayProcedure        *procedure,
                                                   GObject              *config,
                                                   ClayDrawable         *drawable);


G_DEFINE_TYPE (Destripe, destripe, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (DESTRIPE_TYPE)
DEFINE_STD_SET_I18N


static void
destripe_class_init (DestripeClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = destripe_query_procedures;
  plug_in_class->create_procedure = destripe_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
destripe_init (Destripe *destripe)
{
}

static GList *
destripe_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static ClayProcedure *
destripe_create_procedure (ClayPlugIn  *plug_in,
                               const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            destripe_run, NULL, NULL);

      clay_procedure_set_image_types (procedure, "RGB*, GRAY*");
      clay_procedure_set_sensitivity_mask (procedure,
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLE);

      clay_procedure_set_menu_label (procedure, _("Des_tripe..."));
      clay_procedure_add_menu_path (procedure, "<Image>/Colors/Tone Mapping");

      clay_procedure_set_documentation (procedure,
                                        _("Remove vertical stripe artifacts "
                                          "from the image"),
                                        _("This plug-in tries to remove vertical "
                                          "stripes from an image."),
                                        name);
      clay_procedure_set_attribution (procedure,
                                      "Marc Lehmann <pcg@goof.com>",
                                      "Marc Lehmann <pcg@goof.com>",
                                      PLUG_IN_VERSION);

      clay_procedure_add_int_argument (procedure, "avg-width",
                                       _("_Width"),
                                       _("Averaging filter width"),
                                       2, MAX_AVG, 36,
                                       G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure,
                                           "create-histogram",
                                           _("Create _histogram"),
                                           _("Output a histogram"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
destripe_run (ClayProcedure        *procedure,
              ClayRunMode           run_mode,
              ClayImage            *image,
              ClayDrawable        **drawables,
              ClayProcedureConfig  *config,
              gpointer              run_data)
{
  ClayDrawable *drawable;

  gegl_init (NULL, NULL);

  if (clay_core_object_array_get_length ((GObject **) drawables) != 1)
    {
      GError *error = NULL;

      g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                   _("Procedure '%s' only works with one drawable."),
                   PLUG_IN_PROC);

      return clay_procedure_new_return_values (procedure,
                                               CLAY_PDB_CALLING_ERROR,
                                               error);
    }
  else
    {
      drawable = drawables[0];
    }

  if (run_mode == CLAY_RUN_INTERACTIVE && ! destripe_dialog (procedure, G_OBJECT (config), drawable))
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_CANCEL,
                                             NULL);

  if (clay_drawable_is_rgb (drawable)|| clay_drawable_is_gray (drawable))
    destripe (G_OBJECT (config), drawable, NULL);
  else
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_EXECUTION_ERROR,
                                             NULL);

  if (run_mode != CLAY_RUN_NONINTERACTIVE)
    clay_displays_flush ();

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}

static void
destripe (GObject      *config,
          ClayDrawable *drawable,
          ClayPreview  *preview)
{
  GeglBuffer *src_buffer;
  GeglBuffer *dest_buffer;
  const Babl *format;
  guchar     *src_rows;       /* image data */
  gdouble     progress, progress_inc;
  gint        x1, x2, y1;
  gint        width, height;
  gint        bpp;
  glong      *hist, *corr;        /* "histogram" data */
  gint        tile_width = clay_tile_width ();
  gint        i, x, y, ox, cols;
  gint        avg_width;
  gboolean    histogram;

  g_object_get (config,
                "avg-width",        &avg_width,
                "create-histogram", &histogram,
                NULL);

  progress     = 0.0;
  progress_inc = 0.0;

  if (preview)
    {
      clay_preview_get_position (preview, &x1, &y1);
      clay_preview_get_size (preview, &width, &height);
    }
  else
    {
      clay_progress_init (_("Destriping"));

      if (! clay_drawable_mask_intersect (drawable,
                                          &x1, &y1, &width, &height))
        {
          return;
        }

      progress = 0;
      progress_inc = 0.5 * tile_width / width;
    }

  x2 = x1 + width;

  if (clay_drawable_is_rgb (drawable))
    {
      if (clay_drawable_has_alpha (drawable))
        format = babl_format ("R'G'B'A u8");
      else
        format = babl_format ("R'G'B' u8");
    }
  else
    {
      if (clay_drawable_has_alpha (drawable))
        format = babl_format ("Y'A u8");
      else
        format = babl_format ("Y' u8");
    }

  bpp = babl_format_get_bytes_per_pixel (format);

  /*
   * Setup for filter...
   */

  src_buffer  = clay_drawable_get_buffer (drawable);
  dest_buffer = clay_drawable_get_shadow_buffer (drawable);

  hist = g_new (long, width * bpp);
  corr = g_new (long, width * bpp);
  src_rows = g_new (guchar, tile_width * height * bpp);

  memset (hist, 0, width * bpp * sizeof (long));

  /*
   * collect "histogram" data.
   */

  for (ox = x1; ox < x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 1.0,
                       format, rows,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      for (y = 0; y < height; y++)
        {
          long   *h       = hist + (ox - x1) * bpp;
          guchar *row_end = rows + cols * bpp;

          while (rows < row_end)
            *h++ += *rows++;
        }

      if (! preview)
        clay_progress_update (progress += progress_inc);
    }

  /*
   * average out histogram
   */

  {
    gint extend = (avg_width / 2) * bpp;

    for (i = 0; i < MIN (3, bpp); i++)
      {
        long *h   = hist - extend + i;
        long *c   = corr - extend + i;
        long  sum = 0;
        gint  cnt = 0;

        for (x = -extend; x < width * bpp; x += bpp)
          {
            if (x + extend < width * bpp)
              {
                sum += h[ extend]; cnt++;
              }

            if (x - extend >= 0)
              {
                sum -= h[-extend]; cnt--;
              }

            if (x >= 0)
              {
                if (*h)
                  *c = ((sum / cnt - *h) << 10) / *h;
                else
                  *c = G_MAXINT;
              }

            h += bpp;
            c += bpp;
          }
      }
  }

  /*
   * remove stripes.
   */

  for (ox = x1; ox < x2; ox += tile_width)
    {
      guchar *rows = src_rows;

      cols = x2 - ox;
      if (cols > tile_width)
        cols = tile_width;

      gegl_buffer_get (src_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 1.0,
                       format, rows,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      if (! preview)
        clay_progress_update (progress += progress_inc);

      for (y = 0; y < height; y++)
        {
          long   *c = corr + (ox - x1) * bpp;
          guchar *row_end = rows + cols * bpp;

          if (histogram)
            {
              while (rows < row_end)
                {
                  *rows = MIN (255, MAX (0, 128 + (*rows * *c >> 10)));
                  c++; rows++;
                }
            }
          else
            {
              while (rows < row_end)
                {
                  *rows = MIN (255, MAX (0, *rows + (*rows * *c >> 10) ));
                  c++; rows++;
                }
            }
        }

      gegl_buffer_set (dest_buffer, GEGL_RECTANGLE (ox, y1, cols, height), 0,
                       format, src_rows,
                       GEGL_AUTO_ROWSTRIDE);

      if (! preview)
        clay_progress_update (progress += progress_inc);
    }

  g_free (src_rows);

  g_object_unref (src_buffer);

  if (preview)
    {
      guchar *buffer = g_new (guchar, width * height * bpp);

      gegl_buffer_get (dest_buffer, GEGL_RECTANGLE (x1, y1, width, height), 1.0,
                       format, buffer,
                       GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

      clay_preview_draw_buffer (CLAY_PREVIEW (preview),
                                buffer, width * bpp);

      g_free (buffer);
      g_object_unref (dest_buffer);
    }
  else
    {
      g_object_unref (dest_buffer);

      clay_progress_update (1.0);

      clay_drawable_merge_shadow (drawable, TRUE);
      clay_drawable_update (drawable,
                            x1, y1, width, height);
    }

  g_free (hist);
  g_free (corr);
}

static void
destripe_preview (GtkWidget *widget,
                  GObject   *config)
{
  ClayPreview  *preview  = CLAY_PREVIEW (widget);
  ClayDrawable *drawable = g_object_get_data (config, "drawable");

  destripe (config, drawable, preview);
}


static gboolean
destripe_dialog (ClayProcedure *procedure,
                 GObject       *config,
                 ClayDrawable  *drawable)
{
  GtkWidget     *dialog;
  GtkWidget     *preview;
  GtkWidget     *scale;
  GtkWidget     *button;
  gboolean       run;

  clay_ui_init (PLUG_IN_BINARY);

  dialog = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Destripe"));

  clay_window_set_transient (GTK_WINDOW (dialog));

  preview = clay_procedure_dialog_get_drawable_preview (CLAY_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);

  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                                "avg-width", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  button = clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                             "create-histogram",
                                             GTK_TYPE_CHECK_BUTTON);
  gtk_widget_set_margin_bottom (button, 12);

  g_object_set_data (config, "drawable", drawable);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (destripe_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (clay_preview_invalidate),
                            preview);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "preview", "avg-width", "create-histogram",
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}
