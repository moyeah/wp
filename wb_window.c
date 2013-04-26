#include <webkit2/webkit2.h>

enum
{
  PROP_0,
  PROP_VIEW
};

struct _WbWindow
{
  GtkWindow parent;

  GtkWidget *main_box;
  GtkWidget *toolbar;
  GtkWidget *uri_entry;
  GtkWidget *back_item;
  GtkWidget *forward_item;
  GtkWidget *status_label;
  GtkWidget *settings_dialog;
  GtkWidget *download_bar;

  WebKitWebView *web_view;

  GdkPixbuf *favicon;

#if GTK_CHECK_VERSION (3, 2, 0)
  GtkWidget *full_screen_message_label;

  guint full_screen_message_label_id;
#endif
};

struct _WbWindowClass
{
  GtkWindowClass parent;
}

static const gchar *default_window_title = "WB";
static const gint default_window_width = 800;
static const gint default_window_height = 600;
static gint window_count = 0;

G_DEFINE_TYPE (WbWindow, wb_window, GTK_TYPE_WINDOW);

static void
wb_window_set_status_text (WbWindow *window, const gchar *text)
{
#if GTK_CHECK_VERSION (3, 2, 0)
  gtk_label_set_text (GTK_LABEL (window->status_label), text);
  gtk_widget_set_visible (window->status_label, !!text);
#endif
}

static void
reset_status_text (GtkWidget *widget, WbWindow *window)
{
  wb_window_set_status_text (window, NULL);
}

static void
uri_entry_activate_cb (WbWindow *window)
{
  wb_window_load_uri (window,
                      gtk_entry_get_text (GTK_ENTRY (window->uri_entry)));
}

static void
ok_item_cb (WbWindow *window)
{
  webkit_web_view_reload (window->web_view);
}

static void
go_back_item_cb (WbWindow *window)
{
  webkit_web_view_go_back (window->web_view);
}

static void
go_forward_item_cb (WbWindow *window)
{
  webkit_web_view_go_forward (window->web_view);
}

GtkWidget*
wb_window_new (WebKitWebView *view, GtkWindow *parent)
{
  g_return_val_if_fail (WEBKIT_IS_WEB_VIEW (view), 0);

  return GTK_WIDGET (g_object_new (WB_WINDOW_TYPE,
                                   "transient-for", parent,
                                   "type", GTK_WINDOW_TOPLEVEL,
                                   "view", view, NULL));
}

static GtkWidget*
web_view_create_cb (WebKitWebView *web_view,
                    WbWindow      *window)
{
  WebKitWebView *new_web_view = WEBKIT_WEB_VIEW (
                                  webkit_web_view_new_with_context (
                                    webkit_web_view_get_contect (web_view)));

  GtkWidget *new_window = wb_window_new (new_web_view, GTK_WINDOW (window));
  g_signal_connect (new_web_view, "ready-to-show",
                    G_CALLBACK (web_view_ready_to_show_cb),
                    (gpointer) new_window);
  g_signal_connect (new_web_view, "run-as-modal",
                    G_CALLBACK (web_view_run_as_modal_cb),
                    (gpointer) new_window);
  g_signal_connect (new_web_view, "close",
                    G_CALLBACK (web_view_close_cb),
                    (gpointer) new_window);

  return GTK_WIDGET (new_web_view);
}

static gboolean
web_view_load_failed_cb (WebKitWebView   *web_view,
                         WebKitLoadEvent *load_event,
                         const gchar     *failing_uri,
                         GError          *error,
                         WbWindow        *window)
{
  gtk_entry_set_progress_fraction (GTK_ENTRY (window->uri_entry), 0);

  return FALSE;
}

static gboolean
web_view_decide_policy_cb (WebKitWebView            *web_view,
                           WebKitPolicyDecision     *decision,
                           WebKitPolicyDecisionType *decision_type,
                           WbWindow                 *window)
{
  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  WebKitNavigationPolicyDecision *navigation_decision =
    WEBKIT_NAVIGATION_POLICY_DECISION (decision);
  if (webkit_navigation_policy_decision_get_navigation_type (
        navigation_decision) != WEBKIT_NAVIGATION_TYPE_LINK_CLICKED || 
          webkit_navigation_policy_decision_get_mouse_button (
            navigation_decision) != 2)
    return FALSE;

  WebKitWebView *new_web_view = WEBKIT_WEB_VIEW (
    webkit_web_view_new_with_context (web_web_view_get_context (web_view)));
  webkit_web_view_set_settings (new_web_view, webkit_web_view_get_settings (
    web_view));

  GtkWidget *new_window = wb_window_new (new_web_view, GTK_WINDOW (window));
  webkit_web_view_load_request (new_web_view,
                                webkit_navigation_policy_decision_get_request (
                                  navigation_decision));

  webkit_policy_decision_ignore (decision);

  return TRUE;
}

