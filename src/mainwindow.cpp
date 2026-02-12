#include "mainwindow.h"
#include <sigc++/sigc++.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <locale>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <ctime>
#include <gdk/gdkkeysyms.h>

MainWindow::MainWindow(const Glib::RefPtr<Gtk::Application>& app)
    : m_app(app)
    , m_outer_box(Gtk::Orientation::VERTICAL)
    , m_main_box(Gtk::Orientation::HORIZONTAL)
    , m_left_panel(Gtk::Orientation::VERTICAL)
    , m_right_panel(Gtk::Orientation::VERTICAL)
    , m_history_title_box(Gtk::Orientation::HORIZONTAL)
    , m_history_title("Calculation History")
    , m_control_box(Gtk::Orientation::HORIZONTAL)
    , m_vat_box(Gtk::Orientation::HORIZONTAL)
    , m_vat_rate_btn("19%")
    , m_add_vat_btn("+VAT")
    , m_subtract_vat_btn("-VAT")
    , APPNAME("Tape Calculator")
    , WIDTH(700)
    , HEIGHT(420)
    , m_updating_tape(false)
    , m_tape_edit_mode(false)
{
    set_title(APPNAME);
    set_default_size(WIDTH, HEIGHT);

    // Setup menu actions
    setup_menu();

    // Setup CSS styling
    setup_css();

    // Configure history title
    m_history_title.set_halign(Gtk::Align::START);
    m_history_title.set_margin(15);
    m_history_title.set_hexpand(true);
    m_history_title.add_css_class("history-title");

    // Configure tape view
    m_tape_buffer = Gtk::TextBuffer::create();
    m_tape_view.set_buffer(m_tape_buffer);
    m_tape_view.set_editable(false);  // Read-only (not editable by default)
    m_tape_view.set_cursor_visible(true);  // Show cursor for text selection
    m_tape_view.set_can_focus(true);  // Allow focus for text selection
    m_tape_view.set_wrap_mode(Gtk::WrapMode::WORD);
    m_tape_view.set_monospace(true);
    m_tape_view.set_right_margin(20);
    m_tape_view.set_left_margin(20);
    m_tape_view.set_top_margin(10);
    m_tape_view.set_bottom_margin(10);

    // Create text tag for red colored text (last value before equals)
    m_tape_buffer->create_tag("red-text")->property_foreground() = "#D86A35";

    // Configure edit tape button (styled as text link)
    m_edit_tape_button.set_label("EDIT");
    m_edit_tape_button.set_margin_end(15);
    m_edit_tape_button.set_valign(Gtk::Align::CENTER);
    m_edit_tape_button.set_visible(false);  // Hidden until calculation is complete
    m_edit_tape_button.add_css_class("edit-button");
    m_edit_tape_button.signal_clicked().connect(
        sigc::mem_fun(*this, &MainWindow::on_edit_tape_clicked));

    // Configure history title box (title + edit button)
    m_history_title_box.append(m_history_title);
    m_history_title_box.append(m_edit_tape_button);
    m_history_title_box.set_spacing(10);

    // Configure scrolled window for tape
    m_tape_scroll.set_child(m_tape_view);
    m_tape_scroll.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    m_tape_scroll.set_vexpand(true);

    // Configure display labels
    m_running_total_label.set_halign(Gtk::Align::END);
    m_running_total_label.set_margin(5);
    m_running_total_label.add_css_class("running-total");
    m_running_total_label.set_visible(false); // Hide running total, will show in tape

    m_subtotal_label.set_halign(Gtk::Align::END);
    m_subtotal_label.set_margin(5);
    m_subtotal_label.add_css_class("subtotal");
    m_subtotal_label.set_visible(false); // Hide VAT subtotal display

    m_result_label.set_halign(Gtk::Align::END);
    m_result_label.set_hexpand(true);
    m_result_label.set_margin_start(20);
    m_result_label.set_margin_end(20);
    m_result_label.set_margin_top(10);
    m_result_label.set_margin_bottom(10);
    m_result_label.add_css_class("result-display");
    m_result_label.set_visible(false); // Initially hidden

    // Configure control box (decimal places and VAT rate)
    m_decimal_label.set_text("Dec:");
    m_decimal_label.set_margin_end(5);
    m_decimal_places_spin.set_range(0, 6);
    m_decimal_places_spin.set_increments(1, 1);
    m_decimal_places_spin.set_value(2);
    m_decimal_places_spin.set_width_chars(3);
    m_decimal_places_spin.signal_value_changed().connect(
        sigc::mem_fun(*this, &MainWindow::on_decimal_places_changed));

    m_vat_label.set_text("VAT:");
    m_vat_label.set_margin_start(15);
    m_vat_label.set_margin_end(5);
    m_vat_rate_spin.set_range(0, 100);
    m_vat_rate_spin.set_increments(0.5, 1);
    m_vat_rate_spin.set_value(19);
    m_vat_rate_spin.set_digits(1);
    m_vat_rate_spin.set_width_chars(4);
    m_vat_rate_spin.signal_value_changed().connect(
        sigc::mem_fun(*this, &MainWindow::on_vat_rate_changed));

    // VAT control on the left
    m_control_box.append(m_vat_label);
    m_control_box.append(m_vat_rate_spin);

    // Spacer to push Dec to the right
    auto spacer = Gtk::make_managed<Gtk::Label>();
    spacer->set_hexpand(true);
    m_control_box.append(*spacer);

    // Dec control on the right
    m_control_box.append(m_decimal_label);
    m_control_box.append(m_decimal_places_spin);

    m_control_box.set_margin_start(0);
    m_control_box.set_margin_end(10);
    m_control_box.set_margin_top(5);
    m_control_box.set_margin_bottom(5);
    m_control_box.set_spacing(5);

    // VAT buttons will be added to the grid below

    // Configure button grid (5 rows x 4 columns)
    m_button_grid.set_row_spacing(8);
    m_button_grid.set_column_spacing(8);
    m_button_grid.set_margin(10);
    m_button_grid.set_column_homogeneous(true);
    m_button_grid.set_row_homogeneous(false);
    m_button_grid.set_hexpand(true);
    m_button_grid.set_halign(Gtk::Align::FILL);

    // Row 0: AC, ⌫, %, /
    auto ac_btn = Gtk::make_managed<Gtk::Button>("AC");
    ac_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_clear_clicked));
    ac_btn->add_css_class("clear-button");
    ac_btn->set_focus_on_click(false);
    m_button_grid.attach(*ac_btn, 0, 0, 1, 1);

    auto backspace_btn = Gtk::make_managed<Gtk::Button>("⌫");
    backspace_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_backspace_clicked));
    backspace_btn->add_css_class("backspace-button");
    backspace_btn->set_focus_on_click(false);
    m_button_grid.attach(*backspace_btn, 1, 0, 1, 1);

    auto divide_btn = Gtk::make_managed<Gtk::Button>("÷");  // Standard division symbol
    divide_btn->signal_clicked().connect([this]() { on_operation_clicked('/'); });
    divide_btn->add_css_class("operation-button");
    divide_btn->set_focus_on_click(false);
    m_button_grid.attach(*divide_btn, 2, 0, 1, 1);

    create_operation_button("󰅖", '*', 0, 3);  // Nerd Font multiply icon

    // Row 1: 7, 8, 9, -
    create_number_button(7, 1, 0);
    create_number_button(8, 1, 1);
    create_number_button(9, 1, 2);
    create_operation_button("−", '-', 1, 3);  // Standard minus symbol (U+2212)

    // Row 2: 4, 5, 6, +
    create_number_button(4, 2, 0);
    create_number_button(5, 2, 1);
    create_number_button(6, 2, 2);
    create_operation_button("󰐕", '+', 2, 3);  // Nerd Font plus icon

    // Row 3: 1, 2, 3, (= will span here from above)
    create_number_button(1, 3, 0);
    create_number_button(2, 3, 1);
    create_number_button(3, 3, 2);

    // Row 4: 0, ., %, = (double height from row 3-4)
    auto zero_btn = Gtk::make_managed<Gtk::Button>("0");
    zero_btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_number_clicked), 0));
    zero_btn->add_css_class("number-button");
    zero_btn->set_focus_on_click(false);
    m_button_grid.attach(*zero_btn, 0, 4, 1, 1);

    auto decimal_btn = Gtk::make_managed<Gtk::Button>(".");
    decimal_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_decimal_clicked));
    decimal_btn->add_css_class("number-button");
    decimal_btn->set_focus_on_click(false);
    m_button_grid.attach(*decimal_btn, 1, 4, 1, 1);

    auto percent_btn = Gtk::make_managed<Gtk::Button>("%");
    percent_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_percent_clicked));
    percent_btn->add_css_class("operation-button");
    percent_btn->set_focus_on_click(false);
    m_button_grid.attach(*percent_btn, 2, 4, 1, 1);

    // = button spans rows 3-5 (triple height)
    auto equals_btn = Gtk::make_managed<Gtk::Button>("=");
    equals_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_equals_clicked));
    equals_btn->add_css_class("equals-button");
    equals_btn->set_focus_on_click(false);
    m_button_grid.attach(*equals_btn, 3, 3, 1, 3);

    // Row 5: Save, +VAT, -VAT
    auto save_btn = Gtk::make_managed<Gtk::Button>("SAVE");
    save_btn->signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_save_tape_clicked));
    save_btn->add_css_class("vat-button");
    save_btn->set_focus_on_click(false);
    save_btn->set_tooltip_text("Save tape to file");
    m_button_grid.attach(*save_btn, 0, 5, 1, 1);

    m_add_vat_btn.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_add_vat_clicked));
    m_add_vat_btn.add_css_class("vat-button");
    m_add_vat_btn.set_focus_on_click(false);
    m_button_grid.attach(m_add_vat_btn, 1, 5, 1, 1);

    m_subtract_vat_btn.signal_clicked().connect(sigc::mem_fun(*this, &MainWindow::on_subtract_vat_clicked));
    m_subtract_vat_btn.add_css_class("vat-button");
    m_subtract_vat_btn.set_focus_on_click(false);
    m_button_grid.attach(m_subtract_vat_btn, 2, 5, 1, 1);

    // Assemble left panel (history)
    m_left_panel.append(m_history_title_box);
    m_left_panel.append(m_tape_scroll);
    m_left_panel.append(m_running_total_label);
    m_left_panel.append(m_subtotal_label);
    m_left_panel.append(m_result_label);
    m_left_panel.set_size_request(340, -1);
    m_left_panel.set_hexpand(false);
    m_left_panel.add_css_class("history-panel");

    // Assemble right panel (controls)
    m_right_panel.append(m_control_box);
    m_right_panel.append(m_button_grid);
    m_right_panel.set_margin(10);
    m_right_panel.set_hexpand(false);
    m_right_panel.set_vexpand(false);

    // Assemble main layout
    m_main_box.append(m_left_panel);
    m_main_box.append(m_right_panel);
    m_main_box.set_spacing(0);

    // Fix width but allow height to resize
    m_main_box.set_size_request(WIDTH, -1);

    // Assemble outer layout with menu bar
    m_outer_box.append(m_main_box);
    m_outer_box.set_spacing(0);

    set_child(m_outer_box);

    // Setup keyboard event controller
    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    key_controller->signal_key_pressed().connect(
        sigc::mem_fun(*this, &MainWindow::on_key_pressed), false);
    add_controller(key_controller);

    // Load saved settings (VAT rate, decimal places, etc.)
    load_settings();

    // Initial display update
    update_displays();
}

