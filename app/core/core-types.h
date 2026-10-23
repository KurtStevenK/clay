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

#include "libclaybase/claybasetypes.h"
#include "libclaymath/claymathtypes.h"
#include "libclaycolor/claycolortypes.h"
#include "libclaymodule/claymoduletypes.h"
#include "libclaythumb/claythumb-types.h"

#include "config/config-types.h"

#include "core/core-enums.h"


/*  former base/ defines  */

#define MAX_CHANNELS  5

#define RED           0
#define GREEN         1
#define BLUE          2
#define ALPHA         3

#define GRAY          0
#define ALPHA_G       1

#define INDEXED       0
#define ALPHA_I       1


/*  defines  */

#define CLAY_COORDS_MIN_PRESSURE      0.0
#define CLAY_COORDS_MAX_PRESSURE      1.0
#define CLAY_COORDS_DEFAULT_PRESSURE  1.0

#define CLAY_COORDS_MIN_TILT         -1.0
#define CLAY_COORDS_MAX_TILT          1.0
#define CLAY_COORDS_DEFAULT_TILT      0.0

#define CLAY_COORDS_MIN_WHEEL         0.0
#define CLAY_COORDS_MAX_WHEEL         1.0
#define CLAY_COORDS_DEFAULT_WHEEL     0.5

#define CLAY_COORDS_DEFAULT_DISTANCE  0.0
#define CLAY_COORDS_DEFAULT_ROTATION  0.0
#define CLAY_COORDS_DEFAULT_SLIDER    0.0

#define CLAY_COORDS_DEFAULT_VELOCITY  0.0

#define CLAY_COORDS_DEFAULT_DIRECTION 0.0

#define CLAY_COORDS_DEFAULT_XSCALE    1.0
#define CLAY_COORDS_DEFAULT_YSCALE    1.0

#define CLAY_COORDS_DEFAULT_VALUES    { 0.0, 0.0, \
                                        CLAY_COORDS_DEFAULT_PRESSURE, \
                                        CLAY_COORDS_DEFAULT_TILT,     \
                                        CLAY_COORDS_DEFAULT_TILT,     \
                                        CLAY_COORDS_DEFAULT_WHEEL,    \
                                        CLAY_COORDS_DEFAULT_DISTANCE, \
                                        CLAY_COORDS_DEFAULT_ROTATION, \
                                        CLAY_COORDS_DEFAULT_SLIDER,   \
                                        CLAY_COORDS_DEFAULT_VELOCITY, \
                                        CLAY_COORDS_DEFAULT_DIRECTION,\
                                        CLAY_COORDS_DEFAULT_XSCALE,   \
                                        CLAY_COORDS_DEFAULT_YSCALE }


/*  base classes  */

typedef struct _ClayObject                      ClayObject;
typedef struct _ClayViewable                    ClayViewable;
typedef struct _ClayFilter                      ClayFilter;
typedef struct _ClayItem                        ClayItem;
typedef struct _ClayAuxItem                     ClayAuxItem;
typedef struct _ClayLink                        ClayLink;

typedef struct _Clay                            Clay;
typedef struct _ClayImage                       ClayImage;

typedef struct _ClayDisplay                     ClayDisplay;


/*  containers  */

typedef struct _ClayContainer                   ClayContainer;
typedef struct _ClayList                        ClayList;
typedef struct _ClayDocumentList                ClayDocumentList;
typedef struct _ClayDrawableStack               ClayDrawableStack;
typedef struct _ClayFilteredContainer           ClayFilteredContainer;
typedef struct _ClayFilterStack                 ClayFilterStack;
typedef struct _ClayItemList                    ClayItemList;
typedef struct _ClayItemStack                   ClayItemStack;
typedef struct _ClayLayerStack                  ClayLayerStack;
typedef struct _ClayTaggedContainer             ClayTaggedContainer;
typedef struct _ClayTreeProxy                   ClayTreeProxy;


/*  not really a container  */

typedef struct _ClayItemTree                    ClayItemTree;


/*  context objects  */

