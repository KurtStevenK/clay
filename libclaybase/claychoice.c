/* LIBCLAY - The Clay Library
 * Copyright (C) 1995-2000 Peter Mattis and Spencer Kimball
 *
 * claychoice.c
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

#include "config.h"

#include <gegl.h>
#include <gio/gio.h>
#include <glib-object.h>

#include "claybasetypes.h"

#include "claychoice.h"
#include "clayparamspecs.h"


typedef struct _ClayChoiceDesc
{
  gchar    *label;
  gchar    *help;
  gint      id;
  gboolean  sensitive;

  gboolean  deprecated;
  gchar    *redirect_to;
  gchar    *reason;

  GList    *aliases;
} ClayChoiceDesc;

enum
{
  SENSITIVITY_CHANGED,
  LAST_SIGNAL
};

struct _ClayChoice
{
  GObject     parent_instance;

  GHashTable *choices;
  GList      *keys;
};


static void clay_choice_finalize     (GObject        *object);

static void clay_choice_desc_free    (ClayChoiceDesc *desc);


G_DEFINE_TYPE (ClayChoice, clay_choice, G_TYPE_OBJECT)

#define parent_class clay_choice_parent_class

static guint clay_choice_signals[LAST_SIGNAL] = { 0 };

static void
clay_choice_class_init (ClayChoiceClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  clay_choice_signals[SENSITIVITY_CHANGED] =
    g_signal_new ("sensitivity-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0, /*G_STRUCT_OFFSET (ClayChoiceClass, sensitivity_changed),*/
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  G_TYPE_STRING);

  object_class->finalize = clay_choice_finalize;
}

static void
clay_choice_init (ClayChoice *choice)
{
  choice->choices =
    g_hash_table_new_full (g_str_hash, g_str_equal, g_free,
                           (GDestroyNotify) clay_choice_desc_free);
}

static void
clay_choice_finalize (GObject *object)
{
  ClayChoice *choice = CLAY_CHOICE (object);

  g_hash_table_unref (choice->choices);
  g_list_free_full (choice->keys, (GDestroyNotify) g_free);
}

/* Public API */

/**
 * clay_choice_new:
 *
 * Returns: (transfer full): a #ClayChoice.
 *
 * Since: 3.0
 **/
ClayChoice *
clay_choice_new (void)
{
  ClayChoice *choice;

  choice = g_object_new (CLAY_TYPE_CHOICE, NULL);

  return choice;
}

/**
 * clay_choice_new_with_values:
 * @nick:  the first value.
 * @id:    integer ID for @nick.
 * @label: the label of @nick.
 * @help:  longer help text for @nick.
 * ...:    more triplets of string to pre-fill the created %ClayChoice.
 *
 * Returns: (transfer full): a #ClayChoice.
 *
 * Since: 3.0
 **/
ClayChoice *
clay_choice_new_with_values (const gchar *nick,
                             gint         id,
                             const gchar *label,
                             const gchar *help,
                             ...)
{
  ClayChoice *choice;
  va_list     va_args;

  g_return_val_if_fail (nick != NULL, NULL);
  g_return_val_if_fail (label != NULL, NULL);

  choice = clay_choice_new ();

  va_start (va_args, help);

  do
    {
      clay_choice_add (choice, nick, id, label, help);
      nick  = va_arg (va_args, const gchar *);
      if (nick == NULL)
        break;
      id    = va_arg (va_args, gint);
      label = va_arg (va_args, const gchar *);
      if (label == NULL)
        {
          g_critical ("%s: nick '%s' cannot have a NULL label.", G_STRFUNC, nick);
          break;
        }
      help  = va_arg (va_args, const gchar *);
    }
  while (TRUE);

  va_end (va_args);

  return choice;
}

/**
 * clay_choice_add:
 * @choice: the %ClayChoice.
 * @nick:   the nick of @choice.
 * @id:     optional integer ID for @nick.
 * @label:  the label of @choice.
 * @help:   optional longer help text for @nick.
 *
 * This procedure adds a new possible value to @choice list of values.
 * The @id is an optional integer identifier. This can be useful for instance
 * when you want to work with different enum values mapped to each @nick.
 *
 * Since: 3.0
 **/
