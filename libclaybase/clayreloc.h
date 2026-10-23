/*
 * BinReloc - a library for creating relocatable executables
 * Written by: Hongli Lai <h.lai@chello.nl>
 * http://autopackage.org/
 *
 * This source code is public domain. You can relicense this code
 * under whatever license you want.
 *
 * See http://autopackage.org/docs/binreloc/ for
 * more information and how to use this.
 */

#ifndef __CLAY_RELOC_H__
#define __CLAY_RELOC_H__

G_BEGIN_DECLS


/* These error codes can be returned from _clay_reloc_init() or
 * _clay_reloc_init_lib().
 */

typedef enum
{
  /** Cannot allocate memory. */
  CLAY_RELOC_INIT_ERROR_NOMEM,
  /** Unable to open /proc/self/maps; see errno for details. */
  CLAY_RELOC_INIT_ERROR_OPEN_MAPS,
  /** Unable to read from /proc/self/maps; see errno for details. */
  CLAY_RELOC_INIT_ERROR_READ_MAPS,
  /** The file format of /proc/self/maps is invalid; kernel bug? */
  CLAY_RELOC_INIT_ERROR_INVALID_MAPS,
  /** BinReloc is disabled (the ENABLE_BINRELOC macro is not defined). */
  CLAY_RELOC_INIT_ERROR_DISABLED
} ClayBinrelocInitError;


G_GNUC_INTERNAL gboolean _clay_reloc_init        (GError **error);
G_GNUC_INTERNAL gboolean _clay_reloc_init_lib    (GError **error);

G_GNUC_INTERNAL gchar  * _clay_reloc_find_prefix (const gchar *default_prefix);


G_END_DECLS

#endif /* _CLAYRELOC_H_ */
