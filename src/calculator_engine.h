#ifndef CALCULATOR_ENGINE_H
#define CALCULATOR_ENGINE_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

struct TapeEntry {
    double value;
    char operation;  // '+', '-', '*', '/', '%', 'V', '=', 'S' (separator)
    std::string display_text;
    bool is_vat_operation;
    bool is_separator;
    double vat_rate;
    double vat_amount;  // For VAT operations

    TapeEntry(double v, char op, const std::string& text, bool vat = false, double rate = 0.0, double vat_amt = 0.0)
        : value(v), operation(op), display_text(text), is_vat_operation(vat), is_separator(false), vat_rate(rate), vat_amount(vat_amt) {}

    // Constructor for separator
    static TapeEntry separator() {
        TapeEntry entry(0.0, 'S', "---------------", false, 0.0, 0.0);
        entry.is_separator = true;
        return entry;
    }
};

class CalculatorEngine {
public:
    CalculatorEngine();

    // Input methods
    void inputDigit(int digit);
    void inputDecimalPoint();
    void backspace();

    // Operation methods
    void performOperation(char op);
    void calculateEquals();
    void calculatePercentage();

    // Clear methods
    void clear();
    void clearEntry();
    void undoLastEntry();

    // VAT operations
    void addVAT();
    void subtractVAT();
    void setVATRate(double rate);

    // Configuration
    void setDecimalPlaces(int places);
    int getDecimalPlaces() const { return m_decimal_places; }
    double getVATRate() const { return m_vat_rate; }

    // Display getters
    std::string getCurrentInput() const;
    std::string getRunningTotal() const;
    std::string getSubtotal() const;
    std::string getResult() const;
    const std::vector<TapeEntry>& getTapeHistory() const { return m_tape_history; }

    // State queries
    bool hasError() const { return m_has_error; }
    bool isNewNumberStarted() const { return m_new_number_started; }

private:
    double m_running_total;
    double m_current_input;
    char m_pending_operation;
    std::vector<TapeEntry> m_tape_history;
    int m_decimal_places;
    double m_vat_rate;
    std::string m_input_buffer;
    bool m_new_number_started;
    bool m_has_decimal_point;
    bool m_has_error;
    bool m_show_result;
    std::string m_subtotal_text;

    // Helper methods
    void executeOperation();
    void addToTape(double value, char op, bool is_vat = false);
    std::string formatNumber(double value) const;
    double parseInput() const;
    void resetInput();
};

#endif
