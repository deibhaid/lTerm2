# GTK Shell Prototype

Minimal GTK4 application that will evolve into the Linux front-end for iTerm2.

## Build Prereqs
- GTK 4 development packages (e.g., `gtk4`, `libadwaita-1-dev` optional).
- Meson ≥ 1.2, Ninja, pkg-config, a C17-capable compiler.

## Quick Start
```bash
cd gtk-shell
meson setup builddir
meson compile -C builddir
./builddir/src/iterm2-gtk
```

The current window spawns your login shell inside a PTY, feeds it through `libiterm-core`, and renders the screen grid via a Cairo/Pango drawing area. The tab/status widgets are still placeholders.

## Next Steps
- Expand libiterm-core coverage (full CSI/OSC/SGR handling, scrollback operations).
- Flesh out widgets: tab manager, split pane controller, profile selector, status bar.
- Integrate DBus automation and Linux platform services (notifications, keyring, hotkeys).

