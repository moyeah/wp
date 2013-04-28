#include <string.h>
#include <webkit2/webkit2.h>

#include "wb_window.h"
#include "wb_downloads_bar.h"

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
  GtkWidget *downloads_bar;

  WebKitWebView *web_view;

  GdkPixbuf *favicon;

#if GTK_CHECK_VERSION (3, 2, 0)
  GtkWidget *fullscreen_message_label;

  guint fullscreen_message_label_id;
#endif
};

struct _WbWindowClass
{
  GtkWindowClass parent;
};

static const gchar *default_window_title = "WB";
static const gint default_window_width = 800;
static const gint default_window_height = 600;
static gint window_count = 0;

G_DEFINE_TYPE (WbWindow, wb_window, GTK_TYPE_WINDOW)

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
back_item_clicked_cb (WbWindow *window)
{
  webkit_web_view_go_back (window->web_view);
}

static void
forward_item_clicked_cb (WbWindow *window)
{
  webkit_web_view_go_forward (window->web_view);
}

static void
settings_item_clicked_cb (WbWindow *window)
{
  if (window->settings_dialog)
  {
    gtk_window_present (GTK_WINDOW (window->settings_dialog));
    return;
  }

  window->settings_dialog = wb_settings_dialog_new (
                              webkit_web_view_get_settings (
                                window->web_view));
  gtk_window_set_transient_for (GTK_WINDOW (window->settings_dialog),
                                GTK_WINDOW (window));
  g_object_add_weak_pointer (G_OBJECT (window->settings_dialog),
                             (gpointer *) &window->settings_dialog);
}

static void
web_view_notify_uri_cb (WebKitWebView *web_view,
                        GParamSpec    *pspec,
                        WbWindow      *window)
{
  gtk_entry_set_text (GTK_ENTRY (window->uri_entry),
                      webkit_web_view_get_uri (web_view));
}

static void
web_view_notify_title_cb (WebKitWebView *web_view,
                          GParamSpec    *pspec,
                          WbWindow      *window)
{
  const gchar *title = webkit_web_view_get_title (web_view);
  gtk_window_set_title (GTK_WINDOW (window),
                        title ? title : default_window_title);
}

static gboolean
reset_entry_progress (GtkEntry *entry)
{
  gtk_entry_set_progress_fraction (entry, 0);
  return FALSE;
}

static void
web_view_notify_estimated_load_progress_cb (WebKitWebView *web_view,
                                            GParamSpec    *pspec,
                                            WbWindow      *window)
{
  gdouble progress = webkit_web_view_get_estimated_load_progress (web_view);
  gtk_entry_set_progress_fraction (GTK_ENTRY (window->uri_entry),
                                   progress);
  if (progress == 1.0)
    g_timeout_add (500, (GSourceFunc) reset_entry_progress,
                   window->uri_entry);
}

static void
web_view_download_started_cb (WebKitWebContext *web_context,
                              WebKitDownload   *download,
                              WbWindow         *window)
{
  if (!window->downloads_bar)
  {
    window->downloads_bar = wb_downloads_bar_new ();
    gtk_box_pack_start (GTK_BOX (window->main_box),
                        window->downloads_bar,
                        FALSE, FALSE, 0);
    gtk_box_reorder_child (GTK_BOX (window->main_box),
                           window->downloads_bar, 0);
    g_object_add_weak_pointer (G_OBJECT (window->downloads_bar),
                               (gpointer *) &(window->downloads_bar));
  }

  wb_downloads_bar_add_download (WB_DOWNLOADS_BAR (window->downloads_bar),
                                 download);
}

static void
menu_item_select_cb (WbWindow    *window,
                     GtkMenuItem *item)
{
  GtkAction *action = gtk_activatable_get_related_action (
                        GTK_ACTIVATABLE (item));
  wb_window_set_status_text (window,
                             action ? gtk_action_get_name (action) : NULL);
}