MainWindow::~MainWindow() {}

void MainWindow::setup_menu() {
    // Create edit mode action (always enabled)
    auto action_edit_mode = Gio::SimpleAction::create("edit-mode");
    action_edit_mode->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_edit_mode)));
    m_app->add_action(action_edit_mode);

    // Create file actions (always enabled)
    auto action_new = Gio::SimpleAction::create("new");
    action_new->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_new)));
    m_app->add_action(action_new);

    auto action_open = Gio::SimpleAction::create("open");
    action_open->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_open)));
    m_app->add_action(action_open);

    auto action_save_as = Gio::SimpleAction::create("save-as");
    action_save_as->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_save_as)));
    m_app->add_action(action_save_as);

    auto action_new_window = Gio::SimpleAction::create("new-window");
    action_new_window->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_new_window)));
    m_app->add_action(action_new_window);

    auto action_quit = Gio::SimpleAction::create("quit");
    action_quit->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_quit)));
    m_app->add_action(action_quit);

    // Create edit actions (initially disabled)
    auto action_undo = Gio::SimpleAction::create("undo");
    action_undo->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_undo)));
    action_undo->set_enabled(false);
    m_app->add_action(action_undo);

    auto action_redo = Gio::SimpleAction::create("redo");
    action_redo->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_redo)));
    action_redo->set_enabled(false);
    m_app->add_action(action_redo);

    auto action_cut = Gio::SimpleAction::create("cut");
    action_cut->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_cut)));
    action_cut->set_enabled(false);
    m_app->add_action(action_cut);

    auto action_copy = Gio::SimpleAction::create("copy");
    action_copy->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_copy)));
    action_copy->set_enabled(false);
    m_app->add_action(action_copy);

    auto action_paste = Gio::SimpleAction::create("paste");
    action_paste->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_paste)));
    action_paste->set_enabled(false);
    m_app->add_action(action_paste);

    auto action_select_all = Gio::SimpleAction::create("select-all");
    action_select_all->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_select_all)));
    action_select_all->set_enabled(false);
    m_app->add_action(action_select_all);

    auto action_documentation = Gio::SimpleAction::create("documentation");
    action_documentation->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::on_action_documentation)));
    m_app->add_action(action_documentation);

    // Set keyboard accelerators
    m_app->set_accel_for_action("app.edit-mode", "<Primary>e");
    m_app->set_accel_for_action("app.new", "<Primary>n");
    m_app->set_accel_for_action("app.open", "<Primary>o");
    m_app->set_accel_for_action("app.save-as", "<Primary><Shift>s");
    m_app->set_accel_for_action("app.new-window", "<Primary><Shift>n");
    m_app->set_accel_for_action("app.quit", "<Primary>q");
    m_app->set_accel_for_action("app.undo", "<Primary>z");
    m_app->set_accel_for_action("app.redo", "<Primary><Shift>z");
    m_app->set_accel_for_action("app.cut", "<Primary>x");
    m_app->set_accel_for_action("app.copy", "<Primary>c");
    m_app->set_accel_for_action("app.paste", "<Primary>v");
    m_app->set_accel_for_action("app.select-all", "<Primary>a");

    // Create menu bar
    auto menu_bar = Gio::Menu::create();

    // File menu
    auto file_menu = Gio::Menu::create();
    file_menu->append("_New", "app.new");
    file_menu->append("_Open...", "app.open");

    // Create recent files submenu
    m_recent_files_menu = Gio::Menu::create();
    file_menu->append_submenu("Open _Recent", m_recent_files_menu);

    file_menu->append("Save _As...", "app.save-as");
    file_menu->append("New _Window", "app.new-window");
    file_menu->append("_Quit", "app.quit");
    menu_bar->append_submenu("_File", file_menu);

    // Load recent files and populate menu
    load_recent_files();
    update_recent_files_menu();

    // Edit menu
    auto edit_menu = Gio::Menu::create();
    edit_menu->append("_Edit Mode", "app.edit-mode");
    edit_menu->append("_Undo", "app.undo");
    edit_menu->append("_Redo", "app.redo");
    edit_menu->append("Cu_t", "app.cut");
    edit_menu->append("_Copy", "app.copy");
    edit_menu->append("_Paste", "app.paste");
    edit_menu->append("Select _All", "app.select-all");
    menu_bar->append_submenu("_Edit", edit_menu);

    // Help menu
    auto help_menu = Gio::Menu::create();
    help_menu->append("_Documentation", "app.documentation");
    menu_bar->append_submenu("_Help", help_menu);

    // Create menu bar widget
    auto menu_bar_widget = Gtk::make_managed<Gtk::PopoverMenuBar>(menu_bar);
    m_outer_box.prepend(*menu_bar_widget);
}

