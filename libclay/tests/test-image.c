/* Sometimes odd dimensions may create weird situations. It's usually a
 * good idea to test both even and odd dimensions.
 */
#define NEW_IMAGE_WIDTH  1920
#define NEW_IMAGE_HEIGHT 2001

static ClayValueArray *
clay_c_test_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayImage     **images;
  ClayImage      *new_image;
  ClayTextLayer  *text_layer;

  CLAY_TEST_START("clay_image_new()");
  new_image = clay_image_new (NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, CLAY_RGB);
  CLAY_TEST_END(CLAY_IS_IMAGE (new_image));

  CLAY_TEST_START("clay_get_images()");
  images = clay_get_images ();
  CLAY_TEST_END(clay_core_object_array_get_length ((GObject **) images) == 1 && images[0] == new_image);
  g_free (images);

  CLAY_TEST_START("clay_text_layer_new() with point unit");
  text_layer = clay_text_layer_new (new_image, "hello world", clay_context_get_font (),
                                    20, clay_unit_point ());
  CLAY_TEST_END(CLAY_IS_TEXT_LAYER (text_layer));

  CLAY_TEST_START("clay_image_insert_layer()");
  CLAY_TEST_END(clay_image_insert_layer (new_image, CLAY_LAYER (text_layer), NULL, 0));

  CLAY_TEST_START("clay_text_layer_new() with pixel unit");
  text_layer = clay_text_layer_new (new_image, "hello world", clay_context_get_font (),
                                    20, clay_unit_pixel ());
  CLAY_TEST_END(CLAY_IS_TEXT_LAYER (text_layer));

  CLAY_TEST_START("clay_image_insert_layer()");
  CLAY_TEST_END(clay_image_insert_layer (new_image, CLAY_LAYER (text_layer), NULL, 0));

  CLAY_TEST_RETURN
}
