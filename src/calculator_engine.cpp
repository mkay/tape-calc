#include "calculator_engine.h"
#include <cmath>

CalculatorEngine::CalculatorEngine()
    : m_running_total(0.0)
    , m_current_input(0.0)
    , m_pending_operation('\0')
    , m_decimal_places(2)
    , m_vat_rate(0.19)
    , m_input_buffer("0")
    , m_new_number_started(false)
    , m_has_decimal_point(false)
    , m_has_error(false)
    , m_show_result(false)
    , m_subtotal_text("")
{
}

void CalculatorEngine::inputDigit(int digit) {
    if (m_has_error) {
        clear();
    }

    if (m_new_number_started || m_input_buffer == "0") {
        m_input_buffer = std::to_string(digit);
        m_new_number_started = false;
        m_has_decimal_point = false;
    } else {
        m_input_buffer += std::to_string(digit);
    }

    m_show_result = false;
}

void CalculatorEngine::inputDecimalPoint() {
    if (m_has_error) {
        clear();
    }

    if (m_new_number_started) {
        m_input_buffer = "0.";
        m_new_number_started = false;
        m_has_decimal_point = true;
    } else if (!m_has_decimal_point) {
        m_input_buffer += ".";
        m_has_decimal_point = true;
    }
}

void CalculatorEngine::backspace() {
    if (m_has_error) {
        clear();
        return;
    }

    if (!m_new_number_started && m_input_buffer.length() > 0) {
        if (m_input_buffer.back() == '.') {
            m_has_decimal_point = false;
        }
        m_input_buffer.pop_back();
        if (m_input_buffer.empty() || m_input_buffer == "-") {
            m_input_buffer = "0";
            m_has_decimal_point = false;
        }
    }
}

void CalculatorEngine::performOperation(char op) {
    if (m_has_error && op != '=') {
        return;
    }

    m_current_input = parseInput();

    // Check if we're continuing from an equals result
    if (m_show_result) {
        // Continue calculation with the result - don't add to tape again
        m_running_total = m_current_input;
        m_pending_operation = op;
        m_new_number_started = true;
        m_show_result = false;
        m_subtotal_text = "";
        return;
    }

    if (m_pending_operation != '\0') {
        executeOperation();
        if (m_has_error) {
            return;
        }
    } else {
        m_running_total = m_current_input;
    }

    addToTape(m_current_input, op);
    m_pending_operation = op;
    m_new_number_started = true;
    m_subtotal_text = "";
    m_show_result = false;
}

void CalculatorEngine::calculateEquals() {
    if (m_has_error) {
        return;
    }

    m_current_input = parseInput();

    if (m_pending_operation != '\0') {
        // Add the last input to tape before calculating
        addToTape(m_current_input, m_pending_operation);
        executeOperation();
    } else {
        m_running_total = m_current_input;
    }

    // Add separator and total
    m_tape_history.push_back(TapeEntry::separator());
    addToTape(m_running_total, '=');

    m_pending_operation = '\0';
    m_new_number_started = true;
    m_input_buffer = formatNumber(m_running_total);
    m_subtotal_text = "";
    m_show_result = true;
}

void CalculatorEngine::calculatePercentage() {
    if (m_has_error) {
        return;
    }

    m_current_input = parseInput();

    // Percentage in tape calculators: convert current input to percentage of running total
    if (m_pending_operation != '\0' && m_running_total != 0.0) {
        m_current_input = m_running_total * (m_current_input / 100.0);
        m_input_buffer = formatNumber(m_current_input);
    } else {
        m_current_input = m_current_input / 100.0;
        m_input_buffer = formatNumber(m_current_input);
    }
}

void CalculatorEngine::executeOperation() {
    switch (m_pending_operation) {
        case '+':
            m_running_total += m_current_input;
            break;
        case '-':
            m_running_total -= m_current_input;
            break;
        case '*':
            m_running_total *= m_current_input;
            break;
        case '/':
            if (m_current_input == 0.0) {
                m_has_error = true;
                m_input_buffer = "Error";
                return;
            }
            m_running_total /= m_current_input;
            break;
    }
}