void MainWindow::update_edit_menu_sensitivity() {
    // Enable/disable edit menu items based on edit mode
    bool in_edit_mode = m_tape_edit_mode;

    auto action_undo = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("undo"));
    if (action_undo) action_undo->set_enabled(in_edit_mode);

    auto action_redo = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("redo"));
    if (action_redo) action_redo->set_enabled(in_edit_mode);

    auto action_cut = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("cut"));
    if (action_cut) action_cut->set_enabled(in_edit_mode);

    auto action_copy = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("copy"));
    if (action_copy) action_copy->set_enabled(in_edit_mode);

    auto action_paste = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("paste"));
    if (action_paste) action_paste->set_enabled(in_edit_mode);

    auto action_select_all = std::dynamic_pointer_cast<Gio::SimpleAction>(m_app->lookup_action("select-all"));
    if (action_select_all) action_select_all->set_enabled(in_edit_mode);
}

// Menu action handlers
void MainWindow::on_action_edit_mode() {
    // Toggle edit mode by triggering the edit button
    if (m_edit_tape_button.get_visible()) {
        on_edit_tape_clicked();
        update_edit_menu_sensitivity();
    }
}

void MainWindow::on_action_new() {
    on_clear_clicked();
}

void MainWindow::on_action_open() {
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Open Tape File");

    // Set up file filters
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Text files");
    filter->add_pattern("*.txt");
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    filters->append(filter);
    dialog->set_filters(filters);

    dialog->open(*this, [this](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            auto file = std::dynamic_pointer_cast<Gtk::FileDialog>(result->get_source_object_base())->open_finish(result);
            auto path = file->get_path();

            // Read file and parse tape entries
            std::ifstream infile(path);
            if (infile.is_open()) {
                // Clear current tape
                m_engine.clear();

                std::string line;
                while (std::getline(infile, line)) {
                    // Skip empty lines
                    if (line.empty()) continue;

                    // Check if it's a separator line
                    if (line.find("---") != std::string::npos) {
                        m_engine.loadTapeEntry(TapeEntry::separator());
                        continue;
                    }

                    // Parse operation and value
                    // Format: "operation   value" (operation left-aligned, value right-aligned)
                    if (line.length() < 3) continue;

                    char operation = line[0];

                    // Skip if not a valid operation
                    if (operation != '+' && operation != '-' && operation != '*' &&
                        operation != '/' && operation != '=' && operation != '%') {
                        continue;
                    }

                    // Extract the value (right side of the line)
                    std::string value_str;
                    size_t last_space = line.find_last_of(' ');
                    if (last_space != std::string::npos) {
                        value_str = line.substr(last_space + 1);
                    } else {
                        value_str = line.substr(1);
                    }

                    // Handle VAT lines (contains % and |)
                    if (line.find('%') != std::string::npos && line.find('|') != std::string::npos) {
                        // Format: "+         19% | 19,00"
                        // Extract VAT rate and amount
                        size_t pipe_pos = line.find('|');
                        size_t percent_pos = line.find('%');

                        std::string vat_rate_str = line.substr(1, percent_pos - 1);
                        std::string vat_amount_str = line.substr(pipe_pos + 1);

                        // Trim whitespace
                        vat_rate_str.erase(0, vat_rate_str.find_first_not_of(" \t"));
                        vat_rate_str.erase(vat_rate_str.find_last_not_of(" \t") + 1);
                        vat_amount_str.erase(0, vat_amount_str.find_first_not_of(" \t"));
                        vat_amount_str.erase(vat_amount_str.find_last_not_of(" \t") + 1);

                        // Parse values (replace comma with period for parsing)
                        std::replace(vat_rate_str.begin(), vat_rate_str.end(), ',', '.');
                        std::replace(vat_amount_str.begin(), vat_amount_str.end(), ',', '.');

                        try {
                            double vat_rate = std::stod(vat_rate_str) / 100.0;
                            double vat_amount = std::stod(vat_amount_str);

                            // Create VAT entry
                            char vat_op = (operation == '+') ? 'V' : 'v';
                            TapeEntry entry(vat_amount, vat_op, "", true, vat_rate, vat_amount);
                            m_engine.loadTapeEntry(entry);
                        } catch (...) {
                            // Skip invalid VAT line
                        }
                        continue;
                    }

                    // Parse regular value (replace comma with period for parsing)
                    std::replace(value_str.begin(), value_str.end(), ',', '.');

                    try {
                        double value = std::stod(value_str);

                        // Create tape entry
                        TapeEntry entry(value, operation, "", false);
                        m_engine.loadTapeEntry(entry);
                    } catch (...) {
                        // Skip invalid line
                    }
                }

                infile.close();

                // Recalculate from loaded tape
                m_engine.recalculateFromTape();

                // Update displays
                update_displays();

                // Show EDIT button so Edit mode is available
                m_edit_tape_button.set_visible(true);

                // Add to recent files
                add_recent_file(path);
            }
        } catch (const Glib::Error& e) {
            // User cancelled or error occurred
        }
    });
}

