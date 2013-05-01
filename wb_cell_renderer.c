#include <errno.h>
#include <webkit2/webkit2.h>

#include "wb_cell_renderer.h"

enum
{
  PROP_0,
  PROP_VALUE,
  PROP_ADJUSTMENT
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

struct _WbCellRenderer
{
  GtkCellRenderer parent;

  GValue *value;

  GtkCellRenderer *text;
  GtkCellRenderer *toggle;
  GtkCellRenderer *spin;
};

struct _WbCellRendererClass
{
  GtkCellRendererClass parent;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (WbCellRenderer, wb_cell_renderer, GTK_TYPE_CELL_RENDERER)

static void
cell_renderer_text_edited_cb (WbCellRenderer *renderer,
                              const gchar    *path,
                              const gchar    *new_text)
{
  if (!renderer->value)
    return;

  if (!G_VALUE_HOLDS_STRING (renderer->value))
    return;

  g_value_set_string (renderer->value, new_text);
  g_signal_emit (renderer, signals[CHANGED], 0, path, renderer->value);
}

static void
cell_renderer_spin_edited_cb (WbCellRenderer *renderer,
                              const gchar    *path,
                              const gchar    *new_path,
                              const gchar    *new_text)
{
  if (!renderer->value)
    return;

  if (!G_VALUE_HOLDS_UINT (renderer->value))
    return;

  GtkAdjustment *adjustment;
  g_object_get (G_OBJECT (renderer->spin), "adjustment",
                          &adjustment, NULL);

  if (!adjustment)
    return;

  errno = 0;
  gchar *end_ptr;
  gdouble value = g_strod (new_text, &end_ptr);

  if (errno || value > gtk_adjustment_get_upper (adjustment) ||
        value < gtk_adjustment_get_lower (adjustment) || end_ptr == new_text)
  {
    g_warning ("Invalid input for cell: %s\n", new_text);
    return;
  }

  g_value_set_uint (renderer->value, (guint) value);
  g_signal_emit (renderer, signals[CHANGED], 0, path, renderer->value);
}

static void
wb_cell_renderer_init (WbCellRenderer *renderer)
{
  g_object_set (renderer, "mode",
                GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);

  renderer->toggle = gtk_cell_renderer_toggle_new ();
  g_object_set (G_OBJECT (renderer->toggle),
                "xalign", 0.0, NULL);
  g_object_ref_sink (renderer->toggle);

  renderer->text = gtk_cell_renderer_text_new ();
  g_object_set (G_OBJECT (renderer->text),
                "editable", TRUE, NULL);
  g_object_ref_sink (renderer->text);
  g_signal_connect (renderer->text, "edited",
                    G_CALLBACK (cell_renderer_text_edited_cb),
                    renderer);

  renderer->spin = gtk_cell_renderer_spin_new ();
  g_object_set (G_OBJECT (renderer->spin), "editable", TRUE, NULL);
  g_sinal_connect_swapped (renderer->spin, "edited",
                           G_CALLBACK (cell_renderer_spin_edited_cb),
                           renderer);
}

static void
wb_cell_renderer_get_property (GObject *object, guint       prop_id,
                               GValue  *value,  GParamSpec *pspec)
{
  WbCellRenderer *renderer = WB_CELL_RENDERER (object);

  switch (prop_id)
  {
    case PROP_VALUE:
      g_value_set_boxed (value, renderer->value);
      break;
    case PROP_ADJUSTMENT:
    {
      GtkAdjustment *adjustment = NULL;
      g_object_get (G_OBJECT (renderer->spin), "adjustment",
                    &adjustment, NULL);

      if (adjustment)
      {
        g_value_set_object (value, adjustment);
        g_object_unref (adjustment);
      }
      break;
    }
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
wb_cell_renderer_set_mode_for_value (WbCellRenderer *renderer)
{
  if (!renderer->value)
    return;

  GtkCellRendererMode mode;
  if (G_VALUE_HOLDS_BOOLEAN (renderer->value))
    mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
  else if (G_VALUE_HOLDS_STRING (renderer->value) ||
             G_VALUE_HOLDS_UINT (renderer->value))
    mode = GTK_CELL_RENDERER_MODE_EDITABLE;
  else
    return;

  g_object_set (G_OBJECT (renderer), "mode", mode, NULL);
}

static void
wb_cell_renderer_set_property (GObject      *object, guint       prop_id,
                               const GValue *value,  GParamSpec *pspec)
{
  WbCellRenderer *renderer = WB_CELL_RENDERER (object);

  switch (prop_id)
  {
    case PROP_VALUE:
      if (renderer->value)
        g_boxed_free (G_TYPE_VALUE, renderer->value);
      renderer->value = g_value_dup_boxed (value);
      wb_cell_renderer_set_mode_for_value (renderer);
      break;
    case PROP_ADJUSTMENT:
      g_object_set (G_OBJECT (renderer->spin), "adjustment",
                    g_value_get_object (value), NULL);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
wb_cell_renderer_finalize (GObject *object)
{
  WbCellRenderer *renderer = WB_CELL_RENDERER (object);

  g_object_unref (renderer->toggle);
  g_object_unref (renderer->spin);
  g_object_unref (renderer->text);

  if (renderer->value)
    g_boxed_free (G_TYPE_VALUE, renderer->value);

  G_OBJECT_CLASS (wb_cell_renderer_parent_class)->finalize (object);
}

static gboolean
wb_cell_renderer_activate (GtkCellRenderer      *cell,
                           GdkEvent             *event,
                           GtkWidget            *widget,
                           const gchar          *path,
                           const GdkRectangle   *bg_area, 
                           const GdkRectangle   *cell_area,
                           GtkCellRendererState  flags)
{
  WbCellRenderer *renderer = WB_CELL_RENDERER (cell);

  if (!renderer->value)
    return TRUE;

  if (!G_VALUE_HOLDS_BOOLEAN (renderer->value))
    return TRUE;

  g_value_set_boolean (renderer->value,
                       !g_value_get_boolean (renderer->value));
  g_signal_emit (renderer, signals[CHANGED], 0, path, renderer->value);

  return TRUE;
}

static GtkCellRenderer*
wb_cell_renderer_get_renderer_for_value (WbCellRenderer *renderer)
{
  if (!renderer->value)
    return NULL;

  if (G_VALUE_HOLDS_BOOLEAN (renderer->value))
  {
    g_object_set (G_OBJECT (renderer->toggle),
                  "active", g_value_get_string (renderer->value),
                  NULL);
    return renderer->toggle;
  }

  if (G_VALUE_HOLDS_STRING (renderer->value))
  {
    g_object_set (G_OBJECT (renderer->text),
                  "text", g_value_get_string (renderer->value),
                  NULL);
    return renderer->text;
  }

  if (G_VALUE_HOLDS_UINT (renderer->value))
  {
    gchar *text = g_strdup_printf ("%u", g_value_get_uint (renderer->value));
    g_object_set (G_OBJECT (renderer->spin), "text", text, NULL);
    g_free (text);
    return renderer->spin;
  }

  return NULL;
}

static void
wb_cell_renderer_render (GtkCellRenderer      *cell,
                         cairo_t              *cr,
                         GtkWidget            *widget,
                         const GdkRectangle   *bg_area,
                         const GdkRectangle   *cell_area,
                         GtkCellRendererState  flags)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->render (renderer,  cr,
                                                  widget,    bg_area,
                                                  cell_area, flags);
}

static GtkCellEditable*
wb_cell_renderer_start_editing (GtkCellRenderer      *cell,
                                GdkEvent             *event,
                                GtkWidget            *widget,
                                const gchar          *path,
                                const GdkRectangle   *bg_area,
                                const GdkRectangle   *cell_area,
                                GtkCellRendererState  flags)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return NULL;

  if (!GTK_CELL_RENDERER_GET_CLASS (renderer)->start_editing)
    return NULL;

  return GTK_CELL_RENDERER_GET_CLASS (renderer)->start_editing (renderer,
                                                                event,
                                                                widget,
                                                                path,
                                                                bg_area,
                                                                cell_area,
                                                                flags);
}

static void
wb_cell_renderer_get_preferred_width (GtkCellRenderer *cell,
                                      GtkWidget       *widget,
                                      gint            *minimum,
                                      gint            *natural)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->get_preferred_width (renderer,
                                                               widget,
                                                               minimum,
                                                               natural);
}

static void
wb_cell_renderer_get_preferred_height (GtkCellRenderer *cell,
                                       GtkWidget       *widget,
                                       gint            *minimum,
                                       gint            *natural)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->get_preferred_width (renderer,
                                                               widget,
                                                               minimum,
                                                               natural);
}

static void
wb_cell_renderer_get_preferred_width_for_height (GtkCellRenderer *cell,
                                                 GtkWidget       *widget,
                                                 gint             height,
                                                 gint            *minimum,
                                                 gint            *natural)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->get_preferred_width_for_height (
                                            renderer,
                                            widget,
                                            height,
                                            minimum,
                                            natural);
}

static void
wb_cell_renderer_get_preferred_height_for_width (GtkCellRenderer *cell,
                                                 GtkWidget       *widget,
                                                 gint             width,
                                                 gint            *minimum,
                                                 gint            *natural)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->get_preferred_height_for_width (
                                            renderer,
                                            widget,
                                            width,
                                            minimum,
                                            natural);
}

