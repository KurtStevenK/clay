/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * clayplugin-private.h
 * Copyright (C) 2019 Michael Natterer <mitch@gimp.org>
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

#ifndef __CLAY_PLUG_IN_PRIVATE_H__
#define __CLAY_PLUG_IN_PRIVATE_H__

G_BEGIN_DECLS


G_GNUC_INTERNAL void            _clay_plug_in_query                  (ClayPlugIn      *plug_in);
G_GNUC_INTERNAL void            _clay_plug_in_init                   (ClayPlugIn      *plug_in);
G_GNUC_INTERNAL void            _clay_plug_in_run                    (ClayPlugIn      *plug_in);
G_GNUC_INTERNAL void            _clay_plug_in_quit                   (ClayPlugIn      *plug_in);

G_GNUC_INTERNAL GIOChannel    * _clay_plug_in_get_read_channel       (ClayPlugIn      *plug_in);
G_GNUC_INTERNAL GIOChannel    * _clay_plug_in_get_write_channel      (ClayPlugIn      *plug_in);

G_GNUC_INTERNAL void            _clay_plug_in_read_expect_msg        (ClayPlugIn      *plug_in,
                                                                      ClayWireMessage *msg,
                                                                      gint             type);

G_GNUC_INTERNAL gboolean        _clay_plug_in_set_i18n               (ClayPlugIn      *plug_in,
                                                                      const gchar     *procedure_name,
                                                                      gchar          **gettext_domain,
                                                                      gchar          **catalog_dir);

G_GNUC_INTERNAL ClayProcedure * _clay_plug_in_create_procedure       (ClayPlugIn      *plug_in,
                                                                      const gchar     *procedure_name);

G_GNUC_INTERNAL ClayProcedure * _clay_plug_in_get_procedure          (ClayPlugIn      *plug_in);

G_GNUC_INTERNAL ClayDisplay   * _clay_plug_in_get_display            (ClayPlugIn      *plug_in,
                                                                      gint32           display_id);
G_GNUC_INTERNAL ClayImage     * _clay_plug_in_get_image              (ClayPlugIn      *plug_in,
                                                                      gint32           image_id);
G_GNUC_INTERNAL ClayItem      * _clay_plug_in_get_item               (ClayPlugIn      *plug_in,
                                                                      gint32           item_id);
G_GNUC_INTERNAL ClayDrawableFilter *
                                _clay_plug_in_get_filter             (ClayPlugIn      *plug_in,
                                                                      gint32           filter_id);
G_GNUC_INTERNAL ClayResource  * _clay_plug_in_get_resource           (ClayPlugIn      *plug_in,
                                                                      gint32           resource_id);

G_GNUC_INTERNAL gboolean        _clay_plug_in_manage_memory_manually (ClayPlugIn      *plug_in);


G_END_DECLS

#endif /* __CLAY_PLUG_IN_PRIVATE_H__ */
