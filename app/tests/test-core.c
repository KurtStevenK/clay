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

#include <gegl.h>
#include <gtk/gtk.h>

#include "widgets/widgets-types.h"

#include "widgets/clayuimanager.h"

#include "core/clay.h"
#include "core/claycontext.h"
#include "core/clayimage.h"
#include "core/claylayer.h"
#include "core/claylayer-new.h"

#include "operations/claylevelsconfig.h"

#include "tests.h"

#include "clay-app-test-utils.h"


#define CLAY_TEST_IMAGE_SIZE 100

#define ADD_IMAGE_TEST(function) \
  g_test_add ("/clay-core/" #function, \
              ClayTestFixture, \
              clay, \
              clay_test_image_setup, \
              function, \
              clay_test_image_teardown);

#define ADD_TEST(function) \
  g_test_add ("/clay-core/" #function, \
              ClayTestFixture, \
              clay, \
              NULL, \
              function, \
              NULL);


typedef struct
{
  ClayImage *image;
} ClayTestFixture;


static void clay_test_image_setup    (ClayTestFixture *fixture,
                                      gconstpointer    data);
static void clay_test_image_teardown (ClayTestFixture *fixture,
                                      gconstpointer    data);


/**
 * clay_test_image_setup:
 * @fixture:
 * @data:
 *
 * Test fixture setup for a single image.
 **/
static void
clay_test_image_setup (ClayTestFixture *fixture,
                       gconstpointer    data)
{
  Clay *clay = Clay (data);

  fixture->image = clay_image_new (clay,
                                   CLAY_TEST_IMAGE_SIZE,
                                   CLAY_TEST_IMAGE_SIZE,
                                   CLAY_RGB,
                                   CLAY_PRECISION_FLOAT_LINEAR);
}

/**
 * clay_test_image_teardown:
 * @fixture:
 * @data:
 *
 * Test fixture teardown for a single image.
 **/
static void
clay_test_image_teardown (ClayTestFixture *fixture,
                          gconstpointer    data)
{
  g_object_unref (fixture->image);
}

/**
 * rotate_non_overlapping:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can add a layer
 * and call clay_item_rotate with center at (0, -10)
 * without triggering a failed assertion .
 **/
static void
rotate_non_overlapping (ClayTestFixture *fixture,
                        gconstpointer    data)
{
  Clay        *clay    = Clay (data);
  ClayImage   *image   = fixture->image;
  ClayLayer   *layer;
  ClayContext *context = clay_context_new (clay, "Test", NULL /*template*/);
  gboolean     result;

  g_assert_cmpint (clay_image_get_n_layers (image), ==, 0);

  layer = clay_layer_new (image,
                          CLAY_TEST_IMAGE_SIZE,
                          CLAY_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          CLAY_OPACITY_OPAQUE,
                          CLAY_LAYER_MODE_NORMAL);

  g_assert_cmpint (CLAY_IS_LAYER (layer), ==, TRUE);

  result = clay_image_add_layer (image,
                                 layer,
                                 CLAY_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  clay_item_rotate (CLAY_ITEM (layer), context, CLAY_ROTATE_DEGREES90, 0., -10., TRUE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (clay_image_get_n_layers (image), ==, 1);
  g_object_unref (context);
}

/**
 * add_layer:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can add a layer.
 **/
static void
add_layer (ClayTestFixture *fixture,
           gconstpointer    data)
{
  ClayImage *image = fixture->image;
  ClayLayer *layer;
  gboolean   result;

  g_assert_cmpint (clay_image_get_n_layers (image), ==, 0);

  layer = clay_layer_new (image,
                          CLAY_TEST_IMAGE_SIZE,
                          CLAY_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          CLAY_OPACITY_OPAQUE,
                          CLAY_LAYER_MODE_NORMAL);

  g_assert_cmpint (CLAY_IS_LAYER (layer), ==, TRUE);

  result = clay_image_add_layer (image,
                                 layer,
                                 CLAY_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (clay_image_get_n_layers (image), ==, 1);
}

/**
 * remove_layer:
 * @fixture:
 * @data:
 *
 * Super basic test that makes sure we can remove a layer.
 **/
static void
remove_layer (ClayTestFixture *fixture,
              gconstpointer    data)
{
  ClayImage *image = fixture->image;
  ClayLayer *layer;
  gboolean   result;

  g_assert_cmpint (clay_image_get_n_layers (image), ==, 0);

  layer = clay_layer_new (image,
                          CLAY_TEST_IMAGE_SIZE,
                          CLAY_TEST_IMAGE_SIZE,
                          babl_format ("R'G'B'A u8"),
                          "Test Layer",
                          CLAY_OPACITY_OPAQUE,
                          CLAY_LAYER_MODE_NORMAL);

  g_assert_cmpint (CLAY_IS_LAYER (layer), ==, TRUE);

  result = clay_image_add_layer (image,
                                 layer,
                                 CLAY_IMAGE_ACTIVE_PARENT,
                                 0,
                                 FALSE);

  g_assert_cmpint (result, ==, TRUE);
  g_assert_cmpint (clay_image_get_n_layers (image), ==, 1);

  clay_image_remove_layer (image,
                           layer,
                           FALSE,
                           NULL);

  g_assert_cmpint (clay_image_get_n_layers (image), ==, 0);
}

/**
 * white_graypoint_in_red_levels:
 * @fixture:
 * @data:
 *
 * Makes sure the levels algorithm can handle when the graypoint is
 * white. It's easy to get a divide by zero problem when trying to
 * calculate what gamma will give a white graypoint.
 **/
static void
white_graypoint_in_red_levels (ClayTestFixture *fixture,
                               gconstpointer    data)
{
  GeglColor            *black   = gegl_color_new ("transparent");
  GeglColor            *gray    = gegl_color_new ("white");
  GeglColor            *white   = gegl_color_new ("white");
  ClayHistogramChannel  channel = CLAY_HISTOGRAM_RED;
  ClayLevelsConfig     *config;

  config = g_object_new (CLAY_TYPE_LEVELS_CONFIG, NULL);

  clay_levels_config_adjust_by_colors (config,
                                       channel,
                                       NULL,
                                       black,
                                       gray,
                                       white);

  /* Make sure we didn't end up with an invalid gamma value */
  g_object_set (config,
                "gamma", config->gamma[channel],
                NULL);

  g_clear_object (&black);
  g_clear_object (&gray);
  g_clear_object (&white);
}

int
main (int    argc,
      char **argv)
{
  Clay *clay;
  int   result;

  g_test_init (&argc, &argv, NULL);

  clay_test_utils_set_clay3_directory ("CLAY_TESTING_ABS_TOP_SRCDIR",
                                       "app/tests/claydir");

  /* We share the same application instance across all tests */
  clay = clay_init_for_testing ();

  /* Add tests */
  ADD_IMAGE_TEST (add_layer);
  ADD_IMAGE_TEST (remove_layer);
  ADD_IMAGE_TEST (rotate_non_overlapping);
  ADD_TEST (white_graypoint_in_red_levels);

  /* Run the tests */
  result = g_test_run ();

  /* Don't write files to the source dir */
  clay_test_utils_set_clay3_directory ("CLAY_TESTING_ABS_TOP_BUILDDIR",
                                       "app/tests/claydir-output");

  /* Exit so we don't break script-fu plug-in wire */
  clay_exit (clay, TRUE);

  return result;
}