void CalculatorEngine::addVAT() {
    if (m_has_error) {
        return;
    }

    double base_amount = parseInput();
    if (!m_new_number_started) {
        // User entered a new number
        m_current_input = base_amount;
    } else {
        // Use running total
        base_amount = m_running_total;
        m_current_input = base_amount;
    }

    double vat_amount = base_amount * m_vat_rate;
    double total_with_vat = base_amount + vat_amount;

    m_running_total = total_with_vat;

    // Add VAT entry to tape
    TapeEntry entry(base_amount, 'V', "", true, m_vat_rate, vat_amount);
    m_tape_history.push_back(entry);

    // Add separator and total
    m_tape_history.push_back(TapeEntry::separator());
    addToTape(m_running_total, '+');

    m_input_buffer = formatNumber(m_running_total);
    m_new_number_started = true;
    m_pending_operation = '\0';
    m_show_result = true;
}

void CalculatorEngine::subtractVAT() {
    if (m_has_error) {
        return;
    }

    double total_with_vat = parseInput();
    if (!m_new_number_started) {
        // User entered a new number
        m_current_input = total_with_vat;
    } else {
        // Use running total
        total_with_vat = m_running_total;
        m_current_input = total_with_vat;
    }

    double base_amount = total_with_vat / (1.0 + m_vat_rate);
    double vat_amount = total_with_vat - base_amount;

    m_running_total = base_amount;

    // Add VAT entry to tape
    TapeEntry entry(total_with_vat, 'v', "", true, m_vat_rate, vat_amount);
    m_tape_history.push_back(entry);

    // Add separator and total
    m_tape_history.push_back(TapeEntry::separator());
    addToTape(m_running_total, '-');

    m_input_buffer = formatNumber(m_running_total);
    m_new_number_started = true;
    m_pending_operation = '\0';
    m_show_result = true;
}

void CalculatorEngine::setVATRate(double rate) {
    m_vat_rate = rate;
}

void CalculatorEngine::setDecimalPlaces(int places) {
    if (places >= 0 && places <= 6) {
        m_decimal_places = places;
    }
}

void CalculatorEngine::clear() {
    m_running_total = 0.0;
    m_current_input = 0.0;
    m_pending_operation = '\0';
    m_input_buffer = "0";
    m_new_number_started = false;
    m_has_decimal_point = false;
    m_has_error = false;
    m_show_result = false;
    m_subtotal_text = "";
    m_tape_history.clear();
}

void CalculatorEngine::clearEntry() {
    m_input_buffer = "0";
    m_has_decimal_point = false;
    m_has_error = false;
}

void CalculatorEngine::undoLastEntry() {
    if (m_tape_history.empty()) {
        return;
    }

    // Remove separator if present
    if (m_tape_history.back().is_separator) {
        m_tape_history.pop_back();
    }

    // Remove the last entry
    if (!m_tape_history.empty()) {
        TapeEntry last = m_tape_history.back();
        m_tape_history.pop_back();

        // Recalculate running total by replaying tape
        m_running_total = 0.0;
        m_pending_operation = '\0';

        for (const auto& entry : m_tape_history) {
            if (entry.is_separator) {
                continue;
            }

            if (entry.operation == '=' || entry.operation == 'V' || entry.operation == 'v') {
                m_running_total = entry.value;
            } else {
                if (m_pending_operation == '\0') {
                    m_running_total = entry.value;
                } else {
                    switch (m_pending_operation) {
                        case '+': m_running_total += entry.value; break;
                        case '-': m_running_total -= entry.value; break;
                        case '*': m_running_total *= entry.value; break;
                        case '/': if (entry.value != 0.0) m_running_total /= entry.value; break;
                    }
                }
                m_pending_operation = entry.operation;
            }
        }

        m_input_buffer = formatNumber(m_running_total);
        m_new_number_started = true;
    }
}

