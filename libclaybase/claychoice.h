/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claychoice.h
 * Copyright (C) 2023 Jehan
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

#if !defined (__CLAY_BASE_H_INSIDE__) && !defined (CLAY_BASE_COMPILATION)
#error "Only <libclaybase/claybase.h> can be included directly."
#endif


#ifndef __CLAY_CHOICE_H__
#define __CLAY_CHOICE_H__

G_BEGIN_DECLS


/* For information look into the C source or the html documentation */


#define CLAY_TYPE_CHOICE (clay_choice_get_type ())
G_DECLARE_FINAL_TYPE (ClayChoice, clay_choice, CLAY, CHOICE, GObject)


ClayChoice    * clay_choice_new               (void);
ClayChoice    * clay_choice_new_with_values   (const gchar *nick,
                                               gint         id,
                                               const gchar *label,
                                               const gchar *help,
                                               ...) G_GNUC_NULL_TERMINATED;
void            clay_choice_add               (ClayChoice  *choice,
                                               const gchar *nick,
                                               gint         id,
                                               const gchar *label,
                                               const gchar *help);
void            clay_choice_add_deprecated    (ClayChoice  *choice,
                                               const gchar *nick,
                                               gint         id,
                                               const gchar *redirect_to,
                                               const gchar *reason);

gboolean        clay_choice_is_valid          (ClayChoice   *choice,
                                               const gchar  *nick);
GList         * clay_choice_list_nicks        (ClayChoice   *choice);
gint            clay_choice_get_id            (ClayChoice   *choice,
                                               const gchar  *nick);
const gchar   * clay_choice_get_label         (ClayChoice   *choice,
                                               const gchar  *nick);
const gchar   * clay_choice_get_help          (ClayChoice   *choice,
                                               const gchar  *nick);
gboolean        clay_choice_get_documentation (ClayChoice   *choice,
                                               const gchar  *nick,
                                               const gchar **label,
                                               const gchar **help);
gboolean        clay_choice_is_deprecated      (ClayChoice   *choice,
                                               const gchar  *nick,
                                               const gchar **redirect_to,
                                               const gchar **reason);

void            clay_choice_set_sensitive     (ClayChoice   *choice,
                                               const gchar  *nick,
                                               gboolean      sensitive);


/*
 * CLAY_TYPE_PARAM_CHOICE
 */

#define CLAY_TYPE_PARAM_CHOICE           (clay_param_choice_get_type ())
#define CLAY_IS_PARAM_SPEC_CHOICE(pspec) (G_TYPE_CHECK_INSTANCE_TYPE ((pspec), CLAY_TYPE_PARAM_CHOICE))


GType        clay_param_choice_get_type         (void);

GParamSpec * clay_param_spec_choice             (const gchar  *name,
                                                 const gchar  *nick,
                                                 const gchar  *blurb,
                                                 ClayChoice   *choice,
                                                 const gchar  *default_value,
                                                 GParamFlags   flags);

ClayChoice  * clay_param_spec_choice_get_choice  (GParamSpec *pspec);
const gchar * clay_param_spec_choice_get_default (GParamSpec *pspec);


G_END_DECLS

#endif /* __CLAY_CHOICE_H__ */
