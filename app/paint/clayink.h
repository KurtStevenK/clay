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

#include "claypaintcore.h"
#include "clayink-blob.h"


#define CLAY_TYPE_INK            (clay_ink_get_type ())
#define CLAY_INK(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_INK, ClayInk))
#define CLAY_INK_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_INK, ClayInkClass))
#define CLAY_IS_INK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_INK))
#define CLAY_IS_INK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_INK))
#define CLAY_INK_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_INK, ClayInkClass))


typedef struct _ClayInkClass ClayInkClass;

struct _ClayInk
{
  ClayPaintCore  parent_instance;

  GList         *start_blobs;      /*  starting blobs per stroke (for undo) */

  ClayBlob      *cur_blob;         /*  current blob                         */
  GList         *last_blobs;       /*  blobs for last stroke positions      */
  GList         *blobs_to_render;
};

struct _ClayInkClass
{
  ClayPaintCoreClass  parent_class;
};


void    clay_ink_register (Clay                      *clay,
                           ClayPaintRegisterCallback  callback);

GType   clay_ink_get_type (void);

