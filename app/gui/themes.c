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

#include "config.h"

#include <stdlib.h>

#include <gegl.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <windows.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

#include "libclaybase/claybase.h"
#include "libclayconfig/clayconfig.h"

#include "gui-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"

#include "display/clayimagewindow.h"

#include "widgets/claywidgets-utils.h"

#include "icon-themes.h"
#include "themes.h"

#include "clay-intl.h"


/*  local function prototypes  */

static void   themes_apply_theme                   (Clay                  *clay,
                                                    ClayGuiConfig         *config);
/* Extra colour schemes: a palette shared by every theme, laid over the
 * theme's own light or dark variant (installed in theme-schemes/).
 */
typedef struct
{
  ClayThemeScheme  scheme;
  const gchar     *palette;
  gboolean         dark;
} ThemesPalette;

static const ThemesPalette themes_palettes[] =
{
  { CLAY_THEME_BROWN,     "brown",      TRUE  },
  { CLAY_THEME_PARCHMENT, "parchment",  FALSE },
  { CLAY_THEME_CAPPUCCINO, "cappuccino", FALSE },
  { CLAY_THEME_ESPRESSO,  "espresso",   TRUE  },
  { CLAY_THEME_MIDNIGHT,  "midnight",   TRUE  },
  { CLAY_THEME_FOREST,    "forest",     TRUE  },
  { CLAY_THEME_ROSE,      "rose",       FALSE },
  { CLAY_THEME_SUNSET,    "sunset",     FALSE },
  { CLAY_THEME_VIOLET,    "violet",     TRUE  },
  { CLAY_THEME_NEON,      "neon",       TRUE  },
  { CLAY_THEME_MATRIX,    "matrix",     TRUE  },
  { CLAY_THEME_UBUNTU,    "ubuntu",     TRUE  },
  { CLAY_THEME_DRACULA,   "dracula",    TRUE  },
  { CLAY_THEME_NORD,      "nord",       TRUE  },
  { CLAY_THEME_SOLARIZED, "solarized",  FALSE },
  { CLAY_THEME_GRUVBOX,   "gruvbox",    TRUE  },
  { CLAY_THEME_OCEAN,     "ocean",      TRUE  },
  { CLAY_THEME_SAKURA,    "sakura",     FALSE },
  { CLAY_THEME_AMBER,     "amber",      TRUE  },
  { CLAY_THEME_SEPIA,     "sepia",      FALSE },
};

static const ThemesPalette * themes_get_palette (ClayThemeScheme      scheme);
static void   themes_list_themes_foreach           (gpointer               key,
                                                    gpointer               value,
                                                    gpointer               data);
static gint   themes_name_compare                  (const void            *p1,
                                                    const void            *p2);
static void   themes_theme_paths_notify            (ClayExtensionManager  *manager,
                                                    GParamSpec            *pspec,
                                                    Clay                  *clay);
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static void   themes_theme_settings_portal_changed (GDBusProxy            *proxy,
                                                    const gchar           *sender_name,
                                                    const gchar           *signal_name,
                                                    GVariant              *parameters,
                                                    Clay                  *clay);
#endif


/*  private variables  */

static GHashTable       *themes_hash            = NULL;
static GtkStyleProvider *themes_style_provider  = NULL;
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static GDBusProxy       *themes_settings_portal = NULL;
#endif


/*  public functions  */

void
themes_init (Clay *clay)
{
  ClayGuiConfig *config;
#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  GError        *error = NULL;
#endif

  g_return_if_fail (CLAY_IS_CLAY (clay));

  config = CLAY_GUI_CONFIG (clay->config);

  /* Check for theme extensions. */
  themes_theme_paths_notify (clay->extension_manager, NULL, clay);
  g_signal_connect (clay->extension_manager, "notify::theme-paths",
                    G_CALLBACK (themes_theme_paths_notify),
                    clay);

  themes_style_provider = GTK_STYLE_PROVIDER (gtk_css_provider_new ());

  /*  Use GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1 so theme files
   *  override default styles provided by widgets themselves.
   */
  gtk_style_context_add_provider_for_screen (gdk_screen_get_default (),
                                             themes_style_provider,
                                             GTK_STYLE_PROVIDER_PRIORITY_APPLICATION + 1);

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  themes_settings_portal =
    g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                   G_DBUS_PROXY_FLAGS_NONE, NULL,
                                   "org.freedesktop.portal.Desktop",
                                   "/org/freedesktop/portal/desktop",
                                   "org.freedesktop.portal.Settings",
                                   NULL, &error);

  if (error)
    {
      g_printerr ("Could not access portal: %s", error->message);
      g_clear_error (&error);
    }
  else
    {
      g_signal_connect (themes_settings_portal,
                        "g-signal::SettingChanged",
                        G_CALLBACK (themes_theme_settings_portal_changed),
                        clay);
    }