void
clay_choice_add (ClayChoice  *choice,
                 const gchar *nick,
                 gint         id,
                 const gchar *label,
                 const gchar *help)
{
  ClayChoiceDesc *desc;
  GList          *duplicate;

  g_return_if_fail (label != NULL);

  desc              = g_new0 (ClayChoiceDesc, 1);
  desc->id          = id;
  desc->label       = g_strdup (label);
  desc->help        = help != NULL ? g_strdup (help) : NULL;
  desc->sensitive   = TRUE;
  desc->deprecated  = FALSE;
  desc->redirect_to = NULL;
  desc->reason      = NULL;
  desc->aliases     = NULL;
  g_hash_table_insert (choice->choices, g_strdup (nick), desc);

  duplicate = g_list_find_custom (choice->keys, nick, (GCompareFunc) g_strcmp0);
  if (duplicate != NULL)
    {
      choice->keys = g_list_remove_link (choice->keys, duplicate);
      g_free (duplicate->data);
      g_list_free (duplicate);
    }
  choice->keys = g_list_append (choice->keys, g_strdup (nick));
}

/**
 * clay_choice_add_deprecated:
 * @choice:      the %ClayChoice.
 * @nick:        the nick of the deprecated @choice.
 * @id:          optional integer ID for @nick.
 * @redirect_to: (nullable): the valid nick to redirect the deprecated one to
 * @reason: (nullable): the deprecation reason.
 *
 * This procedure is used to add a deprecated nick. It must either have
 * a valid @redirect_to alias (when you are basically renaming a choice)
 * or a free-text @reason (e.g. for planning choice removal through a
 * deprecation period).
 *
 * If @redirect_to is non %NULL, then @id must be identical to the ID
 * of @redirect_to. The @redirect_to value must not be deprecated itself.
 *
 * Since: 3.4
 **/
void
clay_choice_add_deprecated (ClayChoice  *choice,
                            const gchar *nick,
                            gint         id,
                            const gchar *redirect_to,
                            const gchar *reason)
{
  ClayChoiceDesc *deprecated_desc;
  ClayChoiceDesc *redirect_desc = NULL;
  GList          *duplicate;

  g_return_if_fail (nick != NULL);
  g_return_if_fail (redirect_to == NULL || g_strcmp0 (nick, redirect_to) != 0);
  g_return_if_fail ((redirect_to == NULL && reason != NULL) ||
                    (reason == NULL && redirect_to != NULL && g_strcmp0 (nick, redirect_to) != 0));

  if (redirect_to)
    {
      redirect_desc = g_hash_table_lookup (choice->choices, redirect_to);
      g_return_if_fail (redirect_desc != NULL       &&
                        ! redirect_desc->deprecated &&
                        redirect_desc->id == id);
      redirect_desc->aliases = g_list_prepend (redirect_desc->aliases, g_strdup (nick));
    }

  deprecated_desc              = g_new0 (ClayChoiceDesc, 1);
  deprecated_desc->id          = redirect_desc ? redirect_desc->id : id;
  deprecated_desc->label       = NULL;
  deprecated_desc->help        = NULL;
  deprecated_desc->sensitive   = redirect_desc ? redirect_desc->sensitive : TRUE;
  deprecated_desc->deprecated  = TRUE;
  deprecated_desc->redirect_to = g_strdup (redirect_to);
  deprecated_desc->reason      = g_strdup (reason);
  deprecated_desc->aliases     = NULL;

  g_hash_table_insert (choice->choices, g_strdup (nick), deprecated_desc);

  duplicate = g_list_find_custom (choice->keys, nick, (GCompareFunc) g_strcmp0);
  if (duplicate != NULL)
    {
      choice->keys = g_list_remove_link (choice->keys, duplicate);
      g_free (duplicate->data);
      g_list_free (duplicate);
    }
  choice->keys = g_list_append (choice->keys, g_strdup (nick));
}

/**
 * clay_choice_is_valid:
 * @choice: a %ClayChoice.
 * @nick:   the nick to check.
 *
 * This procedure checks if the given @nick is valid and refers to
 * an existing and sensitive choice.
 *
 * Returns: Whether the choice is valid.
 *
 * Since: 3.0
 **/
gboolean
clay_choice_is_valid (ClayChoice  *choice,
                      const gchar *nick)
{
  ClayChoiceDesc *desc;

  g_return_val_if_fail (CLAY_IS_CHOICE (choice), FALSE);
  g_return_val_if_fail (nick != NULL, FALSE);

  desc = g_hash_table_lookup (choice->choices, nick);
  return (desc != NULL && desc->sensitive);
}

/**
 * clay_choice_list_nicks:
 * @choice: a %ClayChoice.
 *
 * This procedure returns the list of nicks allowed for @choice.
 *
 * Returns: (element-type gchar*) (transfer none): The list of @choice's nicks.
 *
 * Since: 3.0
 **/
GList *
clay_choice_list_nicks (ClayChoice *choice)
{
  /* I don't use g_hash_table_get_keys() on purpose, because I want to retain
   * the adding-time order.
   */
  return choice->keys;
}

