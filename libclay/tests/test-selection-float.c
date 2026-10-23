
static ClayValueArray *
clay_c_test_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayImage     *img;
  ClayDrawable  *layer1;
  ClayDrawable  *layer2;
  ClayDrawable  *group1;
  ClayLayer     *float_layer;

  /* Setup */

  CLAY_TEST_START("clay_image_new()")
  img = clay_image_new (32, 32, CLAY_RGB);
  CLAY_TEST_END(CLAY_IS_IMAGE (img))

  layer1 = CLAY_DRAWABLE (clay_layer_new (img, "layer1", 20, 10,
                          CLAY_RGBA_IMAGE, 100.0, CLAY_LAYER_MODE_NORMAL));
  layer2 = CLAY_DRAWABLE (clay_layer_new (img, "layer2", 10, 20,
                          CLAY_RGBA_IMAGE, 100.0, CLAY_LAYER_MODE_NORMAL));
  group1 = CLAY_DRAWABLE (clay_group_layer_new (img, NULL));

  CLAY_TEST_START("insert layer")
  CLAY_TEST_END(clay_image_insert_layer (img, CLAY_LAYER (layer1), NULL, 0))

  CLAY_TEST_START("insert group layer")
  CLAY_TEST_END(clay_image_insert_layer (img, CLAY_LAYER (group1), NULL, -1))

  CLAY_TEST_START("insert layer inside group")
  CLAY_TEST_END(clay_image_insert_layer (img, CLAY_LAYER (layer2),
                CLAY_LAYER (group1), -1))


  /* Floating selection tests */

  /* 1. Fail with no selection */
  CLAY_TEST_START("Clay.Selection.float - no selection")
  CLAY_TEST_END(clay_selection_float (img, (ClayDrawable *[2]) { layer1, NULL }, 10, 10) == NULL)

  /* 2. Fail on a group layer */
  CLAY_TEST_START("Clay.Selection.float - group layer")
  CLAY_TEST_END(clay_selection_float (img, (ClayDrawable *[2]) { group1, NULL }, 10, 10) == NULL)

  /* 3. Succeed on a normal layer */
  clay_image_select_rectangle (img, CLAY_CHANNEL_OP_REPLACE, 5, 5, 20, 20);

  CLAY_TEST_START("clay_selection_float - normal layer")
  float_layer = clay_selection_float (img, (ClayDrawable *[2]) { layer1, NULL }, 10, 10);
  CLAY_TEST_END(CLAY_IS_LAYER (float_layer))

  CLAY_TEST_START("clay_floating_sel_remove")
  CLAY_TEST_END(clay_floating_sel_remove (float_layer) == TRUE);

  /* 4. Succeed on a layer inside a group */
  clay_image_select_rectangle (img, CLAY_CHANNEL_OP_REPLACE, 5, 5, 20, 20);

  CLAY_TEST_START("clay_selection_float - layer inside group")
  float_layer = clay_selection_float (img, (ClayDrawable *[2]) { layer2, NULL }, 10, 10);
  CLAY_TEST_END(CLAY_IS_LAYER (float_layer))

  CLAY_TEST_START("clay_floating_sel_remove")
  CLAY_TEST_END(clay_floating_sel_remove (float_layer) == TRUE);


  /* Teardown */
  clay_image_delete (img);

  CLAY_TEST_RETURN
}
