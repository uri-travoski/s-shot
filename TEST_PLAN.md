# S-Shot Comprehensive Test Plan

## Overview
This document outlines the complete test plan for **S-Shot** (Lightweight Screenshot & Annotation Tool for Linux). It covers every function, UI element, menu, toolbar icon, annotation item, capture subsystem, clipboard integration, and settings manager.

---

## 1. Capture Subsystem

| Test ID | Feature / Component | Description | Test Method | Expected Result |
|---|---|---|---|---|
| **CAP-01** | Fullscreen Capture | Triggers fullscreen screen grab across all monitors | Automated / Manual | Captures full virtual desktop; opens in editor tab. |
| **CAP-02** | Region Snipping Overlay | Fullscreen overlay for rectangular snip selection | Automated (`testRegionSnippingCapture`) | Displays crosshairs and magnifier loupe; emits exact crop coordinates. |
| **CAP-03** | Region Keyboard Controls | Pressing `Esc` cancels snip; `Enter` confirms active selection | Automated (`testRegionSnippingCapture`) | `Esc` closes cleanly without capture; `Enter` exports cropped region. |
| **CAP-04** | Magnifier Loupe | Pixel zoom loupe with RGB hex readout | Automated | Loupe renders magnified grid with center reticle and hex label. |
| **CAP-05** | Color Picker Overlay | Screen color eyedropper tool | Automated | Shows magnified color swatch, copies HEX to clipboard, and adds to recent colors. |

---

## 2. Editor Window & Menus

| Test ID | Menu / Action | Description | Test Method | Expected Result |
|---|---|---|---|---|
| **MENU-01** | File → New Tab | Creates empty canvas with user-specified dimensions | Automated (`testMainWindowMenus`) | Blank canvas tab created with default white background. |
| **MENU-02** | File → Open | File picker dialog for existing images | Automated | Loads PNG, JPG, BMP, WebP into a new editor tab. |
| **MENU-03** | File → Save | Saves active tab to default save location | Automated (`testDefaultSaveLocation`) | Writes file to disk with timestamp filename and configured format. |
| **MENU-04** | File → Save As... | Save dialog prompting for filename/format | Automated | Saves to selected path and updates tab title. |
| **MENU-05** | File → Close Tab | Close tab with unsaved changes prompt | Automated (`testTabCloseSaveDialogButtons`) | Prompts with `Yes`, `No`, `Cancel` buttons; respects selection. |
| **MENU-06** | Edit → Undo / Redo | History undo/redo stack (`Ctrl+Z`, `Ctrl+Y`) | Automated (`testCanvasSceneUndoRedo`) | Reverts and re-applies item additions, moves, edits, and badge numbers. |
| **MENU-07** | Edit → Copy Image | Copies active canvas or active area selection to clipboard | Automated (`testClipboardCopyHelper`) | Renders all edits/annotations and copies with `image/png` MIME data to clipboard and primary selection. |
| **MENU-08** | Edit → Reset Badge # | Resets badge numbering counter back to 1 | Automated (`testBadgeStepperAndResetNumbering`) | Resets next badge counter and toolbar spinbox to 1. |
| **MENU-09** | View → Zoom Controls | Zoom In, Zoom Out, 100%, Fit to Window | Automated (`testCanvasViewDirectMousewheelZoom`) | Adjusts canvas view scale and updates status bar zoom percentage. |
| **MENU-10** | Options → Settings | Opens preferences modal | Automated | Configures theme, hotkeys, auto-copy, magnifier, format, save location. |
| **MENU-11** | Help → Check for Updates | Manual update check against GitHub releases API | Automated (`testUpdateManagerVersionComparison`) | Compares SemVer; prompts user to update if newer release exists. |
| **MENU-12** | Help → About S-Shot | About dialog with version and licenses | Automated | Displays current version, description, and link to repository. |

---

## 3. Toolbars & Toolbar Icons

| Test ID | Toolbar Widget | Description | Test Method | Expected Result |
|---|---|---|---|---|
| **TB-01** | Main Toolbar Icons & Feedback | New (clean document page icon), Open, Save, Copy, Undo, Redo, Zoom In, Zoom Out, 100%, Fit | Automated (`testToolbarIconsAndActions`) | All icons render crisp vector graphics without missing placeholders. Actions provide immediate visual flash feedback and status bar feedback when clicked. |
| **TB-02** | Left Tool Palette | Vertical bar with 14 tools (Hand, Select, Text, Pen, Highlighter, Arrow, Line, Double Arrow, Rect, Ellipse, Badge, Blur, Bucket, Crop) | Automated (`testCanvasSceneToolIntegration`, `testToolbarIconsAndActions`) | Defaults to Hand/Pan tool. Arrow tool positioned directly below Highlighter. Toggling tools updates cursor and canvas mode. |
| **TB-03** | Stroke Color Button | Opens color picker dialog for stroke/line/badge color | Automated (`testSelectionPropertiesSync`) | Updates stroke color swatch and applies to selected item. |
| **TB-04** | Fill Color Button | Opens color picker dialog with transparent support | Automated (`testSelectionPropertiesSync`) | Updates fill color swatch (`Ø` for transparent) and applies to selected item. |
| **TB-05** | Stroke Width SpinBox | 1 to 50 px line width spinner | Automated (`testSelectionPropertiesSync`) | Visible for stroke tools; updates current line width and selected item. |
| **TB-06** | Blur Intensity SpinBox | 1 to 10 redaction intensity spinner | Automated (`testBlurItemLevels`) | Visible for Blur tool; immediately alters pixelation/blur radius. |
| **TB-07** | Font Family Dropdown | `QFontComboBox` with font list & preview | Automated (`testToolbarFontControls`) | Visible for Text tool; updates current font and selected `TextItem` family. |
| **TB-08** | Font Size SpinBox | 6 to 144 pt font size spinner (defaults to 11 pt) | Automated (`testToolbarFontControls`) | Visible for Text tool; defaults to 11 pt; updates current font size and selected `TextItem` point size. |
| **TB-09** | Badge Stepper SpinBox | `Next #: [ 1 ]` / `Badge #: [ N ]` | Automated (`testMainWindowBadgeIntegration`) | Shows next number or selected badge number; increments automatically as badges are placed. |
| **TB-10** | Reset Numbering Button | `[ Reset Numbering (1) ]` / `[ Reset to 1 ]` | Automated (`testMainWindowBadgeIntegration`) | Immediately resets counter or selected badge number to 1. |

