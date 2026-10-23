/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 2009 Martin Nordholts
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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#define close _close
#endif

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "widgets/widgets-types.h"

#include "widgets/clayuimanager.h"

#include "core/clay.h"
#include "core/claychannel.h"
#include "core/claychannel-select.h"
#include "core/claydrawable.h"
#include "core/claygrid.h"
#include "core/claygrouplayer.h"
#include "core/clayguide.h"
#include "core/clayimage.h"
#include "core/clayimage-grid.h"
#include "core/clayimage-guides.h"
#include "core/clayimage-sample-points.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"
#include "core/claysamplepoint.h"
#include "core/clayselection.h"

#include "path/clayanchor.h"
#include "path/claybezierstroke.h"
#include "path/claypath.h"

#include "plug-in/claypluginmanager-file.h"

#include "file/file-open.h"
#include "file/file-save.h"

#include "tests.h"

#include "clay-app-test-utils.h"


/* we continue to use LEGACY layers for testing, so we can use the
 * same test image for all tests, including loading
 * files/clay-2-6-file.xcf which can't have any non-LEGACY modes
 */

#define CLAY_MAINIMAGE_WIDTH            100
#define CLAY_MAINIMAGE_HEIGHT           90
#define CLAY_MAINIMAGE_TYPE             CLAY_RGB
#define CLAY_MAINIMAGE_PRECISION        CLAY_PRECISION_U8_NON_LINEAR

#define CLAY_MAINIMAGE_LAYER1_NAME      "layer1"
#define CLAY_MAINIMAGE_LAYER1_WIDTH     50
#define CLAY_MAINIMAGE_LAYER1_HEIGHT    51
#define CLAY_MAINIMAGE_LAYER1_FORMAT    babl_format ("R'G'B'A u8")
#define CLAY_MAINIMAGE_LAYER1_OPACITY   CLAY_OPACITY_OPAQUE
#define CLAY_MAINIMAGE_LAYER1_MODE      CLAY_LAYER_MODE_NORMAL_LEGACY

#define CLAY_MAINIMAGE_LAYER2_NAME      "layer2"
#define CLAY_MAINIMAGE_LAYER2_WIDTH     25
#define CLAY_MAINIMAGE_LAYER2_HEIGHT    251
#define CLAY_MAINIMAGE_LAYER2_FORMAT    babl_format ("R'G'B' u8")
#define CLAY_MAINIMAGE_LAYER2_OPACITY   CLAY_OPACITY_TRANSPARENT
#define CLAY_MAINIMAGE_LAYER2_MODE      CLAY_LAYER_MODE_MULTIPLY_LEGACY

#define CLAY_MAINIMAGE_GROUP1_NAME      "group1"

#define CLAY_MAINIMAGE_LAYER3_NAME      "layer3"

#define CLAY_MAINIMAGE_LAYER4_NAME      "layer4"

#define CLAY_MAINIMAGE_GROUP2_NAME      "group2"

#define CLAY_MAINIMAGE_LAYER5_NAME      "layer5"

#define CLAY_MAINIMAGE_VGUIDE1_POS      42
#define CLAY_MAINIMAGE_VGUIDE2_POS      82
#define CLAY_MAINIMAGE_HGUIDE1_POS      3
#define CLAY_MAINIMAGE_HGUIDE2_POS      4

#define CLAY_MAINIMAGE_SAMPLEPOINT1_X   10
#define CLAY_MAINIMAGE_SAMPLEPOINT1_Y   12
#define CLAY_MAINIMAGE_SAMPLEPOINT2_X   41
#define CLAY_MAINIMAGE_SAMPLEPOINT2_Y   49

#define CLAY_MAINIMAGE_RESOLUTIONX      400
#define CLAY_MAINIMAGE_RESOLUTIONY      410

#define CLAY_MAINIMAGE_PARASITE_NAME    "test-parasite"
#define CLAY_MAINIMAGE_PARASITE_DATA    "foo"
#define CLAY_MAINIMAGE_PARASITE_SIZE    4                /* 'f' 'o' 'o' '\0' */

#define CLAY_MAINIMAGE_COMMENT          "Created with code from "\
                                        "app/tests/test-xcf.c in the Clay "\
                                        "source tree, i.e. it was not created "\
                                        "manually and may thus look weird if "\
                                        "opened and inspected in Clay."

#define CLAY_MAINIMAGE_UNIT             clay_unit_pica ()

#define CLAY_MAINIMAGE_GRIDXSPACING     25.0
#define CLAY_MAINIMAGE_GRIDYSPACING     27.0

