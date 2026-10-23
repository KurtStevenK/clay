/* Always include this in all plug-ins */
#include <libclay/clay.h>
#include <libclay/clayui.h>
/* The name of my PDB procedure */
#define PLUG_IN_PROC "plug-in-zemarmot-c-demo-hello-world"
/* Our custom class HelloWorld is derived from ClayPlugIn. */
struct _HelloWorld
{
  ClayPlugIn parent_instance;
};
#define HELLO_WORLD_TYPE (hello_world_get_type())
G_DECLARE_FINAL_TYPE (HelloWorld, hello_world, HELLO_WORLD,, ClayPlugIn)

/* Declarations */
static GList          * hello_world_query_procedures (ClayPlugIn           *plug_in);
static ClayProcedure  * hello_world_create_procedure (ClayPlugIn           *plug_in,
                                                      const gchar          *name);
static ClayValueArray * hello_world_run              (ClayProcedure        *procedure,
                                                      ClayRunMode           run_mode,
                                                      ClayImage            *image,
                                                      ClayDrawable        **drawables,
                                                      ClayProcedureConfig  *config,
                                                      gpointer              run_data);

G_DEFINE_TYPE (HelloWorld, hello_world, CLAY_TYPE_PLUG_IN)
static void
hello_world_class_init (HelloWorldClass *klass)
{
  ClayPlugInClass *plug_in_class = CLAY_PLUG_IN_CLASS (klass);
  plug_in_class->query_procedures = hello_world_query_procedures;
  plug_in_class->create_procedure = hello_world_create_procedure;
}
static void
hello_world_init (HelloWorld *hello_world)
{
}
static GList *
hello_world_query_procedures (ClayPlugIn *plug_in)
{
  return g_list_append (NULL, g_strdup (PLUG_IN_PROC));
}
static ClayProcedure *
hello_world_create_procedure (ClayPlugIn  *plug_in,
                              const gchar *name)
{
  ClayProcedure *procedure = NULL;

  if (g_strcmp0 (name, PLUG_IN_PROC) == 0)
    {
      procedure = clay_image_procedure_new (plug_in, name,
                                            CLAY_PDB_PROC_TYPE_PLUGIN,
                                            hello_world_run, NULL, NULL);

      clay_procedure_set_sensitivity_mask (procedure, CLAY_PROCEDURE_SENSITIVE_ALWAYS);

      clay_procedure_set_menu_label (procedure, "_C Hello World");
      clay_procedure_add_menu_path (procedure, "<Image>/Hell_o Worlds/");

      clay_procedure_set_documentation (procedure,
                                        "Official Hello World Tutorial in C",
                                        "Some longer text to explain about this procedure. "
                                        "This is mostly for other developers calling this procedure.",
                                        NULL);
      clay_procedure_set_attribution (procedure, "Jehan",
                                      "Jehan, ZeMarmot project",
                                      "2025");

      clay_procedure_add_font_argument   (procedure, "font", "Font", NULL,
                                          FALSE, NULL, TRUE,
                                          G_PARAM_READWRITE);
      clay_procedure_add_int_argument    (procedure, "font-size", "Font Size", NULL,
                                          1, 1000, 20,
                                          G_PARAM_READWRITE);
      clay_procedure_add_unit_argument   (procedure, "font-unit", "Font Unit", NULL,
                                          TRUE, FALSE, clay_unit_pixel (),
                                          G_PARAM_READWRITE);
      clay_procedure_add_string_argument (procedure, "text", "Text", NULL,
                                          "Hello World!",
                                          G_PARAM_READWRITE);

    }

  return procedure;
}
#define PLUG_IN_BINARY "c-hello-world"

static ClayValueArray *
hello_world_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  ClayTextLayer *text_layer;
  ClayLayer     *parent   = NULL;
  gint           position = 0;
  gint           n_drawables;

  gchar         *text;
  ClayFont      *font;
  gint           size;
  ClayUnit      *unit;

  gegl_init (NULL, NULL);

  n_drawables = clay_core_object_array_get_length ((GObject **) drawables);

  if (n_drawables > 1)
    {
      GError *error = NULL;

      g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                   "Procedure '%s' works with zero or one layer.",
                   PLUG_IN_PROC);

      return clay_procedure_new_return_values (procedure,
                                               CLAY_PDB_CALLING_ERROR,
                                               error);
    }
  else if (n_drawables == 1)
    {
      ClayDrawable *drawable = drawables[0];

      if (! CLAY_IS_LAYER (drawable))
        {
          GError *error = NULL;

          g_set_error (&error, CLAY_PLUG_IN_ERROR, 0,
                       "Procedure '%s' works with layers only.",
                       PLUG_IN_PROC);

          return clay_procedure_new_return_values (procedure,
                                                   CLAY_PDB_CALLING_ERROR,
                                                   error);
        }

      parent   = CLAY_LAYER (clay_item_get_parent (CLAY_ITEM (drawable)));
      position = clay_image_get_item_position (image, CLAY_ITEM (drawable));
    }

  if (run_mode == CLAY_RUN_INTERACTIVE)
    {
      GtkWidget *dialog;

      clay_ui_init (PLUG_IN_BINARY);
      dialog = clay_procedure_dialog_new (procedure,
                                          CLAY_PROCEDURE_CONFIG (config),
                                          "Hello World");
      clay_window_set_transient (GTK_WINDOW (dialog));
      clay_procedure_dialog_fill (CLAY_PROCEDURE_DIALOG (dialog), NULL);

      if (! clay_procedure_dialog_run (CLAY_PROCEDURE_DIALOG (dialog)))
        return clay_procedure_new_return_values (procedure, CLAY_PDB_CANCEL, NULL);
    }

  g_object_get (config,
                "text",      &text,
                "font",      &font,
                "font-size", &size,
                "font-unit", &unit,
                NULL);

  text_layer = clay_text_layer_new (image, text, font, size, unit);
  clay_image_insert_layer (image, CLAY_LAYER (text_layer), parent, position);

  g_clear_object (&font);
  g_clear_object (&unit);
  g_free (text);

  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}
/* Generate needed main() code */
CLAY_MAIN (HELLO_WORLD_TYPE)
