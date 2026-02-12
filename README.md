# Tape Calculator

A modern GTK4-based tape calculator with a professional adding machine interface for accounting and financial calculations.

## Features

- **Professional tape display** with columnar layout, visual separators, and right-aligned amounts
- **Immediate execution model** - results calculated as you type (like traditional adding machines)
- **Basic operations**: Addition, Subtraction, Multiplication, Division, Percentage
- **VAT calculations**: Add/subtract VAT with configurable rates (e.g., `100 +VAT(19%)` → `119,00`)
- **Editable tape** - modify operations and recalculate results
- **File operations**: New, Open, Save, Save As with timestamped filenames (e.g., `260212-1430.calc.txt`)
- **Smart file tracking**: Modified indicator (*) in title bar, unsaved changes warning on close
- **Quick Save**: Save to current file with Ctrl+S
- **History feature**: Save to history folder for automatic archiving of calculations
- **Open Recent**: Quickly access last 10 opened/saved calculations
- **Browse History**: Browse and open past calculations from history folder
- **Copy Total**: Copy result to clipboard with Shift+Ctrl+C
- **Smart subtotals**: Automatically converts `=` to `ST` when continuing after a total
- **Clear confirmation**: Prevents accidental data loss when pressing AC/Clear
- **Full menu bar**: File, Edit, and Help menus with keyboard shortcuts
- **Edit mode**: Enable Cut, Copy, Paste, and Select All operations when editing
- **Configurable decimal places**: 0-6 decimal places
- **Theme-aware**: Automatically adapts to your GTK theme (light/dark)
- **Settings persistence**: VAT rate, decimal places, and recent files saved between sessions
- **Robust parsing**: Handles European number formats (comma as decimal, period as thousands separator)

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
- **File**: Ctrl+N: New | Ctrl+O: Open | Ctrl+S: Save | Ctrl+Shift+S: Save As | Ctrl+Shift+N: New Window | Ctrl+Q: Quit
- **Edit**: Ctrl+E: Edit Mode | Ctrl+X/C/V: Cut/Copy/Paste | Ctrl+A: Select All | Ctrl+Shift+C: Copy Total

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

### Desktop Integration (Optional)

Install the application with desktop menu integration and file associations:

```bash
# User installation (recommended)
./install-desktop.sh

# OR system-wide installation (requires root)
sudo ./install-desktop.sh
```

This will:
- Install the application to `~/.local/bin` (or `/usr/local/bin` for system-wide)
- Add a launcher to your application menu
- Associate `.calc.txt` files with Tape Calculator
- Install application icons

To uninstall:
```bash
./uninstall-desktop.sh
# OR
sudo ./uninstall-desktop.sh
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
  - Confirmation dialog if there are unsaved calculations
- **Open**: Load a saved calculation (Ctrl+O or File > Open)
- **Open Recent**: Quick access to last 10 files (File > Open Recent)
- **Save**: Quick save to current file (Ctrl+S or File > Save) - enabled when a file is open
- **Save As**: Export with timestamped filename (Ctrl+Shift+S or File > Save As)
- **Browse History**: Browse and open past calculations (File > Browse History)
  - Opens file dialog in `~/.config/tape-calc/history/` folder

**File Tracking:**
- Title bar shows current filename: `Tape Calculator - 260212-1430.calc.txt`
- Modified indicator (*) appears when changes are unsaved: `Tape Calculator - 260212-1430.calc.txt*`
- Enhanced unsaved changes dialog with 4 options:
  - **Cancel** - Stay in the app
  - **Don't Save** - Close without saving
  - **Save As...** - Choose location with file dialog
  - **Save to History** - Auto-save to history folder (default)

**History Feature:**
- Calculations can be saved to history folder: `~/.config/tape-calc/history/`
- Filename format: `YYMMDD-HHMMSS.calc.txt` (e.g., `260212-173045.calc.txt`)
- History files are automatically added to "Open Recent" menu
- Use "Browse History" to explore all past calculations

**File Format:**
- Files use `.calc.txt` extension (plain text format)
- Can be opened with any text editor
- Backward compatible with old `.txt` files

### Editing the Tape
1. Click **EDIT** (appears after pressing `=`) or use Edit > Edit Mode (Ctrl+E)
2. Modify operation symbols or values directly in the tape
3. Click **DONE** to recalculate
4. Edit menu operations (Cut, Copy, Paste, Select All) available in Edit mode

### Quick Copy Result
Use **Copy Total** (Ctrl+Shift+C or Edit > Copy Total) to copy the current result to clipboard. The menu item is automatically enabled when there's a calculation result available.

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