#define CLAY_MAINIMAGE_CHANNEL1_NAME    "channel1"
#define CLAY_MAINIMAGE_CHANNEL1_WIDTH   CLAY_MAINIMAGE_WIDTH
#define CLAY_MAINIMAGE_CHANNEL1_HEIGHT  CLAY_MAINIMAGE_HEIGHT
#define CLAY_MAINIMAGE_CHANNEL1_COLOR   { 1.0, 0.0, 1.0, 1.0 }

#define CLAY_MAINIMAGE_SELECTION_X      5
#define CLAY_MAINIMAGE_SELECTION_Y      6
#define CLAY_MAINIMAGE_SELECTION_W      7
#define CLAY_MAINIMAGE_SELECTION_H      8

#define CLAY_MAINIMAGE_PATH1_NAME       "vectors1"
#define CLAY_MAINIMAGE_PATH1_COORDS     { { 11.0, 12.0, /* pad zeroes */ },\
                                          { 21.0, 22.0, /* pad zeroes */ },\
                                          { 31.0, 32.0, /* pad zeroes */ }, }

#define CLAY_MAINIMAGE_PATH2_NAME       "vectors2"
#define CLAY_MAINIMAGE_PATH2_COORDS     { { 911.0, 912.0, /* pad zeroes */ },\
                                          { 921.0, 922.0, /* pad zeroes */ },\
                                          { 931.0, 932.0, /* pad zeroes */ }, }