/**
 * clay_choice_get_id:
 * @choice: a %ClayChoice.
 * @nick:   the nick to lookup.
 *
 * Returns: the ID of @nick.
 *
 * Since: 3.0
 **/
gint
clay_choice_get_id (ClayChoice  *choice,
                    const gchar *nick)
{
  ClayChoiceDesc *desc;

  g_return_val_if_fail (CLAY_IS_CHOICE (choice), 0);
  g_return_val_if_fail (nick != NULL, 0);

  desc = g_hash_table_lookup (choice->choices, nick);
  g_return_val_if_fail (desc != NULL, 0);

  return desc->id;
}

/**
 * clay_choice_get_label:
 * @choice: a %ClayChoice.
 * @nick:   the nick to lookup.
 *
 * Returns: (transfer none): the label of @nick.
 *
 * Since: 3.0
 **/
const gchar *
clay_choice_get_label (ClayChoice  *choice,
                       const gchar *nick)
{
  ClayChoiceDesc *desc;

  g_return_val_if_fail (CLAY_IS_CHOICE (choice), NULL);
  g_return_val_if_fail (nick != NULL, NULL);

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc && ! desc->deprecated)
    return desc->label;
  else
    return NULL;
}

/**
 * clay_choice_get_help:
 * @choice: a %ClayChoice.
 * @nick:   the nick to lookup.
 *
 * Returns the longer documentation for @nick.
 *
 * Returns: (transfer none): the help text of @nick.
 *
 * Since: 3.0
 **/
const gchar *
clay_choice_get_help (ClayChoice  *choice,
                      const gchar *nick)
{
  ClayChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    return desc->help;
  else
    return NULL;
}

/**
 * clay_choice_get_documentation:
 * @choice: the %ClayChoice.
 * @nick:   the possible value's nick you need documentation for.
 * @label: (transfer none): the label of @nick.
 * @help:  (transfer none): the help text of @nick.
 *
 * Returns the documentation strings for @nick.
 *
 * Returns: %TRUE if @nick is found, %FALSE otherwise.
 *
 * Since: 3.0
 **/
gboolean
clay_choice_get_documentation (ClayChoice   *choice,
                               const gchar  *nick,
                               const gchar **label,
                               const gchar **help)
{
  ClayChoiceDesc *desc;

  desc = g_hash_table_lookup (choice->choices, nick);
  if (desc)
    {
      *label = desc->label;
      *help  = desc->help;
      return TRUE;
    }

  return FALSE;
}

/**
 * clay_choice_is_deprecated:
 * @choice: a %ClayChoice.
 * @nick:   the nick to lookup.
 * @redirect_to: (out) (nullable) (transfer none): the non-deprecated alias, if any.
 * @reason: (out) (nullable) (transfer none): the deprecation reason if @redirect_to is %NULL.
 *
 * Lookup whether @nick is a deprecated choice or not. If it is
 * deprecated, either @redirect_to will be set or @reason will be,
 * depending on whether @nick was simply renamed or if the deprecation
 * requires a more complex human-readable reason.
 *
 * Returns: %TRUE if @nick is deprecated.
 *
 * Since: 3.4
 **/
gboolean
clay_choice_is_deprecated (ClayChoice   *choice,
                           const gchar  *nick,
                           const gchar **redirect_to,
                           const gchar **reason)
{
  ClayChoiceDesc *desc;

  g_return_val_if_fail (CLAY_IS_CHOICE (choice), FALSE);
  g_return_val_if_fail (nick != NULL, FALSE);

  desc = g_hash_table_lookup (choice->choices, nick);
  g_return_val_if_fail (desc != NULL, FALSE);

  if (redirect_to)
    *redirect_to = desc->redirect_to;
  if (reason)
    *reason = desc->reason;

  return desc->deprecated;
}

/**
 * clay_choice_set_sensitive:
 * @choice: the %ClayChoice.
 * @nick:   the nick to lookup.
 *
 * Change the sensitivity of a possible @nick. Technically a non-sensitive @nick
 * means it cannot be chosen anymore (so [method@Clay.Choice.is_valid] will
 * return %FALSE; nevertheless [method@Clay.Choice.list_nicks] and other
 * functions to get information about a choice will still function).
 *
 * Since: 3.0
 **/
