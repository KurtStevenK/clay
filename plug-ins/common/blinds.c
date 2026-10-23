/*
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * This is a plug-in for Clay.
 *
 * Blinds plug-in. Distort an image as though it was stuck to
 * window blinds and the blinds where opened/closed.
 *
 * Copyright (C) 1997 Andy Thomas  alt@picnic.demon.co.uk
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
 * A fair proportion of this code was taken from the Whirl plug-in
 * which was copyrighted by Federico Mena Quintero (as below).
 *
 * Whirl plug-in --- distort an image into a whirlpool
 * Copyright (C) 1997 Federico Mena Quintero
 *
 */

#include "config.h"

#include <string.h>

#include <libclay/clay.h>
#include <libclay/clayui.h>

#include "libclay/stdplugins-intl.h"

/***** Magic numbers *****/

#define PLUG_IN_PROC   "plug-in-blinds"
#define PLUG_IN_BINARY "blinds"
#define PLUG_IN_ROLE   "clay-blinds"

#define MAX_FANS       1024


typedef struct _Blinds      Blinds;
typedef struct _BlindsClass BlindsClass;

struct _Blinds
{
  ClayPlugIn parent_instance;
};

struct _BlindsClass
{
  ClayPlugInClass parent_class;
};


#define BLINDS_TYPE  (blinds_get_type ())
#define BLINDS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BLINDS_TYPE, Blinds))

GType                   blinds_get_type         (void);

static GList          * blinds_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * blinds_create_procedure (ClayPlugIn           *plug_in,
                                                 const gchar          *name);

static ClayValueArray * blinds_run              (ClayProcedure        *procedure,
                                                 ClayRunMode           run_mode,
                                                 ClayImage            *image,
                                                 ClayDrawable        **drawables,
                                                 ClayProcedureConfig  *config,
                                                 gpointer              run_data);

static gboolean         blinds_dialog           (ClayProcedure        *procedure,
                                                 GObject              *config,
                                                 ClayDrawable         *drawable);

static void             dialog_update_preview   (GtkWidget            *widget,
                                                 GObject              *config);
static void             apply_blinds            (GObject              *config,
                                                 ClayDrawable         *drawable);


G_DEFINE_TYPE (Blinds, blinds, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (BLINDS_TYPE)
DEFINE_STD_SET_I18N


/* Array to hold each size of fans. And no there are not each the
 * same size (rounding errors...)
 */

static gint fanwidths[MAX_FANS];

static void
blinds_class_init (BlindsClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = blinds_query_procedures;
  plug_in_class->create_procedure = blinds_create_procedure;
  plug_in_class->set_i18n         = STD_SET_I18N;
}

static void
blinds_init (Blinds *blinds)
{
}

static GList *
blinds_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}

static ClayProcedure *
blinds_create_procedure (ClayPlugIn  *plug_in,
                         const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (! strcmp (name, PLUG_IN_PROC))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            blinds_run, NULL, NULL);

      clay_procedure_set_image_types (procedure, "RGB*, GRAY*");
      clay_procedure_set_sensitivity_mask (procedure,
                                           CLAY_PROCEDURE_SENSITIVE_DRAWABLE);

      clay_procedure_set_menu_label (procedure, _("_Blinds..."));
      clay_procedure_add_menu_path (procedure, "<Image>/Filters/Distorts");

      clay_procedure_set_documentation (procedure,
                                        _("Simulate an image painted on "
                                          "window blinds"),
                                        NULL, name);
      clay_procedure_set_attribution (procedure,
                                      "Andy Thomas",
                                      "Andy Thomas",
                                      "1997");

      clay_procedure_add_int_argument (procedure, "angle-displacement",
                                       _("_Displacement"),
                                       _("Angle of Displacement"),
                                       0, 90, 30,
                                       G_PARAM_READWRITE);

      clay_procedure_add_int_argument (procedure, "num-segments",
                                       _("_Number of segments"),
                                       _("Number of segments in blinds"),
                                       1, MAX_FANS, 3,
                                       G_PARAM_READWRITE);

      clay_procedure_add_choice_argument (procedure, "orientation",
                                          _("Orient_ation"),
                                          _("The orientation"),
                                          clay_choice_new_with_values ("horizontal", CLAY_ORIENTATION_HORIZONTAL, _("Horizontal"),  NULL,
                                                                       "vertical",   CLAY_ORIENTATION_VERTICAL,   _("Vertical"),    NULL,
                                                                       NULL),
                                          "horizontal",
                                          G_PARAM_READWRITE);

      clay_procedure_add_boolean_argument (procedure, "bg-transparent",
                                           _("_Transparent"),
                                           _("Background transparent"),
                                           FALSE,
                                           G_PARAM_READWRITE);
    }

  return procedure;
}