#define ADD_TEST(function) \
  g_test_add_data_func ("/clay-xcf/" #function, clay, function);


ClayImage        * clay_test_load_image                        (Clay            *clay,
                                                                GFile           *file);
static void        clay_write_and_read_file                    (Clay            *clay,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_clay_2_8_features);
static ClayImage * clay_create_mainimage                       (Clay            *clay,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_clay_2_8_features);
static void        clay_assert_mainimage                       (ClayImage       *image,
                                                                gboolean         with_unusual_stuff,
                                                                gboolean         compat_paths,
                                                                gboolean         use_clay_2_8_features);


/**
 * write_and_read_clay_2_6_format:
 * @data:
 *
 * Do a write and read test on a file that could as well be
 * constructed with Clay 2.6.
 **/
static void
write_and_read_clay_2_6_format (gconstpointer data)
{
  Clay *clay = Clay (data);

  clay_write_and_read_file (clay,
                            FALSE /*with_unusual_stuff*/,
                            FALSE /*compat_paths*/,
                            FALSE /*use_clay_2_8_features*/);
}

/**
 * write_and_read_clay_2_6_format_unusual:
 * @data:
 *
 * Do a write and read test on a file that could as well be
 * constructed with Clay 2.6, and make it unusual, like compatible
 * paths and with a floating selection.
 **/
static void
write_and_read_clay_2_6_format_unusual (gconstpointer data)
{
  Clay *clay = Clay (data);

  clay_write_and_read_file (clay,
                            TRUE /*with_unusual_stuff*/,
                            TRUE /*compat_paths*/,
                            FALSE /*use_clay_2_8_features*/);
}

/**
 * load_clay_2_6_file:
 * @data:
 *
 * Loads a file created with Clay 2.6 and makes sure it loaded as
 * expected.
 **/
static void
load_clay_2_6_file (gconstpointer data)
{
  Clay      *clay = Clay (data);
  ClayImage *image;
  gchar     *filename;
  GFile     *file;

  filename = g_build_filename (g_getenv ("CLAY_TESTING_ABS_TOP_SRCDIR"),
                               "app/tests/files/clay-2-6-file.xcf",
                               NULL);
  file = g_file_new_for_path (filename);
  g_free (filename);

  image = clay_test_load_image (clay, file);

  /* The image file was constructed by running
   * clay_write_and_read_file (FALSE, FALSE) in Clay 2.6 by
   * copy-pasting the code to Clay 2.6 and adapting it to changes in
   * the core API, so we can use clay_assert_mainimage() to make sure
   * the file was loaded successfully.
   */
  clay_assert_mainimage (image,
                         FALSE /*with_unusual_stuff*/,
                         FALSE /*compat_paths*/,
                         FALSE /*use_clay_2_8_features*/);
}

/**
 * write_and_read_clay_2_8_format:
 * @data:
 *
 * Writes an XCF file that uses Clay 2.8 features such as layer
 * groups, then reads the file and make sure no relevant information
 * was lost.
 **/
static void
write_and_read_clay_2_8_format (gconstpointer data)
{
  Clay *clay = Clay (data);

  clay_write_and_read_file (clay,
                            FALSE /*with_unusual_stuff*/,
                            FALSE /*compat_paths*/,
                            TRUE /*use_clay_2_8_features*/);
}

ClayImage *
clay_test_load_image (Clay  *clay,
                      GFile *file)
{
  ClayPlugInProcedure *proc;
  ClayImage           *image;
  ClayPDBStatusType    unused;

  proc = clay_plug_in_manager_file_procedure_find (clay->plug_in_manager,
                                                   CLAY_FILE_PROCEDURE_GROUP_OPEN,
                                                   file,
                                                   NULL /*error*/);
  image = file_open_image (clay,
                           clay_get_user_context (clay),
                           NULL /*progress*/,
                           file,
                           0, 0, /* vector width, height */
                           TRUE, /* vector keep ratio */
                           FALSE /*as_new*/,
                           proc,
                           CLAY_RUN_NONINTERACTIVE,
                           NULL, /* file_proc_handles_vector */
                           &unused /*status*/,
                           NULL /*mime_type*/,
                           NULL /*error*/);

  return image;
}

/**
 * clay_write_and_read_file:
 *
 * Constructs the main test image and asserts its state, writes it to
 * a file, reads the image from the file, and asserts the state of the
 * loaded file. The function takes various parameters so the same
 * function can be used for different formats.
 **/
static void
clay_write_and_read_file (Clay     *clay,
                          gboolean  with_unusual_stuff,
                          gboolean  compat_paths,
                          gboolean  use_clay_2_8_features)
{
  ClayImage           *image;
  ClayImage           *loaded_image;
  ClayPlugInProcedure *proc;
  gchar               *filename = NULL;
  gint                 file_handle;
  GFile               *file;

  /* Create the image */
  image = clay_create_mainimage (clay,
                                 with_unusual_stuff,
                                 compat_paths,
                                 use_clay_2_8_features);

  /* Assert valid state */
  clay_assert_mainimage (image,
                         with_unusual_stuff,
                         compat_paths,
                         use_clay_2_8_features);

  /* Write to file */
  file_handle = g_file_open_tmp ("clay-test-XXXXXX.xcf", &filename, NULL);
  g_assert_true (file_handle != -1);
  close (file_handle);
  file = g_file_new_for_path (filename);
  g_free (filename);

  proc = clay_plug_in_manager_file_procedure_find (image->clay->plug_in_manager,
                                                   CLAY_FILE_PROCEDURE_GROUP_SAVE,
                                                   file,
                                                   NULL /*error*/);
  file_save (clay,
             image,
             NULL /*progress*/,
             file,
             proc,
             CLAY_RUN_NONINTERACTIVE,
             FALSE /*change_saved_state*/,
             FALSE /*export_backward*/,
             FALSE /*export_forward*/,
             NULL /*error*/);

  /* Load from file */
  loaded_image = clay_test_load_image (image->clay, file);

  /* Assert on the loaded file. If success, it means that there is no
   * significant information loss when we wrote the image to a file
   * and loaded it again
   */
  clay_assert_mainimage (loaded_image,
                         with_unusual_stuff,
                         compat_paths,
                         use_clay_2_8_features);

  g_file_delete (file, NULL, NULL);
  g_object_unref (file);
}

/**
 * clay_create_mainimage:
 *
 * Creates the main test image, i.e. the image that we use for most of
 * our XCF testing purposes.
 *
 * Returns: The #ClayImage
 **/
static ClayImage *
clay_create_mainimage (Clay     *clay,
                       gboolean  with_unusual_stuff,
                       gboolean  compat_paths,
                       gboolean  use_clay_2_8_features)
{
  ClayImage     *image          = NULL;
  ClayLayer     *layer          = NULL;
  ClayParasite  *parasite       = NULL;
  ClayGrid      *grid           = NULL;
  ClayChannel   *channel        = NULL;
  GeglColor     *channel_color  = gegl_color_new (NULL);
  ClayChannel   *selection      = NULL;
  ClayPath      *path           = NULL;
  ClayCoords     path1_coords[] = CLAY_MAINIMAGE_PATH1_COORDS;
  ClayCoords     path2_coords[] = CLAY_MAINIMAGE_PATH2_COORDS;
  ClayStroke    *stroke         = NULL;
  ClayLayerMask *layer_mask     = NULL;
  gdouble        rgb[4]         = CLAY_MAINIMAGE_CHANNEL1_COLOR;

  gegl_color_set_pixel (channel_color, babl_format ("R'G'B'A double"), &rgb);
  /* Image size and type */
  image = clay_image_new (clay,
                          CLAY_MAINIMAGE_WIDTH,
                          CLAY_MAINIMAGE_HEIGHT,
                          CLAY_MAINIMAGE_TYPE,
                          CLAY_MAINIMAGE_PRECISION);

  /* Layers */
  layer = clay_layer_new (image,
                          CLAY_MAINIMAGE_LAYER1_WIDTH,
                          CLAY_MAINIMAGE_LAYER1_HEIGHT,
                          CLAY_MAINIMAGE_LAYER1_FORMAT,
                          CLAY_MAINIMAGE_LAYER1_NAME,
                          CLAY_MAINIMAGE_LAYER1_OPACITY,
                          CLAY_MAINIMAGE_LAYER1_MODE);
  clay_image_add_layer (image,
                        layer,
                        NULL,
                        0,
                        FALSE/*push_undo*/);
  layer = clay_layer_new (image,
                          CLAY_MAINIMAGE_LAYER2_WIDTH,
                          CLAY_MAINIMAGE_LAYER2_HEIGHT,
                          CLAY_MAINIMAGE_LAYER2_FORMAT,
                          CLAY_MAINIMAGE_LAYER2_NAME,
                          CLAY_MAINIMAGE_LAYER2_OPACITY,
                          CLAY_MAINIMAGE_LAYER2_MODE);
  clay_image_add_layer (image,
                        layer,
                        NULL,
                        0,
                        FALSE /*push_undo*/);

  /* Layer mask */
  layer_mask = clay_layer_create_mask (layer,
                                       CLAY_ADD_MASK_BLACK,
                                       NULL /*channel*/);
  clay_layer_add_mask (layer,
                       layer_mask,
                       TRUE  /* edit_mask */,
                       FALSE /* push_undo */,
                       NULL  /* error     */);

  /* Image compression type
   *
   * We don't do any explicit test, only implicit when we read tile
   * data in other tests
   */

  /* Guides, note we add them in reversed order */
  clay_image_add_hguide (image,
                         CLAY_MAINIMAGE_HGUIDE2_POS,
                         FALSE /*push_undo*/);
  clay_image_add_hguide (image,
                         CLAY_MAINIMAGE_HGUIDE1_POS,
                         FALSE /*push_undo*/);
  clay_image_add_vguide (image,
                         CLAY_MAINIMAGE_VGUIDE2_POS,
                         FALSE /*push_undo*/);
  clay_image_add_vguide (image,
                         CLAY_MAINIMAGE_VGUIDE1_POS,
                         FALSE /*push_undo*/);


  /* Sample points */
  clay_image_add_sample_point_at_pos (image,
                                      CLAY_MAINIMAGE_SAMPLEPOINT1_X,
                                      CLAY_MAINIMAGE_SAMPLEPOINT1_Y,
                                      FALSE /*push_undo*/);
  clay_image_add_sample_point_at_pos (image,
                                      CLAY_MAINIMAGE_SAMPLEPOINT2_X,
                                      CLAY_MAINIMAGE_SAMPLEPOINT2_Y,
                                      FALSE /*push_undo*/);

  /* Tattoo
   * We don't bother testing this, not yet at least
   */

  /* Resolution */
  clay_image_set_resolution (image,
                             CLAY_MAINIMAGE_RESOLUTIONX,
                             CLAY_MAINIMAGE_RESOLUTIONY);


  /* Parasites */
  parasite = clay_parasite_new (CLAY_MAINIMAGE_PARASITE_NAME,
                                CLAY_PARASITE_PERSISTENT,
                                CLAY_MAINIMAGE_PARASITE_SIZE,
                                CLAY_MAINIMAGE_PARASITE_DATA);
  clay_image_parasite_attach (image,
                              parasite, FALSE);
  clay_parasite_free (parasite);
  parasite = clay_parasite_new ("clay-comment",
                                CLAY_PARASITE_PERSISTENT,
                                strlen (CLAY_MAINIMAGE_COMMENT) + 1,
                                CLAY_MAINIMAGE_COMMENT);
  clay_image_parasite_attach (image, parasite, FALSE);
  clay_parasite_free (parasite);


  /* Unit */
  clay_image_set_unit (image,
                       CLAY_MAINIMAGE_UNIT);

  /* Grid */
  grid = g_object_new (CLAY_TYPE_GRID,
                       "xspacing", CLAY_MAINIMAGE_GRIDXSPACING,
                       "yspacing", CLAY_MAINIMAGE_GRIDYSPACING,
                       NULL);
  clay_image_set_grid (image,
                       grid,
                       FALSE /*push_undo*/);
  g_object_unref (grid);

  /* Channel */
  channel = clay_channel_new (image,
                              CLAY_MAINIMAGE_CHANNEL1_WIDTH,
                              CLAY_MAINIMAGE_CHANNEL1_HEIGHT,
                              CLAY_MAINIMAGE_CHANNEL1_NAME,
                              channel_color);
  g_object_unref (channel_color);
  clay_image_add_channel (image,
                          channel,
                          NULL,
                          -1,
                          FALSE /*push_undo*/);

  /* Selection */
  selection = clay_image_get_mask (image);
  clay_channel_select_rectangle (selection,
                                 CLAY_MAINIMAGE_SELECTION_X,
                                 CLAY_MAINIMAGE_SELECTION_Y,
                                 CLAY_MAINIMAGE_SELECTION_W,
                                 CLAY_MAINIMAGE_SELECTION_H,
                                 CLAY_CHANNEL_OP_REPLACE,
                                 FALSE /*feather*/,
                                 0.0 /*feather_radius_x*/,
                                 0.0 /*feather_radius_y*/,
                                 FALSE /*push_undo*/);

  /* Path 1 */
  path = clay_path_new (image,
                        CLAY_MAINIMAGE_PATH1_NAME);
  /* The XCF file can save paths in two kind of ways, one old way
   * and a new way. Parameterize the way so we can test both variants,
   * i.e. clay_path_compat_is_compatible() must return both TRUE
   * and FALSE.
   */
  if (! compat_paths)
    {
      clay_item_set_visible (CLAY_ITEM (path),
                             TRUE,
                             FALSE /*push_undo*/);
    }
  /* TODO: Add test for non-closed stroke. The order of the anchor
   * points changes for open strokes, so it's boring to test
   */
  stroke = clay_bezier_stroke_new_from_coords (path1_coords,
                                               G_N_ELEMENTS (path1_coords),
                                               TRUE /*closed*/);
  clay_path_stroke_add (path, stroke);
  clay_image_add_path (image,
                       path,
                       NULL /*parent*/,
                       -1 /*position*/,
                       FALSE /*push_undo*/);

  /* Path 2 */
  path = clay_path_new (image,
                        CLAY_MAINIMAGE_PATH2_NAME);

  stroke = clay_bezier_stroke_new_from_coords (path2_coords,
                                               G_N_ELEMENTS (path2_coords),
                                               TRUE /*closed*/);
  clay_path_stroke_add (path, stroke);
  clay_image_add_path (image,
                       path,
                       NULL /*parent*/,
                       -1 /*position*/,
                       FALSE /*push_undo*/);

  /* Some of these things are pretty unusual, parameterize the
   * inclusion of this in the written file so we can do our test both
   * with and without
   */
  if (with_unusual_stuff)
    {
      GList *drawables;

      drawables = clay_image_get_selected_drawables (image);

      /* Floating selection */
      clay_selection_float (CLAY_SELECTION (clay_image_get_mask (image)),
                            drawables,
                            clay_get_user_context (clay),
                            TRUE /*cut_image*/,
                            0 /*off_x*/,
                            0 /*off_y*/,
                            NULL /*error*/);
      g_list_free (drawables);
    }

  /* Adds stuff like layer groups */
  if (use_clay_2_8_features)
    {
      ClayLayer *parent;

      /* Add a layer group and some layers:
       *
       *  group1
       *    layer3
       *    layer4
       *    group2
       *      layer5
       */

      /* group1 */
      layer = clay_group_layer_new (image);
      clay_object_set_name (CLAY_OBJECT (layer), CLAY_MAINIMAGE_GROUP1_NAME);
      clay_image_add_layer (image,
                            layer,
                            NULL /*parent*/,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
      parent = layer;

      /* layer3 */
      layer = clay_layer_new (image,
                              CLAY_MAINIMAGE_LAYER1_WIDTH,
                              CLAY_MAINIMAGE_LAYER1_HEIGHT,
                              CLAY_MAINIMAGE_LAYER1_FORMAT,
                              CLAY_MAINIMAGE_LAYER3_NAME,
                              CLAY_MAINIMAGE_LAYER1_OPACITY,
                              CLAY_MAINIMAGE_LAYER1_MODE);
      clay_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);

      /* layer4 */
      layer = clay_layer_new (image,
                              CLAY_MAINIMAGE_LAYER1_WIDTH,
                              CLAY_MAINIMAGE_LAYER1_HEIGHT,
                              CLAY_MAINIMAGE_LAYER1_FORMAT,
                              CLAY_MAINIMAGE_LAYER4_NAME,
                              CLAY_MAINIMAGE_LAYER1_OPACITY,
                              CLAY_MAINIMAGE_LAYER1_MODE);
      clay_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);

      /* group2 */
      layer = clay_group_layer_new (image);
      clay_object_set_name (CLAY_OBJECT (layer), CLAY_MAINIMAGE_GROUP2_NAME);
      clay_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
      parent = layer;

      /* layer5 */
      layer = clay_layer_new (image,
                              CLAY_MAINIMAGE_LAYER1_WIDTH,
                              CLAY_MAINIMAGE_LAYER1_HEIGHT,
                              CLAY_MAINIMAGE_LAYER1_FORMAT,
                              CLAY_MAINIMAGE_LAYER5_NAME,
                              CLAY_MAINIMAGE_LAYER1_OPACITY,
                              CLAY_MAINIMAGE_LAYER1_MODE);
      clay_image_add_layer (image,
                            layer,
                            parent,
                            -1 /*position*/,
                            FALSE /*push_undo*/);
    }

  /* Todo, should be tested somehow:
   *
   * - Color maps
   * - Custom user units
   * - Text layers
   * - Layer parasites
   * - Channel parasites
   * - Different tile compression methods
   */

  return image;
}