void
clay_choice_set_sensitive (ClayChoice  *choice,
                           const gchar *nick,
                           gboolean     sensitive)
{
  ClayChoiceDesc *desc;

  g_return_if_fail (CLAY_IS_CHOICE (choice));
  g_return_if_fail (nick != NULL);

  desc = g_hash_table_lookup (choice->choices, nick);
  g_return_if_fail (desc != NULL);
  if (desc->sensitive != sensitive)
    {
      if (desc->redirect_to)
        {
          clay_choice_set_sensitive (choice, desc->redirect_to, sensitive);
        }
      else
        {
          desc->sensitive = sensitive;
          for (GList *iter = desc->aliases; iter; iter = iter->next)
            {
              ClayChoiceDesc *alias_desc;

              alias_desc = g_hash_table_lookup (choice->choices, iter->data);
              alias_desc->sensitive = sensitive;
              g_signal_emit (choice, clay_choice_signals[SENSITIVITY_CHANGED], 0, (gchar *) iter->data);
            }
          g_signal_emit (choice, clay_choice_signals[SENSITIVITY_CHANGED], 0, nick);
        }
    }
}


/* Private functions */

static void
clay_choice_desc_free (ClayChoiceDesc *desc)
{
  g_free (desc->label);
  g_free (desc->help);
  g_free (desc->redirect_to);
  g_free (desc->reason);
  g_list_free_full (desc->aliases, g_free);
  g_free (desc);
}


/*
 * CLAY_TYPE_PARAM_CHOICE
 */

#define CLAY_PARAM_SPEC_CHOICE(pspec)    (G_TYPE_CHECK_INSTANCE_CAST ((pspec), CLAY_TYPE_PARAM_CHOICE, ClayParamSpecChoice))

typedef struct _ClayParamSpecChoice ClayParamSpecChoice;

struct _ClayParamSpecChoice
{
  GParamSpecString  parent_instance;

  ClayChoice       *choice;
};

static void       clay_param_choice_class_init        (GParamSpecClass *klass);
static void       clay_param_choice_init              (GParamSpec      *pspec);
static void       clay_param_choice_finalize          (GParamSpec      *pspec);
static gboolean   clay_param_choice_validate          (GParamSpec      *pspec,
                                                       GValue          *value);
static gboolean   clay_param_choice_value_is_valid    (GParamSpec      *pspec,
                                                       const GValue    *value);
static gint       clay_param_choice_values_cmp        (GParamSpec      *pspec,
                                                       const GValue    *value1,
                                                       const GValue    *value2);

GType
clay_param_choice_get_type (void)
{
  static GType type = 0;

  if (! type)
    {
      const GTypeInfo info =
      {
        sizeof (GParamSpecClass),
        NULL, NULL,
        (GClassInitFunc) clay_param_choice_class_init,
        NULL, NULL,
        sizeof (ClayParamSpecChoice),
        0,
        (GInstanceInitFunc) clay_param_choice_init
      };

      type = g_type_register_static (G_TYPE_PARAM_STRING,
                                     "ClayParamChoice", &info, 0);
    }

  return type;
}

static void
clay_param_choice_class_init (GParamSpecClass *klass)
{
  klass->value_type     = G_TYPE_STRING;
  klass->finalize       = clay_param_choice_finalize;
  klass->value_validate = clay_param_choice_validate;
  klass->value_is_valid = clay_param_choice_value_is_valid;
  klass->values_cmp     = clay_param_choice_values_cmp;
}

static void
clay_param_choice_init (GParamSpec *pspec)
{
  ClayParamSpecChoice *choice = CLAY_PARAM_SPEC_CHOICE (pspec);

  choice->choice = NULL;
}

static void
clay_param_choice_finalize (GParamSpec *pspec)
{
  ClayParamSpecChoice *spec_choice  = CLAY_PARAM_SPEC_CHOICE (pspec);
  GParamSpecClass     *parent_class = g_type_class_peek (g_type_parent (CLAY_TYPE_PARAM_CHOICE));

  g_object_unref (spec_choice->choice);

  parent_class->finalize (pspec);
}

static gboolean
clay_param_choice_validate (GParamSpec *pspec,
                            GValue     *value)
{
  ClayParamSpecChoice *spec_choice = CLAY_PARAM_SPEC_CHOICE (pspec);
  GParamSpecString    *spec_string = G_PARAM_SPEC_STRING (pspec);
  ClayChoice          *choice      = spec_choice->choice;
  const gchar         *strval      = g_value_get_string (value);

  if (! clay_choice_is_valid (choice, strval))
    {
      if (clay_choice_is_valid (choice, spec_string->default_value))
        {
          g_value_set_string (value, spec_string->default_value);
        }
      else
        {
          /* This might happen if the default value is set insensitive. Then we
           * should just set any valid random nick.
           */
          GList *nicks;

          nicks = clay_choice_list_nicks (choice);
          for (GList *iter = nicks; iter; iter = iter->next)
            if (clay_choice_is_valid (choice, (gchar *) iter->data))
              {
                g_value_set_string (value, (gchar *) iter->data);
                break;
              }
        }
      return TRUE;
    }

  return FALSE;
}

