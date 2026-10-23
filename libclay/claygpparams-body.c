/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2003 Peter Mattis and Spencer Kimball
 *
 * claygpparams-body.c
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

/*  this file is included by both
 *
 *  libclay/claygpparams.c
 *  app/plug-in/claygpparams.c
 */

static ClayImage   * get_image_by_id    (gpointer clay,
                                         gint     id);
static ClayItem    * get_item_by_id     (gpointer clay,
                                         gint     id);
static ClayDisplay * get_display_by_id  (gpointer clay,
                                         gint     id);
static GObject     * get_resource_by_id (gint     id);
static gint          get_resource_id    (GObject *resource);
static ClayUnit    * get_unit_by_id     (gpointer clay,
                                         gint     id);


GParamSpec *
_clay_gp_param_def_to_param_spec (const GPParamDef *param_def)
{
  const gchar *name  = param_def->name;
  const gchar *nick  = param_def->nick;
  const gchar *blurb = param_def->blurb;
  GParamFlags  flags = param_def->flags;

  flags &= ~G_PARAM_STATIC_STRINGS;

  switch (param_def->param_def_type)
    {
    case GP_PARAM_DEF_TYPE_DEFAULT:
      if (! strcmp (param_def->type_name, "ClayParamInt32Array"))
        return clay_param_spec_int32_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "ClayParamDoubleArray"))
        return clay_param_spec_double_array (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "ClayParamValueArray"))
        /* FIXME: ideally we should add a GP_PARAM_DEF_TYPE_VALUE_ARRAY
         * def type which should recursively contain another
         * GPParamDefType so that we'd recreate the spec as it was
         * initially created (limiting the array to specific types,
         * possibly further limited).
         * For now, we just create a value array which accepts
         * everything.
         */
        return clay_param_spec_value_array (name, nick, blurb,
                                            NULL, flags);

      if (! strcmp (param_def->type_name, "ClayParamParasite"))
        return clay_param_spec_parasite (name, nick, blurb, flags);

      if (! strcmp (param_def->type_name, "GParamParam"))
        return g_param_spec_param (name, nick, blurb,
                                   g_type_from_name (param_def->value_type_name),
                                   flags);

      if (! strcmp (param_def->type_name, "GParamObject") &&
          ! strcmp (param_def->value_type_name, "GFile"))
        return g_param_spec_object (name, nick, blurb, G_TYPE_FILE, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "GStrv"))
        return g_param_spec_boxed (name, nick, blurb, G_TYPE_STRV, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "GBytes"))
        return g_param_spec_boxed (name, nick, blurb, G_TYPE_BYTES, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "ClayColorArray"))
        return g_param_spec_boxed (name, nick, blurb, CLAY_TYPE_COLOR_ARRAY, flags);

      if (! strcmp (param_def->type_name, "GParamBoxed") &&
          ! strcmp (param_def->value_type_name, "ClayBablFormat"))
        return g_param_spec_boxed (name, nick, blurb, CLAY_TYPE_BABL_FORMAT, flags);

      break;

    case GP_PARAM_DEF_TYPE_CHOICE:
      if (! strcmp (param_def->type_name, "ClayParamChoice") ||
#ifdef LIBCLAY_COMPILATION
          /* Special case for GeglParamEnum which are passed from core
           * as a ClayChoice because the internal enum type could not
           * always be reconstructed (XXX some could, the ones created
           * as global GEGL enum types, but I could not find how to
           * differentiate them on the other side of the wire).
           */
          ! strcmp (param_def->type_name, "GeglParamEnum")
#else
          FALSE
#endif
          )
        {
          return clay_param_spec_choice (name, nick, blurb,
                                         g_object_ref (param_def->meta.m_choice.choice),
                                         param_def->meta.m_choice.default_val,
                                         flags);
        }

      break;

    case GP_PARAM_DEF_TYPE_INT:
      if (! strcmp (param_def->type_name, "GParamInt"))
        return g_param_spec_int (name, nick, blurb,
                                 param_def->meta.m_int.min_val,
                                 param_def->meta.m_int.max_val,
                                 param_def->meta.m_int.default_val,
                                 flags);

      if (! strcmp (param_def->type_name, "GParamUInt") ||
#ifdef LIBCLAY_COMPILATION
          ! strcmp (param_def->type_name, "GeglParamSeed")
#else
          FALSE
#endif
         )
        return g_param_spec_uint (name, nick, blurb,
                                  param_def->meta.m_int.min_val,
                                  param_def->meta.m_int.max_val,
                                  param_def->meta.m_int.default_val,
                                  flags);

      if (! strcmp (param_def->type_name, "GParamUChar"))
        return g_param_spec_uchar (name, nick, blurb,
                                   param_def->meta.m_int.min_val,
                                   param_def->meta.m_int.max_val,
                                   param_def->meta.m_int.default_val,
                                   flags);
      break;

    case GP_PARAM_DEF_TYPE_UNIT:
      if (! strcmp (param_def->type_name, "ClayParamUnit"))
        return clay_param_spec_unit (name, nick, blurb,
                                     param_def->meta.m_unit.allow_pixels,
                                     param_def->meta.m_unit.allow_percent,
                                     clay_unit_get_by_id (param_def->meta.m_unit.default_val),
                                     flags);
      break;

    case GP_PARAM_DEF_TYPE_ENUM:
      if (! strcmp (param_def->type_name, "GParamEnum"))
        return g_param_spec_enum (name, nick, blurb,
                                  g_type_from_name (param_def->value_type_name),
                                  param_def->meta.m_enum.default_val,
                                  flags);

      if (! strcmp (param_def->type_name, "ClayParamEnum"))
        /* FIXME ClayParamEnum */
        return g_param_spec_enum (name, nick, blurb,
                                  g_type_from_name (param_def->value_type_name),
                                  param_def->meta.m_enum.default_val,
                                  flags);
      break;

    case GP_PARAM_DEF_TYPE_BOOLEAN:
      if (! strcmp (param_def->type_name, "GParamBoolean"))
        return g_param_spec_boolean (name, nick, blurb,
                                     param_def->meta.m_boolean.default_val,
                                     flags);
      break;

    case GP_PARAM_DEF_TYPE_DOUBLE:
      if (! strcmp (param_def->type_name, "GParamDouble"))
        return g_param_spec_double (name, nick, blurb,
                                    param_def->meta.m_double.min_val,
                                    param_def->meta.m_double.max_val,
                                    param_def->meta.m_double.default_val,
                                    flags);
      break;

    case GP_PARAM_DEF_TYPE_STRING:
      if (! strcmp (param_def->type_name, "GParamString"))
        return g_param_spec_string (name, nick, blurb,
                                    param_def->meta.m_string.default_val,
                                    flags);
      break;

    case GP_PARAM_DEF_TYPE_GEGL_COLOR:
      if (! strcmp (param_def->type_name, "GeglParamColor") ||
          ! strcmp (param_def->type_name, "ClayParamColor"))
        {
          GeglColor *default_color = NULL;

          if (param_def->meta.m_gegl_color.default_val)
            {
              GPParamColor *default_val = param_def->meta.m_gegl_color.default_val;
              const Babl   *format      = NULL;
              const Babl   *space       = NULL;
              gint          bpp;

              default_color = gegl_color_new ("black");

              if (default_val->format.profile_data)
                {
                  ClayColorProfile *profile;

                  profile = clay_color_profile_new_from_icc_profile (default_val->format.profile_data,
                                                                     default_val->format.profile_size,
                                                                     NULL);
                  if (profile)
                    {
                      GError *error = NULL;

                      space = clay_color_profile_get_space (profile,
                                                            CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                            &error);

                      if (! space)
                        {
                          g_printerr ("%s: failed to create Babl space from profile: %s\n",
                                      G_STRFUNC, error->message);
                          g_clear_error (&error);
                        }
                      g_object_unref (profile);
                    }
                }

              format = babl_format_with_space (default_val->format.encoding, space);
              bpp    = babl_format_get_bytes_per_pixel (format);

              if (bpp != default_val->size)
                g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                            G_STRFUNC, default_val->format.encoding, bpp, default_val->size);
              else
                gegl_color_set_pixel (default_color, format, default_val->data);
            }

          return clay_param_spec_color (name, nick, blurb,
                                        param_def->meta.m_gegl_color.has_alpha,
                                        default_color, flags);
        }
      break;

    case GP_PARAM_DEF_TYPE_ID:
      if (! strcmp (param_def->type_name, "ClayParamDisplay"))
        return clay_param_spec_display (name, nick, blurb,
                                        param_def->meta.m_id.none_ok,
                                        flags);

      if (! strcmp (param_def->type_name, "ClayParamImage"))
        return clay_param_spec_image (name, nick, blurb,
                                      param_def->meta.m_id.none_ok,
                                      flags);

      if (! strcmp (param_def->type_name, "ClayParamItem"))
        return clay_param_spec_item (name, nick, blurb,
                                     param_def->meta.m_id.none_ok,
                                     flags);

      if (! strcmp (param_def->type_name, "ClayParamDrawable"))
        return clay_param_spec_drawable (name, nick, blurb,
                                         param_def->meta.m_id.none_ok,
                                         flags);

      if (! strcmp (param_def->type_name, "ClayParamLayer"))
        return clay_param_spec_layer (name, nick, blurb,
                                      param_def->meta.m_id.none_ok,
                                      flags);

      if (! strcmp (param_def->type_name, "ClayParamTextLayer"))
        return clay_param_spec_text_layer (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "ClayParamVectorLayer"))
        return clay_param_spec_vector_layer (name, nick, blurb,
                                             param_def->meta.m_id.none_ok,
                                             flags);

      if (! strcmp (param_def->type_name, "ClayParamLinkLayer"))
        return clay_param_spec_link_layer (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "ClayParamRasterizable"))
        return clay_param_spec_rasterizable (name, nick, blurb,
                                             param_def->meta.m_id.none_ok,
                                             flags);

      if (! strcmp (param_def->type_name, "ClayParamGroupLayer"))
        return clay_param_spec_group_layer (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "ClayParamChannel"))
        return clay_param_spec_channel (name, nick, blurb,
                                        param_def->meta.m_id.none_ok,
                                        flags);

      if (! strcmp (param_def->type_name, "ClayParamLayerMask"))
        return clay_param_spec_layer_mask (name, nick, blurb,
                                           param_def->meta.m_id.none_ok,
                                           flags);

      if (! strcmp (param_def->type_name, "ClayParamSelection"))
        return clay_param_spec_selection (name, nick, blurb,
                                          param_def->meta.m_id.none_ok,
                                          flags);

      if (! strcmp (param_def->type_name, "ClayParamPath"))
        return clay_param_spec_path (name, nick, blurb,
                                     param_def->meta.m_id.none_ok,
                                     flags);

      if (! strcmp (param_def->type_name, "ClayParamDrawableFilter"))
        return clay_param_spec_drawable_filter (name, nick, blurb,
                                                param_def->meta.m_id.none_ok,
                                                flags);

      if (! strcmp (param_def->type_name, "ClayParamGradient"))
        return clay_param_spec_gradient (name, nick, blurb,
                                         param_def->meta.m_id.none_ok,
                                         NULL, TRUE, flags);
      break;

    case GP_PARAM_DEF_TYPE_ID_ARRAY:
      if (! strcmp (param_def->type_name, "ClayParamCoreObjectArray"))
        return clay_param_spec_core_object_array (name, nick, blurb,
                                                  g_type_from_name (param_def->meta.m_id_array.type_name),
                                                  flags);
      break;

    case GP_PARAM_DEF_TYPE_EXPORT_OPTIONS:
      if (! strcmp (param_def->type_name, "ClayParamExportOptions"))
        {
          return clay_param_spec_export_options (name, nick, blurb, flags);
        }
      break;

    case GP_PARAM_DEF_TYPE_RESOURCE:
      if (g_type_from_name (param_def->type_name) != 0 &&
          g_type_is_a (g_type_from_name (param_def->type_name), CLAY_TYPE_PARAM_RESOURCE))
        return clay_param_spec_resource (name, nick, blurb, g_type_from_name (param_def->type_name),
                                         param_def->meta.m_resource.none_ok,
                                         param_def->meta.m_resource.default_to_context ?
                                         NULL : CLAY_RESOURCE (get_resource_by_id (param_def->meta.m_resource.default_resource_id)),
                                         param_def->meta.m_resource.default_to_context,
                                         flags);

      break;

    case GP_PARAM_DEF_TYPE_FILE:
      if (! strcmp (param_def->type_name, "ClayParamFile"))
        {
          GFile      *file = NULL;
          GParamSpec *pspec;

          if (param_def->meta.m_file.default_uri &&
              strlen (param_def->meta.m_file.default_uri) > 0)
            file  = g_file_new_for_uri (param_def->meta.m_file.default_uri);

          pspec = clay_param_spec_file (name, nick, blurb,
                                        (ClayFileChooserAction) param_def->meta.m_file.action,
                                        param_def->meta.m_file.none_ok,
                                        file, flags);
          g_clear_object (&file);
          return pspec;
        }
      break;

    case GP_PARAM_DEF_TYPE_CURVE:
      if (! strcmp (param_def->type_name, "ClayParamCurve"))
        return clay_param_spec_curve (name, nick, blurb,
                                      param_def->meta.m_id.none_ok,
                                      flags);
    }

  g_warning ("%s: GParamSpec type unsupported '%s'", G_STRFUNC,
             param_def->type_name);

  return NULL;
}

