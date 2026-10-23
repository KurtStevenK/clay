/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995-1997 Spencer Kimball and Peter Mattis
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

#include "config.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gegl.h>

#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/claycoreconfig.h"

#include "clay.h"
#include "claydocumentlist.h"
#include "clayimagefile.h"


G_DEFINE_TYPE (ClayDocumentList, clay_document_list, CLAY_TYPE_LIST)


static void
clay_document_list_class_init (ClayDocumentListClass *klass)
{
}

static void
clay_document_list_init (ClayDocumentList *list)
{
}

ClayContainer *
clay_document_list_new (Clay *clay)
{
  ClayDocumentList *document_list;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  document_list = g_object_new (CLAY_TYPE_DOCUMENT_LIST,
                                "name",       "document-list",
                                "child-type", CLAY_TYPE_IMAGEFILE,
                                "policy",     CLAY_CONTAINER_POLICY_STRONG,
                                NULL);

  document_list->clay = clay;

  return CLAY_CONTAINER (document_list);
}

ClayImagefile *
clay_document_list_add_file (ClayDocumentList *document_list,
                             GFile            *file,
                             const gchar      *mime_type)
{
  Clay          *clay;
  ClayImagefile *imagefile;
  ClayContainer *container;
  gchar         *uri;

  g_return_val_if_fail (CLAY_IS_DOCUMENT_LIST (document_list), NULL);
  g_return_val_if_fail (G_IS_FILE (file), NULL);

  container = CLAY_CONTAINER (document_list);

  clay = document_list->clay;

  uri = g_file_get_uri (file);

  imagefile = (ClayImagefile *) clay_container_get_child_by_name (container,
                                                                  uri);

  g_free (uri);

  if (imagefile)
    {
      clay_container_reorder (container, CLAY_OBJECT (imagefile), 0);
    }
  else
    {
      imagefile = clay_imagefile_new (clay, file);
      clay_container_add (container, CLAY_OBJECT (imagefile));
      g_object_unref (imagefile);
    }

  clay_imagefile_set_mime_type (imagefile, mime_type);

  if (clay->config->save_document_history)
    clay_recent_list_add_file (clay, file, mime_type);

  return imagefile;
}
