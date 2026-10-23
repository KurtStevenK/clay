/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * claytagcache.c
 * Copyright (C) 2008 Aurimas Juška <aurisj@svn.gnome.org>
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

#include "libclaybase/claybase.h"
#include "libclaymath/claymath.h"
#include "libclayconfig/clayconfig.h"

#include "core-types.h"

#include "config/clayxmlparser.h"

#include "clay-memsize.h"
#include "claycontext.h"
#include "claydata.h"
#include "claylist.h"
#include "claytag.h"
#include "claytagcache.h"
#include "claytagged.h"

#include "clay-intl.h"


#define CLAY_TAG_CACHE_FILE  "tags.xml"

/* #define DEBUG_CLAY_TAG_CACHE  1 */


enum
{
  PROP_0,
  PROP_CLAY
};


typedef struct
{
  GQuark  identifier;
  GQuark  checksum;
  GList  *tags;
  guint   referenced : 1;
} ClayTagCacheRecord;

typedef struct
{
  GArray             *records;
  ClayTagCacheRecord  current_record;
} ClayTagCacheParseData;

struct _ClayTagCachePrivate
{
  GArray *records;
  GList  *containers;
};


static void          clay_tag_cache_finalize           (GObject                *object);

static gint64        clay_tag_cache_get_memsize        (ClayObject             *object,
                                                        gint64                 *gui_size);
static void          clay_tag_cache_object_initialize  (ClayTagged             *tagged,
                                                        ClayTagCache           *cache);
static void          clay_tag_cache_add_object         (ClayTagCache           *cache,
                                                        ClayTagged             *tagged);

static void          clay_tag_cache_load_start_element (GMarkupParseContext    *context,
                                                        const gchar            *element_name,
                                                        const gchar           **attribute_names,
                                                        const gchar           **attribute_values,
                                                        gpointer                user_data,
                                                        GError                **error);
static void          clay_tag_cache_load_end_element   (GMarkupParseContext    *context,
                                                        const gchar            *element_name,
                                                        gpointer                user_data,
                                                        GError                **error);
static void          clay_tag_cache_load_text          (GMarkupParseContext    *context,
                                                        const gchar            *text,
                                                        gsize                   text_len,
                                                        gpointer                user_data,
                                                        GError                **error);
static  void         clay_tag_cache_load_error         (GMarkupParseContext    *context,
                                                        GError                 *error,
                                                        gpointer                user_data);
static const gchar * clay_tag_cache_attribute_name_to_value
                                                       (const gchar           **attribute_names,
                                                        const gchar           **attribute_values,
                                                        const gchar            *name);

static GQuark        clay_tag_cache_get_error_domain   (void);


G_DEFINE_TYPE_WITH_PRIVATE (ClayTagCache, clay_tag_cache, CLAY_TYPE_OBJECT)

#define parent_class clay_tag_cache_parent_class


static void
clay_tag_cache_class_init (ClayTagCacheClass *klass)
{
  GObjectClass    *object_class      = G_OBJECT_CLASS (klass);
  ClayObjectClass *clay_object_class = CLAY_OBJECT_CLASS (klass);

  object_class->finalize         = clay_tag_cache_finalize;

  clay_object_class->get_memsize = clay_tag_cache_get_memsize;
}

static void
clay_tag_cache_init (ClayTagCache *cache)
{
  cache->priv = clay_tag_cache_get_instance_private (cache);

  cache->priv->records    = g_array_new (FALSE, FALSE,
                                         sizeof (ClayTagCacheRecord));
  cache->priv->containers = NULL;
}