#endif

  g_signal_connect (config, "notify::theme",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);
  g_signal_connect_after (config, "notify::icon-theme",
                          G_CALLBACK (themes_theme_change_notify),
                          clay);
  g_signal_connect (config, "notify::theme-color-scheme",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);
  g_signal_connect (config, "notify::prefer-symbolic-icons",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);
  g_signal_connect (config, "notify::override-theme-icon-size",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);
  g_signal_connect (config, "notify::custom-icon-size",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);
  g_signal_connect (config, "notify::font-relative-size",
                    G_CALLBACK (themes_theme_change_notify),
                    clay);

  themes_theme_change_notify (config, NULL, clay);

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
  themes_set_title_bar (clay);
#endif
}

void
themes_exit (Clay *clay)
{
  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (themes_hash)
    {
      g_signal_handlers_disconnect_by_func (clay->config,
                                            themes_theme_change_notify,
                                            clay);

      g_hash_table_destroy (themes_hash);
      themes_hash = NULL;
    }

  g_clear_object (&themes_style_provider);

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  g_clear_object (&themes_settings_portal);
#endif
}

gboolean
themes_theme_is_system (Clay        *clay,
                        const gchar *theme_name)
{
  gboolean is_system = FALSE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (theme_name != NULL, FALSE);

  if (g_strcmp0 (theme_name, "System") == 0)
    {
      GFile *theme_dir;
      GFile *parent_dir;
      GFile *system_dir;

      theme_dir  = themes_get_theme_dir (clay, theme_name);
      parent_dir = g_file_get_parent (theme_dir);
      system_dir = clay_file_new_for_config_path ("${clay_data_dir}/themes", NULL);

      is_system = g_file_equal (system_dir, parent_dir);

      g_object_unref (system_dir);
      g_object_unref (parent_dir);
    }

  return is_system;
}

gboolean
themes_theme_is_default (Clay        *clay,
                         const gchar *theme_name)
{
  gboolean is_default = FALSE;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), FALSE);
  g_return_val_if_fail (theme_name != NULL, FALSE);

  if (g_strcmp0 (theme_name, CLAY_CONFIG_DEFAULT_THEME) == 0)
    {
      GFile *theme_dir;
      GFile *parent_dir;
      GFile *system_dir;

      theme_dir  = themes_get_theme_dir (clay, theme_name);
      parent_dir = g_file_get_parent (theme_dir);
      system_dir = clay_file_new_for_config_path ("${clay_data_dir}/themes", NULL);

      is_default = g_file_equal (system_dir, parent_dir);

      g_object_unref (system_dir);
      g_object_unref (parent_dir);
    }

  return is_default;
}

gchar **
themes_list_themes (Clay *clay,
                    gint *n_themes)
{

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (n_themes != NULL, NULL);

  *n_themes = g_hash_table_size (themes_hash);

  if (*n_themes > 0)
    {
      gchar **themes;
      gchar **index;

      themes = g_new0 (gchar *, *n_themes + 1);

      index = themes;

      g_hash_table_foreach (themes_hash, themes_list_themes_foreach, &index);

      qsort (themes, *n_themes, sizeof (gchar *), themes_name_compare);

      return themes;
    }

  return NULL;
}

GFile *
themes_get_theme_dir (Clay        *clay,
                      const gchar *theme_name)
{
  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);

  if (! theme_name)
    theme_name = CLAY_CONFIG_DEFAULT_THEME;

  return g_hash_table_lookup (themes_hash, theme_name);
}

