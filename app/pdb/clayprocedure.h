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

#include "core/clayviewable.h"


typedef ClayValueArray * (* ClayMarshalFunc) (ClayProcedure         *procedure,
                                              Clay                  *clay,
                                              ClayContext           *context,
                                              ClayProgress          *progress,
                                              const ClayValueArray  *args,
                                              GError               **error);


#define CLAY_TYPE_PROCEDURE            (clay_procedure_get_type ())
#define CLAY_PROCEDURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), CLAY_TYPE_PROCEDURE, ClayProcedure))
#define CLAY_PROCEDURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), CLAY_TYPE_PROCEDURE, ClayProcedureClass))
#define CLAY_IS_PROCEDURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CLAY_TYPE_PROCEDURE))
#define CLAY_IS_PROCEDURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CLAY_TYPE_PROCEDURE))
#define CLAY_PROCEDURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), CLAY_TYPE_PROCEDURE, ClayProcedureClass))


typedef struct _ClayProcedureClass ClayProcedureClass;

struct _ClayProcedure
{
  ClayViewable      parent_instance;

  ClayPDBProcType   proc_type;      /* Type of procedure              */

  gboolean          static_help;    /* Are the strings allocated?     */
  gchar            *blurb;          /* Short procedure description    */
  gchar            *help;           /* Detailed help instructions     */
  gchar            *help_id;        /* Help ID                        */

  gboolean          static_attribution;
  gchar            *authors;        /* Authors field                  */
  gchar            *copyright;      /* Copyright field                */
  gchar            *date;           /* Date field                     */

  gchar            *deprecated;     /* Replacement if deprecated      */

  gchar            *label;          /* Cached label string            */

  gint32            num_args;       /* Number of procedure arguments  */
  GParamSpec      **args;           /* Array of procedure arguments   */

  gint32            num_values;     /* Number of return values        */
  GParamSpec      **values;         /* Array of return values         */

  ClayMarshalFunc   marshal_func;   /* Marshaller for internal procs  */

  gboolean          is_private;     /* Invisible procedure            */
};

struct _ClayProcedureClass
{
  ClayViewableClass parent_class;

  const gchar    * (* get_label)      (ClayProcedure   *procedure);
  const gchar    * (* get_menu_label) (ClayProcedure   *procedure);
  const gchar    * (* get_blurb)      (ClayProcedure   *procedure);
  const gchar    * (* get_help_id)    (ClayProcedure   *procedure);
  gboolean         (* get_sensitive)  (ClayProcedure   *procedure,
                                       ClayObject      *object,
                                       const gchar    **reason);

  ClayValueArray * (* execute)        (ClayProcedure   *procedure,
                                       Clay            *clay,
                                       ClayContext     *context,
                                       ClayProgress    *progress,
                                       ClayValueArray  *args,
                                       GError         **error);
  void             (* execute_async)  (ClayProcedure   *procedure,
                                       Clay            *clay,
                                       ClayContext     *context,
                                       ClayProgress    *progress,
                                       ClayValueArray  *args,
                                       ClayDisplay     *display);
};


GType            clay_procedure_get_type           (void);

ClayProcedure  * clay_procedure_new                (ClayMarshalFunc   marshal_func,
                                                    gboolean          is_private);

void             clay_procedure_set_help           (ClayProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             clay_procedure_set_static_help    (ClayProcedure    *procedure,
                                                    const gchar      *blurb,
                                                    const gchar      *help,
                                                    const gchar      *help_id);
void             clay_procedure_take_help          (ClayProcedure    *procedure,
                                                    gchar            *blurb,
                                                    gchar            *help,
                                                    gchar            *help_id);

void             clay_procedure_set_attribution    (ClayProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             clay_procedure_set_static_attribution
                                                   (ClayProcedure    *procedure,
                                                    const gchar      *authors,
                                                    const gchar      *copyright,
                                                    const gchar      *date);
void             clay_procedure_take_attribution   (ClayProcedure    *procedure,
                                                    gchar            *authors,
                                                    gchar            *copyright,
                                                    gchar            *date);

void             clay_procedure_set_deprecated     (ClayProcedure    *procedure,
                                                    const gchar      *deprecated);

const gchar    * clay_procedure_get_label          (ClayProcedure    *procedure);
const gchar    * clay_procedure_get_menu_label     (ClayProcedure    *procedure);
const gchar    * clay_procedure_get_blurb          (ClayProcedure    *procedure);
const gchar    * clay_procedure_get_help           (ClayProcedure    *procedure);
const gchar    * clay_procedure_get_help_id        (ClayProcedure    *procedure);
gboolean         clay_procedure_get_sensitive      (ClayProcedure    *procedure,
                                                    ClayObject       *object,
                                                    const gchar     **reason);

void             clay_procedure_add_argument       (ClayProcedure    *procedure,
                                                    GParamSpec       *pspec);
void             clay_procedure_add_return_value   (ClayProcedure    *procedure,
                                                    GParamSpec       *pspec);

ClayValueArray * clay_procedure_get_arguments      (ClayProcedure    *procedure);
ClayValueArray * clay_procedure_get_return_values  (ClayProcedure    *procedure,
                                                    gboolean          success,
                                                    const GError     *error);

ClayProcedure  * clay_procedure_create_override    (ClayProcedure    *procedure,
                                                    ClayMarshalFunc   new_marshal_func);

ClayValueArray * clay_procedure_execute            (ClayProcedure    *procedure,
                                                    Clay             *clay,
                                                    ClayContext      *context,
                                                    ClayProgress     *progress,
                                                    ClayValueArray   *args,
                                                    GError          **error);
void             clay_procedure_execute_async      (ClayProcedure    *procedure,
                                                    Clay             *clay,
                                                    ClayContext      *context,
                                                    ClayProgress     *progress,
                                                    ClayValueArray   *args,
                                                    ClayDisplay      *display,
                                                    GError          **error);

gint             clay_procedure_name_compare       (ClayProcedure    *proc1,
                                                    ClayProcedure    *proc2);
