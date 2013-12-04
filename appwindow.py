#!/usr/bin/env python

import pygtk
pygtk.require('2.0')
import gobject
import gtk

import turbine as t

class ApplicationMainWindow(gtk.Window):
  def on_close(self, widget):
    gtk.main_quit()

  def __init__(self, title="Wind Power", parent=None):
    gtk.Window.__init__(self)
    try:
      self.set_screen(parent.get_screen())
    except AttributeError:
      self.connect('destroy', lambda *w: gtk.main_quit())

    self.set_title(title)
    self.set_default_size(800, 600)
    self.set_border_width(10)

    self.__main_vbox = gtk.VBox()
    self.add(self.__main_vbox)

    self.__contents_area = gtk.VBox()
    self.__main_vbox.pack_start(self.__contents_area)

    turbine = t.Turbine()
    self.__contents_area.pack_start(turbine)

    self.__action_area = gtk.HButtonBox()
    self.__action_area.set_layout(gtk.BUTTONBOX_END)
    self.__action_area.set_spacing(5)
    self.__main_vbox.pack_end(self.__action_area, False, False)

    button = gtk.Button(stock='gtk-close')
    button.connect('clicked', lambda *w: gtk.main_quit())
    self.__action_area.add(button)
    del button

    self.show_all()

  def get_main_vbox(self):
    return self.__main_vbox

  def get_contents_area(self):
    return self.__contents_area

  def get_action_area(self):
    return self.__action_area

  def main(self):
    gtk.main()

if __name__ == '__main__':
  ApplicationMainWindow().main()
