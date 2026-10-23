/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1999 Spencer Kimball and Peter Mattis
 *
 * Copyright (C) 2024 Jehan
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

#include <libclay/clay.h>

static ClayPDBStatusType  status   = CLAY_PDB_SUCCESS;
static GError            *error    = NULL;
static const gchar       *testname = NULL;

#define CLAY_TEST_RETURN \
clay_test_cleanup: \
    return clay_procedure_new_return_values (procedure, status, error);

#define CLAY_TEST_START(name) if (error) goto clay_test_cleanup; testname = name; printf ("Starting test '%s': ", testname);

#define CLAY_TEST_END(test) \
  if (! (test)) \
    { \
      printf ("FAILED\n"); \
      error = g_error_new_literal (CLAY_PLUG_IN_ERROR, 0, testname); \
      status = CLAY_PDB_EXECUTION_ERROR; \
    } \
  else \
    { \
      printf ("OK\n"); \
    }

typedef struct _ClayCTest      ClayCTest;
typedef struct _ClayCTestClass ClayCTestClass;

struct _ClayCTest
{
  ClayPlugIn      parent_instance;
};

struct _ClayCTestClass
{
  ClayPlugInClass parent_class;
};


/* Declare local functions.
 */

#define CLAY_C_TEST_TYPE  (clay_c_test_get_type ())
#define CLAY_C_TEST (obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_C_TEST_TYPE, ClayCTest))

GType                   clay_c_test_get_type         (void);

static GList          * clay_c_test_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * clay_c_test_create_procedure (ClayPlugIn           *plug_in,
                                                      const gchar          *name);

static ClayValueArray * clay_c_test_run              (ClayProcedure        *procedure,
                                                      ClayRunMode           run_mode,
                                                      ClayImage            *image,
                                                      ClayDrawable        **drawables,
                                                      ClayProcedureConfig  *config,
                                                      gpointer              run_data);


G_DEFINE_TYPE (ClayCTest, clay_c_test, CLAY_TYPE_PLUG_IN)

CLAY_MAIN (CLAY_C_TEST_TYPE)


static void
clay_c_test_class_init (ClayCTestClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);

  plug_in_class->query_procedures = clay_c_test_query_procedures;
  plug_in_class->create_procedure = clay_c_test_create_procedure;
}

static void
clay_c_test_init (ClayCTest *ctest)
{
}

static GList *
clay_c_test_query_procedures (ClayPlugIn *plug_in)
{
  gchar *testname  = g_path_get_basename (__FILE__);
  gchar *dot;

  dot  = g_strrstr (testname, ".");
  *dot = '\0';

  return g_list_append (NULL, testname);
}

static ClayProcedure *
clay_c_test_create_procedure (ClayPlugIn  *plug_in,
                       const gchar *name)
{
  ClayProcedure *procedure = NULL;
  gchar         *testname  = g_path_get_basename (__FILE__);
  gchar         *dot;

  dot  = g_strrstr (testname, ".");
  *dot = '\0';

  if (! strcmp (name, testname))
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            clay_c_test_run, NULL, NULL);
      clay_procedure_set_image_types (procedure, "*");
      clay_procedure_set_sensitivity_mask(procedure, CLAY_PROCEDURE_SENSITIVE_ALWAYS);
    }

  g_free (testname);

  return procedure;
}