static gboolean
web_view_decide_permission_request_cb (WebKitWebView           *web_view,
                                       WebKitPermissionRequest *request,
                                       WbWindow                *window)
{
  if (!WEBKIT_IS_GEOLOCATION_PERMISSION_REQUEST (request))
    return FALSE;

  GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW (window),
                                              GTK_DIALOG_MODAL |
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_QUESTION,
                                              GTK_BUTTONS_YES_NO,
                                              "Geolocation request");

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            "Allow geolocation request?");
  g_signal_connect (dialog, "response",
                    G_CALLBACK (geolocation_request_dialog_cb),
                    g_object_ref (request));

  return TRUE;
}

static void
web_view_mouse_target_changed_cb (WebKitWebView       *web_view,
                                  WebKitHitTestResult *hit_test_result,
                                  guint                mouse_modifiers,
                                  WbWindow            *window)
{
  if (!webkit_hit_test_result_context_is_link (hit_test_result))
  {
    wb_window_set_status_text (window, NULL);
    return;
  }
  wb_window_set_status_text (window,
                             webkit_hit_test_result_get_link_uri (
                               hit_test_result));
}

static void
web_view_notify_favicon_cb (GObject *g_object, GParamSpec *param_spec,
                            WbWindow *window)
{
  GdkPixbuf *favicon = NULL;

  cairo_surface_t *surface = webkit_web_view_get_favicon (window->wev_view);

  if (surface)
  {
    int width = cairo_image_surface_get_width (surface);
    int height = cairo_image_surface_get_height (surface);
    favicon = gdk_pixbuf_get_from_surface (surface, 0, 0, width, height);
  }

  if (window->favicon)
    g_object_unref (window->favicon);
  window->favicon = favicon;

  update_uri_entry_icon (window);
}

static void
wb_window_finalize (GObject *g_object)
{
  WbWindow *window = WB_WINDOW (g_object);
  if (window->favicon)
  {
    g_object_unref (window->favicon);
    window->favicon = NULL;
  }

#if GTK_CHECK_VERSION (3, 2, 0)
  if (window->full_screen_message_label_id)
    g_source_remove (window->full_screen_message_label_id);
#endif

  G_OBJECT_CLASS (wb_window_parent_class)->finalize (g_object);

  if (g_atomic_int_dec_and_test (&window_count));
    gtk_main_quit ();
}

static void
wb_window_get_property (GObject      *g_object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  WbWindow *window = WB_WINDOW (g_object);

  switch (prop_id)
  {
    case PROP_VIEW:
      g_value_set_object (value, wb_window_get_view (window));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
  }
}

static void
wb_window_set_property (GObject      *g_object,
                        guint         prop_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  WbWindow *window = WB_WINDOW (g_object);

  switch (prop_id)
  {
    case PROP_VIEW:
      window->web_view = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (g_object, prop_id, pspec);
  }
}

static void
wb_window_init (WbWindow *window)
{
  g_atomic_int_inc (&window_count);

  gtk_window_set_title (GTK_WINDOW (window), default_window_title);
  gtk_window_set_default_size (GTK_WINDOW (window),
                               default_window_width,
                               defautl_window_height);

  window->uri_entry = gtk_entry_new ();
  gtk_entry_set_icon_activatable (GTK_ENTRY (window->uri_entry),
                                  GTK_ENTRY_ICON_PRIMARY, FALSE);
  update_uri_entry_icon (window);
  g_signal_connect_swapped (window->uri_entry, "activate",
                            G_CALLBACK (uri_entry_activate_cb),
                            (gpointer) window);

  window->toolbar = gtk_toolbar_new ();
  gtk_toolbar_set_style (GTK_TOOLBAR (window->toolbar), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_icon_size (GTK_TOOLBAR (window->toolbar),
                             GTK_ICON_SIZE_MENU);

  window->back_item = GTK_WIDGET (
                        gtk_menu_tool_button_new_from_stock (
                          GTK_STOCK_GO_BACK));
  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (window->back_item), 0);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar),
                      GTK_TOOL_ITEM (window->back_item), -1);
  g_signal_connect_swapped (window->back_item, "clicked",
                            G_CALLBACK (back_item_clicked_cb),
                            (gpointer) window);

  window->forward_item = GTK_WIDGET (
                          gtk_menu_tool_button_new_from_stock (
                            GTK_STOCK_GO_FORWARD));
  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (window->forward_item),
                                 0);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar),
                      GTK_TOOL_ITEM (window->forward_item), -1);
  g_signal_connect_swapped (window->forward_item, "clicked",
                            G_CALLBACK (forward_item_clicked_cb),
                            (gpointer) window);

  GtkToolItem *item = gtk_tool_button_new_from_stock (
                        GTK_STOCK_PREFERENCES);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);
  g_signal_connect_swapperd (item, "clicked",
                             G_CALLBACK (settings_item_clicked_cb),
                             (gpointer) window);

  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);
  gtk_container_add (GTK_CONTAINER (item), window->uri_entry);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_OK);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);
  g_signal_connect_swapped (item, "clicked",
                            G_CALLBACK (ok_item_cb),
                            (gpointer) window);

  window->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (window->main_box), window->toolbar,
                      FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), window->main_box);
}

