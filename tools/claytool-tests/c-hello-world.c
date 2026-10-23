/* Always include this in all plug-ins */
#include <libclay/clay.h>
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
    procedure = clay_image_procedure_new (plug_in, name,
                                          CLAY_PDB_PROC_TYPE_PLUGIN,
                                          hello_world_run, NULL, NULL);
  return procedure;
}
static ClayValueArray *
hello_world_run (ClayProcedure        *procedure,
                 ClayRunMode           run_mode,
                 ClayImage            *image,
                 ClayDrawable        **drawables,
                 ClayProcedureConfig  *config,
                 gpointer              run_data)
{
  gegl_init (NULL, NULL);
  clay_message ("Hello World!");
  return clay_procedure_new_return_values (procedure, CLAY_PDB_SUCCESS, NULL);
}
/* Generate needed main() code */
CLAY_MAIN (HELLO_WORLD_TYPE)