void CalculatorEngine::loadTapeEntry(const TapeEntry& entry) {
    m_tape_history.push_back(entry);
}

void CalculatorEngine::recalculateFromTape() {
    // Reset calculation state
    m_running_total = 0.0;
    m_pending_operation = '\0';
    m_has_error = false;
    m_show_result = false;
    m_new_number_started = false;

    // Replay tape to recalculate running total
    for (const auto& entry : m_tape_history) {
        if (entry.is_separator) {
            continue;
        }

        // Handle result entries
        if (entry.operation == '=') {
            m_running_total = entry.value;
            m_show_result = true;
            m_pending_operation = '\0';
        }
        // Handle VAT entries
        else if (entry.operation == 'V' || entry.operation == 'v') {
            // VAT entries contain the result value directly
            m_running_total = entry.value;
        }
        // Handle regular operations
        else {
            if (m_pending_operation == '\0') {
                m_running_total = entry.value;
            } else {
                switch (m_pending_operation) {
                    case '+': m_running_total += entry.value; break;
                    case '-': m_running_total -= entry.value; break;
                    case '*': m_running_total *= entry.value; break;
                    case '/':
                        if (entry.value != 0.0) {
                            m_running_total /= entry.value;
                        } else {
                            m_has_error = true;
                        }
                        break;
                }
            }
            m_pending_operation = entry.operation;
        }
    }

    m_input_buffer = formatNumber(m_running_total);
    m_new_number_started = true;
}

std::string CalculatorEngine::getCurrentInput() const {
    return m_input_buffer;
}

std::string CalculatorEngine::getRunningTotal() const {
    if (m_pending_operation != '\0') {
        return std::string(1, m_pending_operation);
    }
    return "";
}

std::string CalculatorEngine::getSubtotal() const {
    return m_subtotal_text;
}

std::string CalculatorEngine::getResult() const {
    if (m_has_error) {
        return "Error";
    }
    return formatNumber(m_running_total);
}

void CalculatorEngine::addToTape(double value, char op, bool is_vat) {
    std::string display_text = formatNumber(value);
    display_text += " ";

    if (is_vat) {
        if (op == 'V') {
            display_text += "+VAT(";
        } else {
            display_text += "-VAT(";
        }
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << (m_vat_rate * 100);
        display_text += oss.str();
        display_text += "%)";
    } else {
        display_text += op;
    }

    m_tape_history.emplace_back(value, op, display_text, is_vat, m_vat_rate);
}

std::string CalculatorEngine::formatNumber(double value) const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(m_decimal_places) << value;
    return oss.str();
}

double CalculatorEngine::parseInput() const {
    if (m_input_buffer.empty() || m_input_buffer == "-") {
        return 0.0;
    }

    // Manual parsing to avoid locale issues
    // Find the decimal point
    size_t decimal_pos = m_input_buffer.find('.');
    double result = 0.0;

    try {
        if (decimal_pos == std::string::npos) {
            // No decimal point, parse as integer
            result = std::stod(m_input_buffer);
        } else {
            // Split into integer and fractional parts
            std::string int_part = m_input_buffer.substr(0, decimal_pos);
            std::string frac_part = m_input_buffer.substr(decimal_pos + 1);

            // Parse integer part
            double int_value = int_part.empty() ? 0.0 : std::stod(int_part);

            // Parse fractional part
            double frac_value = 0.0;
            if (!frac_part.empty()) {
                double frac_num = std::stod(frac_part);
                // Divide by appropriate power of 10
                double divisor = std::pow(10.0, frac_part.length());
                frac_value = frac_num / divisor;
            }

            result = int_value + frac_value;
        }
        return result;
    } catch (...) {
        return 0.0;
    }
}

void CalculatorEngine::resetInput() {
    m_input_buffer = "0";
    m_has_decimal_point = false;
    m_new_number_started = true;
}
