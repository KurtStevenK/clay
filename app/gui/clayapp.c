/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayapp.c
 * Copyright (C) 2021 Niels De Graef <nielsdegraef@gmail.com>
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#include <config.h>

#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "core/core-types.h"

#include "core/clay.h"

#include "claycoreapp.h"

#include "clayapp.h"


enum
{
  PROP_0,
  PROP_NO_SPLASH = CLAY_CORE_APP_PROP_LAST + 1,
};

struct _ClayApp
{
  GtkApplication parent_instance;

  gboolean       no_splash;
};


static void clay_app_get_property (GObject      *object,
                                   guint         property_id,
                                   GValue       *value,
                                   GParamSpec   *pspec);
static void clay_app_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec);


G_DEFINE_TYPE_WITH_CODE (ClayApp, clay_app, GTK_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CORE_APP,
                                                NULL))


static void
clay_app_class_init (ClayAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->get_property = clay_app_get_property;
  gobj_class->set_property = clay_app_set_property;

  clay_core_app_install_properties (gobj_class);

  g_object_class_install_property (gobj_class, PROP_NO_SPLASH,
                                   g_param_spec_boolean ("no-splash", NULL, NULL,
                                                         FALSE,
                                                         CLAY_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT_ONLY));
}

static void
clay_app_init (ClayApp *self)
{
}

static void
clay_app_get_property (GObject    *object,
                       guint       property_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_NO_SPLASH:
      g_value_set_boolean (value, CLAY_APP (object)->no_splash);
      break;

    default:
      clay_core_app_get_property (object, property_id, value, pspec);
      break;
    }
}

static void
clay_app_set_property (GObject      *object,
                       guint         property_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_NO_SPLASH:
      CLAY_APP (object)->no_splash = g_value_get_boolean (value);
      break;

    default:
      clay_core_app_set_property (object, property_id, value, pspec);
      break;
    }
}

/*  public functions  */

GApplication *
clay_app_new (Clay        *clay,
              gboolean     no_splash,
              gboolean     quit,
              gboolean     as_new,
              const char **filenames,
              const char  *batch_interpreter,
              const char **batch_commands)
{
  ClayApp *app;

  app = g_object_new (CLAY_TYPE_APP,
                      "application-id",    CLAY_APPLICATION_ID,
                      /* We have our own code to handle process uniqueness, so
                       * when we reached this code, we are already passed this
                       * (it means that either this is the first process, or we
                       * don't want uniqueness). See bugs #9598 and #9599 for
                       * what happens when we let GIO try to handle uniqueness.
                       *
                       * TODO: since GApplication has code to pass over files
                       * and command line arguments, we may eventually want to
                       * remove our own code for uniqueness and batch command
                       * inter-process communication. This should be tested.
                       */
#if GLIB_CHECK_VERSION(2,74,0)
                      "flags",             G_APPLICATION_DEFAULT_FLAGS | G_APPLICATION_NON_UNIQUE,
#else
                      "flags",             G_APPLICATION_FLAGS_NONE | G_APPLICATION_NON_UNIQUE,
#endif
                      "clay",              clay,
                      "filenames",         filenames,
                      "as-new",            as_new,

                      "quit",              quit,
                      "batch-interpreter", batch_interpreter,
                      "batch-commands",    batch_commands,

                      "no-splash",         no_splash,
                      NULL);

  return G_APPLICATION (app);
}

gboolean
clay_app_get_no_splash (ClayApp *self)
{
  g_return_val_if_fail (CLAY_IS_APP (self), FALSE);
  return CLAY_APP (self)->no_splash;
}