static ClayValueArray *
blinds_run (ClayProcedure        *procedure,
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

  if (run_mode == CLAY_RUN_INTERACTIVE && ! blinds_dialog (procedure, G_OBJECT (config), drawable))
    return clay_procedure_new_return_values (procedure,
                                             CLAY_PDB_CANCEL,
                                             NULL);

  if (clay_drawable_is_rgb  (drawable) ||
      clay_drawable_is_gray (drawable))
    {
      clay_progress_init (_("Adding blinds"));

      apply_blinds (G_OBJECT (config), drawable);

      if (run_mode != CLAY_RUN_NONINTERACTIVE)
        clay_displays_flush ();
    }
  else
    {
      return clay_procedure_new_return_values (procedure,
                                               CLAY_PDB_EXECUTION_ERROR,
                                               NULL);
    }

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}


static gboolean
blinds_dialog (ClayProcedure *procedure,
               GObject       *config,
               ClayDrawable  *drawable)
{
  GtkWidget    *dialog;
  GtkWidget    *preview;
  GtkWidget    *vbox;
  GtkWidget    *hbox;
  GtkWidget    *scale;
  gboolean      run;

  clay_ui_init (PLUG_IN_BINARY);

  dialog = clay_procedure_dialog_new (procedure,
                                      CLAY_PROCEDURE_CONFIG (config),
                                      _("Blinds"));

  clay_procedure_dialog_get_widget (CLAY_PROCEDURE_DIALOG (dialog),
                                    "orientation", CLAY_TYPE_INT_RADIO_FRAME);

  clay_procedure_dialog_get_label (CLAY_PROCEDURE_DIALOG (dialog),
                                   "bg_label", _("Background"), FALSE, FALSE);

  clay_procedure_dialog_fill_frame (CLAY_PROCEDURE_DIALOG (dialog),
                                    "bg-frame", "bg_label", FALSE,
                                    "bg-transparent");
  clay_procedure_dialog_set_sensitive (CLAY_PROCEDURE_DIALOG (dialog), "bg-frame",
                                       clay_drawable_has_alpha (drawable),
                                       NULL, NULL, FALSE);

  hbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                         "blinds-hbox", "orientation",
                                         "bg-frame", NULL);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (hbox),
                                  GTK_ORIENTATION_HORIZONTAL);
  gtk_box_set_spacing (GTK_BOX (hbox), 12);
  gtk_box_set_homogeneous (GTK_BOX (hbox), TRUE);
  gtk_widget_set_margin_bottom (hbox, 12);

  scale = clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                                "angle-displacement", 1.0);
  gtk_widget_set_margin_bottom (scale, 12);

  clay_procedure_dialog_get_spin_scale (CLAY_PROCEDURE_DIALOG (dialog),
                                        "num-segments", 1.0);

  vbox = clay_procedure_dialog_fill_box (CLAY_PROCEDURE_DIALOG (dialog),
                                         "blinds-vbox", "blinds-hbox",
                                         "angle-displacement", "num-segments",
                                         NULL);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 12);
  gtk_widget_set_size_request (vbox, 320, -1);

  preview = clay_procedure_dialog_get_drawable_preview (CLAY_PROCEDURE_DIALOG (dialog),
                                                        "preview", drawable);
  gtk_widget_set_margin_bottom (preview, 12);

  g_signal_connect (preview, "invalidated",
                    G_CALLBACK (dialog_update_preview),
                    config);

  g_signal_connect_swapped (config, "notify",
                            G_CALLBACK (clay_preview_invalidate),
                            preview);

  clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog),
                              "preview", "blinds-vbox",
                              NULL);

  gtk_widget_set_visible (dialog, TRUE);

  run = clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog));

  gtk_widget_destroy (dialog);

  return run;
}

