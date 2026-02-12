#!/bin/bash
# Installation script for Tape Calculator desktop integration

set -e

echo "Installing Tape Calculator desktop integration..."

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

# Create directories if they don't exist
mkdir -p "$APPS_DIR"
mkdir -p "$ICONS_DIR/scalable/apps"
mkdir -p "$ICONS_DIR/48x48/apps"
mkdir -p "$ICONS_DIR/64x64/apps"
mkdir -p "$MIME_DIR"
mkdir -p "$BIN_DIR"

# Copy application binary
echo "Installing binary to $BIN_DIR..."
cp build/tape-calc "$BIN_DIR/tape-calc"
chmod +x "$BIN_DIR/tape-calc"

# Copy desktop file and update Exec path
echo "Installing desktop file..."
sed "s|Exec=tape-calc|Exec=$BIN_DIR/tape-calc|" tape-calc.desktop > "$APPS_DIR/tape-calc.desktop"
chmod 644 "$APPS_DIR/tape-calc.desktop"

# Copy icons
echo "Installing icons..."
cp assets/icon.svg "$ICONS_DIR/scalable/apps/tape-calc.svg"
cp assets/icon_48x48.png "$ICONS_DIR/48x48/apps/tape-calc.png"
cp assets/icon_64x64.png "$ICONS_DIR/64x64/apps/tape-calc.png"

# Copy MIME type definition
echo "Installing MIME type..."
cp tape-calc-mime.xml "$MIME_DIR/tape-calc.xml"

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
echo "✓ Installation complete!"
echo ""
echo "Tape Calculator has been installed to: $BIN_DIR/tape-calc"
echo ""
echo "You can now:"
echo "  • Launch from your application menu"
echo "  • Run 'tape-calc' from the command line"
echo "  • Open .calc.txt files by double-clicking them"
echo ""