GFile *
themes_get_theme_file (Clay        *clay,
                       const gchar *first_component,
                       ...)
{
  ClayGuiConfig *gui_config;
  GFile         *file;
  const gchar   *component;
  va_list        args;

  g_return_val_if_fail (CLAY_IS_CLAY (clay), NULL);
  g_return_val_if_fail (first_component != NULL, NULL);

  gui_config = CLAY_GUI_CONFIG (clay->config);

  file      = g_object_ref (themes_get_theme_dir (clay, gui_config->theme));
  component = first_component;

  va_start (args, first_component);

  do
    {
      GFile *tmp = g_file_get_child (file, component);
      g_object_unref (file);
      file = tmp;
    }
  while ((component = va_arg (args, gchar *)));

  va_end (args);

  if (! g_file_query_exists (file, NULL))
    {
      g_object_unref (file);

      file      = g_object_ref (themes_get_theme_dir (clay, NULL));
      component = first_component;

      va_start (args, first_component);

      do
        {
          GFile *tmp = g_file_get_child (file, component);
          g_object_unref (file);
          file = tmp;
        }
      while ((component = va_arg (args, gchar *)));

      va_end (args);
    }

  return file;
}

void
themes_theme_change_notify (ClayGuiConfig *config,
                            GParamSpec    *pspec,
                            Clay          *clay)
{
  GFile  *theme_css;
  GError *error = NULL;

  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-application-prefer-dark-theme",
                config->theme_scheme != CLAY_THEME_LIGHT,
                NULL);

  themes_apply_theme (clay, config);

  theme_css = clay_directory_file ("theme.css", NULL);

  if (clay->be_verbose)
    g_print ("Parsing '%s'\n",
             clay_file_get_utf8_name (theme_css));

  if (! gtk_css_provider_load_from_file (GTK_CSS_PROVIDER (themes_style_provider),
                                         theme_css, &error))
    {
      g_printerr ("%s: error parsing %s: %s\n", G_STRFUNC,
                  clay_file_get_utf8_name (theme_css), error->message);
      g_clear_error (&error);
    }

  g_object_unref (theme_css);

  gtk_style_context_reset_widgets (gdk_screen_get_default ());

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
  themes_set_title_bar (clay);
#endif
}


/*  private functions  */

