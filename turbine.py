import pygtk
pygtk.require('2.0')
import gobject
import gtk
import itertools
from math import *

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
  def on_cell_edited(self, cell, path_string, new_text, model):
    iter = model.get_iter_from_string(path_string)
    path = model.get_path(iter)[0]
    column = cell.get_data("column")

    try:
      value = fabs(float(eval(new_text)))
      model.set(iter, column, str(value))
      del value
    except (SyntaxError, NameError, TypeError, ValueError):
      model.set(iter, column, new_text)

    del iter, path, column

  def on_add_item_clicked(self, widget, treeView):
    treeView.append_item()

  def on_remove_item_clicked(self, widget, treeView):
    treeView.remove_item()

  def __init__(self):
    gtk.VBox.__init__(self)
    self.set_border_width(5)
    self.set_spacing(5)

    columnsEnum = (COLUMN_SPEED, COLUMN_POWER) = range(2)
    treeView = wp.EditableTreeView(["u [m/s]", "P(u) [kW]"],
                                   columnsEnum,
                                   self.on_cell_edited)
    self.pack_start(treeView)

    buttonBox = wp.HButtonBox(stocks=[gtk.STOCK_ADD, gtk.STOCK_REMOVE],
                              signals=[self.on_add_item_clicked,
                                       self.on_remove_item_clicked],
                              data=treeView)
    self.pack_start(buttonBox, False, False)

    del buttonBox, columnsEnum, treeView

class Turbine(gtk.VBox):
  def on_plot_clicked(self, widget):
    pass

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

    buttonBox = wp.HButtonBox(labels=["_Plot Power Graph"],
                              signals=[self.on_plot_clicked])
    box.set_border_width(5)
    box.pack_start(buttonBox, False, False)

    del frame, box, titles, frames, mainFrame, buttonBox
