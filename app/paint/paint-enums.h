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

#if 0
   This file is parsed by two scripts, enumgen.pl in pdb,
   and clay-mkenums. All enums that are not marked with
   /*< pdb-skip >*/ are exported to libclay and the PDB. Enums that are
   not marked with /*< skip >*/ are registered with the GType system.
   If you want the enum to be skipped by both scripts, you have to use
   /*< pdb-skip, skip >*/.

   The same syntax applies to enum values.
#endif


/*
 * enums that are registered with the type system
 */

#define CLAY_TYPE_BRUSH_APPLICATION_MODE (clay_brush_application_mode_get_type ())

GType clay_brush_application_mode_get_type (void);

typedef enum
{
  CLAY_BRUSH_HARD,
  CLAY_BRUSH_SOFT,
  CLAY_BRUSH_PRESSURE  /*< pdb-skip, skip >*/
} ClayBrushApplicationMode;


#define CLAY_TYPE_PERSPECTIVE_CLONE_MODE (clay_perspective_clone_mode_get_type ())

GType clay_perspective_clone_mode_get_type (void);

typedef enum  /*< pdb-skip >*/
{
  CLAY_PERSPECTIVE_CLONE_MODE_ADJUST,  /*< desc="Modify Perspective" >*/
  CLAY_PERSPECTIVE_CLONE_MODE_PAINT    /*< desc="Perspective Clone"  >*/
} ClayPerspectiveCloneMode;


#define CLAY_TYPE_SOURCE_ALIGN_MODE (clay_source_align_mode_get_type ())

GType clay_source_align_mode_get_type (void);

typedef enum /*< pdb-skip >*/
{
  CLAY_SOURCE_ALIGN_NO,          /*< desc="None"        >*/
  CLAY_SOURCE_ALIGN_YES,         /*< desc="Aligned"     >*/
  CLAY_SOURCE_ALIGN_REGISTERED,  /*< desc="Registered"  >*/
  CLAY_SOURCE_ALIGN_FIXED        /*< desc="Fixed"       >*/
} ClaySourceAlignMode;


/*
 * non-registered enums; register them if needed
 */

typedef enum  /*< skip, pdb-skip >*/
{
  CLAY_PAINT_STATE_INIT,    /*  Setup PaintFunc internals                    */
  CLAY_PAINT_STATE_MOTION,  /*  PaintFunc performs motion-related rendering  */
  CLAY_PAINT_STATE_FINISH   /*  Cleanup and/or reset PaintFunc operation     */
} ClayPaintState;

/*  State of lock blinking  */
typedef enum /*< skip, pdb-skip >*/
{
  CLAY_PAINT_LOCK_NOT_BLINKED,
  CLAY_PAINT_LOCK_BLINK_PENDING,
  CLAY_PAINT_LOCK_BLINKED,
} ClayPaintLockBlinkState;
