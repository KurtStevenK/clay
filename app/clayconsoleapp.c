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

#include <gio/gio.h>

#include "libclaybase/claybase.h"

#include "core/core-types.h"

#include "core/clay.h"

#include "clayconsoleapp.h"
#include "claycoreapp.h"

struct _ClayConsoleApp
{
  GApplication parent_instance;
};

G_DEFINE_TYPE_WITH_CODE (ClayConsoleApp, clay_console_app, G_TYPE_APPLICATION,
                         G_IMPLEMENT_INTERFACE (CLAY_TYPE_CORE_APP, NULL))


static void
clay_console_app_class_init (ClayConsoleAppClass *klass)
{
  GObjectClass *gobj_class = G_OBJECT_CLASS (klass);

  gobj_class->get_property = clay_core_app_get_property;
  gobj_class->set_property = clay_core_app_set_property;

  clay_core_app_install_properties (gobj_class);
}

static void
clay_console_app_init (ClayConsoleApp *self)
{
}

/*  public functions  */

GApplication *
clay_console_app_new (Clay        *clay,
                      gboolean     quit,
                      gboolean     as_new,
                      const char **filenames,
                      const char  *batch_interpreter,
                      const char **batch_commands)
{
  ClayConsoleApp *app;

  app = g_object_new (CLAY_TYPE_CONSOLE_APP,
                      "application-id",    CLAY_APPLICATION_ID,
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
                      NULL);

  return G_APPLICATION (app);
}