typedef struct _ClayContext                     ClayContext;
typedef struct _ClayFillOptions                 ClayFillOptions;
typedef struct _ClayStrokeOptions               ClayStrokeOptions;
typedef struct _ClayToolOptions                 ClayToolOptions;


/*  info objects  */

typedef struct _ClayPaintInfo                   ClayPaintInfo;
typedef struct _ClayToolGroup                   ClayToolGroup;
typedef struct _ClayToolInfo                    ClayToolInfo;
typedef struct _ClayToolItem                    ClayToolItem;


/*  data objects  */

typedef struct _ClayDataFactory                 ClayDataFactory;
typedef struct _ClayDataLoaderFactory           ClayDataLoaderFactory;
typedef struct _ClayResource                    ClayResource;
typedef struct _ClayData                        ClayData;
typedef struct _ClayBrush                       ClayBrush;
typedef struct _ClayBrushCache                  ClayBrushCache;
typedef struct _ClayBrushClipboard              ClayBrushClipboard;
typedef struct _ClayBrushGenerated              ClayBrushGenerated;
typedef struct _ClayBrushPipe                   ClayBrushPipe;
typedef struct _ClayCurve                       ClayCurve;
typedef struct _ClayDynamics                    ClayDynamics;
typedef struct _ClayDynamicsOutput              ClayDynamicsOutput;
typedef struct _ClayGradient                    ClayGradient;
typedef struct _ClayMybrush                     ClayMybrush;
typedef struct _ClayPadActions                  ClayPadActions;
typedef struct _ClayPalette                     ClayPalette;
typedef struct _ClayPaletteMru                  ClayPaletteMru;
typedef struct _ClayPattern                     ClayPattern;
typedef struct _ClayPatternClipboard            ClayPatternClipboard;
typedef struct _ClayToolPreset                  ClayToolPreset;
typedef struct _ClayTagCache                    ClayTagCache;


/*  drawable objects  */

typedef struct _ClayDrawable                    ClayDrawable;
typedef struct _ClayDrawableFilterMask          ClayDrawableFilterMask;
typedef struct _ClayRasterizable                ClayRasterizable;
typedef struct _ClayChannel                     ClayChannel;
typedef struct _ClayLayerMask                   ClayLayerMask;
typedef struct _ClaySelection                   ClaySelection;
typedef struct _ClayLayer                       ClayLayer;
typedef struct _ClayGroupLayer                  ClayGroupLayer;
typedef struct _ClayLinkLayer                   ClayLinkLayer;


/*  auxiliary image items  */

typedef struct _ClayGuide                       ClayGuide;
typedef struct _ClaySamplePoint                 ClaySamplePoint;


/*  undo objects  */

typedef struct _ClayUndo                        ClayUndo;
typedef struct _ClayUndoStack                   ClayUndoStack;
typedef struct _ClayUndoAccumulator             ClayUndoAccumulator;


/* Symmetry transformations */

typedef struct _ClaySymmetry                    ClaySymmetry;
typedef struct _ClayMirror                      ClayMirror;
typedef struct _ClayTiling                      ClayTiling;
typedef struct _ClayMandala                     ClayMandala;


/*  misc objects  */

typedef struct _ClayAsync                       ClayAsync;
typedef struct _ClayAsyncSet                    ClayAsyncSet;
typedef struct _ClayBuffer                      ClayBuffer;
typedef struct _ClayDrawableFilter              ClayDrawableFilter;
typedef struct _ClayEnvironTable                ClayEnvironTable;
typedef struct _ClayExtension                   ClayExtension;
typedef struct _ClayExtensionManager            ClayExtensionManager;
typedef struct _ClayHistogram                   ClayHistogram;
typedef struct _ClayIdTable                     ClayIdTable;
typedef struct _ClayImagefile                   ClayImagefile;
typedef struct _ClayImageProxy                  ClayImageProxy;
typedef struct _ClayInterpreterDB               ClayInterpreterDB;
typedef struct _ClayLineArt                     ClayLineArt;
typedef struct _ClayObjectQueue                 ClayObjectQueue;
typedef struct _ClayParasiteList                ClayParasiteList;
typedef struct _ClayPdbProgress                 ClayPdbProgress;
typedef struct _ClayProjection                  ClayProjection;
typedef struct _ClaySettings                    ClaySettings;
typedef struct _ClaySubProgress                 ClaySubProgress;
typedef struct _ClayTag                         ClayTag;
typedef struct _ClayTreeHandler                 ClayTreeHandler;
typedef struct _ClayTriviallyCancelableWaitable ClayTriviallyCancelableWaitable;
typedef struct _ClayUncancelableWaitable        ClayUncancelableWaitable;


