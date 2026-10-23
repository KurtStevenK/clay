/* xyz.kainzmayer.ClayDesign
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 *
 * action-search-dialog.c
 * Copyright (C) 2012-2013 Srihari Sriraman
 *                         Suhas V
 *                         Vidyashree K
 *                         Zeeshan Ali Ansari
 * Copyright (C) 2013-2015 Jehan <jehan at girinstud.io>
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

#include <gegl.h>
#include <gtk/gtk.h>

#include "libclaybase/claybase.h"

#include "dialogs-types.h"

#include "config/clayguiconfig.h"

#include "core/clay.h"

#include "widgets/clayaction.h"
#include "widgets/clayactiongroup.h"
#include "widgets/clayaction-history.h"
#include "widgets/claydialogfactory.h"
#include "widgets/claysearchpopup.h"

#include "action-search-dialog.h"

#include "clay-intl.h"

#define ACTION_SECTION_INACTIVE 7

static void         action_search_history_and_actions      (ClaySearchPopup   *popup,
                                                            const gchar       *keyword,
                                                            gpointer           data);
static gboolean     action_search_match_keyword            (ClayAction        *action,
                                                            const gchar*       keyword,
                                                            gint              *section,
                                                            Clay              *clay);


/* Public Functions */

GtkWidget *
action_search_dialog_create (Clay *clay)
{
  GtkWidget *dialog;

  dialog = clay_search_popup_new (clay,
                                  "clay-action-search-dialog",
                                  _("Search Actions"),
                                  action_search_history_and_actions,
                                  clay);
  return dialog;
}

/* Private Functions */

static void
action_search_history_and_actions (ClaySearchPopup *popup,
                                   const gchar     *keyword,
                                   gpointer         data)
{
  gchar **actions;
  GList  *list;
  GList  *history_actions = NULL;
  Clay   *clay;

  g_return_if_fail (CLAY_IS_CLAY (data));

  clay = Clay (data);

  if (g_strcmp0 (keyword, "") == 0)
    return;

  history_actions = clay_action_history_search (clay,
                                                action_search_match_keyword,
                                                keyword);

  /* 0. Top result: matching action in run history. */
  for (list = history_actions; list; list = g_list_next (list))
    clay_search_popup_add_result (popup, list->data,
                                  clay_action_is_sensitive (list->data, NULL) ? 0 : ACTION_SECTION_INACTIVE);

  /* 1. Then other matching actions. */
  actions = g_action_group_list_actions (G_ACTION_GROUP (clay->app));

  for (gint i = 0; actions[i] != NULL; i++)
    {
      GAction *action;
      gint     section;

      /* The action search dialog doesn't show any non-historized
       * actions, with a few exceptions. See the difference between
       * clay_action_history_is_blacklisted_action() and
       * clay_action_history_is_excluded_action().
       */
      if (clay_action_history_is_blacklisted_action (actions[i]))
        continue;

      action = g_action_map_lookup_action (G_ACTION_MAP (clay->app), actions[i]);

      g_return_if_fail (CLAY_IS_ACTION (action));

      if (! clay_action_is_visible (CLAY_ACTION (action)))
        continue;

      if (action_search_match_keyword (CLAY_ACTION (action), keyword, &section, clay))
        {
          GList *redundant;

          /* A matching action. Check if we have not already added
           * it as an history action.
           */
          for (redundant = history_actions; redundant; redundant = g_list_next (redundant))
            if (strcmp (clay_action_get_name (redundant->data), actions[i]) == 0)
              break;

          if (redundant == NULL)
            clay_search_popup_add_result (popup, CLAY_ACTION (action), section);
        }
    }

  g_strfreev (actions);

  g_list_free_full (history_actions, (GDestroyNotify) g_object_unref);
}

/**
 * action_search_match_keyword:
 * @action: a #ClayAction to be matched.
 * @keyword: free text keyword to match with @action.
 * @section: relative section telling "how well" @keyword matched
 *           @action. The smaller the @section, the better the match. In
 *           particular this value can be used in the call to
 *           clay_search_popup_add_result() to show best matches at the
 *           top of the list.
 * @clay: the #Clay object. This matters because we will tokenize
 *        keywords, labels and tooltip by language.
 *
 * This function will check if some freely typed text @keyword matches
 * @action's label or tooltip, using a few algorithms to determine the
 * best matches (order of words, start of match, and so on).
 * All text (the user-provided @keyword as well as @actions labels and
 * tooltips) are unicoded normalized, tokenized and case-folded before
 * being compared. Comparisons with ASCII alternatives are also
 * performed, providing even better matches, depending on the user
 * languages (accounting for variant orthography in natural languages).
 *
 * @section will be set to:
 * - 0 for any @action if @keyword is %NULL (match all).
 * - 1 for a full initialism.
 * - 4 for a partial initialism.
 * - 1 if key tokens are found in the same order in the label and match
 *   the start of the label.
 * - 2 if key tokens are found in the label order but don't match the
 *   start of the label.
 * - 3 if key tokens are found with a different order from label.
 * - 5 if @keyword matches the tooltip.
 * - 6  if @keyword is a mix-match on tooltip and label.
 * In the end, @section is incremented by %ACTION_SECTION_INACTIVE if
 * the action is non-sensitive.
 *
 * Returns: %TRUE is a match was successful (in which case, @section
 * will be set as well).
 */