static void
blindsapply (GObject       *config,
             const guchar  *srow,
             guchar        *drow,
             gint           width,
             gint           bpp,
             guchar        *bg)
{
  const guchar  *src;
  guchar  *dst;
  gint     i,j,k;
  gdouble  ang;
  gint     available;
  gint     angledsp;
  gint     numsegs;

  g_object_get (config,
                "angle-displacement", &angledsp,
                "num-segments",       &numsegs,
                NULL);

  /* Make the row 'shrink' around points along its length */
  /* The numsegs determines how many segments to slip it in to */
  /* The angle is the conceptual 'rotation' of each of these segments */

  /* Note the row is considered to be made up of a two dim array actual
   * pixel locations and the RGB color at these locations.
   */

  /* In the process copy the src row to the destination row */

  /* Fill in with background color ? */
  for (i = 0 ; i < width ; i++)
    {
      dst = &drow[i*bpp];

      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = bg[j];
        }
    }

  /* Apply it */

  available = width;
  for (i = 0; i < numsegs; i++)
    {
      /* Width of segs are variable */
      fanwidths[i] = available / (numsegs - i);
      available -= fanwidths[i];
    }

  /* do center points  first - just for fun...*/
  available = 0;
  for (k = 1; k <= numsegs; k++)
    {
      int point;

      point = available + fanwidths[k - 1] / 2;

      available += fanwidths[k - 1];

      src = &srow[point * bpp];
      dst = &drow[point * bpp];

      /* Copy pixels across */
      for (j = 0 ; j < bpp; j++)
        {
          dst[j] = src[j];
        }
    }

  /* Disp for each point */
  ang = (angledsp * 2 * G_PI) / 360; /* Angle in rads */
  ang = (1 - fabs (cos (ang)));

  available = 0;
  for (k = 0 ; k < numsegs; k++)
    {
      int dx; /* Amount to move by */
      int fw;

      for (i = 0 ; i < (fanwidths[k]/2) ; i++)
        {
          /* Copy pixels across of left half of fan */
          fw = fanwidths[k] / 2;
          dx = (int) (ang * ((double) (fw - (double)(i % fw))));

          src = &srow[(available + i) * bpp];
          dst = &drow[(available + i + dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }

          /* Right side */
          j = i + 1;
          src = &srow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2)) * bpp];
          dst = &drow[(available + fanwidths[k] - j
                       - (fanwidths[k] % 2) - dx) * bpp];

          for (j = 0; j < bpp; j++)
            {
              dst[j] = src[j];
            }
        }

      available += fanwidths[k];
    }
}

