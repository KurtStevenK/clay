/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claymenu_model.h
 * Copyright (C) 2023 Jehan
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


#define CLAY_TYPE_MENU_MODEL            (clay_menu_model_get_type ())
#define CLAY_MENU_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_MENU_MODEL, ClayMenuModel))
#define CLAY_MENU_MODEL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_MENU_MODEL, ClayMenuModelClass))
#define CLAY_IS_MENU_MODEL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_MENU_MODEL))
#define CLAY_IS_MENU_MODEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_MENU_MODEL))
#define CLAY_MENU_MODEL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_MENU_MODEL, ClayMenuModelClass))


typedef struct _ClayMenuModelPrivate ClayMenuModelPrivate;
typedef struct _ClayMenuModelClass   ClayMenuModelClass;

struct _ClayMenuModel
{
  GMenuModel             parent_instance;

  ClayMenuModelPrivate  *priv;
};

struct _ClayMenuModelClass
{
  GMenuModelClass        parent_class;
};


GType            clay_menu_model_get_type   (void);

ClayMenuModel  * clay_menu_model_new        (ClayUIManager *manager,
                                             GMenuModel    *model);

ClayMenuModel  * clay_menu_model_get_submodel (ClayMenuModel *model,
                                               const gchar   *path);

const gchar    * clay_menu_model_get_path     (ClayMenuModel *model);

void             clay_menu_model_set_title    (ClayMenuModel *model,
                                               const gchar   *path,
                                               const gchar   *title);
void             clay_menu_model_set_color    (ClayMenuModel *model,
                                               const gchar   *path,
                                               GeglColor     *color);
