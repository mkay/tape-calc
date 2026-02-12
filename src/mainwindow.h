#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "calculator_engine.h"
#include <gtkmm.h>

class MainWindow : public Gtk::Window
{
public:
  MainWindow(const Glib::RefPtr<Gtk::Application>& app);
  ~MainWindow() override;

protected:
  // Application reference
  Glib::RefPtr<Gtk::Application> m_app;

  // Calculator engine
  CalculatorEngine m_engine;

  // Main layout
  Gtk::Box m_outer_box;  // Outer box to contain menu bar and main content
  Gtk::Box m_main_box;
  Gtk::Box m_left_panel;
  Gtk::Box m_right_panel;
  Gtk::Box m_history_title_box;
  Gtk::Label m_history_title;

  // Display area
  Gtk::ScrolledWindow m_tape_scroll;
  Gtk::TextView m_tape_view;
  Glib::RefPtr<Gtk::TextBuffer> m_tape_buffer;
  Gtk::Label m_running_total_label;
  Gtk::Label m_subtotal_label;
  Gtk::Label m_result_label;
  Gtk::Button m_edit_tape_button;

  // Control area
  Gtk::Box m_control_box;
  Gtk::Label m_decimal_label;
  Gtk::SpinButton m_decimal_places_spin;
  Gtk::Label m_vat_label;
  Gtk::SpinButton m_vat_rate_spin;

  // VAT buttons
  Gtk::Box m_vat_box;
  Gtk::Button m_vat_rate_btn;
  Gtk::Button m_add_vat_btn;
  Gtk::Button m_subtract_vat_btn;

  // Calculator button grid
  Gtk::Grid m_button_grid;

  // CSS Provider
  Glib::RefPtr<Gtk::CssProvider> m_css_provider;

  // Constants
  const Glib::ustring APPNAME;
  const int WIDTH;
  const int HEIGHT;

  // Signal handlers
  void on_number_clicked(int digit);
  void on_operation_clicked(char op);
  void on_equals_clicked();
  void on_clear_clicked();
  void on_backspace_clicked();
  void on_decimal_clicked();
  void on_percent_clicked();
  void on_add_vat_clicked();
  void on_subtract_vat_clicked();
  void on_vat_rate_changed();
  void on_decimal_places_changed();
  void on_save_tape_clicked();
  void on_edit_tape_clicked();

  // Helper methods
  void update_displays();
  void update_tape();
  void setup_css();
  void create_button(const Glib::ustring& label, int row, int col, int width = 1);
  void create_number_button(int number, int row, int col);
  void create_operation_button(const Glib::ustring& label, char op, int row, int col);

  // Menu setup
  void setup_menu();
  void update_edit_menu_sensitivity();
  void update_recent_files_menu();

  // Recent files management
  void add_recent_file(const std::string& path);
  void load_recent_files();
  void save_recent_files();
  void clear_recent_files();

  // Menu action handlers
  void on_action_edit_mode();
  void on_action_new();
  void on_action_open();
  void on_action_open_recent(const std::string& file_path);
  void on_action_save();
  void on_action_save_as();
  void on_action_new_window();
  void on_action_quit();
  void on_action_cut();
  void on_action_copy();
  void on_action_paste();
  void on_action_select_all();
  void on_action_copy_total();
  void on_action_documentation();

  // Settings methods
  void load_settings();
  void save_settings();
  std::string get_config_path();

  // Keyboard handler
  bool on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state);

  // File state management
  void set_modified(bool modified);
  void update_window_title();
  bool check_unsaved_changes();  // Returns false if user cancels
  bool save_to_file(const std::string& file_path);

  // Window close handler
  bool on_close_request() override;

private:
  bool m_updating_tape;
  bool m_tape_edit_mode;
  std::vector<std::string> m_recent_files;
  Glib::RefPtr<Gio::Menu> m_recent_files_menu;

  // File state tracking
  bool m_is_modified;
  std::string m_current_file_path;
};

#endif
