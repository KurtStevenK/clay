/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * clayunit.c
 * Copyright (C) 1999-2000 Michael Natterer <mitch@gimp.org>
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

/* This file contains functions to load & save the file containing the
 * user-defined size units, when the application starts/finished.
 */

#include "config.h"

#include <gio/gio.h>

#include "libclaybase/claybase.h"
#include "libclaybase/claybase-private.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "clay.h"
#include "clay-units.h"
#include "clayunit.h"

#include "config/clayconfig-file.h"

#include "clay-intl.h"


/*
 *  All deserialize functions return G_TOKEN_LEFT_PAREN on success,
 *  or the GTokenType they would have expected but didn't get.
 */

static GTokenType   clay_unitrc_unit_info_deserialize (GScanner *scanner,
                                                       Clay     *clay);

static ClayUnit   * clay_units_get_user_unit          (gint      unit_id);


static Clay *the_unit_clay = NULL;


void
clay_units_init (Clay *clay)
{
  ClayUnitVtable vtable = { 0 };

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (the_unit_clay == NULL);

  the_unit_clay = clay;

  vtable.get_user_unit = clay_units_get_user_unit;

  clay_base_init (&vtable);

  clay->user_units = NULL;
}

void
clay_units_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  clay_base_exit ();

  g_list_free_full (clay->user_units, g_object_unref);
  clay->user_units = NULL;
}


/*  unitrc functions  **********/

enum
{
  UNIT_INFO = 1,
  UNIT_FACTOR,
  UNIT_DIGITS,
  UNIT_SYMBOL,
  UNIT_ABBREV,
  UNIT_SINGULAR, /* Obsoleted in Clay 3.0. */
  UNIT_PLURAL    /* Obsoleted in Clay 3.0. */
};

void
clay_unitrc_load (Clay *clay)
{
  GFile      *file;
  GScanner   *scanner;
  GTokenType  token;
  GError     *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = clay_directory_file ("unitrc", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n", clay_file_get_utf8_name (file));

  scanner = clay_scanner_new_file (file, &error);

  if (! scanner && error->code == CLAY_CONFIG_ERROR_OPEN_ENOENT)
    {
      g_clear_error (&error);
      g_object_unref (file);

      file = clay_sysconf_directory_file ("unitrc", NULL);

      scanner = clay_scanner_new_file (file, NULL);
    }

  if (! scanner)
    {
      g_clear_error (&error);
      g_object_unref (file);
      return;
    }

  g_scanner_scope_add_symbol (scanner, 0,
                              "unit-info", GINT_TO_POINTER (UNIT_INFO));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "factor", GINT_TO_POINTER (UNIT_FACTOR));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "digits", GINT_TO_POINTER (UNIT_DIGITS));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "symbol", GINT_TO_POINTER (UNIT_SYMBOL));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "abbreviation", GINT_TO_POINTER (UNIT_ABBREV));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "singular", GINT_TO_POINTER (UNIT_SINGULAR));
  g_scanner_scope_add_symbol (scanner, UNIT_INFO,
                              "plural", GINT_TO_POINTER (UNIT_PLURAL));

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          if (scanner->value.v_symbol == GINT_TO_POINTER (UNIT_INFO))
            {
              g_scanner_set_scope (scanner, UNIT_INFO);
              token = clay_unitrc_unit_info_deserialize (scanner, clay);

              if (token == G_TOKEN_RIGHT_PAREN)
                g_scanner_set_scope (scanner, 0);
            }
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default: /* do nothing */
          break;
        }
    }

  if (token != G_TOKEN_LEFT_PAREN)
    {
      g_scanner_get_next_token (scanner);
      g_scanner_unexp_token (scanner, token, NULL, NULL, NULL,
                             _("fatal parse error"), TRUE);

      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);

      clay_config_file_backup_on_error (file, "unitrc", NULL);
    }

  clay_scanner_unref (scanner);
  g_object_unref (file);
}