static void
action_activate_cb (WbWindow  *window,
                    GtkAction *action)
{
  WebKitBackForwardListItem *item =
    g_object_get_data (G_OBJECT (action),
                       "back-forward-list-item");
  if (!item)
    return;

  webkit_web_view_go_to_back_forward_list_item (window->web_view, item);
}

static GtkWidget*
wb_window_create_back_forward_menu (WbWindow *window,
                                    GList    *list)
{
  if (!list)
    return NULL;

  GtkWidget *menu = gtk_menu_new ();
  GList *list_item;

  for (list_item = list; list_item; list_item = g_list_next (list_item))
  {
    WebKitBackForwardListItem *item =
      (WebKitBackForwardListItem *) list_item->data;
    const gchar *uri = webkit_back_forward_list_item_get_uri (item);
    const gchar *title = webkit_back_forward_list_item_get_title (item);

    GtkAction *action = gtk_action_new (uri, title, NULL, NULL);
    g_object_set_data_full (G_OBJECT (action),
                            "back-forward-list-item",
                            g_object_ref (item), g_object_unref);
    g_signal_connect_swapped (action, "activate",
                              G_CALLBACK (action_activate_cb),
                              window);

    GtkWidget *menu_item = gtk_action_create_menu_item (action);
    g_signal_connect_swapped (menu_item, "select",
                              G_CALLBACK (menu_item_select_cb),
                              window);
    g_object_unref (action);

    gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
  }

  g_signal_connect (menu, "hide",
                    G_CALLBACK (reset_status_text),
                    window);

  return menu;
}

static void
wb_window_update_navigation_actions (WbWindow              *window,
                                     WebKitBackForwardList *back_forward_list)
{
  gtk_widget_set_sensitive (window->back_item,
                            webkit_web_view_can_go_back (window->web_view));
  gtk_widget_set_sensitive (window->forward_item,
                            webkit_web_view_can_go_forward (window->web_view));

  GList *list = webkit_back_forward_list_get_back_list_with_limit (list, 10);
  gtk_menu_tool_button_set_menu (
    GTK_MENU_TOOL_BUTTON (window->back_item),
                          wb_window_create_back_forward_menu (window,
                                                              list));
  g_list_free (list);

  list = webkit_back_forward_list_get_forward_list_with_limit (list, 10);
  gtk_menu_tool_button_set_menu (
    GTK_MENU_TOOL_BUTTON (window->forward_item),
                          wb_window_create_back_forward_menu (window,
                                                              list));
  g_list_free (list);
}

static void
back_forward_list_changed_cb (WebKitBackForwardList *back_forward_list,
                              WebKitBackForwardList *item_added,
                              GList                 *items_removed,
                              WbWindow              *window)
{
  wb_window_update_navigation_actions (window, back_forward_list);
}

static void
dialog_geolocation_response_cb (GtkDialog               *dialog,
                                gint                     response,
                                WebKitPermissionRequest *request)
{
  switch (response)
  {
    case GTK_RESPONSE_YES:
      wekit_permission_request_allow (request);
      break;
    default:
      webkit_permission_request_deny (request);
      break;
  }

  gtk_widget_destroy (GTK_WIDGET (dialog));
  g_object_unref (request);
}

static void
web_view_ready_to_show_cb (WebKitWebView *web_view, WbWindow *window)
{
  WebKitWindowProperties *window_properties =
    webkit_web_view_get_window_properties (web_view);

  GdkRectangle geometry;
  webkit_window_properties_get_geometry (window_properties,
                                         &geometry);
  if (geometry.x >= 0 && geometry.y >= 0)
    gtk_window_move (GTK_WINDOW (window), geometry.x, geometry.y);
  if (geometry.width > 0 && geometry.height > 0)
    gtk_window_resize (GTK_WINDOW (window),
                       geometry.width,
                       geometry.height);

  if (!webkit_window_properties_get_toolbar_visible (window_properties))
    gtk_widget_hide (window->toolbar);
  else if (!webkit_window_properties_get_location_bar_visible (
             window_properties))
    gtk_widget_hide (window->uri_entry);

  if (!webkit_window_properties_get_resizable (window_properties))
    gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
}