static gboolean
action_search_match_keyword (ClayAction  *action,
                             const gchar *keyword,
                             gint        *section,
                             Clay        *clay)
{
  gboolean   matched = FALSE;
  gchar    **key_tokens;
  gchar    **label_tokens;
  gchar    **label_alternates = NULL;
  gchar    *tmp;

  if (keyword == NULL)
    {
      /* As a special exception, a NULL keyword means any action
       * matches.
       */
      if (section)
        *section = clay_action_is_sensitive (action, NULL) ? 0 : ACTION_SECTION_INACTIVE;

      return TRUE;
    }

  key_tokens   = g_str_tokenize_and_fold (keyword, clay->config->language, NULL);
  tmp          = clay_strip_uline (clay_action_get_label (action));
  label_tokens = g_str_tokenize_and_fold (tmp, clay->config->language, &label_alternates);
  g_free (tmp);

  /* Try to match the keyword as an initialism of the action's label.
   * For instance 'gb' will match 'Gaussian Blur...'
   */
  if (g_strv_length (key_tokens) == 1)
    {
      gchar **search_tokens[] = {label_tokens, label_alternates};
      gint    i;

      for (i = 0; i < G_N_ELEMENTS (search_tokens); i++)
        {
          const gchar  *key_token;
          gchar       **label_tokens;

          for (key_token = key_tokens[0], label_tokens = search_tokens[i];
               *key_token && *label_tokens;
               key_token = g_utf8_find_next_char (key_token, NULL), label_tokens++)
            {
              gunichar key_char   = g_utf8_get_char (key_token);
              gunichar label_char = g_utf8_get_char (*label_tokens);

              if (key_char != label_char)
                break;
            }

          if (! *key_token)
            {
              matched = TRUE;

              if (section)
                {
                  /* full match is better than a partial match */
                  *section = ! *label_tokens ? 1 : 4;
                }
              else
                {
                  break;
                }
            }
        }
    }

  if (! matched && g_strv_length (label_tokens) > 0)
    {
      gint     previous_matched = -1;
      gboolean match_start;
      gboolean match_ordered;
      gint     i;

      matched       = TRUE;
      match_start   = TRUE;
      match_ordered = TRUE;
      for (i = 0; key_tokens[i] != NULL; i++)
        {
          gint j;
          for (j = 0; label_tokens[j] != NULL; j++)
            {
              if (g_str_has_prefix (label_tokens[j], key_tokens[i]))
                {
                  goto one_matched;
                }
            }
          for (j = 0; label_alternates[j] != NULL; j++)
            {
              if (g_str_has_prefix (label_alternates[j], key_tokens[i]))
                {
                  goto one_matched;
                }
            }
          matched = FALSE;
one_matched:
          if (previous_matched > j)
            match_ordered = FALSE;
          previous_matched = j;

          if (i != j)
            match_start = FALSE;

          continue;
        }

      if (matched && section)
        {
          /* If the key is the label start, this is a nicer match.
           * Then if key tokens are found in the same order in the label.
           * Finally we show at the end if the key tokens are found with a different order. */
          *section = match_ordered ? (match_start ? 1 : 2) : 3;
        }
    }

  if (! matched && key_tokens[0] && g_utf8_strlen (key_tokens[0], -1) > 2 &&
      clay_action_get_tooltip (action) != NULL)
    {
      gchar    **tooltip_tokens;
      gchar    **tooltip_alternates = NULL;
      gboolean   mixed_match;
      gint       i;

      tooltip_tokens = g_str_tokenize_and_fold (clay_action_get_tooltip (action),
                                                clay->config->language, &tooltip_alternates);

      if (g_strv_length (tooltip_tokens) > 0)
        {
          matched     = TRUE;
          mixed_match = FALSE;

          for (i = 0; key_tokens[i] != NULL; i++)
            {
              gint j;
              for (j = 0; tooltip_tokens[j] != NULL; j++)
                {
                  if (g_str_has_prefix (tooltip_tokens[j], key_tokens[i]))
                    {
                      goto one_tooltip_matched;
                    }
                }
              for (j = 0; tooltip_alternates[j] != NULL; j++)
                {
                  if (g_str_has_prefix (tooltip_alternates[j], key_tokens[i]))
                    {
                      goto one_tooltip_matched;
                    }
                }
              for (j = 0; label_tokens[j] != NULL; j++)
                {
                  if (g_str_has_prefix (label_tokens[j], key_tokens[i]))
                    {
                      mixed_match = TRUE;
                      goto one_tooltip_matched;
                    }
                }
              for (j = 0; label_alternates[j] != NULL; j++)
                {
                  if (g_str_has_prefix (label_alternates[j], key_tokens[i]))
                    {
                      mixed_match = TRUE;
                      goto one_tooltip_matched;
                    }
                }
              matched = FALSE;
one_tooltip_matched:
              continue;
            }
          if (matched && section)
            {
              /* Matching the tooltip is section 5. We don't go looking
               * for start of string or token order for tooltip match.
               * But if the match is mixed on tooltip and label (there are
               * no match for *only* label or *only* tooltip), this is
               * section 6. */
              *section = mixed_match ? 6 : 5;
            }
        }
      g_strfreev (tooltip_tokens);
      g_strfreev (tooltip_alternates);
    }

  g_strfreev (key_tokens);
  g_strfreev (label_tokens);
  g_strfreev (label_alternates);

  if (matched && section && ! clay_action_is_sensitive (action, NULL))
    *section += ACTION_SECTION_INACTIVE;

  return matched;
}
