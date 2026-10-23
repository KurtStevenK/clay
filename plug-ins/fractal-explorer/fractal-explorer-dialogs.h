#ifndef __FRACTALEXPLORER_DIALOGS_H__
#define __FRACTALEXPLORER_DIALOGS_H__

gint        explorer_dialog            (ClayProcedure       *procedure,
                                        ClayProcedureConfig *config);
void        dialog_update_preview      (ClayProcedureConfig *config);

void        set_cmap_preview           (ClayProcedureConfig *config);
void        make_color_map             (ClayProcedureConfig *config);

gchar     * get_line                   (gchar               *buf,
                                        gint                 s,
                                        FILE                *from,
                                        gint                 init);
gint        load_options               (fractalexplorerOBJ  *xxx,
                                        FILE                *fp);
void        explorer_load              (void);

#endif
