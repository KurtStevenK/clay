/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-1997 Peter Mattis and Spencer Kimball
 *
 * This library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.  If not, see
 * <https://www.gnu.org/licenses/>.
 */

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif

#ifndef __CLAY_CPU_ACCEL_H__
#define __CLAY_CPU_ACCEL_H__

G_BEGIN_DECLS


/**
 * ClayCpuAccelFlags:
 * @CLAY_CPU_ACCEL_NONE:        None
 * @CLAY_CPU_ACCEL_X86_MMX:     MMX
 * @CLAY_CPU_ACCEL_X86_3DNOW:   3dNow
 * @CLAY_CPU_ACCEL_X86_MMXEXT:  MMXEXT
 * @CLAY_CPU_ACCEL_X86_SSE:     SSE
 * @CLAY_CPU_ACCEL_X86_SSE2:    SSE2
 * @CLAY_CPU_ACCEL_X86_SSE3:    SSE3
 * @CLAY_CPU_ACCEL_X86_SSSE3:   SSSE3
 * @CLAY_CPU_ACCEL_X86_SSE4_1:  SSE4_1
 * @CLAY_CPU_ACCEL_X86_SSE4_2:  SSE4_2
 * @CLAY_CPU_ACCEL_X86_AVX:     AVX
 * @CLAY_CPU_ACCEL_PPC_ALTIVEC: Altivec
 *
 * Types of detectable CPU accelerations
 **/
typedef enum
{
  CLAY_CPU_ACCEL_NONE        = 0x0,

  /* x86 accelerations */
  CLAY_CPU_ACCEL_X86_MMX     = 0x80000000,
  CLAY_CPU_ACCEL_X86_3DNOW   = 0x40000000,
  CLAY_CPU_ACCEL_X86_MMXEXT  = 0x20000000,
  CLAY_CPU_ACCEL_X86_SSE     = 0x10000000,
  CLAY_CPU_ACCEL_X86_SSE2    = 0x08000000,
  CLAY_CPU_ACCEL_X86_SSE3    = 0x02000000,
  CLAY_CPU_ACCEL_X86_SSSE3   = 0x01000000,
  CLAY_CPU_ACCEL_X86_SSE4_1  = 0x00800000,
  CLAY_CPU_ACCEL_X86_SSE4_2  = 0x00400000,
  CLAY_CPU_ACCEL_X86_AVX     = 0x00200000,

  /* powerpc accelerations */
  CLAY_CPU_ACCEL_PPC_ALTIVEC = 0x04000000
} ClayCpuAccelFlags;


ClayCpuAccelFlags  clay_cpu_accel_get_support (void);


G_END_DECLS

#endif  /* __CLAY_CPU_ACCEL_H__ */