static gboolean
clay_param_choice_value_is_valid (GParamSpec   *pspec,
                                  const GValue *value)
{
  ClayParamSpecChoice *cspec  = CLAY_PARAM_SPEC_CHOICE (pspec);
  const gchar         *strval = g_value_get_string (value);
  ClayChoice          *choice = cspec->choice;
  const gchar         *reason = NULL;
  const gchar         *alias  = NULL;
  gboolean             valid;

  valid = clay_choice_is_valid (choice, strval);
  if (valid && clay_choice_is_deprecated (choice, strval, &alias, &reason) &&
      /* Typically when calling a PDB procedure, we will first set
       * properties to a ClayProcedureConfig, which will later be copied
       * to a second one prefixed with "ClayProcedureConfigRun-".
       * Plug-in developers don't have much say to this second config
       * object, so let's avoid 2 WARNINGs for a single object set.
       */
      ! g_str_has_prefix (g_type_name (pspec->owner_type), "ClayProcedureConfigRun-"))
    {
      if (alias)
        g_critical ("Value \"%s\" is deprecated for property \"%s\" of %s. "
                    "Use \"%s\" instead.", strval, pspec->name,
                    g_type_name (pspec->owner_type), alias);
      else
        g_critical ("Value \"%s\" is deprecated for property \"%s\" of %s. %s",
                    strval, pspec->name, g_type_name (pspec->owner_type), reason);
    }

  return valid;
}

static gint
clay_param_choice_values_cmp (GParamSpec   *pspec,
                              const GValue *value1,
                              const GValue *value2)
{
  const gchar *choice1 = g_value_get_string (value1);
  const gchar *choice2 = g_value_get_string (value2);

  return g_strcmp0 (choice1, choice2);
}

/**
 * clay_param_spec_choice:
 * @name:  Canonical name of the property specified.
 * @nick:  Nick name of the property specified.
 * @blurb: Description of the property specified.
 * @choice: (transfer full): the %ClayChoice describing allowed choices.
 * @flags: Flags for the property specified.
 *
 * Creates a new #ClayParamSpecChoice specifying a
 * #G_TYPE_STRING property.
 * This %ClayParamSpecChoice takes ownership of the reference on @choice.
 *
 * See g_param_spec_internal() for details on property names.
 *
 * Returns: (transfer floating): The newly created #ClayParamSpecChoice.
 *
 * Since: 3.0
 **/
GParamSpec *
clay_param_spec_choice (const gchar *name,
                        const gchar *nick,
                        const gchar *blurb,
                        ClayChoice  *choice,
                        const gchar *default_value,
                        GParamFlags  flags)
{
  ClayParamSpecChoice *choice_spec;
  GParamSpecString    *string_spec;

  g_return_val_if_fail (CLAY_IS_CHOICE (choice), NULL);
  g_return_val_if_fail (clay_choice_is_valid (choice, default_value), NULL);

  choice_spec = g_param_spec_internal (CLAY_TYPE_PARAM_CHOICE,
                                       name, nick, blurb, flags);

  g_return_val_if_fail (choice_spec, NULL);

  string_spec = G_PARAM_SPEC_STRING (choice_spec);

  choice_spec->choice        = choice;
  string_spec->default_value = g_strdup (default_value);

  return G_PARAM_SPEC (choice_spec);
}

/**
 * clay_param_spec_choice_get_choice:
 * @pspec: a #GParamSpec to hold a #ClayParamSpecChoice value.
 *
 * Returns: (transfer none): the choice object defining the valid values.
 *
 * Since: 3.0
 **/
ClayChoice *
clay_param_spec_choice_get_choice (GParamSpec *pspec)
{
  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  return CLAY_PARAM_SPEC_CHOICE (pspec)->choice;
}

/**
 * clay_param_spec_choice_get_default:
 * @pspec: a #GParamSpec to hold a #ClayParamSpecChoice value.
 *
 * Returns: the default value.
 *
 * Since: 3.0
 **/
const gchar *
clay_param_spec_choice_get_default (GParamSpec *pspec)
{
  const GValue *value;

  g_return_val_if_fail (CLAY_IS_PARAM_SPEC_CHOICE (pspec), NULL);

  value = g_param_spec_get_default_value (pspec);

  return g_value_get_string (value);
}
