/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claydockable.h
 * Copyright (C) 2001-2003 Michael Natterer <mitch@gimp.org>
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

#pragma once


#define CLAY_DOCKABLE_DRAG_OFFSET (-6)


#define CLAY_TYPE_DOCKABLE            (clay_dockable_get_type ())
#define CLAY_DOCKABLE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_DOCKABLE, ClayDockable))
#define CLAY_DOCKABLE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_DOCKABLE, ClayDockableClass))
#define CLAY_IS_DOCKABLE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_DOCKABLE))
#define CLAY_IS_DOCKABLE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_DOCKABLE))
#define CLAY_DOCKABLE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_DOCKABLE, ClayDockableClass))


typedef struct _ClayDockablePrivate ClayDockablePrivate;
typedef struct _ClayDockableClass   ClayDockableClass;

/**
 * ClayDockable:
 *
 * A kind of adapter to make other widgets dockable. The widget to
 * dock is put inside the ClayDockable, which is put in a
 * ClayDockbook.
 */
struct _ClayDockable
{
  GtkBin               parent_instance;

  ClayDockablePrivate *p;
};

struct _ClayDockableClass
{
  GtkBinClass  parent_class;
};


GType           clay_dockable_get_type          (void);

GtkWidget     * clay_dockable_new               (const gchar   *name,
                                                 const gchar   *blurb,
                                                 const gchar   *icon_name,
                                                 const gchar   *help_id);

void            clay_dockable_set_dockbook      (ClayDockable  *dockable,
                                                 ClayDockbook  *dockbook);
ClayDockbook  * clay_dockable_get_dockbook      (ClayDockable  *dockable);

void            clay_dockable_set_tab_style     (ClayDockable  *dockable,
                                                 ClayTabStyle   tab_style);
ClayTabStyle    clay_dockable_get_tab_style     (ClayDockable  *dockable);

void            clay_dockable_set_locked        (ClayDockable  *dockable,
                                                 gboolean       lock);
gboolean        clay_dockable_get_locked        (ClayDockable  *dockable);

const gchar   * clay_dockable_get_name          (ClayDockable  *dockable);
const gchar   * clay_dockable_get_blurb         (ClayDockable  *dockable);
const gchar   * clay_dockable_get_help_id       (ClayDockable  *dockable);
const gchar   * clay_dockable_get_icon_name     (ClayDockable  *dockable);
GtkWidget     * clay_dockable_get_icon          (ClayDockable  *dockable,
                                                 GtkIconSize    size);

GtkWidget     * clay_dockable_create_tab_widget (ClayDockable  *dockable,
                                                 ClayContext   *context,
                                                 ClayTabStyle   tab_style,
                                                 GtkIconSize    size);
void            clay_dockable_set_context       (ClayDockable  *dockable,
                                                 ClayContext   *context);
ClayUIManager * clay_dockable_get_menu          (ClayDockable  *dockable,
                                                 const gchar  **ui_path,
                                                 gpointer      *popup_data);

void            clay_dockable_detach            (ClayDockable  *dockable);
