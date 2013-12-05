import pygtk
pygtk.require('2.0')
import gtk
from matplotlib.figure import Figure as Fig
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigCan
from matplotlib.backends.backend_gtkagg import NavigationToolbar2GTKAgg as NavBar
from matplotlib.backend_bases import key_press_handler

class Graph(gtk.Window):
  def on_key_event(event):
    key_press_handler(event, canvas, toolbar)

  def __init__(self, x, y, title="Graph", parent=None):
    gtk.Window.__init__(self)
    try:
      self.set_screen(parent.get_screen())
    except:
      self.connect('destroy', lambda *w: self.destroy())

    self.set_title(title)
    self.set_default_size(800, 600)
    self.set_border_width(2)
    self.set_position(gtk.WIN_POS_CENTER)

    box = gtk.VBox()
    self.add(box)

    fig = Fig(figsize=(5,4), dpi=100)
    ax = fig.add_subplot(111)
    ax.plot(x, y)

    canvas = FigCan(fig)
    canvas.mpl_connect('key_press_event', self.on_key_event)
    box.pack_start(canvas)

    toolbar = NavBar(canvas, self)
    box.pack_start(toolbar, False, False)

    self.show_all()

    del box, fig, ax, canvas, toolbar
