/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * ClayTextProxy
 * Copyright (C) 2009-2010  Michael Natterer <mitch@gimp.org>
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


#define CLAY_TYPE_TEXT_PROXY            (clay_text_proxy_get_type ())
#define CLAY_TEXT_PROXY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_TEXT_PROXY, ClayTextProxy))
#define CLAY_IS_TEXT_PROXY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_TEXT_PROXY))
#define CLAY_TEXT_PROXY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_TEXT_PROXY, ClayTextProxyClass))
#define CLAY_IS_TEXT_PROXY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_TEXT_PROXY))


typedef struct _ClayTextProxy       ClayTextProxy;
typedef struct _ClayTextProxyClass  ClayTextProxyClass;

struct _ClayTextProxy
{
  GtkTextView  parent_instance;
};

struct _ClayTextProxyClass
{
  GtkTextViewClass  parent_class;

  void (* change_size)     (ClayTextProxy *proxy,
                            gdouble        amount);
  void (* change_baseline) (ClayTextProxy *proxy,
                            gdouble        amount);
  void (* change_kerning)  (ClayTextProxy *proxy,
                            gdouble        amount);
};


GType       clay_text_proxy_get_type (void);

GtkWidget * clay_text_proxy_new      (void);