/*  interfaces  */

typedef struct _ClayCancelable                  ClayCancelable;  /* dummy typedef */
typedef struct _ClayPickable                    ClayPickable;    /* dummy typedef */
typedef struct _ClayProgress                    ClayProgress;    /* dummy typedef */
typedef struct _ClayProjectable                 ClayProjectable; /* dummy typedef */
typedef struct _ClaySavable                     ClaySavable;     /* dummy typedef */
typedef struct _ClayTagged                      ClayTagged;      /* dummy typedef */
typedef struct _ClayWaitable                    ClayWaitable;    /* dummy typedef */


/*  non-object types  */

typedef struct _ClayBacktrace                   ClayBacktrace;
typedef struct _ClayBoundSeg                    ClayBoundSeg;
typedef struct _ClayChunkIterator               ClayChunkIterator;
typedef struct _ClayCoords                      ClayCoords;
typedef struct _ClayGradientSegment             ClayGradientSegment;
typedef struct _ClayPaletteEntry                ClayPaletteEntry;
typedef struct _ClayScanConvert                 ClayScanConvert;
typedef struct _ClayTempBuf                     ClayTempBuf;
typedef         guint32                         ClayTattoo;

/* The following hack is made so that we can reuse the definition
 * the cairo definition of cairo_path_t without having to translate
 * between our own version of a bezier description and cairos version.
 *
 * to avoid having to include <cairo.h> in each and every file
 * including this file we only use the "real" definition when cairo.h
 * already has been included and use something else.
 *
 * Note that if you really want to work with ClayBezierDesc (except just
 * passing pointers to it around) you also need to include <cairo.h>.
 */
#ifdef CAIRO_VERSION
typedef cairo_path_t ClayBezierDesc;
#else
typedef void * ClayBezierDesc;
#endif


/*  functions  */

typedef void     (* ClayInitStatusFunc)    (const gchar *text1,
                                            const gchar *text2,
                                            gdouble      percentage);

typedef gboolean (* ClayObjectFilterFunc)  (ClayObject  *object,
                                            gpointer     user_data);

typedef gint64   (* ClayMemsizeFunc)       (gpointer     instance,
                                            gint64      *gui_size);

typedef void     (* ClayRunAsyncFunc)      (ClayAsync   *async,
                                            gpointer     user_data);


/*  structs  */

struct _ClayCoords
{
  /* axes as reported by the device */
  gdouble  x;
  gdouble  y;
  gdouble  pressure;
  gdouble  xtilt;
  gdouble  ytilt;
  gdouble  wheel;
  gdouble  distance;
  gdouble  rotation;
  gdouble  slider;

  /* synthetic axes */
  gdouble  velocity;
  gdouble  direction;

  /* view transform */
  gdouble  xscale;  /* the view scale                */
  gdouble  yscale;
  gdouble  angle;   /* the view rotation angle       */
  gboolean reflect; /* whether the view is reflected */
};

/*  temp hack as replacement for GdkSegment  */

typedef struct _ClaySegment ClaySegment;

struct _ClaySegment
{
  gint x1;
  gint y1;
  gint x2;
  gint y2;
};


#include "gegl/clay-gegl-types.h"
#include "paint/paint-types.h"
#include "text/text-types.h"
#include "path/path-types.h"
#include "pdb/pdb-types.h"
#include "plug-in/plug-in-types.h"

