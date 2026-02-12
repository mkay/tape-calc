# Tape Calculator

A modern GTK4-based tape calculator with a professional adding machine interface for accounting and financial calculations.

## Features

- **Professional tape display** with columnar layout, visual separators, and right-aligned amounts
- **Immediate execution model** - results calculated as you type (like traditional adding machines)
- **Basic operations**: Addition, Subtraction, Multiplication, Division, Percentage
- **VAT calculations**: Add/subtract VAT with configurable rates (e.g., `100 +VAT(19%)` → `119,00`)
- **Editable tape** - modify operations and recalculate results
- **File operations**: New, Open, Save As with timestamped filenames (e.g., `calc-260212-1430.txt`)
- **Open Recent**: Quickly access last 10 opened/saved calculations
- **Full menu bar**: File, Edit, and Help menus with keyboard shortcuts
- **Edit mode**: Enable Cut, Copy, Paste, and Select All operations when editing
- **Configurable decimal places**: 0-6 decimal places
- **Theme-aware**: Automatically adapts to your GTK theme (light/dark)
- **Settings persistence**: VAT rate, decimal places, and recent files saved between sessions
- **Locale-independent parsing**: Works correctly with both period and comma input

### Keyboard Support

**Calculator:**
| Key | Function |
|-----|----------|
| `0-9` | Enter digits |
| `.` or `,` | Decimal point |
| `+` `-` `*` `/` `%` | Operations |
| `Enter` or `=` | Calculate result |
| `Backspace` | Undo last entry |
| `Delete` or `Esc` | Clear all |

**Menus:**
- Ctrl+N: New | Ctrl+O: Open | Ctrl+Shift+S: Save As | Ctrl+Shift+N: New Window | Ctrl+Q: Quit
- Ctrl+E: Edit Mode | Ctrl+Z: Undo | Ctrl+Shift+Z: Redo | Ctrl+X/C/V: Cut/Copy/Paste | Ctrl+A: Select All

## Building

### Dependencies
- GTK 4 (gtkmm-4.0)
- C++20 compiler (GCC 10+ or Clang 10+)
- Meson build system
- Ninja

### Ubuntu/Debian
```bash
sudo apt install build-essential meson ninja-build libgtkmm-4.0-dev
```

### Arch Linux
```bash
sudo pacman -S base-devel meson ninja gtkmm-4.0
```

### Fedora
```bash
sudo dnf install meson ninja-build gtkmm4.0-devel gcc-c++
```

### Build Instructions
```bash
# Clone or navigate to the project directory
cd tape-calc

# Configure the build
meson setup build --buildtype=release

# Compile
meson compile -C build

# Run
./build/tape-calc
```

### Development Setup

For Neovim users with LSP support:
```bash
ln -s build/compile_commands.json .
```

## Usage

### Basic Calculations
Enter numbers and operations, then press `=` or `Enter` to calculate.

**Example**: `44 + 55 =`
```
+         44,00
+         55,00
---------------
=         99,00
```

### VAT Calculations
- **Add VAT**: Enter net amount, click `+VAT` button
- **Remove VAT**: Enter gross amount, click `-VAT` button
- **Change VAT rate**: Use the "VAT:" spinner to set any rate (0-100%)
- **Adjust decimal places**: Use the "Dec:" spinner (0-6 places)

**Example**: `100 +VAT(19%)`
```
+        100,00
+         19% | 19,00
---------------
+        119,00
```

Settings (VAT rate, decimal places, recent files) are automatically saved to `~/.config/tape-calc/settings.conf`.

### File Operations
- **New**: Clear tape and start fresh (Ctrl+N or File > New)
- **Open**: Load a saved calculation (Ctrl+O or File > Open)
- **Open Recent**: Quick access to last 10 files (File > Open Recent)
- **Save As**: Export with timestamped filename (Ctrl+Shift+S or File > Save As)

### Editing the Tape
1. Click **EDIT** (appears after pressing `=`) or use Edit > Edit Mode (Ctrl+E)
2. Modify operation symbols or values directly in the tape
3. Click **DONE** to recalculate
4. Edit menu operations (Cut, Copy, Paste, Select All) available in Edit mode

## Technical Details

- **Engine**: Pure C++ calculation logic with locale-independent parsing and immediate execution model
- **UI**: GTK4/gtkmm interface with responsive layout and theme integration
- **Architecture**: Separation between calculation logic (`calculator_engine.cpp`) and UI (`mainwindow.cpp`)

This calculator follows the **tape calculator** (adding machine) paradigm: immediate execution, permanent record, and professional columnar format.

## Acknowledgments

Based on [GTK4-Calculator](https://github.com/hans-chrstn/GTK4-Calculator) by hans-chrstn, enhanced with tape calculator functionality and VAT calculations. Built with [GTK 4](https://www.gtk.org/), [gtkmm](https://www.gtkmm.org/), and [Meson](https://mesonbuild.com/).

## Disclaimer

This project was created with AI assistance. The code has not been thoroughly reviewed. Verify its correctness and suitability before use. 

## Screenshot

![Screenshot of the plugin at work](assets/screenshot.png)