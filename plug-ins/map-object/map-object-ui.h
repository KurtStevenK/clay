#ifndef __MAPOBJECT_UI_H__
#define __MAPOBJECT_UI_H__

/* Externally visible variables */
/* ============================ */

extern GtkWidget *previewarea;

/* Externally visible functions */
/* ============================ */

gboolean main_dialog (ClayProcedure       *procedure,
                      ClayProcedureConfig *config,
                      ClayDrawable        *drawable);

#endif  /* __MAPOBJECT_UI_H__ */