static void
wb_cell_renderer_get_aligned_area (GtkCellRenderer      *cell,
                                   GtkWidget            *widget,
                                   GtkCellRendererState  flags,
                                   const GdkRectangle   *cell_area,
                                   GdkRectangle         *aligned_area)
{
  GtkCellRenderer *renderer = wb_cell_renderer_get_renderer_for_value (
                                WB_CELL_RENDERER (cell));

  if (!renderer)
    return;

  GTK_CELL_RENDERER_GET_CLASS (renderer)->get_aligned_area (renderer,
                                                            widget,
                                                            flags,
                                                            cell_area,
                                                            aligned_area);
}


static void
wb_cell_renderer_class_init (WbCellRendererClass *klass)
{
  GObjectClass *g_object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS (klass);

  g_object_class->get_property = wb_cell_renderer_get_property;
  g_object_class->set_property = wb_cell_renderer_set_property;
  g_object_class->finalize = wb_cell_renderer_finalize;

  renderer_class->activate = wb_cell_renderer_activate;
  renderer_class->render = wb_cell_renderer_render;
  renderer_class->start_editing = wb_cell_renderer_start_editing;
  renderer_class->get_preferred_width = wb_cell_renderer_get_preferred_width;
  renderer_class->get_preferred_height = wb_cell_renderer_get_preferred_height;
  renderer_class->get_preferred_width_for_height =
    wb_cell_renderer_get_preferred_width_for_height;
  renderer_class->get_preferred_height_for_width =
    wb_cell_renderer_get_preferred_height_for_width;
  renderer_class->get_aligned_area = wb_cell_renderer_get_aligned_area;

  g_object_class_install_property (g_object_class,
                                   PROP_VALUE,
                                   g_param_spec_boxed (
                                     "value",
                                     "Value",
                                     "The cell renderer value",
                                     G_TYPE_VALUE,
                                     G_PARAM_READWRITE));

  g_object_class_install_property (g_object_class,
                                   PROP_ADJUSTMENT,
                                   g_param_spec_boxed (
                                     "adjustment",
                                     "Adjustment",
                                     "The adjustment of spin button",
                                     GTK_TYPE_ADJUSTMENT,
                                     G_PARAM_READWRITE));

  signals[CHANGED] =
    g_signal_new ("changed",
                  G_TYPE_FROM_CLASS (g_object_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  wb_marshall_VOID_STRING_BOXED,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING, G_TYPE_VALUE);
}

GtkCellRenderer*
wb_cell_renderer_new (void)
{
  return GTK_CELL_RENDERER (g_object_new (WB_TYPE_CELL_RENDERER, NULL));
}
