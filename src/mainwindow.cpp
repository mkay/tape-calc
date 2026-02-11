#include "mainwindow.h"
#include <sigc++/sigc++.h>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <locale>
#include <gdk/gdkkeysyms.h>

MainWindow::MainWindow()
    : m_main_box(Gtk::Orientation::HORIZONTAL)
    , m_left_panel(Gtk::Orientation::VERTICAL)
    , m_right_panel(Gtk::Orientation::VERTICAL)
    , m_control_box(Gtk::Orientation::HORIZONTAL)
    , m_vat_box(Gtk::Orientation::HORIZONTAL)
    , m_history_title_box(Gtk::Orientation::HORIZONTAL)
    , m_history_title("Calculation History")
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
    m_tape_view.set_editable(false);  // Not editable by default
    m_tape_view.set_cursor_visible(false);
    m_tape_view.set_can_focus(false);  // Not focusable
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

    set_child(m_main_box);

    // Setup keyboard event controller
    auto key_controller = Gtk::EventControllerKey::create();
    key_controller->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    key_controller->signal_key_pressed().connect(
        sigc::mem_fun(*this, &MainWindow::on_key_pressed), false);
    add_controller(key_controller);

    // Initial display update
    update_displays();
}

MainWindow::~MainWindow() {}

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
}

void MainWindow::on_decimal_places_changed() {
    int places = m_decimal_places_spin.get_value_as_int();
    m_engine.setDecimalPlaces(places);
    update_displays();
}

void MainWindow::on_save_tape_clicked() {
    auto dialog = Gtk::FileDialog::create();
    dialog->set_title("Save Calculation Tape");
    dialog->set_initial_name("calculation_tape.txt");

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
                std::ofstream outfile(file->get_path());
                if (outfile.is_open()) {
                    outfile << tape_content;
                    outfile.close();
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
            tape_text += " ---------------\n";
        } else if (entry.is_vat_operation) {
            // Format VAT entry: " +    19,00% | 20,88"
            char op = entry.operation == 'V' ? '+' : '-';
            std::ostringstream line;
            line << " " << op << std::right << std::setw(12)
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

            line << " " << display_op << std::right << std::setw(13) << num_str.str() << "\n";

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
        tape_text += " " + pending_op + "\n";
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

            // Format: " op     value"
            std::ostringstream line;
            line << " " << op << std::right << std::setw(13) << current_input;
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
        auto end_iter = m_tape_buffer->get_iter_at_offset(pending_op_start + pending_op.length() + 2);  // " -\n"
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
    } else {
        // Exit edit mode and recalculate
        m_tape_edit_mode = false;
        m_tape_view.set_editable(false);
        m_tape_view.set_cursor_visible(false);
        m_tape_view.set_can_focus(false);
        m_edit_tape_button.set_label("EDIT");

        // Remove visual feedback
        m_edit_tape_button.remove_css_class("active");
        m_tape_view.remove_css_class("edit-mode");

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

            if (line.size() >= 2 && line[1] == '=') {
                break;  // Stop at result line
            }

            // Parse operation and value
            if (line.size() >= 2 && (line[1] == '+' || line[1] == '-' || line[1] == '*' || line[1] == '/')) {
                char op = line[1];

                // Extract numeric value
                std::string value_str = line.substr(2);

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

                // Parse the number (handle comma as decimal separator)
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
                m_engine.performOperation(operations[i]);

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
    // Handle Ctrl+N to open new window
    if ((state & Gdk::ModifierType::CONTROL_MASK) != Gdk::ModifierType{} &&
        (keyval == GDK_KEY_n || keyval == GDK_KEY_N)) {
        auto app = get_application();
        if (app) {
            auto window = new MainWindow();
            app->add_window(*window);
            window->set_hide_on_close(false);
            window->present();
        }
        return true;
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