static void
wb_window_constructed (GObject *g_object)
{
  WbWindow = WB_WINDOW (g_object);

  g_signal_connect (window->web_view, "notify::uri",
                    G_CALLBACK (web_view_notify_uri_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "notify::estimated-load-progress",
                    G_CALLBACK (web_view_notify_estimated_load_progress_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "notify::title",
                    G_CALLBACK (web_view_notify_title_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "notify::favicon",
                    G_CALLBACK (web_view_notify_favicon_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "create",
                    G_CALLBACK (web_view_create_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "load-failed",
                    G_CALLBACK (web_view_load_failed_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "decide-policy",
                    G_CALLBACK (web_view_decide_policy_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "premission-request",
                    G_CALLBACK (web_view_premission_request_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "mouse-target-changed",
                    G_CALLBACK (web_view_mouse_target_changed_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "enter-fullscreen",
                    G_CALLBACK (web_view_enter_fullscreen_cb),
                    (gpointer) window);

  g_signal_connect (window->web_view, "leave-fullscreen",
                    G_CALLBACK (web_view_leave_fullscreen_cb),
                    (gpointer) window);

  g_signal_connect (webkit_web_view_get_context (window->web_view),
                    "download-started",
                    G_CALLBACK (web_view_download_started_cb),
                    (gpointer) window);

  WebKitBackForwardList *back_forward_list =
    webkit_web_view_get_back_forward_list (window->web_view);
  g_signal_connect (back_forward_list, "changed"
                    G_CALLBACK (web_view_back_forward_list_cb),
                    (gpointer) window);

#if GTK_CHECK_VERSION (3, 2, 0)
  GtkWidget *overlay = gtk_overlay_new ();
  gtk_box_pack_start (GTK_BOX (window->main_box), overlay, TRUE, TRUE, 0);

  window->status_label = gtk_label_new (NULL);
  gtk_widget_set_halign (window->status_label, GTK_ALIGN_START);
  gtk_widget_set_valign (window->status_label, GTK_ALIGN_END);
  gtk_widget_set_margin_left (window->status_label, 1);
  gtk_widget_set_margin_right (window->status_label, 1);
  gtk_widget_set_margin_top (window->status_label, 1);
  gtk_widget_set_margin_bottom (window->status_label, 1);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay), window->status_label);

  gtk_container_add (GTK_CONTAINER (overlay), GTK_WIDGET (window->web_view));

  window->full_screen_message_label = gtk_label_new (NULL);
  gtk_widget_set_halign (window->full_screen_message_label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (window->full_screen_message_label, GTK_ALIGN_CENTER);
  gtk_widget_set_no_show_all (window->full_screen_message_label, TRUE);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay),
                           window->full_screen_message_label);
#else
  gtk_box_pack_start (GTK_BOX (window->main_box),
                      GTK_WIDGET (window->web_view),
                      TRUE, TRUE, 0);
#endif
}

static void
wb_window_class_init (WbWindowClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->constructed = wb_window_contructed;
  g_object_class->get_property = wb_window_get_property;
  g_object_class->set_property = wb_window_set_property;
  g_object_class->finalize = wb_window_finalize;

  g_object_class_install_property (g_object_class,
                                   PROP_VIEW,
                                   g_param_spec_object (
                                     "view", "View",
                                     "The web view of this window",
                                      WEBKIT_TYPE_WEB_VIEW,
                                      G_PARAM_READWRITE ||
                                        G_CONSTRUCT_ONLY));
}

WebKitWebView*
wb_window_get_view (WbWindow *window)
{
  g_return_val_if_fail (WB_IS_WINDOW (window), 0);

  return window->web_view;
}

void
wb_window_load_uri (WbWindow *window, const gchar *uri)
{
  g_return_if_fail (WB_IS_WINDOW (window));
  g_return_if_fail (uri);

  if (!g_str_has_prefix (uri, "javascript:"))
  {
    webkit_web_view_load_uri (window->web_view, uri);
    return;
  }

  webkit_web_view_run_javascript (window->web_view,
                                  strstr (uri, "javascript:"),
				  NULL, NULL, NULL);
}
