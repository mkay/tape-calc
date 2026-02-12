#!/bin/bash
# Uninstallation script for Tape Calculator desktop integration

set -e

echo "Uninstalling Tape Calculator desktop integration..."

# Determine installation directories
if [ "$EUID" -eq 0 ]; then
    # System-wide installation
    APPS_DIR="/usr/share/applications"
    ICONS_DIR="/usr/share/icons/hicolor"
    MIME_DIR="/usr/share/mime/packages"
    BIN_DIR="/usr/local/bin"
else
    # User installation
    APPS_DIR="$HOME/.local/share/applications"
    ICONS_DIR="$HOME/.local/share/icons/hicolor"
    MIME_DIR="$HOME/.local/share/mime/packages"
    BIN_DIR="$HOME/.local/bin"
fi

# Remove files
echo "Removing files..."
rm -f "$BIN_DIR/tape-calc"
rm -f "$APPS_DIR/tape-calc.desktop"
rm -f "$ICONS_DIR/scalable/apps/tape-calc.svg"
rm -f "$ICONS_DIR/48x48/apps/tape-calc.png"
rm -f "$ICONS_DIR/64x64/apps/tape-calc.png"
rm -f "$MIME_DIR/tape-calc.xml"

# Update databases
echo "Updating system databases..."
if [ "$EUID" -eq 0 ]; then
    update-desktop-database "$APPS_DIR" || true
    update-mime-database /usr/share/mime || true
    gtk-update-icon-cache -f -t /usr/share/icons/hicolor || true
else
    update-desktop-database "$APPS_DIR" || true
    update-mime-database "$HOME/.local/share/mime" || true
    gtk-update-icon-cache -f -t "$HOME/.local/share/icons/hicolor" || true
fi

echo ""
echo "✓ Uninstallation complete!"
echo ""
echo "Note: User settings in ~/.config/tape-calc/ were preserved."
echo "Remove them manually if desired."
echo ""