void MainWindow::on_action_save_as() {
    on_save_tape_clicked();
}

void MainWindow::on_action_new_window() {
    auto window = new MainWindow(m_app);
    m_app->add_window(*window);
    window->set_hide_on_close(false);
    window->present();
}

void MainWindow::on_action_quit() {
    m_app->quit();
}

void MainWindow::on_action_undo() {
    on_backspace_clicked();
}

void MainWindow::on_action_redo() {
    // Redo functionality not yet implemented
    // Would need to add redo history to calculator engine
}

void MainWindow::on_action_cut() {
    // Cut selected text from tape view
    auto clipboard = get_clipboard();
    m_tape_buffer->cut_clipboard(clipboard, true);
}

void MainWindow::on_action_copy() {
    // Copy selected text from tape view
    auto clipboard = get_clipboard();
    m_tape_buffer->copy_clipboard(clipboard);
}

void MainWindow::on_action_paste() {
    // Paste is not very useful in a calculator, but we can support it
    // This would paste into the tape view if it's editable (edit mode)
    if (m_tape_edit_mode) {
        auto clipboard = get_clipboard();
        m_tape_buffer->paste_clipboard(clipboard);
    }
}

void MainWindow::on_action_select_all() {
    // Select all text in tape view
    m_tape_buffer->select_range(m_tape_buffer->begin(), m_tape_buffer->end());
}

void MainWindow::on_action_documentation() {
    // Open documentation URL in default browser
    auto launcher = Gtk::UriLauncher::create("https://github.com/mkay/tape-calc");
    launcher->launch(*this, [](const Glib::RefPtr<Gio::AsyncResult>&) {
        // Callback - we don't need to do anything when the URL is launched
    });
}

// Recent files management
void MainWindow::add_recent_file(const std::string& path) {
    // Remove if already exists
    auto it = std::find(m_recent_files.begin(), m_recent_files.end(), path);
    if (it != m_recent_files.end()) {
        m_recent_files.erase(it);
    }

    // Add to front
    m_recent_files.insert(m_recent_files.begin(), path);

    // Keep only last 10
    if (m_recent_files.size() > 10) {
        m_recent_files.resize(10);
    }

    // Save and update menu
    save_recent_files();
    update_recent_files_menu();
}

void MainWindow::load_recent_files() {
    m_recent_files.clear();

    std::string config_path = get_config_path();
    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(config_file, line)) {
        if (line.substr(0, 12) == "recent_file=") {
            std::string file_path = line.substr(12);
            // Only add if file still exists
            if (std::filesystem::exists(file_path)) {
                m_recent_files.push_back(file_path);
            }
        }
    }
    config_file.close();
}

void MainWindow::save_recent_files() {
    std::string config_path = get_config_path();

    // Read existing settings
    std::map<std::string, std::string> settings;
    std::ifstream config_file(config_path);
    if (config_file.is_open()) {
        std::string line;
        while (std::getline(config_file, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos && line.substr(0, 12) != "recent_file=") {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                settings[key] = value;
            }
        }
        config_file.close();
    }

    // Write settings with recent files
    std::ofstream out_file(config_path);
    if (out_file.is_open()) {
        // Write non-recent settings
        for (const auto& [key, value] : settings) {
            out_file << key << "=" << value << "\n";
        }
        // Write recent files
        for (const auto& file : m_recent_files) {
            out_file << "recent_file=" << file << "\n";
        }
        out_file.close();
    }
}

void MainWindow::update_recent_files_menu() {
    // Clear existing menu items
    m_recent_files_menu->remove_all();

    if (m_recent_files.empty()) {
        // Add a disabled "No recent files" item
        auto no_files_item = Gio::MenuItem::create("No recent files", "");
        m_recent_files_menu->append_item(no_files_item);
        return;
    }

    // Add menu items for each recent file
    for (size_t i = 0; i < m_recent_files.size(); ++i) {
        const auto& file_path = m_recent_files[i];

        // Extract filename for display
        std::filesystem::path path(file_path);
        std::string display_name = path.filename().string();

        // Create action name
        std::string action_name = "recent-" + std::to_string(i);

        // Remove old action if it exists
        m_app->remove_action(action_name);

        // Create action
        auto action = Gio::SimpleAction::create(action_name);
        action->signal_activate().connect([this, file_path](const Glib::VariantBase&) {
            on_action_open_recent(file_path);
        });
        m_app->add_action(action);

        // Add menu item
        m_recent_files_menu->append(display_name, "app." + action_name);
    }

    // Add separator and "Clear List" option
    auto separator_section = Gio::Menu::create();

    // Create "Clear List" action if it doesn't exist
    if (!m_app->lookup_action("clear-recent")) {
        auto clear_action = Gio::SimpleAction::create("clear-recent");
        clear_action->signal_activate().connect(sigc::hide(sigc::mem_fun(*this, &MainWindow::clear_recent_files)));
        m_app->add_action(clear_action);
    }

    separator_section->append("Clear List", "app.clear-recent");
    m_recent_files_menu->append_section("", separator_section);
}

void MainWindow::clear_recent_files() {
    m_recent_files.clear();
    save_recent_files();
    update_recent_files_menu();
}

