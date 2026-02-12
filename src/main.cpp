#include <gtkmm/application.h>
#include "mainwindow.h"

int main (int argc, char *argv[]) {
  auto app = Gtk::Application::create("mishima.tape-calc", Gio::Application::Flags::HANDLES_OPEN);

  // Handle launching without files
  app->signal_activate().connect([&app]() {
    auto window = new MainWindow(app);
    app->add_window(*window);
    window->set_hide_on_close(false);
    window->present();
  });

  // Handle opening files from command line or file manager
  app->signal_open().connect([&app](const std::vector<Glib::RefPtr<Gio::File>>& files, const Glib::ustring& hint) {
    auto window = new MainWindow(app);
    app->add_window(*window);
    window->set_hide_on_close(false);
    window->present();

    // Load the first file (if multiple files are passed, only open the first one)
    if (!files.empty()) {
      window->load_file(files[0]->get_path());
    }
  });

  return app->run(argc, argv);
}
