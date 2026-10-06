#!/usr/bin/env bash
# ==============================================================================
# S-Shot Installer Script for Linux
# Installs S-Shot binary, desktop launcher, and scalable icon.
# ==============================================================================

set -e

APP_NAME="s-shot"
DISPLAY_NAME="S-Shot"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Default install mode
PREFIX=""
USER_MODE=false
UNINSTALL=false

print_help() {
    cat << EOH
Usage: $0 [OPTIONS]

Options:
  --user            Install for current user only (into ~/.local) [Default if non-root]
  --system          Install system-wide (into /usr/local) [Requires root/sudo]
  --prefix=PATH     Custom install prefix (e.g. /opt/s-shot or /usr)
  --uninstall       Uninstall S-Shot from selected prefix
  -h, --help        Show this help message
EOH
}

for arg in "$@"; do
    case "$arg" in
        --user)
            USER_MODE=true
            ;;
        --system)
            USER_MODE=false
            PREFIX="/usr/local"
            ;;
        --prefix=*)
            PREFIX="${arg#*=}"
            ;;
        --uninstall)
            UNINSTALL=true
            ;;
        -h|--help)
            print_help
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            print_help
            exit 1
            ;;
    esac
done

# Determine default prefix
if [ -z "$PREFIX" ]; then
    if [ "$USER_MODE" = true ] || [ "$EUID" -ne 0 ]; then
        PREFIX="$HOME/.local"
        USER_MODE=true
    else
        PREFIX="/usr/local"
    fi
fi

BIN_DIR="$PREFIX/bin"
DESKTOP_DIR="$PREFIX/share/applications"
ICON_DIR="$PREFIX/share/icons/hicolor/scalable/apps"

# ------------------------------------------------------------------------------
# Uninstall Mode
# ------------------------------------------------------------------------------
if [ "$UNINSTALL" = true ]; then
    echo "Uninstalling $DISPLAY_NAME from $PREFIX..."
    rm -f "$BIN_DIR/$APP_NAME"
    rm -f "$DESKTOP_DIR/$APP_NAME.desktop"
    rm -f "$ICON_DIR/$APP_NAME.svg"

    if command -v update-desktop-database >/dev/null 2>&1; then
        update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
    fi
    if command -v gtk-update-icon-cache >/dev/null 2>&1; then
        gtk-update-icon-cache -q "$PREFIX/share/icons/hicolor" 2>/dev/null || true
    fi

    echo "✓ $DISPLAY_NAME has been successfully uninstalled."
    exit 0
fi

# ------------------------------------------------------------------------------
# Install Mode
# ------------------------------------------------------------------------------
echo "=================================================="
echo " Installing $DISPLAY_NAME to: $PREFIX"
echo "=================================================="

# Ensure binary is built
BINARY_PATH="$SCRIPT_DIR/build/s-shot"
if [ ! -f "$BINARY_PATH" ]; then
    echo "Binary not found at $BINARY_PATH. Building now..."
    cmake -B "$SCRIPT_DIR/build" -DCMAKE_BUILD_TYPE=Release "$SCRIPT_DIR"
    cmake --build "$SCRIPT_DIR/build" -j"$(nproc)"
fi

if [ ! -f "$BINARY_PATH" ]; then
    echo "Error: Failed to find or build binary at $BINARY_PATH."
    exit 1
fi

# Create target directories
mkdir -p "$BIN_DIR"
mkdir -p "$DESKTOP_DIR"
mkdir -p "$ICON_DIR"

# Copy files
echo "Installing binary to $BIN_DIR/$APP_NAME..."
install -m 755 "$BINARY_PATH" "$BIN_DIR/$APP_NAME"

echo "Installing icon to $ICON_DIR/$APP_NAME.svg..."
install -m 644 "$SCRIPT_DIR/resources/icons/s-shot.svg" "$ICON_DIR/$APP_NAME.svg"

echo "Installing desktop launcher to $DESKTOP_DIR/$APP_NAME.desktop..."
install -m 644 "$SCRIPT_DIR/resources/s-shot.desktop" "$DESKTOP_DIR/$APP_NAME.desktop"

# Refresh desktop caches
if command -v update-desktop-database >/dev/null 2>&1; then
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true
fi
if command -v gtk-update-icon-cache >/dev/null 2>&1; then
    gtk-update-icon-cache -q "$PREFIX/share/icons/hicolor" 2>/dev/null || true
fi

echo ""
echo "=================================================="
echo "✓ $DISPLAY_NAME installed successfully!"
echo "  Binary:  $BIN_DIR/$APP_NAME"
echo "  Desktop: $DESKTOP_DIR/$APP_NAME.desktop"
echo "  Icon:    $ICON_DIR/$APP_NAME.svg"
echo "=================================================="
if [ "$USER_MODE" = true ]; then
    echo "Tip: Make sure $BIN_DIR is in your PATH."
fi
echo "Run 's-shot' to launch or 's-shot --tray' for system tray mode."