static void
web_view_run_as_modal_cb (WebKitWebView *web_view, WbWindow *window)
{
  gtk_window_set_modal (GTK_WINDOW (window), TRUE);
}

static void
web_view_close_cb (WebKitWebView *web_view, WbWindow *window)
{
  gtk_widget_destroy (GTK_WIDGET (window));
}

#if GTK_CHECK_VERSION (3, 2, 0)
static gboolean
fullscreen_message_timeout_cb (WbWindow *window)
{
  gtk_widget_hide (window->fullscreen_message_label);
  window->fullscreen_message_label_id = 0;
  return FALSE;
}
#endif

static gboolean
web_view_enter_fullscreen_cb (WebKitWebView *web_view, WbWindow *window)
{
#if GTK_CHECK_VERSION (3, 2, 0)
  const gchar *title_or_uri = webkit_web_view_get_title (window->web_view);
  if (!title_or_uri)
    title_or_uri = webkit_web_view_get_uri (window->web_view);

  gchar *message = g_strdup_printf (
                     "%s is now full screen. Press ESQ of f to exist.",
                     title_or_uri);
  gtk_label_set_text (GTK_LABEL (window->fullscreen_message_label),
                      message);
  g_free (message);

  window->fullscreen_message_label_id =
    g_timeout_add_seconds (2, (GSourceFunc) fullscreen_message_timeout_cb,
                           window);
#endif

  gtk_widget_hide (window->toolbar);

  return FALSE;
}

static gboolean
web_view_leave_fullscreen_cb (WebKitWebView *web_view, WbWindow *window)
{
#if GTK_CHECK_VERSION (3, 2, 0)
  if (window->fullscreen_message_label_id)
  {
    g_source_remove (window->fullscreen_message_label_id);
    window->fullscreen_message_label_id = 0;
  }

  gtk_widget_hide (window->fullscreen_message_label);
#endif

  gtk_widget_show (window->toolbar);

  return FALSE;
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
                    new_window);
  g_signal_connect (new_web_view, "run-as-modal",
                    G_CALLBACK (web_view_run_as_modal_cb),
                    new_window);
  g_signal_connect (new_web_view, "close",
                    G_CALLBACK (web_view_close_cb),
                    new_window);

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
    webkit_web_view_new_with_context (webkit_web_view_get_context (web_view)));
  webkit_web_view_set_settings (
    new_web_view, webkit_web_view_get_settings (web_view));

  GtkWidget *new_window = wb_window_new (new_web_view, GTK_WINDOW (window));
  webkit_web_view_load_request (new_web_view,
                                webkit_navigation_policy_decision_get_request (
                                  navigation_decision));

  webkit_policy_decision_ignore (decision);

  return TRUE;
}

static gboolean
web_view_premission_request_cb (WebKitWebView           *web_view,
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
                    G_CALLBACK (dialog_geolocation_response_cb),
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

  cairo_surface_t *surface = webkit_web_view_get_favicon (window->web_view);

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
  if (window->fullscreen_message_label_id)
    g_source_remove (window->fullscreen_message_label_id);
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
                               default_window_height);

  window->uri_entry = gtk_entry_new ();
  gtk_entry_set_icon_activatable (GTK_ENTRY (window->uri_entry),
                                  GTK_ENTRY_ICON_PRIMARY, FALSE);
  update_uri_entry_icon (window);
  g_signal_connect_swapped (window->uri_entry, "activate",
                            G_CALLBACK (uri_entry_activate_cb),
                            window);

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
                            window);

  window->forward_item = GTK_WIDGET (
                          gtk_menu_tool_button_new_from_stock (
                            GTK_STOCK_GO_FORWARD));
  gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (window->forward_item),
                                 0);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar),
                      GTK_TOOL_ITEM (window->forward_item), -1);
  g_signal_connect_swapped (window->forward_item, "clicked",
                            G_CALLBACK (forward_item_clicked_cb),
                            window);

  GtkToolItem *item = gtk_tool_button_new_from_stock (
                        GTK_STOCK_PREFERENCES);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);
  g_signal_connect_swapperd (item, "clicked",
                             G_CALLBACK (settings_item_clicked_cb),
                             window);

  item = gtk_tool_item_new ();
  gtk_tool_item_set_expand (item, TRUE);
  gtk_container_add (GTK_CONTAINER (item), window->uri_entry);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);

  item = gtk_tool_button_new_from_stock (GTK_STOCK_OK);
  gtk_toolbar_insert (GTK_TOOLBAR (window->toolbar), item, -1);
  g_signal_connect_swapped (item, "clicked",
                            G_CALLBACK (ok_item_cb),
                            window);

  window->main_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_box_pack_start (GTK_BOX (window->main_box), window->toolbar,
                      FALSE, FALSE, 0);

  gtk_container_add (GTK_CONTAINER (window), window->main_box);
}