static void
clay_assert_path (ClayImage   *image,
                  const gchar *name,
                  ClayCoords   coords[],
                  gsize        coords_size,
                  gboolean     visible)
{
  ClayPath    *path           = NULL;
  ClayStroke  *stroke         = NULL;
  GArray      *control_points = NULL;
  gboolean     closed         = FALSE;
  gint         i              = 0;

  path = clay_image_get_path_by_name (image, name);
  stroke = clay_path_stroke_get_next (path, NULL);
  g_assert_true (stroke != NULL);
  control_points = clay_stroke_control_points_get (stroke,
                                                   &closed);
  g_assert_true (closed);
  g_assert_cmpint (control_points->len,
                   ==,
                   coords_size);
  for (i = 0; i < control_points->len; i++)
    {
      g_assert_cmpint (coords[i].x,
                       ==,
                       g_array_index (control_points,
                                      ClayAnchor,
                                      i).position.x);
      g_assert_cmpint (coords[i].y,
                       ==,
                       g_array_index (control_points,
                                      ClayAnchor,
                                      i).position.y);
    }

  g_assert_true (clay_item_get_visible (CLAY_ITEM (path)) ? TRUE : FALSE ==
                 visible ? TRUE : FALSE);
}

/**
 * clay_assert_mainimage:
 * @image:
 *
 * Verifies that the passed #ClayImage contains all the information
 * that was put in it by clay_create_mainimage().
 **/
