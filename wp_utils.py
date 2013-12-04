import pygtk
pygtk.require('2.0')
import gobject
import gtk
import itertools
from math import *

class Frame(gtk.Frame):
    def __init__(self, title=None):
      gtk.Frame.__init__(self)

      label = gtk.Label()
      label.set_markup(str("<b>" + title + "</b>"))

      self.set_label_widget(label)

      del label

class Entry(gtk.Entry):
    def set_warning(self, warning=True):
      if(warning):
        self.set_icon_from_stock(0, gtk.STOCK_DIALOG_WARNING)
        return
      self.set_icon_from_stock(0, None)

    def get_abs_value(self):
      try:
        value = fabs(float(eval(self.get_text())))
      except (SyntaxError, NameError, TypeError, ValueError):
        value = False 
      return value

    def set_abs_value(self):
      if(self.isValueOK()):
        self.set_warning(False)
        self.set_text(str(self.get_abs_value()))
        return
      self.set_warning()

    def isValueOK(self):
      if(self.get_text_length() < 1):
        return False
      if(not self.get_abs_value()):
        return False
      return True

    def on_focus_out_event(self, widget, event):
      self.set_abs_value()

    def on_activate(self, widget):
      self.set_abs_value()

    def __init__(self, label_with_mnemonic, table, left_attach=0, top_attach=0):
      gtk.Entry.__init__(self)

      label = gtk.Label()
      label.set_alignment(xalign=0.0, yalign=0.5)
      label.set_text_with_mnemonic(label_with_mnemonic)
      label.set_mnemonic_widget(self)
      table.attach(label,
                   left_attach, left_attach + 1,
                   top_attach, top_attach + 1,
                   xoptions=gtk.FILL)

      table.attach(self,
                   left_attach + 2, left_attach + 3,
                   top_attach, top_attach + 1)
      self.connect('focus-out-event', self.on_focus_out_event)
      self.connect('activate', self.on_activate)

      del label

class EditableTreeView(gtk.ScrolledWindow):
    def __init__(self, columns, columnsEnum):
      gtk.ScrolledWindow.__init__(self)
      self.set_policy(gtk.POLICY_AUTOMATIC, gtk.POLICY_AUTOMATIC)

      model = self.__create_model()

      treeview = gtk.TreeView(model)
      self.add(treeview)

      self.__add_columns(treeview, columns, columnsEnum)

      del model, treeview

    def __create_model(self):
      return gtk.ListStore(gobject.TYPE_STRING,
                           gobject.TYPE_STRING)

    def __add_columns(self, treeview, columns, columnsEnum):
      model = treeview.get_model()

      for column, columnEnum in itertools.izip(columns, columnsEnum):
        renderer = gtk.CellRendererText()
        renderer.set_alignment(1, 0.5)
        renderer.set_data("column", columnEnum)

        column = gtk.TreeViewColumn(column, renderer, text=columnEnum, editable=True)
        column.set_resizable(True)
        column.set_alignment(0.5)

        treeview.append_column(column)

      del model, renderer, column
