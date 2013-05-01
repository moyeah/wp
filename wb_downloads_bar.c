#include <webkit2/webkit2.h>

#include "wb_downloads_bar.h"

#define WB_TYPE_DOWNLOAD (wb_download_get_type ())
#define WB_DOWNLOAD(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), WB_TYPE_DOWNLOAD, WbDownload))

typedef struct _WbDownload      WbDownload;
typedef struct _WbDownloadClass WbDownloadClass;

struct _WbDownloadsBar
{
  GtkInfoBar parent;
};

struct _WbDownloadsBarClass
{
  GtkInfoBarClass parent;
};

G_DEFINE_TYPE (WbDownloadsBar, wb_downloads_bar, GTK_TYPE_INFO_BAR)

static void
downloads_bar_child_remove_cb (GtkContainer   *info_bar,
                               GtkWidget      *widget,
                               WbDownloadsBar *downloads_bar)
{
  GList *children = gtk_container_get_children (info_bar);

  if (g_list_length (children) == 1)
    gtk_info_bar_response (GTK_INFO_BAR (downloads_bar), GTK_RESPONSE_CLOSE);

  g_list_free (children);
}

static void
wb_downloads_bar_init (WbDownloadsBar *downloads_bar)
{
  GtkWidget *content_box =
    gtk_info_bar_get_content_area (GTK_INFO_BAR (downloads_bar));
  gtk_orientable_set_orientation (GTK_ORIENTABLE (content_box),
                                  GTK_ORIENTATION_VERTICAL);
  g_signal_connect_after (content_box, "remove",
                          G_CALLBACK (downloads_bar_child_remove_cb),
                          downloads_bar);

  GtkWidget *title = gtk_label_new (NULL);
  gtk_label_set_markup (
    GTK_LABEL (title),
    "<span size='xx-large' weight='bold'>Downloads</span>");
  gtk_misc_set_alignment (GTK_MISC (title), 0., 0.5);
  gtk_box_pack_start (GTK_BOX (content_box), title, FALSE, FALSE, 12);
}

static void
wb_downloads_bar_response (GtkInfoBar *info_bar,
                           gint        response_id)
{
  gtk_widget_destroy (GTK_WIDGET (info_bar));
}

static void
wb_downloads_bar_class_init (WbDownloadsBarClass *klass)
{
  GtkInfoBarClass *info_bar_class = GTK_INFO_BAR_CLASS (klass);
  info_bar_class->response = wb_downloads_bar_response;
}

GtkWidget*
wb_downloads_bar_new ()
{
  GtkInfoBar *downloads_bar =
    GTK_INFO_BAR (g_object_new (WB_TYPE_DOWNLOADS_BAR, NULL));
  gtk_info_bar_add_buttons (downloads_bar, GTK_STOCK_CLOSE,
                            GTK_RESPONSE_CLOSE, NULL);

  return GTK_WIDGET (downloads_bar);
}

struct _WbDownload
{
  GtkBox parent;

  WebKitDownload *download;

  guint64 content_length;
  guint64 downloaded_size;

  gboolean finished;

  GtkWidget *status_label;
  GtkWidget *remaining_label;
  GtkWidget *progress_bar;
  GtkWidget *action_button;
};

struct _WbDownloadClass
{
  GtkBoxClass parent;
};

G_DEFINE_TYPE (WbDownload, wb_download, GTK_TYPE_BOX)

static void
action_button_clicked_cb (GtkButton  *button,
                          WbDownload *wb_download)
{
  if (!wb_download->finished)
  {
    webkit_download_cancel (wb_download->download);
    return;
  }

  gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (wb_download)),
                webkit_download_get_destination (wb_download->download),
                gtk_get_current_event_time (), NULL);
  gtk_widget_destroy (GTK_WIDGET (wb_download));
}

static void
wb_download_init (WbDownload *download)
{
  GtkWidget *main_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_box_pack_start (GTK_BOX (download), main_box, FALSE, FALSE, 0);

  GtkWidget *vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (main_box), vbox, TRUE, TRUE, 0);

  GtkWidget *status_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (vbox), status_box, TRUE, TRUE, 0);

  download->status_label = gtk_label_new ("Starting Download");
  gtk_label_set_ellipsize (GTK_LABEL (download->status_label),
                           PANGO_ELLIPSIZE_END);
  gtk_misc_set_alignment (GTK_MISC (download->status_label), 0., 0.5);
  gtk_box_pack_start (GTK_BOX (status_box), download->status_label,
                      TRUE, TRUE, 0);

  download->remaining_label = gtk_label_new (NULL);
  gtk_misc_set_alignment (GTK_MISC (download->remaining_label), 1., 0.5);
  gtk_box_pack_end (GTK_BOX (status_box), download->remaining_label,
                    TRUE, TRUE, 0);

  download->progress_bar = gtk_progress_bar_new ();
  gtk_box_pack_start (GTK_BOX (vbox), download->progress_bar,
                      FALSE, FALSE, 0);

  download->action_button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_box_pack_end (GTK_BOX (main_box), download->action_button,
                    FALSE, FALSE, 0);
  g_signal_connect (download->action_button, "clicked",
                    G_CALLBACK (action_button_clicked_cb),
                    download);
}

static void
wb_download_finalize (GObject *object)
{
  WbDownload *wb_download = WB_DOWNLOAD (object);

  if (wb_download->download)
  {
    g_object_unref (wb_download->download);
    wb_download->download = NULL;
  }

  G_OBJECT_CLASS (wb_download_parent_class)->finalize (object);
}