void MainWindow::on_action_open_recent(const std::string& file_path) {
    // Check if file still exists
    if (!std::filesystem::exists(file_path)) {
        // Show error dialog
        auto dialog = Gtk::AlertDialog::create("File not found: " + file_path);
        dialog->show(*this);

        // Remove from recent files
        auto it = std::find(m_recent_files.begin(), m_recent_files.end(), file_path);
        if (it != m_recent_files.end()) {
            m_recent_files.erase(it);
            save_recent_files();
            update_recent_files_menu();
        }
        return;
    }

    // Read file and parse tape entries (same as on_action_open)
    std::ifstream infile(file_path);
    if (infile.is_open()) {
        // Clear current tape
        m_engine.clear();

        std::string line;
        while (std::getline(infile, line)) {
            // Skip empty lines
            if (line.empty()) continue;

            // Check if it's a separator line
            if (line.find("---") != std::string::npos) {
                m_engine.loadTapeEntry(TapeEntry::separator());
                continue;
            }

            // Parse operation and value
            if (line.length() < 3) continue;

            char operation = line[0];

            // Skip if not a valid operation
            if (operation != '+' && operation != '-' && operation != '*' &&
                operation != '/' && operation != '=' && operation != '%') {
                continue;
            }

            // Extract the value (right side of the line)
            std::string value_str;
            size_t last_space = line.find_last_of(' ');
            if (last_space != std::string::npos) {
                value_str = line.substr(last_space + 1);
            } else {
                value_str = line.substr(1);
            }

            // Handle VAT lines (contains % and |)
            if (line.find('%') != std::string::npos && line.find('|') != std::string::npos) {
                size_t pipe_pos = line.find('|');
                size_t percent_pos = line.find('%');

                std::string vat_rate_str = line.substr(1, percent_pos - 1);
                std::string vat_amount_str = line.substr(pipe_pos + 1);

                // Trim whitespace
                vat_rate_str.erase(0, vat_rate_str.find_first_not_of(" \t"));
                vat_rate_str.erase(vat_rate_str.find_last_not_of(" \t") + 1);
                vat_amount_str.erase(0, vat_amount_str.find_first_not_of(" \t"));
                vat_amount_str.erase(vat_amount_str.find_last_not_of(" \t") + 1);

                // Parse values
                std::replace(vat_rate_str.begin(), vat_rate_str.end(), ',', '.');
                std::replace(vat_amount_str.begin(), vat_amount_str.end(), ',', '.');

                try {
                    double vat_rate = std::stod(vat_rate_str) / 100.0;
                    double vat_amount = std::stod(vat_amount_str);

                    char vat_op = (operation == '+') ? 'V' : 'v';
                    TapeEntry entry(vat_amount, vat_op, "", true, vat_rate, vat_amount);
                    m_engine.loadTapeEntry(entry);
                } catch (...) {
                    // Skip invalid VAT line
                }
                continue;
            }

            // Parse regular value
            std::replace(value_str.begin(), value_str.end(), ',', '.');

            try {
                double value = std::stod(value_str);
                TapeEntry entry(value, operation, "", false);
                m_engine.loadTapeEntry(entry);
            } catch (...) {
                // Skip invalid line
            }
        }

        infile.close();

        // Recalculate from loaded tape
        m_engine.recalculateFromTape();

        // Update displays
        update_displays();

        // Show EDIT button so Edit mode is available
        m_edit_tape_button.set_visible(true);

        // Add to recent files
        add_recent_file(file_path);
    }
}

