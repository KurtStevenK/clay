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

#include "claycolortool.h"


#define CLAY_TYPE_BUCKET_FILL_TOOL            (clay_bucket_fill_tool_get_type ())
#define CLAY_BUCKET_FILL_TOOL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_BUCKET_FILL_TOOL, ClayBucketFillTool))
#define CLAY_BUCKET_FILL_TOOL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_BUCKET_FILL_TOOL, ClayBucketFillToolClass))
#define CLAY_IS_BUCKET_FILL_TOOL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_BUCKET_FILL_TOOL))
#define CLAY_IS_BUCKET_FILL_TOOL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_BUCKET_FILL_TOOL))
#define CLAY_BUCKET_FILL_TOOL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_BUCKET_FILL_TOOL, ClayBucketFillToolClass))

#define CLAY_BUCKET_FILL_TOOL_GET_OPTIONS(t)  (CLAY_BUCKET_FILL_OPTIONS (clay_tool_get_options (CLAY_TOOL (t))))


typedef struct _ClayBucketFillTool      ClayBucketFillTool;
typedef struct _ClayBucketFillToolClass ClayBucketFillToolClass;
typedef struct _ClayBucketFillToolPrivate ClayBucketFillToolPrivate;

struct _ClayBucketFillTool
{
  ClayColorTool              parent_instance;

  ClayBucketFillToolPrivate *priv;
};

struct _ClayBucketFillToolClass
{
  ClayColorToolClass  parent_class;
};


void    clay_bucket_fill_tool_register (ClayToolRegisterCallback  callback,
                                        gpointer                  data);

GType   clay_bucket_fill_tool_get_type (void);
