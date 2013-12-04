import pygtk
pygtk.require('2.0')
import gobject
import gtk
import itertools

import wp_utils as wp

class Speed(gtk.Table):
  def __init__(self):
    gtk.Table.__init__(self)
    self.set_border_width(5)
    self.set_row_spacings(5)
    self.set_col_spacings(5)

    entries = ["Cut-_in (uin) [m/s]",
               "Cut-_out (uout) [m/s]"]
    ta = 0
    for entry in entries:
      wp.Entry(entry, self, top_attach=ta)
      ta += 1

    del entries

class Power(gtk.VBox):
  def on_add_item_clicked(self, widget, treeView):
    treeView.append_item()

  def on_remove_item_clicked(self, widget, treeView):
    treeView.remove_item()

  def __init__(self):
    gtk.VBox.__init__(self)
    self.set_border_width(5)
    self.set_spacing(5)

    buttonBox = gtk.HButtonBox()
    buttonBox.set_layout(gtk.BUTTONBOX_END)
    buttonBox.set_spacing(5)
    self.pack_start(buttonBox, False, False)

    columns = ["u [m/s]", "P(u) [kW]"]
    columnsEnum = (COLUMN_SPEED, COLUMN_POWER) = range(2)
    treeView = wp.EditableTreeView(columns, columnsEnum)
    self.pack_start(treeView)

    button = gtk.Button(stock=gtk.STOCK_ADD)
    button.connect("clicked", self.on_add_item_clicked, treeView)
    buttonBox.add(button)

    button = gtk.Button(stock=gtk.STOCK_REMOVE)
    button.connect("clicked", self.on_remove_item_clicked, treeView)
    buttonBox.add(button)

    del buttonBox, button, columns, columnsEnum, treeView

class Turbine(gtk.VBox):
  def __init__(self):
    gtk.VBox.__init__(self) 

    frame = wp.Frame("Turbine")
    self.pack_start(frame)

    box = gtk.VBox()
    frame.add(box)

    titles = ["Speed", "Power"]
    frames = [Speed(), Power()]
    for title, frame in itertools.izip(titles, frames):
      mainFrame = wp.Frame(title)
      mainFrame.set_border_width(5)
      mainFrame.add(frame)
      if(title == "Power"):
        box.pack_start(mainFrame)
      else:
        box.pack_start(mainFrame, False, False)

    del frame, box, titles, frames, mainFrame