void MainWindow::setup_css() {
    m_css_provider = Gtk::CssProvider::create();
    m_css_provider->load_from_string(R"(
        .history-panel {
            border-right: 1px solid alpha(@borders, 0.3);
        }

        .history-title {
            font-size: 14pt;
            font-weight: bold;
        }

        .result-display {
            font-size: 24pt;
            font-weight: bold;
            padding: 10px 20px 10px 10px;
            background: transparent;
            margin: 5px 20px 5px 10px;
        }

        .result-display.negative-result {
            color: #D86A35;
        }

        .running-total {
            font-size: 18pt;
            color: #2196F3;
            font-weight: bold;
            padding: 10px 20px;
        }

        .subtotal {
            font-size: 14pt;
            color: #4CAF50;
            padding: 5px 20px;
        }

        /* Base button style - only for calculator buttons, not window controls */
        .number-button,
        .clear-button,
        .backspace-button,
        .operation-button,
        .equals-button,
        .vat-button {
            font-size: 20pt;
            font-weight: 600;
            min-height: 55px;
            border-radius: 12px;
            min-width: 60px;
        }

        /* Number buttons (0-9, .) - lighter background */
        .number-button {
            background-image: linear-gradient(to bottom, rgba(200, 200, 200, 0.15), rgba(180, 180, 180, 0.15));
        }

        .number-button:hover {
            background-image: linear-gradient(to bottom, rgba(220, 220, 220, 0.25), rgba(200, 200, 200, 0.25));
        }

        /* Clear button - burnt orange/red */
        .clear-button {
            background-image: linear-gradient(to bottom, #D86A35, #C75724);
            color: white;
        }

        .clear-button:hover {
            background-image: linear-gradient(to bottom, #E87B46, #D86A35);
        }

        /* Backspace button - blue-gray */
        .backspace-button {
            background-image: linear-gradient(to bottom, #7C8FA5, #6B7F95);
            color: white;
        }

        .backspace-button:hover {
            background-image: linear-gradient(to bottom, #8D9FB5, #7C8FA5);
        }

        /* Operation buttons (+, -, ×, /, %) - same as equals */
        .operation-button {
            background-image: linear-gradient(to bottom, rgba(170, 170, 170, 0.35), rgba(150, 150, 150, 0.35));
        }

        .operation-button:hover {
            background-image: linear-gradient(to bottom, rgba(190, 190, 190, 0.45), rgba(170, 170, 170, 0.45));
        }

        /* Equals button - same as operation buttons */
        .equals-button {
            background-image: linear-gradient(to bottom, rgba(170, 170, 170, 0.35), rgba(150, 150, 150, 0.35));
        }

        .equals-button:hover {
            background-image: linear-gradient(to bottom, rgba(190, 190, 190, 0.45), rgba(170, 170, 170, 0.45));
        }

        /* VAT buttons - same as equals/operations */
        .vat-button {
            font-size: 11pt;
            font-weight: 600;
            min-height: 55px;
            border-radius: 12px;
            background-image: linear-gradient(to bottom, rgba(170, 170, 170, 0.35), rgba(150, 150, 150, 0.35));
        }

        .vat-button:hover {
            background-image: linear-gradient(to bottom, rgba(190, 190, 190, 0.45), rgba(170, 170, 170, 0.45));
        }

        /* Edit button - styled as text link */
        .edit-button {
            font-size: 10pt;
            font-weight: normal;
            min-height: 0;
            min-width: 0;
            padding: 0;
            margin: 0;
            border: none;
            background: none;
            box-shadow: none;
            outline: none;
            color: alpha(@theme_fg_color, 0.7);
            text-decoration: none;
        }

        .edit-button:hover {
            background: none;
            box-shadow: none;
            outline: none;
            color: alpha(@theme_fg_color, 0.9);
            text-decoration: underline;
        }

        .edit-button:focus {
            outline: none;
            box-shadow: none;
        }

        /* Edit button in active edit mode */
        .edit-button.active {
            background: none;
            box-shadow: none;
            outline: none;
            color: #D86A35;
            font-weight: bold;
            text-decoration: none;
        }

        .edit-button.active:hover {
            text-decoration: underline;
        }

        /* Tape in edit mode */
        textview.edit-mode {
            background: rgba(255, 240, 220, 0.15);
            border: 2px solid #D86A35;
            border-radius: 4px;
        }

        textview {
            font-family: monospace;
            font-size: 13pt;
        }

        /* Compact decimal control */
        spinbutton {
            min-width: 40px;
            padding: 0;
        }

        spinbutton button {
            min-width: 16px;
            min-height: 16px;
            padding: 2px;
            margin: 0;
        }
    )");

    Gtk::StyleContext::add_provider_for_display(
        Gdk::Display::get_default(),
        m_css_provider,
        GTK_STYLE_PROVIDER_PRIORITY_USER);
}

void MainWindow::create_number_button(int number, int row, int col) {
    auto btn = Gtk::make_managed<Gtk::Button>(std::to_string(number));
    btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_number_clicked), number));
    btn->add_css_class("number-button");
    btn->set_focus_on_click(false);
    m_button_grid.attach(*btn, col, row, 1, 1);
}

void MainWindow::create_operation_button(const Glib::ustring& label, char op, int row, int col) {
    auto btn = Gtk::make_managed<Gtk::Button>(label);
    btn->signal_clicked().connect(sigc::bind(sigc::mem_fun(*this, &MainWindow::on_operation_clicked), op));
    btn->add_css_class("operation-button");
    btn->set_focus_on_click(false);
    m_button_grid.attach(*btn, col, row, 1, 1);
}

void MainWindow::on_number_clicked(int digit) {
    m_engine.inputDigit(digit);
    update_displays();
}

void MainWindow::on_operation_clicked(char op) {
    m_engine.performOperation(op);
    update_displays();
}

void MainWindow::on_equals_clicked() {
    m_engine.calculateEquals();
    update_displays();
    m_edit_tape_button.set_visible(true);  // Show EDIT button after calculation
}

void MainWindow::on_clear_clicked() {
    // Exit edit mode first if active
    if (m_tape_edit_mode) {
        m_tape_edit_mode = false;
        m_tape_view.set_editable(false);
        m_edit_tape_button.set_label("EDIT");
        m_edit_tape_button.remove_css_class("active");
        m_tape_view.remove_css_class("edit-mode");
    }

    m_engine.clear();
    update_displays();
    m_edit_tape_button.set_visible(false);  // Hide EDIT button when cleared
}

void MainWindow::on_backspace_clicked() {
    // If new number hasn't started (just after operation), undo last entry
    // Otherwise, delete character from input
    if (m_engine.isNewNumberStarted() || m_engine.getCurrentInput() == "0") {
        m_engine.undoLastEntry();
    } else {
        m_engine.backspace();
    }
    update_displays();
}

void MainWindow::on_decimal_clicked() {
    m_engine.inputDecimalPoint();
    update_displays();
}

void MainWindow::on_percent_clicked() {
    m_engine.calculatePercentage();
    update_displays();
}

void MainWindow::on_add_vat_clicked() {
    m_engine.addVAT();
    update_displays();
}

void MainWindow::on_subtract_vat_clicked() {
    m_engine.subtractVAT();
    update_displays();
}

void MainWindow::on_vat_rate_changed() {
    // Get VAT rate from spin button (as percentage) and convert to decimal
    double vat_percentage = m_vat_rate_spin.get_value();
    double vat_rate = vat_percentage / 100.0;
    m_engine.setVATRate(vat_rate);
    save_settings();
}

void MainWindow::on_decimal_places_changed() {
    int places = m_decimal_places_spin.get_value_as_int();
    m_engine.setDecimalPlaces(places);
    update_displays();
    save_settings();
}

void MainWindow::on_save_tape_clicked() {
    // Generate timestamped filename: calc-yymmdd-hhmm.txt
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm* tm_now = std::localtime(&time_t_now);

    char filename_buf[32];
    std::strftime(filename_buf, sizeof(filename_buf), "calc-%y%m%d-%H%M.txt", tm_now);
    std::string suggested_filename(filename_buf);

    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Save Calculation Tape");
    dialog->set_initial_name(suggested_filename);

    // Set text file filter
    auto filter = Gtk::FileFilter::create();
    filter->set_name("Text files");
    filter->add_pattern("*.txt");
    auto filters = Gio::ListStore<Gtk::FileFilter>::create();
    filters->append(filter);
    dialog->set_filters(filters);

    dialog->save(*this, [this](const Glib::RefPtr<Gio::AsyncResult>& result) {
        try {
            auto file = std::dynamic_pointer_cast<Gtk::FileDialog>(result->get_source_object_base())->save_finish(result);
            if (file) {
                // Get the tape content
                std::string tape_content = m_tape_buffer->get_text();

                // Write to file
                std::string file_path = file->get_path();
                std::ofstream outfile(file_path);
                if (outfile.is_open()) {
                    outfile << tape_content;
                    outfile.close();

                    // Add to recent files
                    add_recent_file(file_path);
                }
            }
        } catch (const Gtk::DialogError& err) {
            // User cancelled, ignore
        } catch (const Glib::Error& err) {
            g_warning("Error saving file: %s", err.what());
        } catch (const std::exception& err) {
            g_warning("Error saving file: %s", err.what());
        }
    });
}

void MainWindow::update_displays() {
    // Update result display - always show running total
    std::string result = m_engine.getResult();
    m_result_label.set_text(result);
    m_result_label.set_visible(true);

    // Make result red if negative
    try {
        double value = std::stod(result);
        if (value < 0) {
            m_result_label.add_css_class("negative-result");
        } else {
            m_result_label.remove_css_class("negative-result");
        }
    } catch (...) {
        // If we can't parse (e.g., "Error"), remove the class
        m_result_label.remove_css_class("negative-result");
    }

    // Update tape (will show current input as you type)
    update_tape();
}

void MainWindow::update_tape() {
    m_updating_tape = true;  // Prevent triggering on_tape_changed

    const auto& history = m_engine.getTapeHistory();

    std::string tape_text;
    int current_line = 0;
    std::vector<int> line_starts;  // Track where each line starts in the text
    std::vector<int> minus_lines;  // Track which lines have minus operations
    int result_line = -1;  // Track the result line (after separator)
    double result_value = 0.0;  // Track the result value
    char previous_operation = '+';  // Default first operation is addition

    for (size_t i = 0; i < history.size(); i++) {
        const auto& entry = history[i];

        line_starts.push_back(tape_text.length());

        if (entry.is_separator) {
            tape_text += "---------------\n";
        } else if (entry.is_vat_operation) {
            // Format VAT entry: "+    19,00% | 20,88"
            char op = entry.operation == 'V' ? '+' : '-';
            std::ostringstream line;
            line << op << std::right << std::setw(13)
                 << std::fixed << std::setprecision(0) << (entry.vat_rate * 100)
                 << "% | " << std::fixed << std::setprecision(m_engine.getDecimalPlaces())
                 << entry.vat_amount << "\n";
            tape_text += line.str();
        } else {
            // Format regular entry using PREVIOUS operation (what will be applied to this value)
            // Exception: result lines use '='
            char display_op = (entry.operation == '=') ? '=' : previous_operation;

            std::ostringstream line;
            std::ostringstream num_str;
            num_str << std::fixed << std::setprecision(m_engine.getDecimalPlaces()) << entry.value;

            line << display_op << std::right << std::setw(14) << num_str.str() << "\n";

            // Track lines with minus operations for red coloring
            if (entry.operation != '=') {
                if (previous_operation == '-') {
                    minus_lines.push_back(current_line);
                }
                // Update previous operation for next iteration
                previous_operation = entry.operation;
            } else {
                // This is the result line
                result_line = current_line;
                result_value = entry.value;
            }

            tape_text += line.str();
        }
        current_line++;
    }

    // Record the end of history entries (before adding pending/current input)
    int history_end_pos = tape_text.length();

    // Add pending operation line if there's a pending operation and user is typing
    std::string pending_op = m_engine.getRunningTotal();
    int pending_op_start = -1;
    if (!pending_op.empty() && m_engine.isNewNumberStarted() && !m_engine.hasError()) {
        pending_op_start = tape_text.length();
        tape_text += pending_op + "\n";
    }

    // Add current input if user is actively typing
    if (!m_engine.isNewNumberStarted() && !m_engine.hasError()) {
        std::string current_input = m_engine.getCurrentInput();
        if (!current_input.empty() && current_input != "0") {
            // Replace dot with comma for display (locale formatting)
            std::replace(current_input.begin(), current_input.end(), '.', ',');

            // Get the pending operation
            std::string op = m_engine.getRunningTotal();
            if (op.empty()) op = " ";

            // Format: "op     value"
            std::ostringstream line;
            line << op << std::right << std::setw(14) << current_input;
            tape_text += line.str();
        }
    }

    m_tape_buffer->set_text(tape_text);

    // Apply red/orange color to all lines with minus operations
    for (int line_num : minus_lines) {
        if (line_num >= 0 && line_num < (int)line_starts.size()) {
            int start_pos = line_starts[line_num];
            int end_pos = (line_num + 1 < (int)line_starts.size())
                          ? line_starts[line_num + 1] - 1  // -1 to exclude newline
                          : history_end_pos;

            auto start_iter = m_tape_buffer->get_iter_at_offset(start_pos);
            auto end_iter = m_tape_buffer->get_iter_at_offset(end_pos);
            m_tape_buffer->apply_tag_by_name("red-text", start_iter, end_iter);
        }
    }

    // Apply red/orange color to result line if result is negative
    if (result_value < 0 && result_line >= 0 && result_line < (int)line_starts.size()) {
        int start_pos = line_starts[result_line];
        int end_pos = (result_line + 1 < (int)line_starts.size())
                      ? line_starts[result_line + 1] - 1
                      : history_end_pos;

        auto start_iter = m_tape_buffer->get_iter_at_offset(start_pos);
        auto end_iter = m_tape_buffer->get_iter_at_offset(end_pos);
        m_tape_buffer->apply_tag_by_name("red-text", start_iter, end_iter);
    }

    // Apply red/orange color to pending operation line (only if it's a minus)
    if (pending_op_start >= 0 && pending_op == "-") {
        auto start_iter = m_tape_buffer->get_iter_at_offset(pending_op_start);
        auto end_iter = m_tape_buffer->get_iter_at_offset(pending_op_start + pending_op.length() + 1);  // "-\n"
        m_tape_buffer->apply_tag_by_name("red-text", start_iter, end_iter);
    }

    // Auto-scroll to bottom
    auto mark = m_tape_buffer->create_mark(m_tape_buffer->end());
    m_tape_view.scroll_to(mark);

    m_updating_tape = false;  // Re-enable on_tape_changed
}

void MainWindow::on_edit_tape_clicked() {
    if (!m_tape_edit_mode) {
        // Enter edit mode
        m_tape_edit_mode = true;
        m_tape_view.set_editable(true);
        m_tape_view.set_cursor_visible(true);
        m_tape_view.set_can_focus(true);
        m_tape_view.grab_focus();
        m_edit_tape_button.set_label("DONE");

        // Add visual feedback
        m_edit_tape_button.add_css_class("active");
        m_tape_view.add_css_class("edit-mode");

        // Update menu sensitivity
        update_edit_menu_sensitivity();
    } else {
        // Exit edit mode and recalculate
        m_tape_edit_mode = false;
        m_tape_view.set_editable(false);
        // Keep cursor visible and focusable for text selection
        m_edit_tape_button.set_label("EDIT");

        // Remove visual feedback
        m_edit_tape_button.remove_css_class("active");
        m_tape_view.remove_css_class("edit-mode");

        // Update menu sensitivity
        update_edit_menu_sensitivity();

        // Parse the edited tape and recalculate everything
        std::string tape_text = m_tape_buffer->get_text();
        std::istringstream stream(tape_text);
        std::string line;

        // Clear the calculator engine
        m_engine.clear();

        std::vector<double> values;
        std::vector<char> operations;

        // Parse each line
        while (std::getline(stream, line)) {
            // Skip empty lines, separator lines, and result lines
            if (line.empty() || line.find("---") != std::string::npos) {
                continue;
            }

            if (line.size() >= 2 && line[0] == '=') {
                break;  // Stop at result line
            }

            // Parse operation and value
            if (line.size() >= 2 && (line[0] == '+' || line[0] == '-' || line[0] == '*' || line[0] == '/')) {
                char op = line[0];

                // Extract numeric value
                std::string value_str = line.substr(1);

                // Skip VAT lines
                if (value_str.find('|') != std::string::npos) {
                    continue;
                }

                // Trim whitespace
                size_t start = value_str.find_first_not_of(" \t");
                if (start != std::string::npos) {
                    value_str = value_str.substr(start);
                }

                size_t end = value_str.find_last_not_of(" \t\r\n");
                if (end != std::string::npos) {
                    value_str = value_str.substr(0, end + 1);
                }

                // Parse the number (handle European format: period as thousands separator, comma as decimal)
                // First remove all periods (thousands separators)
                value_str.erase(std::remove(value_str.begin(), value_str.end(), '.'), value_str.end());
                // Then replace comma with period (decimal separator for C locale)
                std::replace(value_str.begin(), value_str.end(), ',', '.');

                // Parse using C locale to ensure '.' is recognized as decimal separator
                std::istringstream iss(value_str);
                iss.imbue(std::locale("C"));
                double value = 0.0;
                if (iss >> value) {
                    values.push_back(value);
                    operations.push_back(op);
                } else {
                    continue;
                }
            }
        }

        // Rebuild calculation using the engine
        if (!values.empty()) {
            std::ostringstream oss;
            oss.imbue(std::locale("C"));  // Use C locale to ensure '.' as decimal separator

            // Handle first value
            // If first operation is '-', we need to subtract from 0
            if (operations[0] == '-') {
                // Input 0, then subtract the first value
                m_engine.inputDigit(0);
                m_engine.performOperation('-');
            }

            // Input first value with proper formatting
            oss << std::fixed << std::setprecision(m_engine.getDecimalPlaces()) << values[0];
            std::string val_str = oss.str();
            for (char c : val_str) {
                if (c == '.') {
                    m_engine.inputDecimalPoint();
                } else if (c >= '0' && c <= '9') {
                    m_engine.inputDigit(c - '0');
                }
            }

            // Apply operations with subsequent values
            // Loop through all values except the last one
            for (size_t i = 0; i < values.size() - 1; i++) {
                m_engine.performOperation(operations[i + 1]);

                // Input next value
                oss.str("");
                oss.clear();
                oss << std::fixed << std::setprecision(m_engine.getDecimalPlaces()) << values[i + 1];
                val_str = oss.str();

                for (char c : val_str) {
                    if (c == '.') {
                        m_engine.inputDecimalPoint();
                    } else if (c >= '0' && c <= '9') {
                        m_engine.inputDigit(c - '0');
                    }
                }
            }

            // Calculate final result
            m_engine.calculateEquals();
        }

        // Update display with recalculated tape
        update_displays();
    }
}

bool MainWindow::on_key_pressed(guint keyval, guint keycode, Gdk::ModifierType state) {
    // Handle Ctrl+N to open new window - now handled by menu action
    if ((state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{} &&
        (keyval == GDK_KEY_n || keyval == GDK_KEY_N)) {
        // This is now handled by the menu action
        return false;  // Let the action handle it
    }

    // Handle Ctrl+Q to quit
    if ((state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{} &&
        (keyval == GDK_KEY_q || keyval == GDK_KEY_Q)) {
        close();
        return true;
    }

    // If in edit mode, let tape handle all keys
    if (m_tape_edit_mode) {
        return false;
    }

    // Handle number keys
    if (keyval >= GDK_KEY_0 && keyval <= GDK_KEY_9) {
        on_number_clicked(keyval - GDK_KEY_0);
        return true;
    }
    // Handle numpad keys - support both NumLock ON and OFF states
    switch (keyval) {
        case 0xffb0: // GDK_KEY_KP_0 (NumLock ON)
        case 0xff9e: // GDK_KEY_KP_Insert (NumLock OFF)
            on_number_clicked(0);
            return true;
        case 0xffb1: // GDK_KEY_KP_1 (NumLock ON)
        case 0xff9c: // GDK_KEY_KP_End (NumLock OFF)
            on_number_clicked(1);
            return true;
        case 0xffb2: // GDK_KEY_KP_2 (NumLock ON)
        case 0xff99: // GDK_KEY_KP_Down (NumLock OFF)
            on_number_clicked(2);
            return true;
        case 0xffb3: // GDK_KEY_KP_3 (NumLock ON)
        case 0xff9b: // GDK_KEY_KP_Page_Down (NumLock OFF)
            on_number_clicked(3);
            return true;
        case 0xffb4: // GDK_KEY_KP_4 (NumLock ON)
        case 0xff96: // GDK_KEY_KP_Left (NumLock OFF)
            on_number_clicked(4);
            return true;
        case 0xffb5: // GDK_KEY_KP_5 (NumLock ON)
        case 0xff9d: // GDK_KEY_KP_Begin (NumLock OFF)
            on_number_clicked(5);
            return true;
        case 0xffb6: // GDK_KEY_KP_6 (NumLock ON)
        case 0xff98: // GDK_KEY_KP_Right (NumLock OFF)
            on_number_clicked(6);
            return true;
        case 0xffb7: // GDK_KEY_KP_7 (NumLock ON)
        case 0xff95: // GDK_KEY_KP_Home (NumLock OFF)
            on_number_clicked(7);
            return true;
        case 0xffb8: // GDK_KEY_KP_8 (NumLock ON)
        case 0xff97: // GDK_KEY_KP_Up (NumLock OFF)
            on_number_clicked(8);
            return true;
        case 0xffb9: // GDK_KEY_KP_9 (NumLock ON)
        case 0xff9a: // GDK_KEY_KP_Page_Up (NumLock OFF)
            on_number_clicked(9);
            return true;
    }

    // Handle operation keys
    switch (keyval) {
        case GDK_KEY_plus:
        case GDK_KEY_KP_Add:
            on_operation_clicked('+');
            return true;
        case GDK_KEY_minus:
        case GDK_KEY_KP_Subtract:
            on_operation_clicked('-');
            return true;
        case GDK_KEY_asterisk:
        case GDK_KEY_KP_Multiply:
            on_operation_clicked('*');
            return true;
        case GDK_KEY_slash:
        case GDK_KEY_KP_Divide:
            on_operation_clicked('/');
            return true;
        case GDK_KEY_Return:
        case GDK_KEY_KP_Enter:
        case GDK_KEY_equal:
            on_equals_clicked();
            return true;
        case GDK_KEY_period:
        case GDK_KEY_comma:
        case GDK_KEY_KP_Decimal:
        case GDK_KEY_KP_Separator:  // Numpad comma on some layouts
        case 0xff9f:  // GDK_KEY_KP_Delete - your numpad comma key
            on_decimal_clicked();
            return true;
        case GDK_KEY_percent:
            on_percent_clicked();
            return true;
        case GDK_KEY_BackSpace:
            on_backspace_clicked();
            return true;
        case GDK_KEY_Delete:
        case GDK_KEY_Escape:
            on_clear_clicked();
            return true;
    }

    return false;
}

std::string MainWindow::get_config_path() {
    const char* home = std::getenv("HOME");
    if (!home) {
        home = std::getenv("USERPROFILE"); // Windows fallback
    }
    if (!home) {
        return "";
    }

    std::string config_dir = std::string(home) + "/.config/tape-calc";
    std::filesystem::create_directories(config_dir);
    return config_dir + "/settings.conf";
}

void MainWindow::load_settings() {
    std::string config_path = get_config_path();
    if (config_path.empty()) {
        return;
    }

    std::ifstream config_file(config_path);
    if (!config_file.is_open()) {
        return; // No config file yet, use defaults
    }

    std::string line;
    while (std::getline(config_file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') {
            continue;
        }

        // Parse key=value
        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Apply settings
        if (key == "vat_rate") {
            try {
                double vat_rate = std::stod(value);
                m_vat_rate_spin.set_value(vat_rate);
                m_engine.setVATRate(vat_rate / 100.0);
            } catch (...) {
                // Invalid value, skip
            }
        } else if (key == "decimal_places") {
            try {
                int places = std::stoi(value);
                if (places >= 0 && places <= 6) {
                    m_decimal_places_spin.set_value(places);
                    m_engine.setDecimalPlaces(places);
                }
            } catch (...) {
                // Invalid value, skip
            }
        }
    }
}

void MainWindow::save_settings() {
    std::string config_path = get_config_path();
    if (config_path.empty()) {
        return;
    }

    std::ofstream config_file(config_path);
    if (!config_file.is_open()) {
        return;
    }

    config_file << "# Tape Calculator Settings\n";
    config_file << "# This file is auto-generated\n\n";
    config_file << "vat_rate=" << m_vat_rate_spin.get_value() << "\n";
    config_file << "decimal_places=" << m_decimal_places_spin.get_value() << "\n";
}
