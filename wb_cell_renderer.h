#ifndef __WB_CELL_RENDERER_H__
#define __WB_CELL_RENDERER_H__

G_BEGIN_DECLS

#define WB_TYPE_CELL_RENDERER         (wb_cell_renderer_get_type ())
#define WB_CELL_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_CAST    ((obj),   WB_TYPE_CELL_RENDERER, WbCellRenderer))
#define WB_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST       ((klass), WB_TYPE_CELL_RENDERER, WbCellRendererClass))
#define WB_IS_CELL_RENDERER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj),   WB_TYPE_CELL_RENDERER))
#define WB_IS_CELL_RENDERER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE    ((klass), WB_TYPE_CELL_RENDERER))
#define WB_CELL_RENDERER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS  ((obj),   WB_TYPE_CELL_RENDERER, WbCellRendererClass))

typedef struct _WbCellRenderer      WbCellRenderer;
typedef struct _WbCellRendererClass WbCellRendererClass;

GType            wb_cell_renderer_get_type (void) G_GNUC_CONST;
GtkCellRenderer* wb_cell_renderer_new      (void);

G_END_DECLS

#endif /* __WB_CELL_RENDERER_H__ */
