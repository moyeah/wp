#include <webkit2/webkit2.h>

#include "wb_settings_dialog.h"

enum
{
  PROP_0,

  PROP_SETTINGS
};

enum
{
  SETTINGS_LIST_COLUMN_NAME,
  SETTINGS_LIST_COLUMN_NICK,
  SETTINGS_LIST_COLUMN_BLURB,
  SETTINGS_LIST_COLUMN_VALUE,
  SETTINGS_LIST_COLUMN_ADJUSTMENT,

  SETTINGS_LIST_N_COLUMNS
};

struct _WbSettingsDialog
{
  GtkDialog parent;

  GtkWidget *settings_list;

  WebKitSettings *settings;
};

struct _WbSettingsDialogClass
{
  GtkDialogClass parent;
};

static const gchar *default_dialog_title = "WB - Settings";
static const gint default_dialog_width = 800;
static const gint default_dialog_height = 600;

G_DEFINE_TYPE (WbSettingsDialog, wb_settings_dialog, GTK_TYPE_DIALOG)

static void
renderer_changed_cb (GtkCellRenderer  *renderer,
                     const char       *path,
                     const GValue     *value,
                     WbSettingsDialog *dialog)
{
  GtkTreeModel *model =
    gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->settings_list));
  GtkTreePath *tree_path = gtk_tree_path_new_from_string (path);
  GtkTreeIter *iter;

  gtk_tree_model_get_iter (model, &iter, tree_path);

  char *name;
  gtk_tree_model_get (model, &iter, SETTINGS_LIST_COLUMN_NAME, &name, -1);
  g_object_set_property (G_OBJECT (dialog->settings), name, value);
  g_free (name);

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      SETTINGS_LIST_COLUMN_VALUE, value, -1);
  gtk_tree_path_free (tree_path);
}

static void
wb_settings_dialog_init (WbSettingsDialog *dialog)
{
  GtkBox *content_area =
    GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog)));
  gtk_box_set_spacing (content_area, 2);

  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               default_dialog_width,
                               default_dialog_height);
  gtk_window_set_title (GTK_WINDOW (dialog), default_dialog_title);
  gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
  gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE,
                         GTK_RESPONSE_CLOSE);
  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  GtkWidget *scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  dialog->settings_list = gtk_tree_view_new ();

  GtkCellRenderer *renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_insert_column_with_attributes (
    GTK_TREE_VIEW (dialog->settings_list),
    0, "Name", renderer,
    "text", SETTINGS_LIST_COLUMN_NICK,
    NULL);

  renderer = wb_cell_renderer_new ();
  gtk_tree_view_insert_column_with_attributes (
    GTK_TREE_VIEW (dialog->settings_list),
    1, "Value", renderer,
    "value", SETTINGS_LIST_COLUMN_VALUE,
    "adjustment", SETTINGS_LIST_COLUMN_ADJUSTMENT,
    NULL);
  g_signal_connect (renderer, "changed",
                    G_CALLBACK (renderer_changed_cb),
                    dialog);

  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (dialog->settings_list),
                                    SETTINGS_LIST_COLUMN_BLURB);

  gtk_container_add (GTK_CONTAINER (scrolled_window), dialog->settings_list);
  gtk_box_pack_start (content_area, scrolled_window, TRUE, TRUE, 0);

  g_signal_connect (dialog, "response",
                    G_CALLBACK (gtk_widget_destroy), NULL);
}

static void
wb_settings_dialog_constructed (GObject *object)
{
  WbSettingsDialog *dialog = WB_SETTINGS_DIALOG (object);
  WebKitSettings *settings = dialog->settings;
  GtkListStore *model = gtk_list_store_new (SETTINGS_LIST_N_COLUMNS,
                                            G_TYPE_STRING, G_TYPE_STRING,
                                            G_TYPE_STRING, G_TYPE_VALUE,
                                            G_TYPE_OBJECT);

  guint properties_count;
  GParamSpec **properties = g_object_class_list_properties (
                              G_OBJECT_GET_CLASS (settings),
                              &properties_count);

  guint i;
  for (i = 0; i < properties_count; i++)
  {
    GParamSpec *property = properties[i];
    const char *name = g_param_spec_get_name (property);
    const char *nick = g_param_spec_get_nick (property);

    GValue value = {0, { { 0 } } };
    g_value_init (&value, G_PARAM_SPEC_VALUE_TYPE (property));
    g_object_get_property (G_OBJECT (settings), name, &value);

    GtkAdjustment *adjustment = NULL;
    if (G_PARAM_SPEC_VALUE_TYPE (property) == G_TYPE_UINT)
    {
      GParamSpecUInt *uint_property = G_PARAM_SPEC_UINT (property);
      adjustment = gtk_adjustment_new (uint_property->default_value,
                                       uint_property->minimum,
                                       uint_property->maximum,
                                       1, 1, 1);
    }

    char *blurb = g_markup_escape_text (g_param_spec_get_blurv (property), -1);
    GtkTreeIter iter;
    gtk_list_store_append (model, &iter);
    gtk_list_store_set (model, &iter,
                       SETTINGS_LIST_COLUMN_NAME, name,
                       SETTINGS_LIST_COLUMN_NICK, nick,
                       SETTINGS_LIST_COLUMN_BLURB, blurb,
                       SETTINGS_LIST_COLUMN_VALUE, &value,
                       SETTINGS_LIST_COLUMN_ADJUSTMENT, adjustment,
                       -1);
    g_free (blurb);
    g_value_unset (&value);
  }
  g_free (properties);

  gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->settings_list),
                           GTK_TREE_MODEL (model));
  g_object_unref (model);
}

static void
wb_settings_dialog_set_property (GObject      *object, guint       prop_id,
                                 const GValue *value,  GParamSpec *pspec)
{
  WbSettingsDialog *dialog = WB_SETTINGS_DIALOG (object);

  switch (prop_id)
  {
    case PROP_SETTINGS:
      dialog->settings = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
wb_settings_dialog_class_init (WbSettingsDialogClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->constructed = wb_settings_dialog_constructed;
  g_object_class->set_property = wb_settings_dialog_set_property;

  g_object_class_install_property (g_object_class,
                                   PROP_SETTINGS,
                                   g_param_spec_obejct (
                                     "settings",
                                     "Settings",
                                     "The WebKitSettings",
                                     WEBKIT_TYPE_SETTINGS,
                                     G_PARAM_WRITABLE ||
                                       G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget*
wb_settings_dialog_new (WebKitSettings *settings)
{
  g_return_val_if_fail (WEBKIT_IS_SETTINGS (settings), NULL);

  return GTK_WIDGET (g_object_new (WB_SETTINGS_DIALOG,
                                   "settings", settings,
                                   NULL));
}