static void
wb_download_class_init (WbDownloadClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = wb_download_finalize;
}

static void
download_notify_response_cb (WebKitDownload *download,
                             GParamSpec     *pspec,
                             WbDownload     *wb_download)
{
  WebKitURIResponse *response = webkit_download_get_response (download);
  wb_download->content_length =
    webkit_uri_response_get_content_length (response);
  gchar *text = g_strdup_printf ("Downloading %s",
                                 webkit_uri_response_get_uri (response));
  gtk_label_set_text (GTK_LABEL (wb_download->status_label), text);
  g_free (text);
}

static gchar*
remaining_time (WbDownload *wb_download)
{
  guint64 total = wb_download->content_length;
  guint64 current = wb_download->downloaded_size;
  gdouble elapsed_time =
    webkit_download_get_elapsed_time (wb_download->download);

  if (current <= 0)
     return NULL;

  gdouble per_byte_time = elapsed_time / current;
  gdouble interval = per_byte_time * (total - current);

  int hours = (int) (interval / 3600);
  interval -= hours * 3600;
  int mins = (int) (interval / 60);
  interval -= mins * 60;
  int secs = (int) interval;

  if (hours > 0)
  {
    if (mins > 0)
      return g_strdup_printf (ngettext ("%u:%02u hour left",
                                        "%u:%02u hours left",
                                        hours), hours, mins);
    return g_strdup_printf (ngettext ("%u hour left",
                                      "%u hours left",
                                      hours), hours);
  }

  if (mins > 0)
    return g_strdup_printf (ngettext ("%u:%02u minute left",
                                      "%u:%02u minutes left",
                                      mins), mins, secs);
  return g_strdup_printf (ngettext ("%u second left",
                                    "%u seconds left",
                                    secs), secs);
}

static void
download_notify_estimated_progress_cb (WebKitDownload *download,
                                       GParamSpec     *pspec,
                                       WbDownload     *wb_download)
{
  gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (wb_download->progress_bar),
                                 webkit_download_get_estimated_progress (
                                   download));
  gchar *remaining = remaining_time (wb_download);
  gtk_label_set_text (GTK_LABEL (wb_download->remaining_label), remaining);
  g_free (remaining);
}

static void
download_received_data_cb (WebKitDownload *download,
                           guint64         data_length,
                           WbDownload     *wb_download)
{
  wb_download->downloaded_size += data_length;
}

static void
download_finished_cb (WebKitDownload *download,
                      WbDownload     *wb_download)
{
  gchar *text = g_strdup_printf ("Download completed: %s",
                                 webkit_download_get_destination (download));
  gtk_label_set_text (GTK_LABEL (wb_download->status_label), text);
  g_free (text);

  gtk_label_set_text (GTK_LABEL (wb_download->remaining_label), NULL);
  gtk_button_set_image (GTK_BUTTON (wb_download->action_button),
                        gtk_image_new_from_stock (GTK_STOCK_OPEN,
                                                  GTK_ICON_SIZE_BUTTON));
  gtk_button_set_label (GTK_BUTTON (wb_download->action_button), "Open...");
  wb_download->finished = TRUE;
}

static void
download_failed_cb (WebKitDownload *download,
                    GError         *error,
                    WbDownload     *wb_download)
{
  g_signal_handlers_disconnect_by_func (wb_download->download,
                                        download_finished_cb,
                                        wb_download);

  if (g_error_matches (error, WEBKIT_DOWNLOAD_ERROR,
                       WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER))
  {
    gtk_widget_destroy (GTK_WIDGET (wb_download));
    return;
  }

  gchar *error_message = g_strdup_printf ("Download failed: %s",
                                          error->message);
  gtk_label_set_text (GTK_LABEL (wb_download->status_label),
                      error_message);
  g_free (error_message);

  gtk_label_set_text (GTK_LABEL (wb_download->remaining_label), NULL);
  gtk_widget_set_sensitive (wb_download->action_button, FALSE);
}

GtkWidget*
wb_download_new (WebKitDownload *download)
{
  WbDownload *wb_download =
    WB_DOWNLOAD (g_object_new (WB_TYPE_DOWNLOAD,
                 "orientation", GTK_ORIENTATION_VERTICAL,
                 NULL));

  wb_download->download = g_object_ref (download);
  g_signal_connect (wb_download->download, "notify::response",
                    G_CALLBACK (download_notify_response_cb),
                    wb_download);
  g_signal_connect (wb_download->download, "notify::estimated-progress",
                    G_CALLBACK (download_notify_estimated_progress_cb),
                    wb_download);
  g_signal_connect (wb_download->download, "received-data",
                    G_CALLBACK (download_received_data_cb),
                    wb_download);
  g_signal_connect (wb_download->download, "finished",
                    G_CALLBACK (download_finished_cb),
                    wb_download);
  g_signal_connect (wb_download->download, "failed",
                    G_CALLBACK (download_failed_cb),
                    wb_download);

  return GTK_WIDGET (wb_download);
}

void
wb_downloads_bar_add_download (WbDownloadsBar *downloads_bar,
                               WebKitDownload *download)
{
  GtkWidget *wb_download = wb_download_new (download);
  GtkWidget *content_box =
    gtk_info_bar_get_content_area (GTK_INFO_BAR (downloads_bar));
  gtk_box_pack_start (GTK_BOX (content_box), wb_download, FALSE, FALSE, 0);
}