static void
dialog_update_preview (GtkWidget *widget,
                       GObject   *config)
{
  ClayDrawablePreview *preview  = CLAY_DRAWABLE_PREVIEW (widget);
  ClayDrawable        *drawable = clay_drawable_preview_get_drawable (preview);
  gint                 y;
  const guchar        *p;
  guchar              *buffer;
  GBytes              *cache;
  const guchar        *cache_start;
  GeglColor           *color;
  guchar               bg[4];
  gint                 width;
  gint                 height;
  gint                 bpp;
  ClayOrientationType  orientation;
  gint                 bg_trans;

  g_object_get (config,
                "bg-transparent", &bg_trans,
                NULL);
  orientation = clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                                     "orientation");

  clay_preview_get_size (CLAY_PREVIEW (preview), &width, &height);
  cache = clay_drawable_get_thumbnail_data (drawable,
                                            width, height,
                                            &width, &height, &bpp);
  p = cache_start = g_bytes_get_data (cache, NULL);

  color = clay_context_get_background ();

  if (bg_trans)
    clay_color_set_alpha (color, 0.0);

  if (clay_drawable_is_gray (drawable))
    {
      guchar luminance[2];

      gegl_color_get_pixel (color, babl_format ("Y'A u8"), luminance);
      bg[0] = luminance[0];
      bg[3] = luminance[1];
    }
  else
    {
      gegl_color_get_pixel (color, babl_format ("R'G'B'A u8"), bg);
    }
  g_object_unref (color);

  buffer = g_new (guchar, width * height * bpp);

  if (orientation == CLAY_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < height; y++)
        {
          blindsapply (config, p,
                       buffer + y * width * bpp,
                       width,
                       bpp, bg);
          p += width * bpp;
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transformation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint i;
      guchar *sr = g_new (guchar, height * 4);
      guchar *dr = g_new0 (guchar, height * 4);
      guchar dummybg[4] = {0, 0, 0, 0};

      /* Fill in with background color ? */
      for (i = 0 ; i < width ; i++)
        {
          gint j;
          gint bd = bpp;
          guchar *dst;
          dst = &buffer[i * bd];

          for (j = 0 ; j < bd; j++)
            {
              dst[j] = bg[j];
            }
        }

      for ( y = 0 ; y < height; y++)
        {
          sr[y] = y+1;
        }

      /* Bit of a fiddle since blindsapply really works on an image
       * row not a set of bytes. - preview can't be > 255
       * or must make dr sr int rows.
       */
      blindsapply (config, sr, dr, height, 1, dummybg);

      for (y = 0; y < height; y++)
        {
          if (dr[y] == 0)
            {
              /* Draw background line */
              p = buffer;
            }
          else
            {
              /* Draw line from src */
              p = cache_start +
                (width * bpp * (dr[y] - 1));
            }
          memcpy (buffer + y * width * bpp,
                  p,
                  width * bpp);
        }
      g_free (sr);
      g_free (dr);
    }

  clay_preview_draw_buffer (CLAY_PREVIEW (preview), buffer, width * bpp);
  clay_preview_set_bounds (CLAY_PREVIEW (preview), 0, 0, width, height);
  clay_preview_set_size (CLAY_PREVIEW (preview), width, height);

  g_free (buffer);
  g_bytes_unref (cache);
}

/* STEP tells us how many rows/columns to gulp down in one go... */
/* Note all the "4" literals around here are to do with the depths
 * of the images. Makes it easier to deal with for my small brain.
 */

#define STEP 40