static void
clay_tag_cache_finalize (GObject *object)
{
  ClayTagCache *cache = CLAY_TAG_CACHE (object);

  if (cache->priv->records)
    {
      gint i;

      for (i = 0; i < cache->priv->records->len; i++)
        {
          ClayTagCacheRecord *rec = &g_array_index (cache->priv->records,
                                                    ClayTagCacheRecord, i);

          g_list_free_full (rec->tags, (GDestroyNotify) g_object_unref);
        }

      g_array_free (cache->priv->records, TRUE);
      cache->priv->records = NULL;
    }

  if (cache->priv->containers)
    {
      g_list_free (cache->priv->containers);
      cache->priv->containers = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static gint64
clay_tag_cache_get_memsize (ClayObject *object,
                            gint64     *gui_size)
{
  ClayTagCache *cache   = CLAY_TAG_CACHE (object);
  gint64        memsize = 0;

  memsize += clay_g_list_get_memsize (cache->priv->containers, 0);
  memsize += cache->priv->records->len * sizeof (ClayTagCacheRecord);

  return memsize + CLAY_OBJECT_CLASS (parent_class)->get_memsize (object,
                                                                  gui_size);
}

/**
 * clay_tag_cache_new:
 *
 * Returns: creates new ClayTagCache object.
 **/
ClayTagCache *
clay_tag_cache_new (void)
{
  return g_object_new (CLAY_TYPE_TAG_CACHE, NULL);
}

static void
clay_tag_cache_container_add_callback (ClayTagCache  *cache,
                                       ClayTagged    *tagged,
                                       ClayContainer *not_used)
{
  clay_tag_cache_add_object (cache, tagged);
}

/**
 * clay_tag_cache_add_container:
 * @cache:      a ClayTagCache object.
 * @container:  container containing ClayTagged objects.
 *
 * Adds container of ClayTagged objects to tag cache. Before calling this
 * function tag cache must be loaded using clay_tag_cache_load(). When tag
 * cache is saved to file, tags are collected from objects in priv->containers.
 **/
void
clay_tag_cache_add_container (ClayTagCache  *cache,
                              ClayContainer *container)
{
  g_return_if_fail (CLAY_IS_TAG_CACHE (cache));
  g_return_if_fail (CLAY_IS_CONTAINER (container));

  cache->priv->containers = g_list_append (cache->priv->containers, container);
  clay_container_foreach (container, (GFunc) clay_tag_cache_object_initialize,
                          cache);

  g_signal_connect_swapped (container, "add",
                            G_CALLBACK (clay_tag_cache_container_add_callback),
                            cache);
}

static void
clay_tag_cache_add_object (ClayTagCache *cache,
                           ClayTagged   *tagged)
{
  gchar  *identifier;
  GQuark  identifier_quark = 0;
  gchar  *checksum;
  GQuark  checksum_quark = 0;
  GList  *list;
  gint    i;

  identifier = clay_tagged_get_identifier (tagged);

  if (identifier)
    {
      identifier_quark = g_quark_try_string (identifier);
      g_free (identifier);
    }

  if (identifier_quark)
    {
      for (i = 0; i < cache->priv->records->len; i++)
        {
          ClayTagCacheRecord *rec = &g_array_index (cache->priv->records,
                                                    ClayTagCacheRecord, i);

          if (rec->identifier == identifier_quark)
            {
              for (list = rec->tags; list; list = g_list_next (list))
                {
                  clay_tagged_add_tag (tagged, CLAY_TAG (list->data));
                }

              rec->referenced = TRUE;
              return;
            }
        }
    }

  checksum = clay_tagged_get_checksum (tagged);

  if (checksum)
    {
      checksum_quark = g_quark_try_string (checksum);
      g_free (checksum);
    }

  if (checksum_quark)
    {
      for (i = 0; i < cache->priv->records->len; i++)
        {
          ClayTagCacheRecord *rec = &g_array_index (cache->priv->records,
                                                    ClayTagCacheRecord, i);

          if (rec->checksum == checksum_quark)
            {
#if DEBUG_CLAY_TAG_CACHE
              g_printerr ("remapping identifier: %s ==> %s\n",
                          rec->identifier ? g_quark_to_string (rec->identifier) : "(NULL)",
                          identifier_quark ? g_quark_to_string (identifier_quark) : "(NULL)");
#endif

              rec->identifier = identifier_quark;

              for (list = rec->tags; list; list = g_list_next (list))
                {
                  clay_tagged_add_tag (tagged, CLAY_TAG (list->data));
                }

              rec->referenced = TRUE;
              return;
            }
        }
    }

}

static void
clay_tag_cache_object_initialize (ClayTagged   *tagged,
                                  ClayTagCache *cache)
{
  clay_tag_cache_add_object (cache, tagged);
}

static void
clay_tag_cache_tagged_to_cache_record_foreach (ClayTagged  *tagged,
                                               GList      **cache_records)
{
  gchar *identifier = clay_tagged_get_identifier (tagged);

  if (identifier)
    {
      ClayTagCacheRecord *cache_rec = g_new (ClayTagCacheRecord, 1);
      gchar              *checksum;

      checksum = clay_tagged_get_checksum (tagged);

      cache_rec->identifier = g_quark_from_string (identifier);
      cache_rec->checksum   = g_quark_from_string (checksum);
      cache_rec->tags       = g_list_copy (clay_tagged_get_tags (tagged));

      g_free (checksum);

      *cache_records = g_list_prepend (*cache_records, cache_rec);
    }

  g_free (identifier);
}

/**
 * clay_tag_cache_save:
 * @cache:      a ClayTagCache object.
 *
 * Saves tag cache to cache file.
 **/
void
clay_tag_cache_save (ClayTagCache *cache)
{
  GString       *buf;
  GList         *saved_records;
  GList         *iterator;
  GFile         *file;
  GOutputStream *output;
  GError        *error = NULL;
  gint           i;

  g_return_if_fail (CLAY_IS_TAG_CACHE (cache));

  saved_records = NULL;
  for (i = 0; i < cache->priv->records->len; i++)
    {
      ClayTagCacheRecord *current_record = &g_array_index (cache->priv->records,
                                                           ClayTagCacheRecord, i);

      if (! current_record->referenced && current_record->tags)
        {
          /* keep tagged objects which have tags assigned
           * but were not loaded.
           */
          ClayTagCacheRecord *record_copy = g_new (ClayTagCacheRecord, 1);

          record_copy->identifier = current_record->identifier;
          record_copy->checksum   = current_record->checksum;
          record_copy->tags       = g_list_copy (current_record->tags);

          saved_records = g_list_prepend (saved_records, record_copy);
        }
    }

  for (iterator = cache->priv->containers;
       iterator;
       iterator = g_list_next (iterator))
    {
      clay_container_foreach (CLAY_CONTAINER (iterator->data),
                              (GFunc) clay_tag_cache_tagged_to_cache_record_foreach,
                              &saved_records);
    }

  saved_records = g_list_reverse (saved_records);

  buf = g_string_new ("");
  g_string_append (buf, "<?xml version='1.0' encoding='UTF-8'?>\n");
  g_string_append (buf, "<tags>\n");

  for (iterator = saved_records; iterator; iterator = g_list_next (iterator))
    {
      ClayTagCacheRecord *cache_rec = iterator->data;
      GList              *tag_iterator;
      gchar              *identifier_string;
      gchar              *tag_string;

      identifier_string = g_markup_escape_text (g_quark_to_string (cache_rec->identifier), -1);
      g_string_append_printf (buf, "\n  <resource identifier=\"%s\" checksum=\"%s\">\n",
                              identifier_string,
                              g_quark_to_string (cache_rec->checksum));
      g_free (identifier_string);

      for (tag_iterator = cache_rec->tags;
           tag_iterator;
           tag_iterator = g_list_next (tag_iterator))
        {
          ClayTag *tag = CLAY_TAG (tag_iterator->data);

          if (! clay_tag_get_internal (tag))
            {
              tag_string = g_markup_escape_text (clay_tag_get_name (tag), -1);
              g_string_append_printf (buf, "    <tag>%s</tag>\n", tag_string);
              g_free (tag_string);
            }
        }

      g_string_append (buf, "  </resource>\n");
    }

  g_string_append (buf, "</tags>\n");

  file = clay_directory_file (CLAY_TAG_CACHE_FILE, NULL);

  output = G_OUTPUT_STREAM (g_file_replace (file,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      g_printerr ("%s\n", error->message);
    }
  else if (! g_output_stream_write_all (output, buf->str, buf->len,
                                        NULL, NULL, &error))
    {
      GCancellable *cancellable = g_cancellable_new ();

      g_printerr (_("Error writing '%s': %s\n"),
                  clay_file_get_utf8_name (file), error->message);

      /* Cancel the overwrite initiated by g_file_replace(). */
      g_cancellable_cancel (cancellable);
      g_output_stream_close (output, cancellable, NULL);
      g_object_unref (cancellable);
    }
  else if (! g_output_stream_close (output, NULL, &error))
    {
      g_printerr (_("Error closing '%s': %s\n"),
                  clay_file_get_utf8_name (file), error->message);
    }

  if (output)
    g_object_unref (output);

  g_clear_error (&error);
  g_object_unref (file);
  g_string_free (buf, TRUE);

  for (iterator = saved_records;
       iterator;
       iterator = g_list_next (iterator))
    {
      ClayTagCacheRecord *cache_rec = iterator->data;

      g_list_free (cache_rec->tags);
      g_free (cache_rec);
    }

  g_list_free (saved_records);
}

/**
 * clay_tag_cache_load:
 * @cache:      a ClayTagCache object.
 *
 * Loads tag cache from file.
 **/
void
clay_tag_cache_load (ClayTagCache *cache)
{
  GFile                 *file;
  GMarkupParser          markup_parser;
  ClayXmlParser         *xml_parser;
  ClayTagCacheParseData  parse_data;
  GError                *error = NULL;

  g_return_if_fail (CLAY_IS_TAG_CACHE (cache));

  /* clear any previous priv->records */
  cache->priv->records = g_array_set_size (cache->priv->records, 0);

  parse_data.records = g_array_new (FALSE, FALSE, sizeof (ClayTagCacheRecord));
  memset (&parse_data.current_record, 0, sizeof (ClayTagCacheRecord));

  markup_parser.start_element = clay_tag_cache_load_start_element;
  markup_parser.end_element   = clay_tag_cache_load_end_element;
  markup_parser.text          = clay_tag_cache_load_text;
  markup_parser.passthrough   = NULL;
  markup_parser.error         = clay_tag_cache_load_error;

  xml_parser = clay_xml_parser_new (&markup_parser, &parse_data);

  file = clay_directory_file (CLAY_TAG_CACHE_FILE, NULL);

  if (clay_xml_parser_parse_gfile (xml_parser, file, &error))
    {
      cache->priv->records = g_array_append_vals (cache->priv->records,
                                                  parse_data.records->data,
                                                  parse_data.records->len);
    }
  else
    {
      /* Ignore "No such file or directory" errors. The default tags.xml
       * will be created in this case.
       */
      if (error->domain != G_FILE_ERROR || error->code != G_FILE_ERROR_NOENT)
        g_printerr ("Failed to parse tag cache: %s\n",
                    error ? error->message : "Unknown error");
      g_clear_error (&error);
    }

  g_object_unref (file);
  clay_xml_parser_free (xml_parser);
  g_array_free (parse_data.records, TRUE);
}

static  void
clay_tag_cache_load_start_element (GMarkupParseContext *context,
                                   const gchar         *element_name,
                                   const gchar        **attribute_names,
                                   const gchar        **attribute_values,
                                   gpointer             user_data,
                                   GError             **error)
{
  ClayTagCacheParseData *parse_data = user_data;

  if (! strcmp (element_name, "resource"))
    {
      const gchar *identifier;
      const gchar *checksum;

      identifier = clay_tag_cache_attribute_name_to_value (attribute_names,
                                                           attribute_values,
                                                           "identifier");
      checksum   = clay_tag_cache_attribute_name_to_value (attribute_names,
                                                           attribute_values,
                                                           "checksum");

      if (! identifier)
        {
          g_set_error (error,
                       clay_tag_cache_get_error_domain (),
                       1001,
                       "Resource tag does not contain required attribute identifier.");
          return;
        }

      memset (&parse_data->current_record, 0, sizeof (ClayTagCacheRecord));

      parse_data->current_record.identifier = g_quark_from_string (identifier);
      parse_data->current_record.checksum   = g_quark_from_string (checksum);
    }
}

static void
clay_tag_cache_load_end_element (GMarkupParseContext *context,
                                 const gchar         *element_name,
                                 gpointer             user_data,
                                 GError             **error)
{
  ClayTagCacheParseData *parse_data = user_data;

  if (strcmp (element_name, "resource") == 0)
    {
      parse_data->records = g_array_append_val (parse_data->records,
                                                parse_data->current_record);
      memset (&parse_data->current_record, 0, sizeof (ClayTagCacheRecord));
    }
}

static void
clay_tag_cache_load_text (GMarkupParseContext  *context,
                          const gchar          *text,
                          gsize                 text_len,
                          gpointer              user_data,
                          GError              **error)
{
  ClayTagCacheParseData *parse_data = user_data;
  const gchar           *current_element;
  gchar                  buffer[2048];
  ClayTag               *tag;

  current_element = g_markup_parse_context_get_element (context);

  if (g_strcmp0 (current_element, "tag") == 0)
    {
      if (text_len >= sizeof (buffer))
        {
          g_set_error (error, clay_tag_cache_get_error_domain (), 1002,
                       "Tag value is too long.");
          return;
        }

      memcpy (buffer, text, text_len);
      buffer[text_len] = '\0';

      tag = clay_tag_new (buffer);
      if (tag)
        {
          parse_data->current_record.tags = g_list_append (parse_data->current_record.tags,
                                                           tag);
        }
      else
        {
          g_warning ("dropping invalid tag '%s' from '%s'\n", buffer,
                     g_quark_to_string (parse_data->current_record.identifier));
        }
    }
}

static  void
clay_tag_cache_load_error (GMarkupParseContext *context,
                           GError              *error,
                           gpointer             user_data)
{
  g_printerr ("Tag cache parse error: %s\n", error->message);
}

static const gchar*
clay_tag_cache_attribute_name_to_value (const gchar **attribute_names,
                                        const gchar **attribute_values,
                                        const gchar  *name)
{
  while (*attribute_names)
    {
      if (! strcmp (*attribute_names, name))
        {
          return *attribute_values;
        }

      attribute_names++;
      attribute_values++;
    }

  return NULL;
}

static GQuark
clay_tag_cache_get_error_domain (void)
{
  return g_quark_from_static_string ("clay-tag-cache-error-quark");
}