---

## 4. Annotation Tools & Items

| Test ID | Annotation Tool | Description | Test Method | Expected Result |
|---|---|---|---|---|
| **ITEM-01** | Pen Tool (`PenItem`) | Freehand smooth path drawing | Automated (`testPenItemDrawing`) | Collects mouse points, builds smooth cubic bezier path, respects stroke width and color. |
| **ITEM-02** | Highlighter Tool | Semi-transparent highlighter drawing | Automated (`testHighlighterDrawingAndBlending`) | Renders with alpha channel, preserves underlying base pixmap details. |
| **ITEM-03** | Arrow / Double Arrow | Single and double directional arrows | Automated (`testCanvasSceneToolIntegration`) | Draws main shaft line with proportional arrow heads at endpoints. |
| **ITEM-04** | Rectangle & Ellipse | Geometric shapes with stroke and fill | Automated (`testCanvasSceneToolIntegration`) | Draws bounding rect/ellipse; supports solid fill and transparent fill. |
| **ITEM-05** | Text Tool (`TextItem`) | Multi-line text boxes with font styling | Automated (`testTextItemMultiLineAndBackground`) | Supports inline editing, custom font family, point size, text color, and textbox background. |
| **ITEM-06** | Badge Stepper (`BadgeItem`) | Numbered stepper circles (1, 2, 3...) | Automated (`testBadgeStepperAndResetNumbering`) | Dynamic font scaling (1-999+), right-click context menu, double-click in-place editing, undo/redo support. |
| **ITEM-07** | Blur / Redaction (`BlurItem`) | Redaction pixelate/blur filter | Automated (`testBlurItemEffect`) | Obfuscates sensitive base pixmap area; dynamically re-samples underlying pixmap on move. |
| **ITEM-08** | Bucket Fill Tool | Flood fill on base screenshot | Automated (`testBucketFillTool`) | Flood fills contiguous color regions within tolerance. |
| **ITEM-09** | Area Selection Tool | Crop, Cut, Move, Delete, Copy floating patch | Automated (`testCanvasSceneAreaSelectionAndCrop`) | Selects sub-rect; supports moving floating pixmap patch with undo/redo. |

---

## 5. Clipboard & System Integration

| Test ID | Component | Description | Test Method | Expected Result |
|---|---|---|---|---|
| **SYS-01** | Clipboard Helper MIME | Multi-format clipboard payload | Automated (`testClipboardCopyHelper`) | Generates `image/png`, `image/x-png`, and `image/bmp` data so GTK, Cinnamon, Wine, and web browsers can paste seamlessly. |
| **SYS-02** | Primary Selection Buffer | Linux X11 middle-click selection | Automated (`testClipboardCopyHelper`) | Populates `QClipboard::Selection` in addition to `QClipboard::Clipboard`. |
| **SYS-03** | Rendered Canvas Copy | Full edited image copying | Automated (`testClipboardCopyHelper`) | Copies rendered scene with all annotations, text, badges, and blur (not raw base pixmap). |
| **SYS-04** | Area Selection Copy | Copying cropped region of edited canvas | Automated (`testClipboardCopyHelper`) | Copies rendered crop of selected area containing all annotations. |
| **SYS-05** | Settings Persistence | Load/Save preferences to QSettings | Automated (`testAutoCheckUpdatesSetting`) | Persists all user settings across application restarts. |
| **SYS-06** | Desktop & App Icon | Freedesktop metadata and multi-res icons | Automated (`testIconManager`) | Validates `s-shot.desktop` categories, keywords, and raster PNG icons in hicolor paths. |
| **SYS-07** | System Tray Integration | Tray icon and clean text-only context menu | Automated | Displays tray icon without missing assets; context menu operates cleanly. |

---

## 6. Execution & Acceptance Criteria
- 100% of automated unit and integration tests must pass with zero crashes or leaks.
- All toolbar widgets must sync state cleanly in both light and dark themes.
- Releases must build and bundle cleanly into DEB, RPM, Flatpak, and AppImage artifacts via CI/CD.