void
clay_unitrc_save (Clay *clay)
{
  ClayConfigWriter *writer;
  GFile            *file;
  GList            *iter;
  GError           *error = NULL;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  file = clay_directory_file ("unitrc", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (file));

  writer =
    clay_config_writer_new_from_file (file,
                                      TRUE,
                                      "Clay units\n\n"
                                      "This file contains the user unit database. "
                                      "You can edit this list with the unit "
                                      "editor. You are not supposed to edit it "
                                      "manually, but of course you can do.\n"
                                      "This file will be entirely rewritten each "
                                      "time you exit.",
                                      NULL);

  g_object_unref (file);

  if (!writer)
    return;

  for (iter = clay->user_units; iter; iter = iter->next)
    {
      ClayUnit *unit = iter->data;

      if (clay_unit_get_deletion_flag (unit) == FALSE)
        {
          gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

          clay_config_writer_open (writer, "unit-info");
          clay_config_writer_string (writer,
                                     clay_unit_get_name (unit));

          clay_config_writer_open (writer, "factor");
          clay_config_writer_print (writer,
                                    g_ascii_dtostr (buf, sizeof (buf),
                                                    clay_unit_get_factor (unit)),
                                    -1);
          clay_config_writer_close (writer);

          clay_config_writer_open (writer, "digits");
          clay_config_writer_printf (writer,
                                     "%d", clay_unit_get_digits (unit));
          clay_config_writer_close (writer);

          clay_config_writer_open (writer, "symbol");
          clay_config_writer_string (writer,
                                     clay_unit_get_symbol (unit));
          clay_config_writer_close (writer);

          clay_config_writer_open (writer, "abbreviation");
          clay_config_writer_string (writer,
                                     clay_unit_get_abbreviation (unit));
          clay_config_writer_close (writer);

          clay_config_writer_close (writer);
        }
    }

  if (! clay_config_writer_finish (writer, "end of units", &error))
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }
}


/*  private functions  */

static GTokenType
clay_unitrc_unit_info_deserialize (GScanner *scanner,
                                   Clay     *clay)
{
  gchar      *name         = NULL;
  gdouble     factor       = 1.0;
  gint        digits       = 2.0;
  gchar      *symbol       = NULL;
  gchar      *abbreviation = NULL;
  gchar      *singular     = NULL;
  gchar      *plural       = NULL;
  GTokenType  token;

  if (! clay_scanner_parse_string (scanner, &name))
    return G_TOKEN_STRING;

  token = G_TOKEN_LEFT_PAREN;

  while (g_scanner_peek_next_token (scanner) == token)
    {
      token = g_scanner_get_next_token (scanner);

      switch (token)
        {
        case G_TOKEN_LEFT_PAREN:
          token = G_TOKEN_SYMBOL;
          break;

        case G_TOKEN_SYMBOL:
          switch (GPOINTER_TO_INT (scanner->value.v_symbol))
            {
            case UNIT_FACTOR:
              token = G_TOKEN_FLOAT;
              if (! clay_scanner_parse_double (scanner, &factor))
                goto cleanup;
              break;

            case UNIT_DIGITS:
              token = G_TOKEN_INT;
              if (! clay_scanner_parse_int (scanner, &digits))
                goto cleanup;
              break;

            case UNIT_SYMBOL:
              token = G_TOKEN_STRING;
              if (! clay_scanner_parse_string (scanner, &symbol))
                goto cleanup;
              break;

            case UNIT_ABBREV:
              token = G_TOKEN_STRING;
              if (! clay_scanner_parse_string (scanner, &abbreviation))
                goto cleanup;
              break;

            case UNIT_SINGULAR:
              /* UNIT_SINGULAR and UNIT_PLURAL are deprecated. We still
               * support reading them if they exist to stay backward
               * compatible with older unitrc, in which case, we use
               * UNIT_PLURAL on behalf of the name.
               */
              token = G_TOKEN_STRING;
              if (! clay_scanner_parse_string (scanner, &singular))
                goto cleanup;
              break;

            case UNIT_PLURAL:
              token = G_TOKEN_STRING;
              if (! clay_scanner_parse_string (scanner, &plural))
                goto cleanup;
             break;

            default:
              break;
            }
          token = G_TOKEN_RIGHT_PAREN;
          break;

        case G_TOKEN_RIGHT_PAREN:
          token = G_TOKEN_LEFT_PAREN;
          break;

        default:
          break;
        }
    }

  if (token == G_TOKEN_LEFT_PAREN)
    {
      token = G_TOKEN_RIGHT_PAREN;

      if (g_scanner_peek_next_token (scanner) == token)
        {
          ClayUnit *unit = _clay_unit_new (clay,
                                           plural && strlen (plural) > 0 ? plural : name,
                                           factor, digits, symbol, abbreviation);

          /*  make the unit definition persistent  */
          clay_unit_set_deletion_flag (unit, FALSE);
        }
    }

 cleanup:

  g_free (name);
  g_free (symbol);
  g_free (abbreviation);
  g_free (singular);
  g_free (plural);

  return token;
}

/**
 * clay_units_get_user_unit:
 * @unit_id:
 *
 * This function will return the user-created ClayUnit with ID @unit_id.
 */
static ClayUnit *
clay_units_get_user_unit (gint unit_id)
{
  g_return_val_if_fail (the_unit_clay != NULL, NULL);
  g_return_val_if_fail (unit_id >= CLAY_UNIT_END && unit_id != CLAY_UNIT_PERCENT, NULL);

  unit_id -= CLAY_UNIT_END;

  return g_list_nth_data (the_unit_clay->user_units, unit_id);
}
