#ifndef __WB_DOWNLOADS_BAR_H__
#define __WB_DOWNLOADS_BAR_H__

G_BEGIN_DECLS

#define WB_TYPE_DOWNLOADS_BAR         (wb_downloads_bar_get_type ())
#define WB_DOWNLOADS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_CAST    ((obj),   WB_TYPE_DOWNLOADS_BAR, WbDownloadsBar))
#define WB_DOWNLOADS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST       ((klass), WB_TYPE_DOWNLOADS_BAR, WbDownloadsBarClass))
#define WB_IS_DOWNLOADS_BAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   WB_TYPE_DOWNLOADS_BAR))
#define WB_IS_DOWNLOADS_BAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), WB_TYPE_DOWNLOADS_BAR))
#define WB_DOWNLOADS_BAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   WB_TYPE_DOWNLOADS_BAR, WbDownloadsBarClass))

typedef struct _WbDownloadsBar      WbDownloadsBar;
typedef struct _WbDownloadsBarClass WbDownloadsBarClass;

GType      wb_downloads_bar_get_type     (void) G_GNUC_CONST;
GtkWidget* wb_downloads_bar_new          (void);
void       wb_downloads_bar_add_download (WbDownloadsBar *downloads_bar,
                                          WebKitDownload *download);

G_END_DECLS

#endif /* __WB_DOWNLOADS_BAR_H__ */
