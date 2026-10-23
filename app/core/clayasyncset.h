/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayasyncset.h
 * Copyright (C) 2018 Ell
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


#define CLAY_TYPE_ASYNC_SET            (clay_async_set_get_type ())
#define CLAY_ASYNC_SET(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ASYNC_SET, ClayAsyncSet))
#define CLAY_ASYNC_SET_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ASYNC_SET, ClayAsyncSetClass))
#define CLAY_IS_ASYNC_SET(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ASYNC_SET))
#define CLAY_IS_ASYNC_SET_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ASYNC_SET))
#define CLAY_ASYNC_SET_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ASYNC_SET, ClayAsyncSetClass))


typedef struct _ClayAsyncSetPrivate ClayAsyncSetPrivate;
typedef struct _ClayAsyncSetClass   ClayAsyncSetClass;

struct _ClayAsyncSet
{
  GObject              parent_instance;

  ClayAsyncSetPrivate *priv;
};

struct _ClayAsyncSetClass
{
  GObjectClass  parent_class;
};


GType          clay_async_set_get_type (void);

ClayAsyncSet * clay_async_set_new      (void);

void           clay_async_set_add      (ClayAsyncSet *async_set,
                                        ClayAsync    *async);
void           clay_async_set_remove   (ClayAsyncSet *async_set,
                                        ClayAsync    *async);
void           clay_async_set_clear    (ClayAsyncSet *async_set);
gboolean       clay_async_set_is_empty (ClayAsyncSet *async_set);
