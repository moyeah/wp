#ifndef __WB_SETTINGS_DIALOG_H__
#define __WB_SETTINGS_DIALOG_H__

G_BEGIN_DECLS

#define WB_TYPE_SETTINGS_DIALOG            (wb_settings_dialog_get_type ())
#define WB_SETTINGS_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj),   WB_TYPE_SETTINGS_DIALOG, WbSettingsDialog))
#define WB_SETTINGS_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST    ((klass), WB_TYPE_SETTINGS_DIALOG, WbSettingsDialogClass))
#define WB_IS_SETTINGS_DIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   WB_TYPE_SETTINGS_DIALOG))
#define WB_IS_SETTINGS_DIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), WB_TYPE_SETTINGS_DIALOG))
#define WB_SETTINGS_DIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   WB_TYPE_SETTINGS_DIALOG, WbSettingsDialogClass))

typedef struct _WbSettingsDialog      WbSettingsDialog;
typedef struct _WbSettingsDialogClass WbSettingsDialogClass;

GType      wb_settings_dialog_get_type (void) G_GNUC_CONST;
GtkWidget* wb_settings_dialog_new      (WebKitSettings *settings);

G_END_DECLS

#endif /* __WB_SETTINGS_DIALOG_H__ */
