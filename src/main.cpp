#include <gtkmm/application.h>
#include "mainwindow.h"

int main (int argc, char *argv[]) {
  auto app = Gtk::Application::create("mishima.tape-calc", Gio::Application::Flags::NON_UNIQUE);

  app->signal_activate().connect([&app]() {
    auto window = new MainWindow();
    app->add_window(*window);
    window->set_hide_on_close(false);
    window->present();
  });

  return app->run(argc, argv);
}
