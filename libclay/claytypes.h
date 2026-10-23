/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * claytypes.h
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#ifndef __CLAY_TYPES_H__
#define __CLAY_TYPES_H__

#include <libclaybase/claybasetypes.h>

G_BEGIN_DECLS

/* For information look into the html documentation */


typedef struct _ClayPDB                  ClayPDB;
typedef struct _ClayPlugIn               ClayPlugIn;
typedef struct _ClayProcedure            ClayProcedure;
typedef struct _ClayBatchProcedure       ClayBatchProcedure;
typedef struct _ClayImageProcedure       ClayImageProcedure;
typedef struct _ClayFileProcedure        ClayFileProcedure;
typedef struct _ClayVectorLoadProcedure  ClayVectorLoadProcedure;
typedef struct _ClayLoadProcedure        ClayLoadProcedure;
typedef struct _ClayExportProcedure      ClayExportProcedure;
typedef struct _ClayThumbnailProcedure   ClayThumbnailProcedure;
typedef struct _ClayProcedureConfig      ClayProcedureConfig;

typedef struct _ClayImage                ClayImage;
typedef struct _ClayItem                 ClayItem;
typedef struct _ClayDrawable             ClayDrawable;
typedef struct _ClayGroupLayer           ClayGroupLayer;
typedef struct _ClayLayer                ClayLayer;
typedef struct _ClayChannel              ClayChannel;
typedef struct _ClayLayerMask            ClayLayerMask;
typedef struct _ClayLinkLayer            ClayLinkLayer;
typedef struct _ClaySelection            ClaySelection;
typedef struct _ClayTextLayer            ClayTextLayer;
typedef struct _ClayPath                 ClayPath;
typedef struct _ClayDrawableFilter       ClayDrawableFilter;
typedef struct _ClayDrawableFilterConfig ClayDrawableFilterConfig;
typedef struct _ClayVectorLayer          ClayVectorLayer;

typedef struct _ClayDisplay              ClayDisplay;

typedef struct _ClayResource             ClayResource;
typedef struct _ClayBrush                ClayBrush;
typedef struct _ClayCurve                ClayCurve;
typedef struct _ClayFont                 ClayFont;
typedef struct _ClayGradient             ClayGradient;
typedef struct _ClayPattern              ClayPattern;
typedef struct _ClayPalette              ClayPalette;


/* FIXME move somewhere else */

/**
 * ClayPixbufTransparency:
 * @CLAY_PIXBUF_KEEP_ALPHA:   Create a pixbuf with alpha
 * @CLAY_PIXBUF_SMALL_CHECKS: Show transparency as small checks
 * @CLAY_PIXBUF_LARGE_CHECKS: Show transparency as large checks
 *
 * How to deal with transparency when creating thubnail pixbufs from
 * images and drawables.
 **/
typedef enum
{
  CLAY_PIXBUF_KEEP_ALPHA,
  CLAY_PIXBUF_SMALL_CHECKS,
  CLAY_PIXBUF_LARGE_CHECKS
} ClayPixbufTransparency;


G_END_DECLS

#endif /* __CLAY_TYPES_H__ */