gboolean
_clay_param_spec_to_gp_param_def (GParamSpec *pspec,
                                  GPParamDef *param_def,
                                  gboolean    check_only)
{
  GType pspec_type = G_PARAM_SPEC_TYPE (pspec);
  GType value_type = G_PARAM_SPEC_VALUE_TYPE (pspec);
  gboolean success = TRUE;

  param_def->param_def_type  = GP_PARAM_DEF_TYPE_DEFAULT;
  param_def->type_name       = (gchar *) g_type_name (pspec_type);
  param_def->value_type_name = (gchar *) g_type_name (value_type);
  param_def->name            = (gchar *) g_param_spec_get_name (pspec);
  param_def->nick            = (gchar *) g_param_spec_get_nick (pspec);
  param_def->blurb           = (gchar *) g_param_spec_get_blurb (pspec);
  param_def->flags           = pspec->flags;

  if (pspec_type == G_TYPE_PARAM_INT ||
#ifdef LIBCLAY_COMPILATION
      FALSE
#else
      pspec_type == GEGL_TYPE_PARAM_INT
#endif
      )
    {
      GParamSpecInt *ispec = G_PARAM_SPEC_INT (pspec);

      if (! strcmp (param_def->type_name, "GeglParamInt"))
        param_def->type_name = "GParamInt";

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = ispec->minimum;
      param_def->meta.m_int.max_val     = ispec->maximum;
      param_def->meta.m_int.default_val = ispec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_UINT ||
#ifdef LIBCLAY_COMPILATION
           FALSE
#else
           pspec_type == GEGL_TYPE_PARAM_SEED
#endif
           )
    {
      GParamSpecUInt *uspec = G_PARAM_SPEC_UINT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = uspec->minimum;
      param_def->meta.m_int.max_val     = uspec->maximum;
      param_def->meta.m_int.default_val = uspec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_UCHAR)
    {
      GParamSpecUChar *uspec = G_PARAM_SPEC_UCHAR (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_INT;

      param_def->meta.m_int.min_val     = uspec->minimum;
      param_def->meta.m_int.max_val     = uspec->maximum;
      param_def->meta.m_int.default_val = uspec->default_value;
    }
  else if (pspec_type == CLAY_TYPE_PARAM_UNIT)
    {
      GObject *default_value;

      default_value = clay_param_spec_object_get_default (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_UNIT;

      param_def->meta.m_unit.allow_pixels  = clay_param_spec_unit_pixel_allowed (pspec);
      param_def->meta.m_unit.allow_percent = clay_param_spec_unit_percent_allowed (pspec);
      param_def->meta.m_unit.default_val   = clay_unit_get_id (CLAY_UNIT (default_value));
    }
#ifndef LIBCLAY_COMPILATION
  /* This trick is only for core side when it needs to send the param
   * spec of an enum argument of a GEGL Operation, because for all enum
   * types created with enum_start|end macros in GEGL code, we are not
   * able to recreate the enum type on the other side of the wire.
   * Callers for instance will typically use int values instead.
   * What we do instead is to reuse ClayChoice (which was exactly
   * created for such internal-only enums, though it was initially for
   * plug-ins only. In particular it means the value nicks will be used
   * to set such argument (as a string property).
   */
  else if (GEGL_IS_PARAM_SPEC_ENUM (pspec))
    {
      GParamSpecEnum    *espec  = G_PARAM_SPEC_ENUM (pspec);
      GeglParamSpecEnum *gespec = GEGL_PARAM_SPEC_ENUM (pspec);
      ClayChoice        *choice = clay_choice_new ();
      GEnumClass        *enum_class;
      GEnumValue        *value;

      param_def->param_def_type            = GP_PARAM_DEF_TYPE_CHOICE;
      param_def->meta.m_choice.default_val = NULL;

      enum_class = g_type_class_ref (value_type);
      for (value = enum_class->values;
           value->value_name;
           value++)
        {
          GSList *iter;

          if (value->value < enum_class->minimum || value->value > enum_class->maximum)
            continue;

          for (iter = gespec->excluded_values; iter; iter = iter->next)
            if (GPOINTER_TO_INT (iter->data) == value->value)
              break;

          if (iter != NULL)
            /* Excluded value. */
            continue;

          if (espec->default_value == value->value)
            param_def->meta.m_choice.default_val = (gchar *) value->value_nick;
          else if (param_def->meta.m_choice.default_val == NULL)
            /* Make double-sure defaults is set. */
            param_def->meta.m_choice.default_val = (gchar *) value->value_nick;

          clay_choice_add (choice, value->value_nick, value->value, value->value_name, NULL);
        }

      param_def->meta.m_choice.choice = choice;

      g_type_class_unref (enum_class);
    }
#endif
  else if (G_IS_PARAM_SPEC_ENUM (pspec))
    {
      GParamSpecEnum *espec = G_PARAM_SPEC_ENUM (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_ENUM;

      param_def->meta.m_enum.default_val = espec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_BOOLEAN)
    {
      GParamSpecBoolean *bspec = G_PARAM_SPEC_BOOLEAN (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_BOOLEAN;

      param_def->meta.m_boolean.default_val = bspec->default_value;
    }
  else if (pspec_type == G_TYPE_PARAM_DOUBLE ||
#ifdef LIBCLAY_COMPILATION
      FALSE
#else
      pspec_type == GEGL_TYPE_PARAM_DOUBLE
#endif
      )
    {
      GParamSpecDouble *dspec = G_PARAM_SPEC_DOUBLE (pspec);

      /* We only support passing various GEGL param types from app to
       * libclay so far. It's used to send GEGL operations' arguments
       * specifications.
       * We transform them into simpler GLib param types. Additional
       * features of GEGL params are mostly for UI usage.
       */
      if (! strcmp (param_def->type_name, "GeglParamDouble"))
        param_def->type_name = "GParamDouble";

      param_def->param_def_type = GP_PARAM_DEF_TYPE_DOUBLE;

      param_def->meta.m_double.min_val     = dspec->minimum;
      param_def->meta.m_double.max_val     = dspec->maximum;
      param_def->meta.m_double.default_val = dspec->default_value;
    }
  /* Must be before G_IS_PARAM_SPEC_STRING() because it's a parent. */
  else if (pspec_type == CLAY_TYPE_PARAM_CHOICE)
    {
      GParamSpecString *sspec = G_PARAM_SPEC_STRING (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_CHOICE;

      param_def->meta.m_choice.default_val = sspec->default_value;
      param_def->meta.m_choice.choice      = clay_param_spec_choice_get_choice (pspec);
    }
  else if (G_IS_PARAM_SPEC_STRING (pspec) &&
#ifdef LIBCLAY_COMPILATION
           pspec_type != GEGL_TYPE_PARAM_STRING
#else
           TRUE
#endif
          )
    {
      GParamSpecString *gsspec = G_PARAM_SPEC_STRING (pspec);

      if (! strcmp (param_def->type_name, "ClayParamString") ||
          ! strcmp (param_def->type_name, "GeglParamString") ||
          ! strcmp (param_def->type_name, "GeglParamFilePath"))
        param_def->type_name = "GParamString";

      param_def->param_def_type = GP_PARAM_DEF_TYPE_STRING;

      param_def->meta.m_string.default_val = gsspec->default_value;
    }
  else if (GEGL_IS_PARAM_SPEC_COLOR (pspec) ||
           CLAY_IS_PARAM_SPEC_COLOR (pspec) ||
           (pspec_type == G_TYPE_PARAM_OBJECT && value_type == GEGL_TYPE_COLOR))
    {
      GPParamColor *default_val = NULL;
      GeglColor    *default_color;

      param_def->param_def_type              = GP_PARAM_DEF_TYPE_GEGL_COLOR;
      param_def->meta.m_gegl_color.has_alpha = TRUE;

      if (GEGL_IS_PARAM_SPEC_COLOR (pspec))
        {
          default_color = gegl_param_spec_color_get_default (pspec);
        }
      else if (CLAY_IS_PARAM_SPEC_COLOR (pspec))
        {
          default_color = GEGL_COLOR (clay_param_spec_object_get_default (pspec));
          param_def->meta.m_gegl_color.has_alpha = clay_param_spec_color_has_alpha (pspec);
        }
      else
        {
          const GValue *value = g_param_spec_get_default_value (pspec);

          default_color = g_value_get_object (value);
          param_def->type_name = "GeglParamColor";
        }

      if (default_color != NULL)
        {
          const Babl *format;

          format      = gegl_color_get_format (default_color);
          default_val = g_new0 (GPParamColor, 1);

          default_val->size            = babl_format_get_bytes_per_pixel (format);
          default_val->format.encoding = (gchar *) g_strdup (babl_format_get_encoding (format));

          default_val->format.profile_data = NULL;
          default_val->format.profile_size = 0;
          if (babl_format_get_space (format) != babl_space ("sRGB"))
            {
              const char *icc;
              int         icc_length;

              icc = babl_space_get_icc (babl_format_get_space (format), &icc_length);

              if (icc_length > 0)
                {
                  default_val->format.profile_data = g_new0 (guint8, icc_length);
                  memcpy (default_val->format.profile_data, icc, icc_length);
                }
              default_val->format.profile_size = icc_length;
            }
        }
      param_def->meta.m_gegl_color.default_val = default_val;
    }
  else if (pspec_type == CLAY_TYPE_PARAM_IMAGE)
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = clay_param_spec_image_none_allowed (pspec);
    }
  else if (CLAY_IS_PARAM_SPEC_ITEM (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = clay_param_spec_item_none_allowed (pspec);
    }
  else if (CLAY_IS_PARAM_SPEC_DRAWABLE_FILTER (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = clay_param_spec_drawable_filter_none_allowed (pspec);
    }
  else if (pspec_type == CLAY_TYPE_PARAM_DISPLAY)
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID;

      param_def->meta.m_id.none_ok = clay_param_spec_display_none_allowed (pspec);
    }
  else if (CLAY_IS_PARAM_SPEC_RESOURCE (pspec))
    {
      GObject  *default_value = NULL;
      gboolean  default_to_context;

      param_def->param_def_type = GP_PARAM_DEF_TYPE_RESOURCE;

      if (clay_param_spec_object_has_default (pspec))
        default_value = clay_param_spec_object_get_default (pspec);

      param_def->meta.m_resource.none_ok = clay_param_spec_resource_none_allowed (pspec);
      default_to_context = clay_param_spec_resource_defaults_to_context (pspec);
      param_def->meta.m_resource.default_to_context = default_to_context;
      if (default_value != NULL && ! default_to_context)
        param_def->meta.m_resource.default_resource_id = get_resource_id (default_value);
      else
        param_def->meta.m_resource.default_resource_id = 0;
    }
  else if (pspec_type == CLAY_TYPE_PARAM_FILE)
    {
      ClayParamSpecObject *ospec = CLAY_PARAM_SPEC_OBJECT (pspec);

      param_def->param_def_type = GP_PARAM_DEF_TYPE_FILE;

      param_def->meta.m_file.action      = (gint32) clay_param_spec_file_get_action (pspec);
      param_def->meta.m_file.none_ok     = clay_param_spec_file_none_allowed (pspec);
      param_def->meta.m_file.default_uri =
        ospec->_default_value ?  g_file_get_uri (G_FILE (ospec->_default_value)) : NULL;
    }
  else if (CLAY_IS_PARAM_SPEC_CORE_OBJECT_ARRAY (pspec))
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_ID_ARRAY;

      param_def->meta.m_id_array.type_name =
        (gchar *) g_type_name (clay_param_spec_core_object_array_get_object_type (pspec));
    }
  else if (pspec_type == CLAY_TYPE_PARAM_EXPORT_OPTIONS)
    {
      param_def->param_def_type = GP_PARAM_DEF_TYPE_EXPORT_OPTIONS;
    }
  else if (CLAY_IS_PARAM_SPEC_CURVE (pspec) ||
           (pspec_type == G_TYPE_PARAM_OBJECT && value_type == CLAY_TYPE_CURVE))
    {
      param_def->param_def_type       = GP_PARAM_DEF_TYPE_CURVE;
      param_def->type_name            = "ClayParamCurve";
      param_def->meta.m_curve.none_ok = TRUE;
    }
  else if (pspec_type == G_TYPE_PARAM_OBJECT &&
           value_type != G_TYPE_FILE &&
           value_type != GEGL_TYPE_COLOR)
    {
      const gchar *type_name  = NULL;

      if (g_type_is_a (value_type, CLAY_TYPE_DISPLAY))
        {
          /* g_type_is_a() because the core has a ClayDisplay subclasses */
          type_name = "ClayParamDisplay";
        }
      else if (value_type == CLAY_TYPE_IMAGE)
        {
          type_name = "ClayParamImage";
        }
      else if (value_type == CLAY_TYPE_ITEM)
        {
          type_name = "ClayParamItem";
        }
      else if (value_type == CLAY_TYPE_DRAWABLE)
        {
          type_name = "ClayParamDrawable";
        }
      else if (value_type == CLAY_TYPE_LAYER)
        {
          type_name = "ClayParamLayer";
        }
      else if (value_type == CLAY_TYPE_TEXT_LAYER)
        {
          type_name = "ClayParamTextLayer";
        }
      else if (value_type == CLAY_TYPE_VECTOR_LAYER)
        {
          type_name = "ClayParamVectorLayer";
        }
      else if (value_type == CLAY_TYPE_LINK_LAYER)
        {
          type_name = "ClayParamLinkLayer";
        }
      else if (value_type == CLAY_TYPE_RASTERIZABLE)
        {
          type_name = "ClayParamRasterizable";
        }
      else if (value_type == CLAY_TYPE_GROUP_LAYER)
        {
          type_name = "ClayParamGroupLayer";
        }
      else if (value_type == CLAY_TYPE_CHANNEL)
        {
          type_name = "ClayParamChannel";
        }
      else if (value_type == CLAY_TYPE_LAYER_MASK)
        {
          type_name = "ClayParamLayerMask";
        }
      else if (value_type == CLAY_TYPE_SELECTION)
        {
          type_name = "ClayParamSelection";
        }
      else if (value_type == CLAY_TYPE_PATH)
        {
          type_name = "ClayParamPath";
        }
      else if (value_type == CLAY_TYPE_RESOURCE)
        {
          type_name = "ClayParamResource";
        }
      else if (value_type == CLAY_TYPE_BRUSH)
        {
          type_name = "ClayParamBrush";
        }
      else if (value_type == CLAY_TYPE_FONT)
        {
          type_name = "ClayParamFont";
        }
      else if (value_type == CLAY_TYPE_GRADIENT)
        {
          type_name = "ClayParamGradient";
        }
      else if (value_type == CLAY_TYPE_PALETTE)
        {
          type_name = "ClayParamPalette";
        }
      else if (value_type == CLAY_TYPE_PATTERN)
        {
          type_name = "ClayParamPattern";
        }

      if (type_name)
        {
          param_def->param_def_type    = GP_PARAM_DEF_TYPE_ID;
          param_def->type_name         = (gchar *) type_name;
          param_def->meta.m_id.none_ok = TRUE;
        }
      else
        {
          success = FALSE;
          if (! check_only)
            g_warning ("%s: GParamSpecObject for unsupported type '%s:%s'",
                       G_STRFUNC,
                       param_def->type_name, param_def->value_type_name);
        }
    }
  else
    {
      success = FALSE;
    }

  return success;
}

static ClayImage *
get_image_by_id (gpointer clay,
                 gint     id)
{
#ifdef LIBCLAY_COMPILATION
  return clay_image_get_by_id (id);
#else
  return clay_image_get_by_id (clay, id);
#endif
}

static ClayItem *
get_item_by_id (gpointer clay,
                gint     id)
{
#ifdef LIBCLAY_COMPILATION
  return clay_item_get_by_id (id);
#else
  return clay_item_get_by_id (clay, id);
#endif
}

static ClayDrawableFilter *
get_filter_by_id (gpointer clay,
                  gint     id)
{
#ifdef LIBCLAY_COMPILATION
  return clay_drawable_filter_get_by_id (id);
#else
  return clay_drawable_filter_get_by_id (clay, id);
#endif
}

static ClayDisplay *
get_display_by_id (gpointer clay,
                   gint     id)
{
#ifdef LIBCLAY_COMPILATION
  return clay_display_get_by_id (id);
#else
  return clay_display_get_by_id (clay, id);
#endif
}

static GObject *
get_resource_by_id (gint id)
{
#ifdef LIBCLAY_COMPILATION
  return (GObject *) clay_resource_get_by_id (id);
#else
  return (GObject *) clay_data_get_by_id (id);
#endif
}

static gint
get_resource_id (GObject *resource)
{
#ifdef LIBCLAY_COMPILATION
  return clay_resource_get_id (CLAY_RESOURCE (resource));
#else
  return clay_data_get_id (CLAY_DATA (resource));
#endif
}

static ClayUnit *
get_unit_by_id (gpointer clay,
                gint     id)
{
  return clay_unit_get_by_id (id);
}


/* Deserialize a gp_param (from the wire) to an instance of object or
 * primitive type.
 *
 * This is used on both the core and plugin (libclay) side,
 * each having its own class definitions for a same named class.
 * Thus this creates different objects, depending on which side it is.
 * See the conditionally compiled constructors/fetchers above.
 */
static void
clay_gp_param_to_value (gpointer        clay,
                        const GPParam  *param,
                        GType           type,
                        GParamSpec     *pspec,
                        GValue         *value)
{
  g_return_if_fail (param != NULL);
  g_return_if_fail (value != NULL);
  /* pspec is nullable. */

  /* See also changing of types by caller. */
  if (type == G_TYPE_NONE || type == G_TYPE_INVALID)
    {
      type = g_type_from_name (param->type_name);
      if (type == 0)
        {
          if (! strcmp (param->type_name, "ClayResource"))
            type = g_type_from_name ("ClayData");
          else if (! strcmp (param->type_name, "ClayData"))
            type = g_type_from_name ("ClayResource");
          else
            g_critical ("%s: type name %s is not registered", G_STRFUNC,
                        param->type_name);
        }
    }
  /* assert type is not G_TYPE_NONE and type is not G_TYPE_INVALID. */

  g_value_init (value, type);

  if (type == G_TYPE_INT)
    {
      g_value_set_int (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_UINT (value))
    {
      g_value_set_uint (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      g_value_set_enum (value, param->data.d_int);
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      g_value_set_boolean (value, param->data.d_int ? TRUE : FALSE);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      g_value_set_double (value, param->data.d_double);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      g_value_set_string (value, param->data.d_string);
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      g_value_set_boxed (value, param->data.d_strv);
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      g_value_set_boxed (value, param->data.d_bytes);
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_FILE))
    {
      g_value_take_object (value, (param->data.d_string ?
                                   g_file_new_for_uri (param->data.d_string) :
                                   NULL));
    }
  else if (CLAY_VALUE_HOLDS_BABL_FORMAT (value))
    {
      const Babl       *format = NULL;
      const Babl       *space  = NULL;
      const gchar      *encoding;
      ClayColorProfile *profile;

      encoding = param->data.d_format.encoding;
      profile  = clay_color_profile_new_from_icc_profile (param->data.d_format.profile_data,
                                                          param->data.d_format.profile_size,
                                                          NULL);

      if (profile)
        {
          GError *error = NULL;

          space = clay_color_profile_get_space (profile,
                                                CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);

          if (! space)
            {
              g_printerr ("%s: failed to create Babl space from profile: %s\n",
                          G_STRFUNC, error->message);
              g_clear_error (&error);
            }
          g_object_unref (profile);
        }

      format = babl_format_with_space (encoding, space);

      g_value_set_boxed (value, format);
    }
  else if (CLAY_VALUE_HOLDS_COLOR (value))
    {
      GeglColor        *color;
      const Babl       *format = NULL;
      const Babl       *space  = NULL;
      const gchar      *encoding;
      ClayColorProfile *profile;
      gint              bpp;

      encoding = param->data.d_gegl_color.format.encoding;
      profile = clay_color_profile_new_from_icc_profile (param->data.d_gegl_color.format.profile_data,
                                                         param->data.d_gegl_color.format.profile_size,
                                                         NULL);

      if (profile)
        {
          GError *error = NULL;

          space = clay_color_profile_get_space (profile,
                                                CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                &error);

          if (! space)
            {
              g_printerr ("%s: failed to create Babl space from profile: %s\n",
                          G_STRFUNC, error->message);
              g_clear_error (&error);
            }
          g_object_unref (profile);
        }

      format = babl_format_with_space (encoding, space);
      color  = gegl_color_new ("black");

      bpp = babl_format_get_bytes_per_pixel (format);
      if (bpp != param->data.d_gegl_color.size)
        g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                    G_STRFUNC, encoding, bpp, param->data.d_gegl_color.size);
      else
        gegl_color_set_pixel (color, format, param->data.d_gegl_color.data);

      g_value_take_object (value, color);
    }
  else if (CLAY_VALUE_HOLDS_PARASITE (value))
    {
      g_value_set_boxed (value, &param->data.d_parasite);
    }
  else if (CLAY_VALUE_HOLDS_INT32_ARRAY (value))
    {
      clay_value_set_int32_array (value,
                                  (gint32 *) param->data.d_array.data,
                                  param->data.d_array.size /
                                  sizeof (gint32));
    }
  else if (CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value))
    {
      clay_value_set_double_array (value,
                                   (const gdouble *)
                                   param->data.d_array.data,
                                   param->data.d_array.size /
                                   sizeof (gdouble));
    }
  else if (CLAY_VALUE_HOLDS_COLOR_ARRAY (value))
    {
      GeglColor **colors;

      colors = g_new0 (GeglColor *, param->data.d_color_array.size + 1);

      for (gint i = 0; i < param->data.d_color_array.size; i++)
        {
          GeglColor        *color;
          const Babl       *format  = NULL;
          const Babl       *space   = NULL;
          ClayColorProfile *profile = NULL;
          const gchar      *encoding;
          gint              bpp;

          encoding = param->data.d_color_array.colors[i].format.encoding;
          if (param->data.d_color_array.colors[i].format.profile_size > 0)
            profile = clay_color_profile_new_from_icc_profile (param->data.d_color_array.colors[i].format.profile_data,
                                                               param->data.d_color_array.colors[i].format.profile_size,
                                                               NULL);

          if (profile)
            {
              GError *error = NULL;

              space = clay_color_profile_get_space (profile,
                                                    CLAY_COLOR_RENDERING_INTENT_RELATIVE_COLORIMETRIC,
                                                    &error);

              if (! space)
                {
                  g_printerr ("%s: failed to create Babl space from profile: %s\n",
                              G_STRFUNC, error->message);
                  g_clear_error (&error);
                }
              g_object_unref (profile);
            }
          format = babl_format_with_space (encoding, space);
          color  = gegl_color_new ("black");

          bpp = babl_format_get_bytes_per_pixel (format);
          if (bpp != param->data.d_color_array.colors[i].size)
            g_printerr ("%s: encoding \"%s\" expects %d bpp but data size is %d bpp.\n",
                        G_STRFUNC, encoding, bpp, param->data.d_color_array.colors[i].size);
          else
            gegl_color_set_pixel (color, format, param->data.d_color_array.colors[i].data);

          colors[i] = color;
        }

      g_value_take_boxed (value, colors);
    }
  else if (CLAY_VALUE_HOLDS_CORE_OBJECT_ARRAY (value))
    {
      GType     object_type = G_TYPE_INVALID;
      GObject **objects;
      gint      i;

      if (param->data.d_id_array.type_name != NULL)
        {
          /* An empty array from a NULL has an arbitrary type_name set earlier. */
          object_type = g_type_from_name (param->data.d_id_array.type_name);
        }
      else if (pspec != NULL)
        {
          object_type = clay_param_spec_core_object_array_get_object_type (pspec);
        }

      if (param->data.d_id_array.size > 1 && ! g_type_is_a (object_type, G_TYPE_OBJECT))
        {
          g_warning ("%s: ClayCoreObjectArray of unknown type.", G_STRFUNC);
          return;
        }

      /* size might be zero. */

      objects = g_new0 (GObject *, param->data.d_id_array.size + 1);

      for (i = 0; i < param->data.d_id_array.size; i++)
        {
          gint id = param->data.d_id_array.data[i];

          if (object_type == CLAY_TYPE_IMAGE)
            {
              objects[i] = (GObject *) get_image_by_id (clay, id);
            }
          else if (g_type_is_a (object_type, CLAY_TYPE_ITEM))
            {
              objects[i] = (GObject *) get_item_by_id (clay, id);
            }
          else if (g_type_is_a (object_type, CLAY_TYPE_DRAWABLE_FILTER))
            {
              objects[i] = (GObject *) get_filter_by_id (clay, id);
            }
          else if (g_type_is_a (object_type, CLAY_TYPE_DISPLAY))
            {
              objects[i] = (GObject *) get_display_by_id (clay, id);
            }
          else if (g_type_is_a (object_type, CLAY_TYPE_RESOURCE))
            {
              objects[i] = (GObject *) get_resource_by_id (id);
            }
        }

      /* Even when size is zero, set gvalue to an empty ClayObjectArray object,
       * having a valid but possibly arbitrary type of its elements.
       */
      g_value_set_boxed (value, objects);
    }
  else if (CLAY_VALUE_HOLDS_IMAGE (value))
    {
      g_value_set_object (value, get_image_by_id (clay, param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_ITEM (value))
    {
      g_value_set_object (value, get_item_by_id (clay, param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_DRAWABLE_FILTER (value))
    {
      g_value_set_object (value, get_filter_by_id (clay, param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_DISPLAY (value))
    {
      g_value_set_object (value, get_display_by_id (clay, param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_RESOURCE (value) &&
           ! CLAY_VALUE_HOLDS_CURVE (value))
    {
      g_value_set_object (value, get_resource_by_id (param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_UNIT (value))
    {
      g_value_set_object (value, get_unit_by_id (clay, param->data.d_int));
    }
  else if (CLAY_VALUE_HOLDS_CURVE (value))
    {
      ClayCurve *curve;

      curve = g_object_new (CLAY_TYPE_CURVE, NULL);

      g_object_set (curve,
                    "curve-type", param->data.d_curve.curve_type,
                    NULL);

      if (param->data.d_curve.curve_type == CLAY_CURVE_SMOOTH)
        {
          for (gint j = 0; j < param->data.d_curve.n_points; j++)
            {
              clay_curve_add_point (curve, param->data.d_curve.points[j * 2],
                                    param->data.d_curve.points[(j * 2) + 1]);

              clay_curve_set_point_type (curve, j,
                                         param->data.d_curve.point_types[j]);
            }
        }
      else /* if (param->data.d_curve.curve_type == CLAY_CURVE_FREE) */
        {
          gint n_samples = param->data.d_curve.n_samples;

          clay_curve_set_n_samples (curve, n_samples);
          for (gint j = 0; j < n_samples; j++)
            clay_curve_set_sample (curve,
                                   (gdouble) j / (gdouble) (n_samples - 1),
                                   param->data.d_curve.samples[j]);
        }

      g_value_set_object (value, curve);

      g_object_unref (curve);
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_EXPORT_OPTIONS))
    {
      ClayExportOptions *options;

      /* Recreating the same options when it'll have settings. The
       * "capabilities" property doesn't need to be recreated. It is
       * managed by the ClayExportProcedure code.
       */
      options = g_object_new (CLAY_TYPE_EXPORT_OPTIONS, NULL);

      g_value_set_object (value, options);

      g_object_unref (options);
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      GParamSpec *pspec =
        _clay_gp_param_def_to_param_spec (&param->data.d_param_def);

      g_value_take_param (value, pspec);
    }
  else if (CLAY_VALUE_HOLDS_VALUE_ARRAY (value))
    {
      ClayValueArray *values;

      values = _clay_gp_params_to_value_array (clay, NULL, 0,
                                               param->data.d_value_array.values,
                                               param->data.d_value_array.n_values,
                                               FALSE);
      g_value_take_boxed (value, values);
    }
  else
    {
      g_warning ("%s: unsupported deserialization to GValue of type '%s'\n",
                 G_STRFUNC, param->type_name);
    }
}

ClayValueArray *
_clay_gp_params_to_value_array (gpointer        clay,
                                GParamSpec    **pspecs,
                                gint            n_pspecs,
                                const GPParam  *params,
                                gint            n_params,
                                gboolean        return_values)
{
  ClayValueArray *args;
  gint            i;

  g_return_val_if_fail ((params != NULL && n_params  > 0) ||
                        (params == NULL && n_params == 0), NULL);

  args = clay_value_array_new (n_params);

  for (i = 0; i < n_params; i++)
    {
      GParamSpec *pspec = NULL;
      GValue      value = G_VALUE_INIT;
      GType       type;

      /*  first get the GType from the passed GPParam  */
      type = g_type_from_name (params[i].type_name);

      /*  then try to try to be more specific by looking at the param
       *  spec (return values have one additional value (the status),
       *  skip that, it's not in the array of param specs)
       */
      if (i > 0 || ! return_values)
        {
          gint pspec_index = i;

          if (return_values)
            pspec_index--;

          /*  are there param specs left?  */
          if (pspec_index < n_pspecs)
            {
              GType pspec_type = G_PARAM_SPEC_VALUE_TYPE (pspecs[pspec_index]);

              pspec = pspecs[pspec_index];

              if (type != pspec_type)
                {
                  if (g_type_is_a (pspec_type, type))
                    {
                      /*  if the param spec's GType is more specific
                       *  than the one from the GPParam, use the param
                       *  spec's GType.
                       */
                      type = pspec_type;
                    }
                  else if (type == G_TYPE_INT)
                    {
                      if (g_type_is_a (pspec_type, G_TYPE_ENUM))
                        {
                          /*  if the param spec's type is enum, but an
                           *  int was passed, use the enum type.
                           */
                          type = pspec_type;
                        }
                      else if (g_type_is_a (pspec_type, G_TYPE_BOOLEAN))
                        {
                          /*  if the param spec's type is boolean, but
                           *  an int was passed, use the boolean
                           *  type.
                           */
                          type = pspec_type;
                        }
                    }
                }
            }
        }

      clay_gp_param_to_value (clay, &params[i], type, pspec, &value);

      clay_value_array_append (args, &value);
      g_value_unset (&value);
    }

  return args;
}

static void
clay_value_to_gp_param (const GValue *value,
                        GPParam      *param,
                        gboolean      full_copy)
{
  GType type;

  g_return_if_fail (value != NULL);
  g_return_if_fail (param != NULL);

  type = G_VALUE_TYPE (value);

  param->param_type = -1;

  if (full_copy)
    param->type_name = g_strdup (g_type_name (type));
  else
    param->type_name = (gchar *) g_type_name (type);

  if (type == G_TYPE_INT)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_int (value);
    }
  else if (type == G_TYPE_UINT)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_uint (value);
    }
  else if (type == G_TYPE_UCHAR)
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_uchar (value);
    }
  else if (G_VALUE_HOLDS_ENUM (value))
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_enum (value);
    }
  else if (G_VALUE_HOLDS_BOOLEAN (value))
    {
      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = g_value_get_boolean (value);
    }
  else if (G_VALUE_HOLDS_DOUBLE (value))
    {
      param->param_type = GP_PARAM_TYPE_DOUBLE;

      param->data.d_double = g_value_get_double (value);
    }
  else if (G_VALUE_HOLDS_STRING (value))
    {
      param->param_type = GP_PARAM_TYPE_STRING;

      if (full_copy)
        param->data.d_string = g_value_dup_string (value);
      else
        param->data.d_string = (gchar *) g_value_get_string (value);
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), G_TYPE_FILE))
    {
      GFile *file = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_FILE;

      param->data.d_string = file ? g_file_get_uri (file) : NULL;
    }
  else if (CLAY_VALUE_HOLDS_BABL_FORMAT (value))
    {
      const Babl *format;
      int         icc_length = 0;

      param->param_type = GP_PARAM_TYPE_BABL_FORMAT;

      format = g_value_get_boxed (value);

      /* TODO: For indexed colors, we'll convert to R'G'B'(A) u8 as that
       * is currently what we support. As indexed format support improves,
       * we'll want to make this space and encoding agnostic. */
      if (babl_format_is_palette (format))
        {
          const Babl *indexed_format = NULL;
          gint        bpp;

          g_warning ("%s: GValue of type '%s' holds an indexed format. "
                     "This is unsupported and replaced with \"R'G'B' u8\".",
                     G_STRFUNC, param->type_name);
          bpp = babl_format_get_bytes_per_pixel (format);
          if (bpp == 1)
            indexed_format = babl_format_with_space ("R'G'B' u8",
                                                     babl_format_get_space (format));
          else if (bpp == 2)
            indexed_format = babl_format_with_space ("R'G'B'A u8",
                                                     babl_format_get_space (format));

          /* TODO: This is to notify us in the future when indexed image can have more than
           * 256 colors - we'll need to update encoding support accordingly */
          g_return_if_fail (indexed_format != NULL);

          format = indexed_format;
        }

      param->data.d_format.encoding     = (gchar *) babl_format_get_encoding (format);
      param->data.d_format.profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                         &icc_length);
      param->data.d_format.profile_size = icc_length;
    }
  else if (CLAY_VALUE_HOLDS_COLOR (value))
    {
      GeglColor  *color;
      const Babl *format;
      int         icc_length = 0;

      param->param_type = GP_PARAM_TYPE_GEGL_COLOR;

      color  = g_value_get_object (value);
      format = gegl_color_get_format (color);

      /* TODO: For indexed colors, we'll convert to R'G'B'(A) u8 as that
       * is currently what we support. As indexed format support improves,
       * we'll want to make this space and encoding agnostic. */
      if (babl_format_is_palette (format))
        {
          const Babl *indexed_format = NULL;
          gint        bpp;

          bpp = babl_format_get_bytes_per_pixel (format);
          if (bpp == 1)
            indexed_format = babl_format_with_space ("R'G'B' u8",
                                                     babl_format_get_space (format));
          else if (bpp == 2)
            indexed_format = babl_format_with_space ("R'G'B'A u8",
                                                     babl_format_get_space (format));

          /* TODO: This is to notify us in the future when indexed image can have more than
           * 256 colors - we'll need to update encoding support accordingly */
          g_return_if_fail (indexed_format != NULL);

          format = indexed_format;
        }

      param->data.d_gegl_color.size = babl_format_get_bytes_per_pixel (format);
      gegl_color_get_pixel (color, format, &param->data.d_gegl_color.data);

      param->data.d_gegl_color.format.encoding     = (gchar *) babl_format_get_encoding (format);
      param->data.d_gegl_color.format.profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                                    &icc_length);
      param->data.d_gegl_color.format.profile_size = icc_length;
    }
  else if (CLAY_VALUE_HOLDS_PARASITE (value))
    {
      ClayParasite *parasite = (full_copy ?
                                g_value_dup_boxed (value) :
                                g_value_get_boxed (value));

      param->param_type = GP_PARAM_TYPE_PARASITE;

      if (parasite)
        {
          param->data.d_parasite.name  = parasite->name;
          param->data.d_parasite.flags = parasite->flags;
          param->data.d_parasite.size  = parasite->size;
          param->data.d_parasite.data  = parasite->data;

          if (full_copy)
            {
              parasite->name  = NULL;
              parasite->flags = 0;
              parasite->size  = 0;
              parasite->data  = NULL;

              clay_parasite_free (parasite);
            }
        }
      else
        {
          param->data.d_parasite.name  = NULL;
          param->data.d_parasite.flags = 0;
          param->data.d_parasite.size  = 0;
          param->data.d_parasite.data  = NULL;
        }
    }
  else if (CLAY_VALUE_HOLDS_INT32_ARRAY (value) ||
           CLAY_VALUE_HOLDS_DOUBLE_ARRAY (value))
    {
      ClayArray *array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_ARRAY;

      if (array)
        {
          param->data.d_array.size = array->length;

          if (full_copy)
            param->data.d_array.data = g_memdup2 (array->data,
                                                  array->length);
          else
            param->data.d_array.data = array->data;
        }
      else
        {
          param->data.d_array.size = 0;
          param->data.d_array.data = NULL;
        }
    }
  else if (CLAY_VALUE_HOLDS_COLOR_ARRAY (value))
    {
      GeglColor **colors = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_COLOR_ARRAY;

      if (colors != NULL)
        {
          param->data.d_color_array.size = clay_color_array_get_length (colors);
          param->data.d_color_array.colors = g_new0 (GPParamColor,
                                                     param->data.d_color_array.size);
          for (gint i = 0; i < param->data.d_color_array.size; i++)
            {
              const Babl *format;
              int         icc_length = 0;

              format = gegl_color_get_format (colors[i]);

              param->data.d_color_array.colors[i].size = babl_format_get_bytes_per_pixel (format);
              gegl_color_get_pixel (colors[i], format, &param->data.d_color_array.colors[i].data);

              param->data.d_color_array.colors[i].format.encoding = (gchar *) babl_format_get_encoding (format);

              if (babl_format_get_space (format) != babl_space ("sRGB"))
                param->data.d_color_array.colors[i].format.profile_data = (guint8 *) babl_space_get_icc (babl_format_get_space (format),
                                                                                                         &icc_length);
              param->data.d_gegl_color.format.profile_size = icc_length;
            }
        }
      else
        {
          param->data.d_color_array.size = 0;
          param->data.d_color_array.colors = NULL;
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_BYTES))
    {
      GBytes *bytes = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_BYTES;

      if (bytes != NULL)
        {
          if (full_copy)
            param->data.d_bytes = g_bytes_new (g_bytes_get_data (bytes, NULL),
                                               g_bytes_get_size (bytes));
          else
            param->data.d_bytes = g_bytes_new_static (g_bytes_get_data (bytes, NULL),
                                                      g_bytes_get_size (bytes));
        }
    }
  else if (G_VALUE_HOLDS (value, G_TYPE_STRV))
    {
      gchar **array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_STRV;

      if (full_copy)
        param->data.d_strv = g_strdupv (array);
      else
        param->data.d_strv = array;
    }
  else if (CLAY_VALUE_HOLDS_CORE_OBJECT_ARRAY (value))
    {
      GObject **array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_ID_ARRAY;
      param->data.d_id_array.type_name = NULL;

      if (array && array[0])
        {
          GType element_type = G_TYPE_NONE;
          gsize array_length;
          gint  i;

          for (i = 0; array[i] != NULL; i++)
            {
              if (element_type != G_TYPE_NONE)
                {
                  if (! g_type_is_a (G_TYPE_FROM_INSTANCE (array[i]), element_type))
                    {
                      g_warning ("%s: ClayCoreObjectArray with element type %s holds unsupported element of type '%s'", G_STRFUNC,
                                 g_type_name (element_type), g_type_name (G_TYPE_FROM_INSTANCE (array[i])));
                      break;
                    }
                  continue;
                }

              if (CLAY_IS_IMAGE (array[i]))
                {
                  element_type = CLAY_TYPE_IMAGE;
                }
              else if (CLAY_IS_ITEM (array[i]))
                {
                  element_type = CLAY_TYPE_ITEM;
                }
              else if (CLAY_IS_DRAWABLE_FILTER (array[i]))
                {
                  element_type = CLAY_TYPE_DRAWABLE_FILTER;
                }
              else if (CLAY_IS_DISPLAY (array[i]))
                {
                  element_type = CLAY_TYPE_DISPLAY;
                }
              else if (CLAY_IS_RESOURCE (array[i]))
                {
                  element_type = CLAY_TYPE_RESOURCE;
                }
              else
                {
                  g_warning ("%s: ClayCoreObjectArray holds unsupported type '%s'", G_STRFUNC,
                             g_type_name (G_TYPE_FROM_INSTANCE (array[i])));
                  break;
                }
            }
          array_length = i;
          param->data.d_id_array.size = array_length;

          if (array_length > 0)
            {
              if (full_copy)
                param->data.d_id_array.type_name = g_strdup (g_type_name (element_type));
              else
                param->data.d_id_array.type_name = (gchar *) g_type_name (element_type);

              /* must be free'd also for full_copy == FALSE */
              param->data.d_id_array.data = g_new (gint32, array_length);
            }

          for (i = 0; i < array_length; i++)
            {
              if (CLAY_IS_IMAGE (array[i]))
                {
                  param->data.d_id_array.data[i] =
                    clay_image_get_id (CLAY_IMAGE (array[i]));
                }
              else if (CLAY_IS_ITEM (array[i]))
                {
                  param->data.d_id_array.data[i] =
                    clay_item_get_id (CLAY_ITEM (array[i]));
                }
              else if (CLAY_IS_DRAWABLE_FILTER (array[i]))
                {
                  param->data.d_id_array.data[i] =
                    clay_drawable_filter_get_id (CLAY_DRAWABLE_FILTER (array[i]));
                }
              else if (CLAY_IS_DISPLAY (array[i]))
                {
                  param->data.d_id_array.data[i] =
                    clay_display_get_id (CLAY_DISPLAY (array[i]));
                }
              else if (CLAY_IS_RESOURCE (array[i]))
                {
                  param->data.d_id_array.data[i] = get_resource_id (array[i]);
                }
              else
                {
                  param->data.d_id_array.data[i] = -1;
                }
            }
        }
      else
        {
          /* GValue intended to hold an array of objects is NULL.
           * For convenience, we allow this, meaning empty.
           */
          param->data.d_id_array.size = 0;
          param->data.d_id_array.data = NULL;
        }
    }
  else if (CLAY_VALUE_HOLDS_IMAGE (value))
    {
      ClayImage *image = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = image ? clay_image_get_id (image) : -1;
    }
  else if (CLAY_VALUE_HOLDS_ITEM (value))
    {
      ClayItem *item = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = item ? clay_item_get_id (item) : -1;
    }
  else if (CLAY_VALUE_HOLDS_DRAWABLE_FILTER (value))
    {
      ClayDrawableFilter *filter = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = filter ? clay_drawable_filter_get_id (filter) : -1;
    }
  else if (CLAY_VALUE_HOLDS_DISPLAY (value))
    {
      ClayDisplay *display = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = display ? clay_display_get_id (display) : -1;
    }
  else if (CLAY_VALUE_HOLDS_RESOURCE (value) &&
           ! CLAY_VALUE_HOLDS_CURVE (value))
    {
      GObject *resource = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = resource ? get_resource_id (resource) : -1;
    }
  else if (CLAY_VALUE_HOLDS_UNIT (value))
    {
      ClayUnit *unit = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_INT;

      param->data.d_int = unit ? clay_unit_get_id (unit) : -1;
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_EXPORT_OPTIONS))
    {
      param->param_type = GP_PARAM_TYPE_EXPORT_OPTIONS;
    }
  else if (g_type_is_a (G_VALUE_TYPE (value), CLAY_TYPE_CURVE))
    {
      ClayCurve *curve = g_value_get_object (value);

      param->param_type = GP_PARAM_TYPE_CURVE;

      param->data.d_curve.curve_type = clay_curve_get_curve_type (curve);

      if (param->data.d_curve.curve_type == CLAY_CURVE_SMOOTH)
        {
          param->data.d_curve.n_points    = clay_curve_get_n_points (curve);
          param->data.d_curve.points      = g_new0 (gdouble,
                                                    2 * param->data.d_curve.n_points);
          param->data.d_curve.point_types = (guint32 *) g_new0 (ClayCurvePointType,
                                                                param->data.d_curve.n_points);

          for (gint j = 0; j < param->data.d_curve.n_points; j++)
            {
              gdouble x;
              gdouble y;

              clay_curve_get_point (curve, j, &x, &y);
              param->data.d_curve.points[j * 2]       = x;
              param->data.d_curve.points[(j * 2) + 1] = y;

              param->data.d_curve.point_types[j] =
                clay_curve_get_point_type (curve, j);
            }

          param->data.d_curve.n_samples = 0;
          param->data.d_curve.samples   = NULL;
        }
      else /* if (param->data.d_curve.curve_type == CLAY_CURVE_FREE) */
        {
          gint n_samples = clay_curve_get_n_samples (curve);

          param->data.d_curve.n_samples = n_samples;
          param->data.d_curve.samples   = g_new0 (gdouble, n_samples);

          for (gint j = 0; j < n_samples; j++)
            param->data.d_curve.samples[j] = clay_curve_get_sample (curve,
                                                                    (gdouble) j / (gdouble) (n_samples - 1));

          param->data.d_curve.n_points    = 0;
          param->data.d_curve.points      = NULL;
          param->data.d_curve.point_types = NULL;
        }
    }
  else if (G_VALUE_HOLDS_PARAM (value))
    {
      param->param_type = GP_PARAM_TYPE_PARAM_DEF;

      _clay_param_spec_to_gp_param_def (g_value_get_param (value),
                                        &param->data.d_param_def, FALSE);
    }
  else if (CLAY_VALUE_HOLDS_VALUE_ARRAY (value))
    {
      ClayValueArray *array = g_value_get_boxed (value);

      param->param_type = GP_PARAM_TYPE_VALUE_ARRAY;

      param->data.d_value_array.n_values = clay_value_array_length (array);
      param->data.d_value_array.values   = NULL;
      if (param->data.d_value_array.n_values > 0)
        param->data.d_value_array.values = _clay_value_array_to_gp_params (array, full_copy);
    }

  if (param->param_type == -1)
    g_warning ("%s: GValue holds unsupported type '%s'", G_STRFUNC, param->type_name);
}

GPParam *
_clay_value_array_to_gp_params (const ClayValueArray  *args,
                                gboolean               full_copy)
{
  GPParam *params;
  gint     length;
  gint     i;

  g_return_val_if_fail (args != NULL, NULL);

  length = clay_value_array_length (args);

  params = g_new0 (GPParam, length);

  for (i = 0; i < length; i++)
    {
      GValue *value = clay_value_array_index (args, i);

      clay_value_to_gp_param (value, &params[i], full_copy);
    }

  return params;
}

void
_clay_gp_params_free (GPParam  *params,
                      gint      n_params,
                      gboolean  full_copy)
{
  gint i;

  for (i = 0; i < n_params; i++)
    {
      if (full_copy)
        g_free (params[i].type_name);

      switch (params[i].param_type)
        {
        case GP_PARAM_TYPE_INT:
        case GP_PARAM_TYPE_DOUBLE:
          break;

        case GP_PARAM_TYPE_STRING:
          if (full_copy)
            g_free (params[i].data.d_string);
          break;

        case GP_PARAM_TYPE_FILE:
          /* always free the uri */
          g_free (params[i].data.d_string);
          break;

        case GP_PARAM_TYPE_BABL_FORMAT:
        case GP_PARAM_TYPE_GEGL_COLOR:
          break;

        case GP_PARAM_TYPE_COLOR_ARRAY:
          g_free (params[i].data.d_color_array.colors);
          break;

        case GP_PARAM_TYPE_ARRAY:
          if (full_copy)
            g_free (params[i].data.d_array.data);
          break;

        case GP_PARAM_TYPE_STRV:
          if (full_copy)
            g_strfreev (params[i].data.d_strv);
          break;

        case GP_PARAM_TYPE_BYTES:
          g_bytes_unref (params[i].data.d_bytes);
          break;

        case GP_PARAM_TYPE_ID_ARRAY:
          if (full_copy)
            g_free (params[i].data.d_id_array.type_name);

          /* always free the array */
          g_free (params[i].data.d_id_array.data);
          break;

        case GP_PARAM_TYPE_PARASITE:
          if (full_copy)
            {
              g_free (params[i].data.d_parasite.name);
              g_free (params[i].data.d_parasite.data);
            }
          break;

        case GP_PARAM_TYPE_EXPORT_OPTIONS:
          break;

        case GP_PARAM_TYPE_PARAM_DEF:
          if (params[i].data.d_param_def.param_def_type == GP_PARAM_DEF_TYPE_CHOICE &&
              g_strcmp0 (params[i].data.d_param_def.type_name, "GeglParamEnum") == 0)
            g_clear_object (&params[i].data.d_param_def.meta.m_choice.choice);
          break;

        case GP_PARAM_TYPE_VALUE_ARRAY:
          _clay_gp_params_free (params[i].data.d_value_array.values,
                                params[i].data.d_value_array.n_values,
                                full_copy);
          break;

        case GP_PARAM_TYPE_CURVE:
          if (full_copy)
            {
              g_free (params[i].data.d_curve.points);
              g_free (params[i].data.d_curve.point_types);
              g_free (params[i].data.d_curve.samples);
            }
          break;
        }
    }

  g_free (params);
}
