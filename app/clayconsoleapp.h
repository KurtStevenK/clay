
/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * clayapp.h
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

#pragma once


#define CLAY_TYPE_CONSOLE_APP (clay_console_app_get_type ())
G_DECLARE_FINAL_TYPE (ClayConsoleApp,
                      clay_console_app,
                      CLAY, CONSOLE_APP,
                      GApplication)


GApplication * clay_console_app_new (Clay         *clay,
                                     gboolean      quit,
                                     gboolean      as_new,
                                     const char  **filenames,
                                     const char   *batch_interpreter,
                                     const char  **batch_commands);