static void
themes_apply_theme (Clay          *clay,
                    ClayGuiConfig *config)
{
  GFile           *theme_css;
  GOutputStream   *output;
  GError          *error = NULL;
  gboolean         prefer_dark_theme;
  ClayThemeScheme  color_scheme;
  const ThemesPalette *palette;

  g_return_if_fail (CLAY_IS_CLAY (clay));
  g_return_if_fail (CLAY_IS_GUI_CONFIG (config));

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
  if (themes_settings_portal && config->theme_scheme == CLAY_THEME_SYSTEM)
    {
      GVariant *tuple_variant;
      GVariant *variant;

      tuple_variant = g_dbus_proxy_call_sync (themes_settings_portal,
                                              "ReadOne",
                                              g_variant_new ("(ss)", "org.freedesktop.appearance", "color-scheme"),
                                              G_DBUS_CALL_FLAGS_NONE,
                                              G_MAXINT,
                                              NULL,
                                              &error);
      if (error)
        {
          g_clear_error (&error);

          tuple_variant = g_dbus_proxy_call_sync (themes_settings_portal,
                                                  "Read",
                                                  g_variant_new ("(ss)", "org.freedesktop.appearance", "color-scheme"),
                                                  G_DBUS_CALL_FLAGS_NONE,
                                                  G_MAXINT,
                                                  NULL,
                                                  &error);
          if (!error)
            {
              /*
               * Since the org.freedesktop.portal.Settings.Read method returns
               * two layers of variant, re-assign the tuple.
               * https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html#org-freedesktop-portal-settings-read
               */
              g_variant_get (tuple_variant, "(v)", &tuple_variant);

              g_variant_get (tuple_variant, "v", &variant);
            }
        }
      else
        {
          g_variant_get (tuple_variant, "(v)", &variant);
        }

      if (!error)
        {
          /*
           * 1 means "Prefer dark", see:
           * https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.Settings.html#description
           */
          prefer_dark_theme = (g_variant_get_uint32 (variant) == 1);

          /*
           * Note that normally it's a tri-state flag, with a
           * prefer-light and no-preference case too, except that it
           * looks like both in KDE and GNOME at least, they only set
           * prefer-dark or no-preference and for us, the latter should
           * also mean dark.
           * Therefore for this setting to actually mean something, we
           * are currently breaking the spec by having no-preference
           * mean prefer-light. This should be fixed if/when the main
           * desktops actually implement all 3 options some day.
           */
          color_scheme = prefer_dark_theme ? CLAY_THEME_DARK : CLAY_THEME_LIGHT;
        }
      else
        {
          g_printerr ("%s\n", error->message);
          g_clear_error (&error);

          color_scheme = (config->theme_scheme == CLAY_THEME_SYSTEM) ? CLAY_THEME_DARK : config->theme_scheme;
          prefer_dark_theme = (color_scheme == CLAY_THEME_DARK ||
                               color_scheme == CLAY_THEME_GRAY);
        }
    }
  else
#elif defined(G_OS_WIN32)
  if (config->theme_scheme == CLAY_THEME_SYSTEM)
    {
      prefer_dark_theme = clay_is_win32_system_theme_dark ();
      color_scheme = prefer_dark_theme ? CLAY_THEME_DARK : CLAY_THEME_LIGHT;
    }
  else
#elif defined(__APPLE__)
  if (config->theme_scheme == CLAY_THEME_SYSTEM)
    {
      prefer_dark_theme = themes_macos_is_dark_mode_active ();
      color_scheme = prefer_dark_theme ? CLAY_THEME_DARK : CLAY_THEME_LIGHT;
    }
  else
#endif
    {
      color_scheme = (config->theme_scheme == CLAY_THEME_SYSTEM) ? CLAY_THEME_DARK : config->theme_scheme;
      prefer_dark_theme = (color_scheme == CLAY_THEME_DARK ||
                           color_scheme == CLAY_THEME_GRAY);
    }

  palette = themes_get_palette (color_scheme);
  if (palette)
    {
      prefer_dark_theme = palette->dark;
      color_scheme      = palette->dark ? CLAY_THEME_DARK : CLAY_THEME_LIGHT;
    }

  g_object_set (gtk_settings_get_for_screen (gdk_screen_get_default ()),
                "gtk-application-prefer-dark-theme", prefer_dark_theme,
                NULL);

  theme_css = clay_directory_file ("theme.css", NULL);

  if (clay->be_verbose)
    g_print ("Writing '%s'\n", clay_file_get_utf8_name (theme_css));

  output = G_OUTPUT_STREAM (g_file_replace (theme_css,
                                            NULL, FALSE, G_FILE_CREATE_NONE,
                                            NULL, &error));
  if (! output)
    {
      clay_message_literal (clay, NULL, CLAY_MESSAGE_ERROR, error->message);
      g_clear_error (&error);
    }
  else
    {
      GFile  *theme_dir = themes_get_theme_dir (clay, config->theme);
      GFile  *css_user;
      GSList *css_files = NULL;
      GSList *iter;

      if (theme_dir)
        {
          GFile *file = NULL;
          GFile *fallback;
          GFile *light;
          GFile *gray;
          GFile *dark;

          fallback = g_file_get_child (theme_dir, "clay.css");
          if (! g_file_query_exists (fallback, NULL))
            g_clear_object (&fallback);

          light = g_file_get_child (theme_dir, "clay-light.css");
          if (! g_file_query_exists (light, NULL))
            g_clear_object (&light);

          gray  = g_file_get_child (theme_dir, "clay-gray.css");
          if (! g_file_query_exists (gray, NULL))
            g_clear_object (&gray);

          dark  = g_file_get_child (theme_dir, "clay-dark.css");
          if (! g_file_query_exists (dark, NULL))
            g_clear_object (&dark);

          switch (color_scheme)
            {
            case CLAY_THEME_LIGHT:
              if (light != NULL)
                file = g_object_ref (light);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (gray != NULL)
                file = g_object_ref (gray);
              else if (dark != NULL)
                file = g_object_ref (dark);
              break;
            case CLAY_THEME_GRAY:
              if (gray != NULL)
                file = g_object_ref (gray);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (dark != NULL)
                file = g_object_ref (dark);
              else if (light != NULL)
                file = g_object_ref (light);
              break;
            case CLAY_THEME_DARK:
              if (dark != NULL)
                file = g_object_ref (dark);
              else if (fallback != NULL)
                file = g_object_ref (fallback);
              else if (gray != NULL)
                file = g_object_ref (gray);
              else if (light != NULL)
                file = g_object_ref (light);
              break;
            case CLAY_THEME_SYSTEM:
            default:
              g_return_if_reached ();
            }

          if (file != NULL)
            css_files = g_slist_prepend (css_files, file);

          if (file != NULL && palette != NULL)
            {
              gchar *name = g_strconcat (palette->palette, ".css", NULL);

              css_files = g_slist_prepend (css_files,
                                           clay_data_directory_file ("theme-schemes",
                                                                     name, NULL));
              g_free (name);
            }

          if (file == NULL)
            {
              clay_message (clay, NULL, CLAY_MESSAGE_ERROR,
                            _("Invalid theme: directory '%s' contains neither "
                              "clay-dark.css, clay-gray.css, clay-light.css nor clay.css."),
                            clay_file_get_utf8_name (theme_dir));
            }

          g_clear_object (&fallback);
          g_clear_object (&light);
          g_clear_object (&gray);
          g_clear_object (&dark);
        }
      else
        {
          gchar *tmp;

          tmp = g_build_filename (clay_data_directory (),
                                  "themes", CLAY_CONFIG_DEFAULT_THEME, "clay.css",
                                  NULL);
          css_files = g_slist_prepend (css_files, g_file_new_for_path (tmp));
          g_free (tmp);

          switch (color_scheme)
            {
            case CLAY_THEME_LIGHT:
              tmp = g_build_filename (clay_data_directory (),
                                      "themes", CLAY_CONFIG_DEFAULT_THEME, "clay-light.css",
                                      NULL);
              break;
            case CLAY_THEME_GRAY:
              tmp = g_build_filename (clay_data_directory (),
                                      "themes", CLAY_CONFIG_DEFAULT_THEME, "clay-gray.css",
                                      NULL);
              break;
            case CLAY_THEME_DARK:
              tmp = g_build_filename (clay_data_directory (),
                                      "themes", CLAY_CONFIG_DEFAULT_THEME, "clay-dark.css",
                                      NULL);
              break;
            case CLAY_THEME_SYSTEM:
            default:
              g_return_if_reached ();
            }

          css_files = g_slist_prepend (css_files, g_file_new_for_path (tmp));
          g_free (tmp);
        }

      /* An icon theme may ship a clay.css, e.g. for animated icon colours */
      if (icon_themes_get_theme_dir (clay, config->icon_theme))
        css_files = g_slist_prepend (
          css_files,
          g_file_get_child (icon_themes_get_theme_dir (clay, config->icon_theme),
                            "clay.css"));

      css_files = g_slist_prepend (
        css_files, clay_sysconf_directory_file ("clay.css", NULL));

      css_user  = clay_directory_file ("clay.css", NULL);
      css_files = g_slist_prepend (
        css_files, css_user);

      css_files = g_slist_reverse (css_files);

      g_output_stream_printf (
        output, NULL, NULL, &error,
        "/* Clay theme.css\n"
        " *\n"
        " * This file is written on Clay startup and on every theme change.\n"
        " * It is NOT supposed to be edited manually. Edit your personal\n"
        " * clay.css file instead (%s).\n"
        " */\n"
        "\n",
        clay_file_get_utf8_name (css_user));

      for (iter = css_files; ! error && iter; iter = g_slist_next (iter))
        {
          GFile *file = iter->data;

          if (g_file_query_exists (file, NULL))
            {
              gchar *path;

              path = g_file_get_uri (file);

              g_output_stream_printf (
                output, NULL, NULL, &error,
                "@import url(\"%s\");\n",
                path);

              g_free (path);
            }
        }

      if (! error)
        {
          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n"
            "* { -gtk-icon-style: %s; }\n"
            "\n"
            "%s",
            icon_themes_current_prefer_symbolic (clay) ? "symbolic" : "regular",
            prefer_dark_theme ? "/* prefer-dark-theme */\n" : "");

        }

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
      /* when using the system theme (which is Adwaita on Windows/macOS), use
         the WinUI/Cocoa accent color instead of the hardcoded Adwaita blue */
      if (! error && ! strcmp (config->theme, "System"))
        {
          gchar *gtk_theme = NULL;
          gchar *accent    = NULL;

          g_object_get (gtk_settings_get_default (),
                        "gtk-theme-name", &gtk_theme, NULL);

          if (! strcmp (gtk_theme, "Adwaita") || ! strcmp (gtk_theme, "Adwaita-dark"))
            accent = themes_get_system_accent_color (prefer_dark_theme);

          if (accent)
            {
              g_output_stream_printf (
                output, NULL, NULL, &error,
                "\n"
                "@define-color system_accent %s;\n"
                "@define-color accent_color @system_accent;\n"
                "@define-color accent_bg_color @system_accent;\n"
                "@define-color theme_selected_bg_color @system_accent;\n"
                "@define-color theme_unfocused_selected_bg_color @system_accent;\n"
                "\n"
                "selection,\n"
                "*:selected, *:selected:focus,\n"
                "row:selected, flowboxchild:selected,\n"
                "treeview.view:selected, treeview.view:selected:focus,\n"
                "menuitem:hover, .menu menuitem:hover, .context-menu menuitem:hover {\n"
                "  background-color: @system_accent;\n"
                "  color: #ffffff;\n"
                "}\n"
                "entry:focus, spinbutton:focus:not(.vertical) {\n"
                "  border-color: @system_accent;\n"
                "  box-shadow: inset 0 0 0 1px @system_accent;\n"
                "}\n"
                "checkbutton check:checked, checkbutton check:indeterminate,\n"
                "radiobutton radio:checked, radiobutton radio:indeterminate {\n"
                "  background-image: none;\n"
                "  background-color: @system_accent;\n"
                "  border-color: shade(@system_accent, 0.8);\n"
                "}\n"
                "switch:checked {\n"
                "  background-color: @system_accent;\n"
                "  border-color: shade(@system_accent, 0.8);\n"
                "}\n"
                "switch:checked > slider {\n"
                "  border-color: shade(@system_accent, 0.8);\n"
                "}\n"
                "progressbar > trough > progress,\n"
                "levelbar block.filled {\n"
                "  background-color: @system_accent;\n"
                "}\n"
                "scale highlight,\n"
                "scale trough > progress {\n"
                "  background-color: @system_accent;\n"
                "  border-color: shade(@system_accent, 0.8);\n"
                "}\n"
                "button.suggested-action {\n"
                "  background-image: none;\n"
                "  background-color: @system_accent;\n"
                "  border-color: shade(@system_accent, 0.8);\n"
                "  color: #ffffff;\n"
                "}\n"
                "notebook > header > tabs > tab:checked {\n"
                "  box-shadow: inset 0 -4px @system_accent;\n"
                "}\n"
                "menubar > menuitem:hover {\n"
                "  box-shadow: inset 0 -3px @system_accent;\n"
                "}\n"
                "*:link, *:visited,\n"
                "button:link, button:visited,\n"
                "button:link > label, button:visited > label {\n"
                "  color: @system_accent;\n"
                "}\n"
                "*:link:hover, button:link > label:hover,\n"
                "*:link:active, button:link > label:active {\n"
                "  color: shade(@system_accent, 1.15);\n"
                "}\n",
                accent);
            }

          g_free (accent);
          g_free (gtk_theme);
        }
#endif

      if (! error && config->override_icon_size)
        {
          const gchar *tool_icon_size   = "large-toolbar";
          const gchar *tab_icon_size    = "small-toolbar";
          const gchar *button_icon_size = "small-toolbar";
          gint         pal_padding      = 4;
          gint         tab_padding      = 1;
          gint         sep_padding      = 1;

          switch (config->custom_icon_size)
            {
            case CLAY_ICON_SIZE_SMALL:
              tool_icon_size   = "small-toolbar";
              tab_icon_size    = "small-toolbar";
              button_icon_size = "small-toolbar";
              pal_padding      = 1;
              tab_padding      = 0;
              sep_padding      = 1;
              break;
            case CLAY_ICON_SIZE_MEDIUM:
              tool_icon_size   = "large-toolbar";
              tab_icon_size    = "small-toolbar";
              button_icon_size = "small-toolbar";
              pal_padding      = 4;
              tab_padding      = 1;
              sep_padding      = 1;
              break;
            case CLAY_ICON_SIZE_LARGE:
              tool_icon_size   = "dnd";
              tab_icon_size    = "large-toolbar";
              button_icon_size = "large-toolbar";
              pal_padding      = 5;
              tab_padding      = 5;
              sep_padding      = 2;
              break;
            case CLAY_ICON_SIZE_HUGE:
              tool_icon_size   = "dialog";
              tab_icon_size    = "dnd";
              button_icon_size = "dnd";
              pal_padding      = 5;
              tab_padding      = 8;
              sep_padding      = 4;
              break;
            }

          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n"
            "* { -ClayToolPalette-tool-icon-size: %s; }"
            "\n"
            "* { -ClayDock-tool-icon-size: %s; }"
            "\n"
            "* { -ClayViewableDialog-tool-icon-size: %s; }"
            "\n"
            "* { -ClayDockbook-tab-icon-size: %s; }"
            "\n"
            "* { -ClayColorNotebook-tab-icon-size: %s; }"
            "\n"
            "* { -ClayPrefsBox-tab-icon-size: %s; }"
            "\n"
            "* { -ClayEditor-button-icon-size: %s; }"
            "\n"
            "* { -ClayDisplayShell-button-icon-size: %s; }"
            "\n"
            "* { -ClayFgBgEditor-tool-icon-size: %s; }"
            "\n"
            "toolpalette button { padding: %dpx; }"
            "\n"
            "button, tab { padding: %dpx; }"
            "\n"
            "paned separator { padding: %dpx; }",
            tool_icon_size, tool_icon_size, tool_icon_size, tab_icon_size,
            tab_icon_size, tab_icon_size, button_icon_size, button_icon_size,
            tool_icon_size, pal_padding, tab_padding, sep_padding);
        }

      if (! error && config->font_relative_size != 1.0)
        {
          /* Intermediate buffer for locale-independent float to string
           * conversion. See issue #11048.
           */
          gchar font_size_string[G_ASCII_DTOSTR_BUF_SIZE];

          g_output_stream_printf (output, NULL, NULL, &error,
                                  "\n"
                                  "* { font-size: %srem; }",
                                  g_ascii_dtostr (font_size_string,
                                                  G_ASCII_DTOSTR_BUF_SIZE,
                                                  config->font_relative_size));
        }

      if (! error)
        {
          g_output_stream_printf (
            output, NULL, NULL, &error,
            "\n\n"
            "/* end of theme.css */\n");
        }

      if (error)
        {
          GCancellable *cancellable = g_cancellable_new ();

          clay_message (clay, NULL, CLAY_MESSAGE_ERROR,
                        _("Error writing '%s': %s"),
                        clay_file_get_utf8_name (theme_css), error->message);
          g_clear_error (&error);

          /* Cancel the overwrite initiated by g_file_replace(). */
          g_cancellable_cancel (cancellable);
          g_output_stream_close (output, cancellable, NULL);
          g_object_unref (cancellable);
        }
      else if (! g_output_stream_close (output, NULL, &error))
        {
          clay_message (clay, NULL, CLAY_MESSAGE_ERROR,
                        _("Error closing '%s': %s"),
                        clay_file_get_utf8_name (theme_css), error->message);
          g_clear_error (&error);
        }

      g_slist_free_full (css_files, g_object_unref);
      g_object_unref (output);
    }

  g_object_unref (theme_css);
}

