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
  ClayImage         **images;
  ClayLayer         **layers;
  GeglBufferIterator *iter;
  GeglBuffer         *buffer1;
  GeglBuffer         *buffer2;
  const Babl         *format;
  ClayImage          *new_image;
  ClayImage          *original_image;
  ClayTextLayer      *text_layer;
  ClayLayer          *layer;
  ClayLayer          *export_layer;
  ClayExportOptions  *options;
  ClayExportReturn    delete;
  gboolean            identical_buffers;

  new_image = clay_image_new (NEW_IMAGE_WIDTH, NEW_IMAGE_HEIGHT, CLAY_RGB);
  text_layer = clay_text_layer_new (new_image, "hello world", clay_context_get_font (),
                                    20, clay_unit_point ());
  clay_image_insert_layer (new_image, CLAY_LAYER (text_layer), NULL, 0);
  text_layer = clay_text_layer_new (new_image, "annyeong uju", clay_context_get_font (),
                                    20, clay_unit_point ());
  clay_image_insert_layer (new_image, CLAY_LAYER (text_layer), NULL, 0);

  options = g_object_new (CLAY_TYPE_EXPORT_OPTIONS,
                          "capabilities", CLAY_EXPORT_CAN_HANDLE_RGB | CLAY_EXPORT_CAN_HANDLE_ALPHA,
                          NULL);

  CLAY_TEST_START("Verify start state (1)");
  images = clay_get_images ();
  CLAY_TEST_END(clay_core_object_array_get_length ((GObject **) images) == 1 && images[0] == new_image);
  g_free (images);

  CLAY_TEST_START("Verify start state (2)");
  layers = clay_image_get_layers (new_image);
  CLAY_TEST_END(clay_core_object_array_get_length ((GObject **) layers) == 2);
  g_free (layers);

  original_image = new_image;

  CLAY_TEST_START("clay_export_options_get_image() created a new image");
  delete = clay_export_options_get_image (options, &new_image);
  images = clay_get_images ();
  CLAY_TEST_END(delete == CLAY_EXPORT_EXPORT && clay_core_object_array_get_length ((GObject **) images) == 2 && new_image != original_image);
  g_free (images);

  CLAY_TEST_START("The new image has a single layer");
  layers = clay_image_get_layers (new_image);
  CLAY_TEST_END(clay_core_object_array_get_length ((GObject **) layers) == 1);

  export_layer = layers[0];
  g_free (layers);

  layer = clay_image_merge_visible_layers (original_image, CLAY_CLIP_TO_IMAGE);

  CLAY_TEST_START("Compare export buffer with original image's merged buffer");
  buffer1 = clay_drawable_get_buffer (CLAY_DRAWABLE (layer));
  buffer2 = clay_drawable_get_buffer (CLAY_DRAWABLE (export_layer));
  format  = gegl_buffer_get_format (buffer1);
  iter = gegl_buffer_iterator_new (buffer1, NULL, 0, format,
                                   GEGL_ACCESS_READ, GEGL_ABYSS_NONE, 2);
  gegl_buffer_iterator_add (iter, buffer2, NULL, 0, format,
                            GEGL_ACCESS_READ, GEGL_ABYSS_NONE);

  identical_buffers = TRUE;
  while (gegl_buffer_iterator_next (iter))
    {
      const gfloat *src1   = (const gfloat *) iter->items[0].data;
      const gfloat *src2   = (const gfloat *) iter->items[1].data;
      gint          count  = iter->length;

      if (memcmp (src1, src2, count * babl_format_get_bytes_per_pixel (format)) != 0)
        {
          identical_buffers = FALSE;
          break;
        }
    }
  CLAY_TEST_END(identical_buffers == TRUE);


  CLAY_TEST_RETURN
}
