# lTerm2 Feature Tracker

Single source of truth for parity between the macOS upstream terminal and the new GTK/Linux build.

## Snapshot
- **Total features tracked:** 24
- **Completed in GTK build:** 3
- **In progress:** 4
- **Remaining:** 17
- **Last updated:** 2025-11-24

## Legend
- `Complete`: feature matches macOS behavior in GTK build.
- `In progress`: partial implementation landed; more work needed.
- `Pending`: no GTK implementation yet.

## Feature List
| Category | Feature | Description | Status | Notes |
| --- | --- | --- | --- | --- |
| Terminal Core | Terminal parser/tokenizer | VT100/VT220 state machine, CSI/OSC/DCS | Complete | `libiterm-core` parser migrated to C |
| Terminal Core | Screen/grid + scrollback | Cell buffer, cursor, wrap, scrollback | Complete | `iterm_screen` with grid/scrollback wired to parser |
| Terminal Core | GTK renderer bridge | Terminal view, Cairo/Pango output | Complete | `terminal_view` + `core_bridge` render demo |
| Terminal Core | PTY/process plumbing | PTYTask replacement, session I/O | In progress | PTY abstraction + GTK shell spawn + keyboard input path forwarding to PTY |
| Terminal Core | Mouse reporting | X10/SGR/URXVT modes | Pending | Depends on input handling |
| Terminal Core | Advanced sequences (SGR, DEC modes) | Colors, fonts, DECSC/DECRC, DECSET | Pending | Parser stubs exist only |
| Terminal Core | Split panes | Arbitrary split tree, resize, drag/drop | Pending | Needs GTK container |
| Terminal Core | Tabs & windows | Tab bar, window management, hotkeys | Pending | Placeholder only |
| Profiles | Profile management | Import/export, per-profile settings | Pending | Requires storage + UI |
| Profiles | Automatic profile switching | Rules (directory, hostname, etc.) | Pending | Dependent on session metadata |
| Input/Hotkeys | Custom key mappings | Grid UI, modifiers, presets | Pending | Need GTK keymap model |
| Input/Hotkeys | Global hotkey window | System-wide quick terminal | Pending | Requires X11/Wayland hotkey service |
| UX Enhancements | Search & Find | Inline find, regex, highlight | Pending | Requires text extraction APIs |
| UX Enhancements | Instant Replay | Scrollback snapshots, playback | Pending | Needs history buffer & UI |
| UX Enhancements | Status bar | Configurable widgets, tmux sync | In progress | GTK window shows placeholder |
| UX Enhancements | Annotations & images | Inline annotations, inline images | Pending | Need SIXEL/OSC 1337 support |
| UX Enhancements | Broadcast/linked input | Send input to multiple panes | Pending | Depends on session manager |
| Automation | Triggers & notifications | Regex triggers, scripts, sounds | Pending | Depends on scripting host |
| Automation | Scripts API (AppleScript parity) | DBus/gRPC automation surface | Pending | Design in GTK_SHELL_DESIGN |
| Integrations | tmux integration | Native tmux controller | Pending | Requires libiterm-core hooks |
| Integrations | Notifications & badges | Desktop notifications, dock badges | Pending | Map to libnotify / portals |
| Integrations | Clipboard & OSC 52 | Clipboard sync, secure paste dialog | In progress | OSC 52 demo wired to labels |
| Security | Secure keyboard entry & privacy | Disable key logging, warn user | Pending | Needs OS-level hooks |
| Configuration | Preferences UI | Panels for settings, profiles, keys | Pending | GTK design TBD |
| Packaging | Meson build + installers | Meson/Ninja, Flatpak/AppImage | In progress | Meson builds added |

## Update Process
1. When a feature moves forward, update the corresponding row’s status/notes.
2. Adjust the counts in the snapshot section to reflect the new totals.
3. Commit alongside related code changes for traceability.

