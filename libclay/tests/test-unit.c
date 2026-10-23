#define N_DEFAULT_USER_UNITS 6

typedef struct
{
  gdouble   factor;
  gint      digits;
  gchar    *name;
  gchar    *symbol;
  gchar    *abbreviation;
} ClayUnitDef;

static const ClayUnitDef _clay_unit_defs[CLAY_UNIT_END] =
{
  /* pseudo unit */
  { 0.0,  0, "pixels",      "px", "px", },

  /* standard units */
  { 1.0,  2, "inches",      "''", "in", },

  { 25.4, 1, "millimeters", "mm", "mm", },

  /* professional units */
  { 72.0, 0, "points",      "pt", "pt", },

  { 6.0,  1, "picas",       "pc", "pc", },
};

static ClayValueArray *
clay_c_test_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayUnit *unit;
  ClayUnit *unit2;
  gint      n_user_units = 0;
  gint      i;

  CLAY_TEST_START("clay_unit_inch()");
  unit = clay_unit_inch ();
  CLAY_TEST_END(CLAY_IS_UNIT (unit));

  CLAY_TEST_START("clay_unit_inch() always returns an unique object");
  unit2 = clay_unit_inch ();
  CLAY_TEST_END(CLAY_IS_UNIT (unit2) && unit == unit2);

  for (i = 0; i < CLAY_UNIT_END; i++)
    {
      gchar *test_name = g_strdup_printf ("Testing built-in unit %d", i);

      CLAY_TEST_START(test_name);
      g_free (test_name);

      unit = clay_unit_get_by_id (i);

      CLAY_TEST_END(CLAY_IS_UNIT (unit) &&
                    g_strcmp0 (clay_unit_get_name (unit), _clay_unit_defs[i].name) == 0                 &&
                    g_strcmp0 (clay_unit_get_symbol (unit), _clay_unit_defs[i].symbol) == 0             &&
                    g_strcmp0 (clay_unit_get_abbreviation (unit), _clay_unit_defs[i].abbreviation) == 0 &&
                    clay_unit_get_factor (unit) == _clay_unit_defs[i].factor                            &&
                    clay_unit_get_digits (unit)  == _clay_unit_defs[i].digits);

      if (i == CLAY_UNIT_INCH)
        {
          CLAY_TEST_START("clay_unit_inch() is the same as clay_unit_get_by_id (CLAY_UNIT_INCH)");
          CLAY_TEST_END(unit == unit2);
        }
    }

  CLAY_TEST_START("Counting default user units");
  unit = clay_unit_get_by_id (CLAY_UNIT_END);
  while (CLAY_IS_UNIT (unit))
    unit = clay_unit_get_by_id (CLAY_UNIT_END + ++n_user_units);
  /* A bare config contains 6 defaults units in /etc/. */
  CLAY_TEST_END(n_user_units == N_DEFAULT_USER_UNITS);

  CLAY_TEST_START("clay_unit_new()");
  unit2 = clay_unit_new ("name", 2.0, 1, "symbol", "abbreviation");
  CLAY_TEST_END(CLAY_IS_UNIT (unit2));

  CLAY_TEST_START("Verifying the new user unit's ID");
  CLAY_TEST_END(clay_unit_get_id (unit2) == CLAY_UNIT_END + n_user_units);

  CLAY_TEST_START("Verifying the new user unit's unicity");
  CLAY_TEST_END(unit2 == clay_unit_get_by_id (CLAY_UNIT_END + n_user_units));

  CLAY_TEST_START("Counting again user units");
  unit = clay_unit_get_by_id(CLAY_UNIT_END);
  n_user_units = 0;
  while (CLAY_IS_UNIT (unit))
    unit = clay_unit_get_by_id (CLAY_UNIT_END + ++n_user_units);
  CLAY_TEST_END(n_user_units == N_DEFAULT_USER_UNITS + 1);

  CLAY_TEST_RETURN
}
