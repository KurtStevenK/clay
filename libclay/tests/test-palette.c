#define CLAY_TEST_PALETTE      "Bears"
#define CLAY_TEST_PALETTE_SIZE 256
#define CLAY_TEST_COLOR_IDX    3
#define CLAY_TEST_COLOR_FORMAT "R'G'B' u8"
#define CLAY_TEST_COLOR_R_U8   72
#define CLAY_TEST_COLOR_G_U8   56
#define CLAY_TEST_COLOR_B_U8   56
#define EPSILON                1e-6f

static ClayValueArray *
clay_c_test_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayPalette     *palette;
  ClayPalette     *palette2;
  GeglColor      **colors;
  gint             n_colors;
  guchar          *colormap;
  gfloat          *fcolormap;
  gint             n_colormap_colors;
  gsize            n_bytes;
  const Babl      *format;
  const Babl      *format2;
  gint             format_bpp;
  gint             format2_bpp;
  GeglColor       *color;
  guint8           rgb[3];
  gfloat           rgba[4];
  ClayValueArray  *retvals;

  CLAY_TEST_START("clay_palette_get_by_name()")
  palette = clay_palette_get_by_name (CLAY_TEST_PALETTE);
  CLAY_TEST_END(CLAY_IS_PALETTE (palette))

  CLAY_TEST_START("clay_palette_get_by_name() is unique")
  palette2 = clay_palette_get_by_name (CLAY_TEST_PALETTE);
  CLAY_TEST_END(CLAY_IS_PALETTE (palette2) && palette == palette2)

  CLAY_TEST_START("clay_palette_get_color_count()")
  n_colors = clay_palette_get_color_count (palette);
  CLAY_TEST_END(n_colors == CLAY_TEST_PALETTE_SIZE)

  CLAY_TEST_START("clay_palette_get_colors()")
  colors = clay_palette_get_colors (palette);
  CLAY_TEST_END(colors != NULL && clay_color_array_get_length (colors) == CLAY_TEST_PALETTE_SIZE)

  CLAY_TEST_START("Checking fourth palette color's format")
  color = colors[CLAY_TEST_COLOR_IDX];
  format = gegl_color_get_format (color);
  CLAY_TEST_END(format == babl_format (CLAY_TEST_COLOR_FORMAT))

  CLAY_TEST_START("Checking fourth palette color's RGB components")
  gegl_color_get_pixel (color, format, rgb);
  CLAY_TEST_END(rgb[0] == CLAY_TEST_COLOR_R_U8 && rgb[1] == CLAY_TEST_COLOR_G_U8 && rgb[2] == CLAY_TEST_COLOR_B_U8)

  CLAY_TEST_START("clay_palette_get_colormap()")
  colormap = clay_palette_get_colormap (palette, format, &n_colormap_colors, &n_bytes);
  format_bpp = babl_format_get_bytes_per_pixel (format);
  CLAY_TEST_END(colormap != NULL && n_colormap_colors == n_colors &&
                n_colormap_colors * format_bpp == n_bytes)

  CLAY_TEST_START("Comparing fourth palette color's RGB components from colormap")
  CLAY_TEST_END(colormap[format_bpp * CLAY_TEST_COLOR_IDX] == CLAY_TEST_COLOR_R_U8     &&
                colormap[format_bpp * CLAY_TEST_COLOR_IDX + 1] == CLAY_TEST_COLOR_G_U8 &&
                colormap[format_bpp * CLAY_TEST_COLOR_IDX + 2]== CLAY_TEST_COLOR_B_U8)

  g_free (colormap);

  CLAY_TEST_START("clay_palette_get_colormap() in \"R'G'B'A float\" format")
  format2     = babl_format ("RGBA float");
  format2_bpp = babl_format_get_bytes_per_pixel (format2);
  fcolormap   = (gfloat *) clay_palette_get_colormap (palette, format2, &n_colormap_colors, &n_bytes);
  CLAY_TEST_END(fcolormap != NULL && n_colormap_colors == n_colors &&
                format2 != format                                  &&
                format2_bpp > format_bpp                           &&
                n_colormap_colors * format2_bpp == n_bytes)

  CLAY_TEST_START("Comparing fourth palette color's RGB components from colormap in float format")
  gegl_color_get_pixel (color, format2, rgba);
  /* XXX: whether to a colormap or to a GeglColor, I was expecting the
   * conversion from palette format to target format to be done through
   * exact same babl code path, hence equality to be perfect (i.e. no
   * epsilon needed for float comparison). This is true on my machine,
   * not on CI. Is it again some SSE2 inconsistency where sometimes
   * conversion goes to SSE2, sometimes not, for the same run and same
   * babl fish? Something else? To be further investigated.
   */
  CLAY_TEST_END(fabsf (fcolormap[4 * CLAY_TEST_COLOR_IDX] - rgba[0]) < EPSILON     &&
                fabsf (fcolormap[4 * CLAY_TEST_COLOR_IDX + 1] - rgba[1]) < EPSILON &&
                fabsf (fcolormap[4 * CLAY_TEST_COLOR_IDX + 2] - rgba[2]) < EPSILON)

  g_free (fcolormap);

  /* Run the same tests through PDB. */

  CLAY_TEST_START("clay-palette-get-by-name")
  retvals = clay_procedure_run (clay_pdb_lookup_procedure (clay_get_pdb (), "clay-palette-get-by-name"),
                                "name", CLAY_TEST_PALETTE, NULL);
  CLAY_TEST_END(g_value_get_enum (clay_value_array_index (retvals, 0)) == CLAY_PDB_SUCCESS)

  CLAY_TEST_START("clay-palette-get-by-name and clay_palette_get_by_name() get identical result")
  palette2 = g_value_get_object (clay_value_array_index(retvals, 1));
  CLAY_TEST_END(CLAY_IS_PALETTE (palette2) && palette == palette2)

  clay_value_array_unref (retvals);

  CLAY_TEST_START("clay-palette-get-color-count")
  retvals = clay_procedure_run (clay_pdb_lookup_procedure (clay_get_pdb (), "clay-palette-get-color-count"),
                                "palette", palette, NULL);
  CLAY_TEST_END(g_value_get_enum (clay_value_array_index (retvals, 0)) == CLAY_PDB_SUCCESS)

  CLAY_TEST_START("clay-palette-get-color-count returns the right number of colors")
  n_colors = g_value_get_int (clay_value_array_index (retvals, 1));
  CLAY_TEST_END(n_colors == CLAY_TEST_PALETTE_SIZE)

  clay_value_array_unref (retvals);

  CLAY_TEST_START("clay-palette-get-colors")
  retvals = clay_procedure_run (clay_pdb_lookup_procedure (clay_get_pdb (), "clay-palette-get-colors"),
                                "palette", palette, NULL);
  CLAY_TEST_END(g_value_get_enum (clay_value_array_index (retvals, 0)) == CLAY_PDB_SUCCESS)

  CLAY_TEST_START("clay-palette-get-colors returns ClayColorArray")
  colors = g_value_get_boxed (clay_value_array_index (retvals, 1));
  CLAY_TEST_END(colors != NULL && clay_color_array_get_length (colors) == CLAY_TEST_PALETTE_SIZE)

  CLAY_TEST_START("clay_value_array_get_color_array()")
  colors = clay_value_array_get_color_array (retvals, 1);
  CLAY_TEST_END(colors != NULL && clay_color_array_get_length (colors) == CLAY_TEST_PALETTE_SIZE)

  CLAY_TEST_START("Checking fourth palette color's format (returned by PDB)")
  color = colors[CLAY_TEST_COLOR_IDX];
  format = gegl_color_get_format (color);
  CLAY_TEST_END(format == babl_format (CLAY_TEST_COLOR_FORMAT))

  CLAY_TEST_START("Checking fourth palette color's RGB values (returned by PDB)")
  gegl_color_get_pixel (color, format, rgb);
  CLAY_TEST_END(rgb[0] == CLAY_TEST_COLOR_R_U8 && rgb[1] == CLAY_TEST_COLOR_G_U8 && rgb[2] == CLAY_TEST_COLOR_B_U8)

  clay_value_array_unref (retvals);

  CLAY_TEST_RETURN
}
