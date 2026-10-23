/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claypickbutton.h
 * Copyright (C) 2002 Michael Natterer <mitch@gimp.org>
 *
 * based on gtk-2-0/gtk/gtkcolorsel.c
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

#if !defined (__CLAY_WIDGETS_H_INSIDE__) && !defined (CLAY_WIDGETS_COMPILATION)
#error "Only <libclaywidgets/claywidgets.h> can be included directly."
#endif

#ifndef __CLAY_PICK_BUTTON_H__
#define __CLAY_PICK_BUTTON_H__

G_BEGIN_DECLS


#define CLAY_TYPE_PICK_BUTTON (clay_pick_button_get_type ())
G_DECLARE_DERIVABLE_TYPE (ClayPickButton, clay_pick_button, CLAY, PICK_BUTTON, GtkButton)

struct _ClayPickButtonClass
{
  GtkButtonClass  parent_class;

  void (* color_picked) (ClayPickButton  *button,
                         const GeglColor *color);

  /* Padding for future expansion */
  void (* _clay_reserved0) (void);
  void (* _clay_reserved1) (void);
  void (* _clay_reserved2) (void);
  void (* _clay_reserved3) (void);
  void (* _clay_reserved4) (void);
  void (* _clay_reserved5) (void);
  void (* _clay_reserved6) (void);
  void (* _clay_reserved7) (void);
  void (* _clay_reserved8) (void);
  void (* _clay_reserved9) (void);
};


GtkWidget * clay_pick_button_new      (void);

/*  for internal use only  */
G_GNUC_INTERNAL void _clay_pick_button_default_pick (ClayPickButton *button);


G_END_DECLS

#endif /* __CLAY_PICK_BUTTON_H__ */
