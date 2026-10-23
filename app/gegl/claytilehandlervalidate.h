/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

#include <gegl-buffer-backend.h>


/***
 * ClayTileHandlerValidate is a GeglTileHandler that renders the
 * projection.
 */

#define CLAY_TYPE_TILE_HANDLER_VALIDATE            (clay_tile_handler_validate_get_type ())
#define CLAY_TILE_HANDLER_VALIDATE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TILE_HANDLER_VALIDATE, ClayTileHandlerValidate))
#define CLAY_TILE_HANDLER_VALIDATE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  CLAY_TYPE_TILE_HANDLER_VALIDATE, ClayTileHandlerValidateClass))
#define CLAY_IS_TILE_HANDLER_VALIDATE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TILE_HANDLER_VALIDATE))
#define CLAY_IS_TILE_HANDLER_VALIDATE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  CLAY_TYPE_TILE_HANDLER_VALIDATE))
#define CLAY_TILE_HANDLER_VALIDATE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  CLAY_TYPE_TILE_HANDLER_VALIDATE, ClayTileHandlerValidateClass))


typedef struct _ClayTileHandlerValidate      ClayTileHandlerValidate;
typedef struct _ClayTileHandlerValidateClass ClayTileHandlerValidateClass;

struct _ClayTileHandlerValidate
{
  GeglTileHandler  parent_instance;

  GeglNode        *graph;
  cairo_region_t  *dirty_region;
  const Babl      *format;
  gint             tile_width;
  gint             tile_height;
  gboolean         whole_tile;
  gint             validating;
  gint             suspend_validate;
};

struct _ClayTileHandlerValidateClass
{
  GeglTileHandlerClass  parent_class;

  /*  signals  */
  void (* invalidated)     (ClayTileHandlerValidate *validate,
                            const GeglRectangle     *rect);

  /*  virtual functions  */
  void (* begin_validate)  (ClayTileHandlerValidate *validate);
  void (* end_validate)    (ClayTileHandlerValidate *validate);
  void (* validate)        (ClayTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            const Babl              *format,
                            gpointer                 dest_buf,
                            gint                     dest_stride);
  void (* validate_buffer) (ClayTileHandlerValidate *validate,
                            const GeglRectangle     *rect,
                            GeglBuffer              *buffer);
};


GType                     clay_tile_handler_validate_get_type          (void);

GeglTileHandler         * clay_tile_handler_validate_new               (GeglNode                *graph);

void                      clay_tile_handler_validate_assign            (ClayTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
void                      clay_tile_handler_validate_unassign          (ClayTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer);
ClayTileHandlerValidate * clay_tile_handler_validate_get_assigned      (GeglBuffer              *buffer);

void                      clay_tile_handler_validate_invalidate        (ClayTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);
void                      clay_tile_handler_validate_undo_invalidate   (ClayTileHandlerValidate *validate,
                                                                        const GeglRectangle     *rect);

void                      clay_tile_handler_validate_begin_validate    (ClayTileHandlerValidate *validate);
void                      clay_tile_handler_validate_end_validate      (ClayTileHandlerValidate *validate);

void                      clay_tile_handler_validate_validate          (ClayTileHandlerValidate *validate,
                                                                        GeglBuffer              *buffer,
                                                                        const GeglRectangle     *rect,
                                                                        gboolean                 intersect,
                                                                        gboolean                 chunked);

gboolean                  clay_tile_handler_validate_buffer_set_extent (GeglBuffer              *buffer,
                                                                        const GeglRectangle     *extent);

void                      clay_tile_handler_validate_buffer_copy       (GeglBuffer              *src_buffer,
                                                                        const GeglRectangle     *src_rect,
                                                                        GeglBuffer              *dst_buffer,
                                                                        const GeglRectangle     *dst_rect);
