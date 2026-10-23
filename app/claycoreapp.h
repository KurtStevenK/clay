/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claycoreapp.h
 * Copyright (C) 2022 Lukas Oberhuber <lukaso@gmail.com>
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

#pragma once


#define CLAY_APPLICATION_ID "xyz.kainzmayer.ClayDesign"

enum
{
  CLAY_CORE_APP_PROP_0,
  CLAY_CORE_APP_PROP_CLAY,
  CLAY_CORE_APP_PROP_FILENAMES,
  CLAY_CORE_APP_PROP_AS_NEW,

  CLAY_CORE_APP_PROP_QUIT,
  CLAY_CORE_APP_PROP_BATCH_INTERPRETER,
  CLAY_CORE_APP_PROP_BATCH_COMMANDS,

  CLAY_CORE_APP_PROP_LAST = CLAY_CORE_APP_PROP_BATCH_COMMANDS,
};

#define CLAY_TYPE_CORE_APP clay_core_app_get_type()
G_DECLARE_INTERFACE (ClayCoreApp,
                     clay_core_app,
                     CLAY, CORE_APP,
                     GObject)

struct _ClayCoreAppInterface
{
  GTypeInterface parent_iface;

  /* Padding to allow adding up to 12 new virtual functions without
   * breaking ABI. */
  gpointer padding[12];
};

Clay *             clay_core_app_get_clay              (ClayCoreApp *self);

gboolean           clay_core_app_get_quit              (ClayCoreApp *self);

gboolean           clay_core_app_get_as_new            (ClayCoreApp *self);

const gchar **     clay_core_app_get_filenames         (ClayCoreApp *self);

const gchar *      clay_core_app_get_batch_interpreter (ClayCoreApp *self);

const gchar **     clay_core_app_get_batch_commands    (ClayCoreApp *self);

void               clay_core_app_set_exit_status       (ClayCoreApp *self,
                                                        gint         exit_status);

gboolean           clay_core_app_get_exit_status       (ClayCoreApp *self);


/* Protected functions. */

void               clay_core_app_install_properties    (GObjectClass *klass);
void               clay_core_app_get_property          (GObject      *object,
                                                        guint         property_id,
                                                        GValue       *value,
                                                        GParamSpec   *pspec);
void               clay_core_app_set_property          (GObject      *object,
                                                        guint         property_id,
                                                        const GValue *value,
                                                        GParamSpec   *pspec);
