## GTK Shell (C/GTK4) Overview

### Goals
- Provide a Linux-first UI while preserving the upstream macOS UX patterns (tabs, splits, profiles, triggers, search, status bar).
- Keep platform-agnostic terminal logic in reusable libraries (future `libiterm-core`).
- Make the GTK shell modular so new features (AI panel, scripting console, tmux dashboard) can be added incrementally.

### Architecture
| Component | Responsibility | Notes |
| --- | --- | --- |
| `app/` | Process lifecycle, command-line parsing, DBus registration | Wraps GtkApplication and the future automation API. |
| `ui/window/` | Top-level window, menu bar, global shortcuts, drag/drop | Owns tab/split containers, dispatches actions to controllers. |
| `ui/tab_manager/` | Tabs, split panes, layout management | Abstract tree structure to support arbitrary splits and future tiling. |
| `ui/terminal_view/` | GTK widget embedding rendering surface | Talks to libiterm-core to render cells via Cairo/GL; handles input. |
| `profiles/` | Profile models, persistence, migration from macOS formats | Reuses JSON/plist parsing from existing tools, stored under `~/.config/iterm`. |
| `integrations/` | Clipboard, notifications, keyring, URL handling | Backed by GDK, libnotify, libsecret, and xdg-desktop-portal. |
| `automation/` | DBus/gRPC surface mirroring the upstream scripting model | Eventually supports triggers, bookmarks, scripts. |

### Near-Term Deliverables
1. **Bootstrap Meson project** under `gtk-shell/` that builds a GTK4 window with placeholder terminal area.
2. **Define abstraction headers** for terminal core hooks (render frames, handle key events, session lifecycle).
3. **Implement minimal UI**: window, tab bar placeholder, profile pulldown, status placeholder.
4. **Add dev tooling**: `ninja test`, formatting (clang-format), run scripts.

### Dependencies
- GTK4, GLib, libadwaita (optional for GNOME styling).
- Future: libvterm/libiterm-core, libnotify, libsecret, xkbcommon, dbus-glib.

### Migration Steps
1. **Core extraction** – start peeling rendering/session logic from `sources/PTY*` into portable C modules.
2. **Bridge layer** – define thin C API that maps GTK events to core functions.
3. **Feature parity** – iterate through the upstream feature set (search, triggers, tmux, status bar) and port UI + glue.

### Open Questions
- How much Objective-C code will be rewritten vs. bridged via GNUStep/libobjc2?
- Rendering backend choice (pure Cairo, GTK GLArea + OpenGL, or hw-accelerated pipeline).
- Packaging target priorities (Flatpak vs. AppImage vs. native packages).

This document will evolve as we solidify the module boundaries and begin extracting shared code.

