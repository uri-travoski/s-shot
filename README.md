# S-Shot

**S-Shot** is an ultra-lightweight, high-performance Linux screenshot taking and annotation tool with a **ksnip-inspired** user interface and workflows.

It is designed to consume minimal system resources (~7–12 MB private RAM idle in the system tray, well under 60 MB under active multi-tab usage), while providing all the annotation and capture features needed for daily workflows.

---

## Features

- **System Tray Integration**:
  - Runs quietly in the background with minimal memory footprint (~7.8 MB idle).
  - Uses the custom green and white SVG icon.
  - Right-click tray menu includes:
    1. **Open**: Opens an existing image in the annotation editor.
    2. **Editor**: Opens the annotation editor to paste an image or create a blank canvas.
    3. **Capture Fullscreen**: Grabs the entire virtual display.
    4. **Capture Selected Region**: Interactive snipping overlay with 8x magnifier loupe, crosshair, pixel dimensions, and live color readout.
    5. **Colour Picker**: Interactive screen loupe displaying real-time RGB and HEX values; clicking copies `#HEX` to clipboard with a desktop notification.
    6. **Settings**: Configures autostart, save directories, formats, and hotkeys.
    7. **About**: Shows app details and a live RAM footprint meter.
    8. **Quit**: Terminates the application.
  - Closing the editor window minimizes to the system tray.

- **Annotation Editor (ksnip-like UI & Multi-Tab Support)**:
  - Multi-tab editing: open and edit multiple screenshots or images simultaneously.
  - Left-hand vertical toolbar containing all annotation tools:
    - **Pan / Hand Tool**: Smooth click-and-drag viewport panning across large images.
    - **Select Tool**: Select and move vector annotations, **plus area selection** on the screenshot canvas with instant floating action buttons:
      - 📋 **Copy**: Copies the selected area to clipboard.
      - ✂ **Cut**: Copies to clipboard and clears the area on the image.
      - 🗑 **Delete**: Erases the selected area.
      - ⛶ **Crop**: Crops the canvas to the selected area.
    - **Interactive Canvas Resize Handles**: 8 perimeter handles allowing dragging to expand the canvas from any edge or corner with full Undo/Redo.
    - **Pen**: Smooth freehand drawing.
    - **Highlighter**: Translucent highlighting brush preserving underlying text.
    - **Line**: Straight line tool.
    - **Arrow**: Clean directional arrow with arrowhead.
    - **Double Arrow**: Two-way directional arrow.
    - **Rectangle**: Outlined and filled rectangles.
    - **Ellipse**: Outlined and filled circles/ellipses.
    - **Text**: Custom text with editable font, size, and styling.
    - **Number / Stepper Badge**: Auto-incrementing circular badges (`1`, `2`, `3`...) with a one-click counter reset.
    - **Blur / Pixelate**: Redaction tool to obscure sensitive information using mosaic blocks.
    - **Crop**: Dedicated crop tool with live handles.
  - Top Toolbar: New, Open, Save, Save As, Copy to Clipboard, Paste image, Undo, Redo, Zoom In, Zoom Out, 100%, Fit to Window.
  - Properties Bar: Stroke color picker, Fill color picker, Stroke width (1–50px), Stepper reset.
  - Status Bar: Image dimensions, zoom factor, and cursor coordinates.

- **Settings, Themes & Autostart**:
  - **Dark / Light Theme**: Configurable under Settings. **Light Theme** gives a sleek, neutral **Grey UI** with high-contrast tool icons and grey canvas background; **Dark Theme** provides a clean dark styling. Changes apply live without restart.
  - **Start with PC**: Automatically creates `~/.config/autostart/s-shot.desktop` to launch minimized in the system tray on login.
  - **Save Location**: Configurable default directory (e.g. `~/Pictures/Screenshots`).
  - **Formats**: PNG, JPG, BMP, WebP.
  - **Global Hotkeys**: Global shortcuts for Fullscreen, Region, Color Picker, and Editor via X11.

---

## Installation

### 1. Debian / Ubuntu / Linux Mint (`.deb`)
```bash
sudo dpkg -i s-shot-1.0.0-Linux.deb
# or
sudo apt install ./s-shot-1.0.0-Linux.deb
```

### 2. Fedora / RHEL / openSUSE (`.rpm`)
```bash
sudo rpm -i s-shot-1.0.0-Linux.rpm
# or on Fedora:
sudo dnf install ./s-shot-1.0.0-Linux.rpm
```

### 3. Generic Linux Installer (`install.sh`)
Works on any Linux distribution (Arch, Void, Gentoo, Alpine, etc.):

- **User install (No root/sudo required)**:
  ```bash
  ./install.sh --user
  ```
  Installs to `~/.local/bin/s-shot`, `~/.local/share/applications`, and `~/.local/share/icons`.

- **System-wide install (Requires sudo)**:
  ```bash
  sudo ./install.sh --system
  ```
  Installs to `/usr/local/bin/s-shot`, `/usr/local/share/applications`, and `/usr/local/share/icons`.

- **Uninstall**:
  ```bash
  ./install.sh --uninstall
  ```

---

## Building from Source

### Dependencies
- C++20 compiler (`g++` or `clang++`)
- `cmake` (>= 3.16)
- Qt 6 (Core, Gui, Widgets, Svg, Network): `qt6-base-dev`, `qt6-svg-dev`
- X11 development headers: `libx11-dev`

### Build Instructions
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

Run directly:
```bash
./build/s-shot --tray      # Launch in system tray
./build/s-shot             # Launch annotation editor
./build/s-shot --region    # Trigger region capture
```

---

## License
GPLv3. See [LICENSE](LICENSE) for details.
