/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * clayasync.h
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


#define CLAY_TYPE_ASYNC            (clay_async_get_type ())
#define CLAY_ASYNC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_ASYNC, ClayAsync))
#define CLAY_ASYNC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_ASYNC, ClayAsyncClass))
#define CLAY_IS_ASYNC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_ASYNC))
#define CLAY_IS_ASYNC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_ASYNC))
#define CLAY_ASYNC_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_ASYNC, ClayAsyncClass))


typedef void (* ClayAsyncCallback) (ClayAsync *async,
                                    gpointer   data);


typedef struct _ClayAsyncPrivate ClayAsyncPrivate;
typedef struct _ClayAsyncClass   ClayAsyncClass;

struct _ClayAsync
{
  GObject           parent_instance;

  ClayAsyncPrivate *priv;
};

struct _ClayAsyncClass
{
  GObjectClass  parent_class;

  /*  signals  */
  void   (* waiting) (ClayAsync *async);
};


GType       clay_async_get_type                (void);

ClayAsync * clay_async_new                     (void);

gboolean    clay_async_is_synced               (ClayAsync         *async);

void        clay_async_add_callback            (ClayAsync         *async,
                                                ClayAsyncCallback  callback,
                                                gpointer           data);
void        clay_async_add_callback_for_object (ClayAsync         *async,
                                                ClayAsyncCallback  callback,
                                                gpointer           data,
                                                gpointer           gobject);
void        clay_async_remove_callback         (ClayAsync         *async,
                                                ClayAsyncCallback  callback,
                                                gpointer           data);

gboolean    clay_async_is_stopped              (ClayAsync         *async);

void        clay_async_finish                  (ClayAsync         *async,
                                                gpointer           result);
void        clay_async_finish_full             (ClayAsync         *async,
                                                gpointer           result,
                                                GDestroyNotify     result_destroy_func);
gboolean    clay_async_is_finished             (ClayAsync         *async);
gpointer    clay_async_get_result              (ClayAsync         *async);

void        clay_async_abort                   (ClayAsync         *async);

gboolean    clay_async_is_canceled             (ClayAsync         *async);

void        clay_async_cancel_and_wait         (ClayAsync         *async);


/*  stats  */

gint        clay_async_get_n_running           (void);