static const ThemesPalette *
themes_get_palette (ClayThemeScheme scheme)
{
  gint i;

  for (i = 0; i < G_N_ELEMENTS (themes_palettes); i++)
    if (themes_palettes[i].scheme == scheme)
      return &themes_palettes[i];

  return NULL;
}

static void
themes_list_themes_foreach (gpointer key,
                            gpointer value,
                            gpointer data)
{
  gchar ***index = data;

  **index = g_strdup ((gchar *) key);

  (*index)++;
}

static gint
themes_name_compare (const void *p1,
                     const void *p2)
{
  return strcmp (* (char **) p1, * (char **) p2);
}

static void
themes_theme_paths_notify (ClayExtensionManager *manager,
                           GParamSpec           *pspec,
                           Clay                 *clay)
{
  ClayGuiConfig *config;

  g_return_if_fail (CLAY_IS_CLAY (clay));

  if (themes_hash)
    g_hash_table_remove_all (themes_hash);
  else
    themes_hash = g_hash_table_new_full (g_str_hash,
                                         g_str_equal,
                                         g_free,
                                         g_object_unref);

  config = CLAY_GUI_CONFIG (clay->config);
  if (config->theme_path)
    {
      GList *path;
      GList *list;

      g_object_get (clay->extension_manager,
                    "theme-paths", &path,
                    NULL);
      path = g_list_copy_deep (path, (GCopyFunc) g_object_ref, NULL);
      path = g_list_concat (path, clay_config_path_expand_to_files (config->theme_path, NULL));

      for (list = path; list; list = g_list_next (list))
        {
          GFile           *dir = list->data;
          GFileEnumerator *enumerator;

          enumerator =
            g_file_enumerate_children (dir,
                                       G_FILE_ATTRIBUTE_STANDARD_NAME ","
                                       G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN ","
                                       G_FILE_ATTRIBUTE_STANDARD_TYPE,
                                       G_FILE_QUERY_INFO_NONE,
                                       NULL, NULL);

          if (enumerator)
            {
              GFileInfo *info;

              while ((info = g_file_enumerator_next_file (enumerator,
                                                          NULL, NULL)))
                {
                  if (! g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_STANDARD_IS_HIDDEN) &&
                      g_file_info_get_attribute_uint32 (info, G_FILE_ATTRIBUTE_STANDARD_TYPE) == G_FILE_TYPE_DIRECTORY)
                    {
                      GFile       *file;
                      const gchar *name;
                      gchar       *basename;

                      file = g_file_enumerator_get_child (enumerator, info);
                      name = clay_file_get_utf8_name (file);

                      basename = g_path_get_basename (name);

                      if (g_hash_table_contains (themes_hash, basename))
                        {
                          g_printerr ("Skipping duplicate theme \"%s\": '%s'\n",
                                      basename, name);
                          g_free (basename);
                          g_object_unref (file);
                        }
                      else
                        {
                          if (clay->be_verbose)
                            g_print ("Adding theme '%s' (%s)\n",
                                     basename, name);

                          g_hash_table_insert (themes_hash, basename, file);
                        }
                    }

                  g_object_unref (info);
                }

              g_object_unref (enumerator);
            }
        }

      g_list_free_full (path, (GDestroyNotify) g_object_unref);
    }
}