static void
clay_assert_mainimage (ClayImage *image,
                       gboolean   with_unusual_stuff,
                       gboolean   compat_paths,
                       gboolean   use_clay_2_8_features)
{
  const ClayParasite *parasite               = NULL;
  gchar              *parasite_data          = NULL;
  guint32             parasite_size          = -1;
  ClayLayer          *layer                  = NULL;
  GList              *iter                   = NULL;
  ClayGuide          *guide                  = NULL;
  ClaySamplePoint    *sample_point           = NULL;
  gint                sample_point_x         = 0;
  gint                sample_point_y         = 0;
  gdouble             xres                   = 0.0;
  gdouble             yres                   = 0.0;
  ClayGrid           *grid                   = NULL;
  gdouble             xspacing               = 0.0;
  gdouble             yspacing               = 0.0;
  ClayChannel        *channel                = NULL;
  GeglColor          *actual_channel_color;
  gdouble             expected_rgb_color[4]  = CLAY_MAINIMAGE_CHANNEL1_COLOR;
  gdouble             rgb[4];
  ClayChannel        *selection              = NULL;
  gint                x                      = -1;
  gint                y                      = -1;
  gint                w                      = -1;
  gint                h                      = -1;
  ClayCoords          path1_coords[]         = CLAY_MAINIMAGE_PATH1_COORDS;
  ClayCoords          path2_coords[]         = CLAY_MAINIMAGE_PATH2_COORDS;

  /* Image size and type */
  g_assert_cmpint (clay_image_get_width (image),
                   ==,
                   CLAY_MAINIMAGE_WIDTH);
  g_assert_cmpint (clay_image_get_height (image),
                   ==,
                   CLAY_MAINIMAGE_HEIGHT);
  g_assert_cmpint (clay_image_get_base_type (image),
                   ==,
                   CLAY_MAINIMAGE_TYPE);

  /* Layers */
  layer = clay_image_get_layer_by_name (image,
                                        CLAY_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpint (clay_item_get_width (CLAY_ITEM (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER1_WIDTH);
  g_assert_cmpint (clay_item_get_height (CLAY_ITEM (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER1_HEIGHT);
  g_assert_cmpstr (babl_get_name (clay_drawable_get_format (CLAY_DRAWABLE (layer))),
                   ==,
                   babl_get_name (CLAY_MAINIMAGE_LAYER1_FORMAT));
  g_assert_cmpstr (clay_object_get_name (CLAY_DRAWABLE (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER1_NAME);
  g_assert_cmpfloat (clay_layer_get_opacity (layer),
                     ==,
                     CLAY_MAINIMAGE_LAYER1_OPACITY);
  g_assert_cmpint (clay_layer_get_mode (layer),
                   ==,
                   CLAY_MAINIMAGE_LAYER1_MODE);
  layer = clay_image_get_layer_by_name (image,
                                        CLAY_MAINIMAGE_LAYER2_NAME);
  g_assert_cmpint (clay_item_get_width (CLAY_ITEM (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER2_WIDTH);
  g_assert_cmpint (clay_item_get_height (CLAY_ITEM (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER2_HEIGHT);
  g_assert_cmpstr (babl_get_name (clay_drawable_get_format (CLAY_DRAWABLE (layer))),
                   ==,
                   babl_get_name (CLAY_MAINIMAGE_LAYER2_FORMAT));
  g_assert_cmpstr (clay_object_get_name (CLAY_DRAWABLE (layer)),
                   ==,
                   CLAY_MAINIMAGE_LAYER2_NAME);
  g_assert_cmpfloat (clay_layer_get_opacity (layer),
                     ==,
                     CLAY_MAINIMAGE_LAYER2_OPACITY);
  g_assert_cmpint (clay_layer_get_mode (layer),
                   ==,
                   CLAY_MAINIMAGE_LAYER2_MODE);

  /* Guides, note that we rely on internal ordering */
  iter = clay_image_get_guides (image);
  g_assert_true (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (clay_guide_get_position (guide),
                   ==,
                   CLAY_MAINIMAGE_VGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert_true (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (clay_guide_get_position (guide),
                   ==,
                   CLAY_MAINIMAGE_VGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert_true (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (clay_guide_get_position (guide),
                   ==,
                   CLAY_MAINIMAGE_HGUIDE1_POS);
  iter = g_list_next (iter);
  g_assert_true (iter != NULL);
  guide = iter->data;
  g_assert_cmpint (clay_guide_get_position (guide),
                   ==,
                   CLAY_MAINIMAGE_HGUIDE2_POS);
  iter = g_list_next (iter);
  g_assert_true (iter == NULL);

  /* Sample points, we rely on the same ordering as when we added
   * them, although this ordering is not a necessity
   */
  iter = clay_image_get_sample_points (image);
  g_assert_true (iter != NULL);
  sample_point = iter->data;
  clay_sample_point_get_position (sample_point,
                                  &sample_point_x, &sample_point_y);
  g_assert_cmpint (sample_point_x,
                   ==,
                   CLAY_MAINIMAGE_SAMPLEPOINT1_X);
  g_assert_cmpint (sample_point_y,
                   ==,
                   CLAY_MAINIMAGE_SAMPLEPOINT1_Y);
  iter = g_list_next (iter);
  g_assert_true (iter != NULL);
  sample_point = iter->data;
  clay_sample_point_get_position (sample_point,
                                  &sample_point_x, &sample_point_y);
  g_assert_cmpint (sample_point_x,
                   ==,
                   CLAY_MAINIMAGE_SAMPLEPOINT2_X);
  g_assert_cmpint (sample_point_y,
                   ==,
                   CLAY_MAINIMAGE_SAMPLEPOINT2_Y);
  iter = g_list_next (iter);
  g_assert_true (iter == NULL);

  /* Resolution */
  clay_image_get_resolution (image, &xres, &yres);
  g_assert_cmpint (xres,
                   ==,
                   CLAY_MAINIMAGE_RESOLUTIONX);
  g_assert_cmpint (yres,
                   ==,
                   CLAY_MAINIMAGE_RESOLUTIONY);

  /* Parasites */
  parasite = clay_image_parasite_find (image,
                                       CLAY_MAINIMAGE_PARASITE_NAME);
  parasite_data = (gchar *) clay_parasite_get_data (parasite, &parasite_size);
  parasite_data = g_strndup (parasite_data, parasite_size);
  g_assert_cmpint (parasite_size,
                   ==,
                   CLAY_MAINIMAGE_PARASITE_SIZE);
  g_assert_cmpstr (parasite_data,
                   ==,
                   CLAY_MAINIMAGE_PARASITE_DATA);
  g_free (parasite_data);

  parasite = clay_image_parasite_find (image,
                                       "clay-comment");
  parasite_data = (gchar *) clay_parasite_get_data (parasite, &parasite_size);
  parasite_data = g_strndup (parasite_data, parasite_size);
  g_assert_cmpint (parasite_size,
                   ==,
                   strlen (CLAY_MAINIMAGE_COMMENT) + 1);
  g_assert_cmpstr (parasite_data,
                   ==,
                   CLAY_MAINIMAGE_COMMENT);
  g_free (parasite_data);

  /* Unit */
  g_assert_true (clay_image_get_unit (image) == CLAY_MAINIMAGE_UNIT);

  /* Grid */
  grid = clay_image_get_grid (image);
  g_object_get (grid,
                "xspacing", &xspacing,
                "yspacing", &yspacing,
                NULL);
  g_assert_cmpint (xspacing,
                   ==,
                   CLAY_MAINIMAGE_GRIDXSPACING);
  g_assert_cmpint (yspacing,
                   ==,
                   CLAY_MAINIMAGE_GRIDYSPACING);


  /* Channel */
  channel = clay_image_get_channel_by_name (image,
                                            CLAY_MAINIMAGE_CHANNEL1_NAME);
  actual_channel_color = clay_channel_get_color (channel);
  g_assert_cmpint (clay_item_get_width (CLAY_ITEM (channel)),
                   ==,
                   CLAY_MAINIMAGE_CHANNEL1_WIDTH);
  g_assert_cmpint (clay_item_get_height (CLAY_ITEM (channel)),
                   ==,
                   CLAY_MAINIMAGE_CHANNEL1_HEIGHT);
  gegl_color_get_pixel (actual_channel_color, babl_format ("R'G'B'A double"), &rgb);
  g_assert_true (memcmp (&expected_rgb_color, &rgb, sizeof (expected_rgb_color)) == 0);

  /* Selection, if the image contains unusual stuff it contains a
   * floating select, and when floating a selection, the selection
   * mask is cleared, so don't test for the presence of the selection
   * mask in that case
   */
  if (! with_unusual_stuff)
    {
      selection = clay_image_get_mask (image);
      clay_item_bounds (CLAY_ITEM (selection), &x, &y, &w, &h);
      g_assert_cmpint (x,
                       ==,
                       CLAY_MAINIMAGE_SELECTION_X);
      g_assert_cmpint (y,
                       ==,
                       CLAY_MAINIMAGE_SELECTION_Y);
      g_assert_cmpint (w,
                       ==,
                       CLAY_MAINIMAGE_SELECTION_W);
      g_assert_cmpint (h,
                       ==,
                       CLAY_MAINIMAGE_SELECTION_H);
    }

  /* Path 1 */
  clay_assert_path (image,
                    CLAY_MAINIMAGE_PATH1_NAME,
                    path1_coords,
                    G_N_ELEMENTS (path1_coords),
                    ! compat_paths /*visible*/);

  /* Path 2 (always visible FALSE) */
  clay_assert_path (image,
                    CLAY_MAINIMAGE_PATH2_NAME,
                    path2_coords,
                    G_N_ELEMENTS (path2_coords),
                    FALSE /*visible*/);

  if (with_unusual_stuff)
    g_assert_true (clay_image_get_floating_selection (image) != NULL);
  else /* if (! with_unusual_stuff) */
    g_assert_true (clay_image_get_floating_selection (image) == NULL);

  if (use_clay_2_8_features)
    {
      /* Only verify the parent relationships, the layer attributes
       * are tested above
       */
      ClayItem *group1 = CLAY_ITEM (clay_image_get_layer_by_name (image, CLAY_MAINIMAGE_GROUP1_NAME));
      ClayItem *layer3 = CLAY_ITEM (clay_image_get_layer_by_name (image, CLAY_MAINIMAGE_LAYER3_NAME));
      ClayItem *layer4 = CLAY_ITEM (clay_image_get_layer_by_name (image, CLAY_MAINIMAGE_LAYER4_NAME));
      ClayItem *group2 = CLAY_ITEM (clay_image_get_layer_by_name (image, CLAY_MAINIMAGE_GROUP2_NAME));
      ClayItem *layer5 = CLAY_ITEM (clay_image_get_layer_by_name (image, CLAY_MAINIMAGE_LAYER5_NAME));

      g_assert_true (clay_item_get_parent (group1) == NULL);
      g_assert_true (clay_item_get_parent (layer3) == group1);
      g_assert_true (clay_item_get_parent (layer4) == group1);
      g_assert_true (clay_item_get_parent (group2) == group1);
      g_assert_true (clay_item_get_parent (layer5) == group2);
    }
}


/**
 * main:
 * @argc:
 * @argv:
 *
 * These tests intend to
 *
 *  - Make sure that we are backwards compatible with files created by
 *    older version of Clay, i.e. that we can load files from earlier
 *    version of Clay
 *
 *  - Make sure that the information put into a #ClayImage is not lost
 *    when the #ClayImage is written to a file and then read again
 **/
int
main (int    argc,
      char **argv)
{
  Clay *clay;
  int   result;

  g_test_init (&argc, &argv, NULL);

  clay_test_utils_set_clay3_directory ("CLAY_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/claydir");

  /* We share the same application instance across all tests. We need
   * the GUI variant for the file procs
   */
  clay = clay_init_for_testing ();

  /* Add tests */
  ADD_TEST (write_and_read_clay_2_6_format);
  ADD_TEST (write_and_read_clay_2_6_format_unusual);
  ADD_TEST (load_clay_2_6_file);
  ADD_TEST (write_and_read_clay_2_8_format);

  /* Don't write files to the source dir */
  clay_test_utils_set_clay3_directory ("CLAY_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/claydir-output");

  /* Run the tests */
  result = g_test_run ();

  /* Exit so we don't break script-fu plug-in wire */
  clay_exit (clay, TRUE);

  return result;
}
