#!/usr/bin/env python3

N_DEFAULT_USER_UNITS=6

clay_unit_defs = [
  # pseudo unit
  {
    'factor':       0.0,
    'digits':       0,
    'name':         "pixels",
    'symbol':       "px",
    'abbreviation': "px"
  },
  # standard units
  {
    'factor':       1.0,
    'digits':       2,
    'name':         "inches",
    'symbol':       "''",
    'abbreviation': "in"
  },
  {
    'factor':       25.4,
    'digits':       1,
    'name':         "millimeters",
    'symbol':       "mm",
    'abbreviation': "mm"
  },
  # professional units
  {
    'factor':       72.0,
    'digits':       0,
    'name':         "points",
    'symbol':       "pt",
    'abbreviation': "pt"
  },
  {
    'factor':       6.0,
    'digits':       1,
    'name':         "picas",
    'symbol':       "pc",
    'abbreviation': "pc"
  }
]

unit = Clay.Unit.inch()
clay_assert('Clay.Unit.inch()', type(unit) == Clay.Unit)

unit2 = Clay.Unit.inch()
clay_assert('Clay.Unit.inch() always returns an unique object', unit == unit2)

for i in range(len(clay_unit_defs)):
  unit = Clay.Unit.get_by_id(i)
  unitdef = clay_unit_defs[i]
  clay_assert('Testing built-in unit {}'.format(i),
              type(unit) == Clay.Unit                            and \
              unit.get_name() == unitdef['name']                 and \
              unit.get_symbol() == unitdef['symbol']             and \
              unit.get_abbreviation() == unitdef['abbreviation'] and \
              unit.get_factor() == unitdef['factor']             and \
              unit.get_digits() == unitdef['digits'])

  if i == Clay.UnitID.INCH:
    clay_assert('Clay.Unit.inch() is the same as Clay.Unit.get_by_id (Clay.UnitID.INCH)',
                unit == unit2)

unit = Clay.Unit.get_by_id(Clay.UnitID.END)
n_user_units = 0
while unit is not None:
  n_user_units += 1
  unit = Clay.Unit.get_by_id(Clay.UnitID.END + n_user_units)
clay_assert('Counting default user units', n_user_units == N_DEFAULT_USER_UNITS)

unit2 = Clay.Unit.new ("name", 2.0, 1, "symbol", "abbreviation");
clay_assert('Clay.Unit.new()', type(unit2) == Clay.Unit)

clay_assert("Verifying the new user unit's ID", unit2.get_id() == Clay.UnitID.END + n_user_units)
clay_assert("Verifying the new user unit's unicity", unit2 == Clay.Unit.get_by_id(Clay.UnitID.END + n_user_units))

unit = Clay.Unit.get_by_id(Clay.UnitID.END)
n_user_units = 0
while unit is not None:
  n_user_units += 1
  unit = Clay.Unit.get_by_id(Clay.UnitID.END + n_user_units)
clay_assert('Counting again user units', n_user_units == N_DEFAULT_USER_UNITS + 1)