static void
wb_window_constructed (GObject *g_object)
{
  WbWindow *window = WB_WINDOW (g_object);

  g_signal_connect (window->web_view, "notify::uri",
                    G_CALLBACK (web_view_notify_uri_cb),
                    window);

  g_signal_connect (window->web_view, "notify::estimated-load-progress",
                    G_CALLBACK (web_view_notify_estimated_load_progress_cb),
                    window);

  g_signal_connect (window->web_view, "notify::title",
                    G_CALLBACK (web_view_notify_title_cb),
                    window);

  g_signal_connect (window->web_view, "notify::favicon",
                    G_CALLBACK (web_view_notify_favicon_cb),
                    window);

  g_signal_connect (window->web_view, "create",
                    G_CALLBACK (web_view_create_cb),
                    window);

  g_signal_connect (window->web_view, "load-failed",
                    G_CALLBACK (web_view_load_failed_cb),
                    window);

  g_signal_connect (window->web_view, "decide-policy",
                    G_CALLBACK (web_view_decide_policy_cb),
                    window);

  g_signal_connect (window->web_view, "premission-request",
                    G_CALLBACK (web_view_premission_request_cb),
                    window);

  g_signal_connect (window->web_view, "mouse-target-changed",
                    G_CALLBACK (web_view_mouse_target_changed_cb),
                    window);

  g_signal_connect (window->web_view, "enter-fullscreen",
                    G_CALLBACK (web_view_enter_fullscreen_cb),
                    window);

  g_signal_connect (window->web_view, "leave-fullscreen",
                    G_CALLBACK (web_view_leave_fullscreen_cb),
                    window);

  g_signal_connect (webkit_web_view_get_context (window->web_view),
                    "download-started",
                    G_CALLBACK (web_view_download_started_cb),
                    window);

  WebKitBackForwardList *back_forward_list =
    webkit_web_view_get_back_forward_list (window->web_view);
  g_signal_connect (back_forward_list, "changed",
                    G_CALLBACK (back_forward_list_changed_cb),
                    window);

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

  window->fullscreen_message_label = gtk_label_new (NULL);
  gtk_widget_set_halign (window->fullscreen_message_label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (window->fullscreen_message_label, GTK_ALIGN_CENTER);
  gtk_widget_set_no_show_all (window->fullscreen_message_label, TRUE);
  gtk_overlay_add_overlay (GTK_OVERLAY (overlay),
                           window->fullscreen_message_label);
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

  g_object_class->constructed = wb_window_constructed;
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
                                        G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget*
wb_window_new (WebKitWebView *web_view, GtkWindow *parent)
{
  g_return_val_if_fail (WEBKIT_IS_WEB_VIEW (web_view), 0);

  return GTK_WIDGET (g_object_new (WB_TYPE_WINDOW,
                                   "transient-for", parent,
                                   "type", GTK_WINDOW_TOPLEVEL,
                                   "view", web_view, NULL));
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
