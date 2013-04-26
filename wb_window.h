#ifndef __WB_WINDOW_H__
#define __WB_WINDOW_H__

G_BEGIN_DECLS

#define WB_WINDOW_TYPE      (wb_window_get_type ())
#define WB_WINDOW           (obj)                   (G_TYPE_CHECK_INSTANCE_CAST ((obj), WB_WINDOW_TYPE, wb_window))
#define WB_WINDOW_CLASS     (klass)                 (G_TYPE_CHECK_CLASS_CAST    ((klass), WB_WINDOW_TYPE, wb_window_class))
#define WB_IS_WINDOW        (obj)                   (G_TYPE_CHECK_INSTANCE_TYPE ((obj), WB_WINDOW_TYPE))
#define WB_IS_WINDOW_CLASS  (klass)                 (G_TYPE_CHECK_CLASS_TYPE    ((klass), WB_WINDOW_TYPE))
#define WB_WINDOW_GET_CLASS (obj)                   (G_TYPE_INSTANCE_GET_CLASS  ((obj), WB_WINDOW_TYPE, wb_window_class))

typedef struct _WbWindow      WbWindow;
typedef struct _WbWindowClass WbWindowClass;

GType wb_window_get_type (void);

GtkWidget*     wb_window_new      (WebKitWebView*, GtkWindow*);
WebKitWebView* wb_window_get_view (WbWindow*);
void           wb_window_load_uri (WbWindow*, const gchar *uri);

G_END_DECLS

#endif /* __WB_WINDOW_H__
