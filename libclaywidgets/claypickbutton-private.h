/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypickbutton-private.h
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

#ifndef __CLAY_PICK_BUTTON_PRIVATE_H__
#define __CLAY_PICK_BUTTON_PRIVATE_H__


typedef struct _ClayPickButtonPrivate
{
  GdkCursor *cursor;
  GtkWidget *grab_widget;
} ClayPickButtonPrivate;


#endif /* ! __CLAY_PICK_BUTTON_PRIVATE_H__ */