static void
apply_blinds (GObject      *config,
              ClayDrawable *drawable)
{
  GeglBuffer          *src_buffer;
  GeglBuffer          *dest_buffer;
  const Babl          *format;
  guchar              *src_rows, *des_rows;
  gint                 bytes;
  gint                 x, y;
  GeglColor           *background;
  guchar               bg[4];
  gint                 sel_x1, sel_y1;
  gint                 sel_width, sel_height;
  ClayOrientationType  orientation;
  gint                 bg_trans;

  g_object_get (config,
                "bg-transparent", &bg_trans,
                NULL);
  orientation = clay_procedure_config_get_choice_id (CLAY_PROCEDURE_CONFIG (config),
                                                     "orientation");

  background = clay_context_get_background ();

  if (bg_trans)
    clay_color_set_alpha (background, 0.0);

  gegl_color_get_pixel (background, babl_format_with_space ("R'G'B'A u8", NULL), bg);
  g_object_unref (background);

  if (! clay_drawable_mask_intersect (drawable,
                                      &sel_x1, &sel_y1,
                                      &sel_width, &sel_height))
    return;

  if (clay_drawable_has_alpha (drawable))
    format = babl_format ("R'G'B'A u8");
  else
    format = babl_format ("R'G'B' u8");

  bytes = babl_format_get_bytes_per_pixel (format);

  src_buffer  = clay_drawable_get_buffer (drawable);
  dest_buffer = clay_drawable_get_shadow_buffer (drawable);

  src_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);
  des_rows = g_new (guchar, MAX (sel_width, sel_height) * bytes * STEP);

  if (orientation == CLAY_ORIENTATION_VERTICAL)
    {
      for (y = 0; y < sel_height; y += STEP)
        {
          gint rr;
          gint step;

          if ((y + STEP) > sel_height)
            step = sel_height - y;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < STEP; rr++)
            blindsapply (config,
                         src_rows + (sel_width * rr * bytes),
                         des_rows + (sel_width * rr * bytes),
                         sel_width, bytes, bg);

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1, sel_y1 + y,
                                           sel_width, step), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          clay_progress_update ((double) y / (double) sel_height);
        }
    }
  else
    {
      /* Horizontal blinds */
      /* Apply the blinds algo to a single column -
       * this act as a transformation matrix for the
       * rows. Make row 0 invalid so we can find it again!
       */
      gint    i;
      gint   *sr  = g_new (gint, sel_height * bytes);
      gint   *dr  = g_new (gint, sel_height * bytes);
      guchar *dst = g_new (guchar, STEP * bytes);
      guchar  dummybg[4];

      memset (dummybg, 0, 4);
      memset (dr, 0, sel_height * bytes); /* all dr rows are background rows */
      for (y = 0; y < sel_height; y++)
        {
          sr[y] = y+1;
        }

      /* Hmmm. does this work portably? */
      /* This "swaps" the integers around that are held in in the
       * sr & dr arrays.
       */
      blindsapply (config, (guchar *) sr, (guchar *) dr,
                   sel_height, sizeof (gint), dummybg);

      /* Fill in with background color ? */
      for (i = 0 ; i < STEP ; i++)
        {
          int     j;
          guchar *bgdst;
          bgdst = &dst[i * bytes];

          for (j = 0 ; j < bytes; j++)
            {
              bgdst[j] = bg[j];
            }
        }

      for (x = 0; x < sel_width; x += STEP)
        {
          int     rr;
          int     step;
          guchar *p;

          if((x + STEP) > sel_width)
            step = sel_width - x;
          else
            step = STEP;

          gegl_buffer_get (src_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 1.0,
                           format, src_rows,
                           GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);

          /* OK I could make this better */
          for (rr = 0; rr < sel_height; rr++)
            {
              if(dr[rr] == 0)
                {
                  /* Draw background line */
                  p = dst;
                }
              else
                {
                  /* Draw line from src */
                  p = src_rows + (step * bytes * (dr[rr] - 1));
                }
              memcpy (des_rows + (rr * step * bytes), p,
                      step * bytes);
            }

          gegl_buffer_set (dest_buffer,
                           GEGL_RECTANGLE (sel_x1 + x, sel_y1,
                                           step, sel_height), 0,
                           format, des_rows,
                           GEGL_AUTO_ROWSTRIDE);

          clay_progress_update ((double) x / (double) sel_width);
        }

      g_free (dst);
      g_free (sr);
      g_free (dr);
    }

  g_free (src_rows);
  g_free (des_rows);

  g_object_unref (src_buffer);
  g_object_unref (dest_buffer);

  clay_progress_update (1.0);

  clay_drawable_merge_shadow (drawable, TRUE);
  clay_drawable_update (drawable,
                        sel_x1, sel_y1, sel_width, sel_height);
}