#if defined(G_OS_UNIX) && ! defined(__APPLE__)
static void
themes_theme_settings_portal_changed (GDBusProxy  *proxy,
                                      const gchar *sender_name,
                                      const gchar *signal_name,
                                      GVariant    *parameters,
                                      Clay        *clay)
{
  const char *namespace;
  const char *name;
  GVariant   *value = NULL;

  if (g_strcmp0 (signal_name, "SettingChanged"))
    return;

  g_variant_get (parameters, "(&s&sv)", &namespace, &name, &value);

  if (g_strcmp0 (namespace, "org.freedesktop.appearance") == 0 &&
      g_strcmp0 (name, "color-scheme") == 0)
    {
      themes_theme_change_notify (CLAY_GUI_CONFIG (clay->config), NULL, clay);
    }

  g_variant_unref (value);
}
#endif

#if defined(G_OS_WIN32) || (defined(PLATFORM_OSX) && MAC_OS_X_VERSION_MIN_REQUIRED >= 101400)
void
themes_set_title_bar (Clay *clay)
{
  GList *windows = clay_get_image_windows (clay);
  GList *iter;

  for (iter = windows; iter; iter = g_list_next (iter))
    {
      GtkWidget *window = GTK_WIDGET (windows->data);

      clay_window_set_title_bar_theme (clay, window);
    }

  if (windows)
    g_list_free (windows);
}
#endif
