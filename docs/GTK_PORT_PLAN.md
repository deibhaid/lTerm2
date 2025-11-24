## GTK/Linux Port Plan

### 1. Objectives
- Deliver a first-class GTK-based terminal emulator (lTerm2) that matches the upstream macOS UX, feature depth, and polish on Linux desktops.
- Reuse as much of the existing terminal, tmux, scripting, and profile logic from `sources/` and `ThirdParty/` while replacing macOS-only dependencies (AppKit, AppleScript, Keychain, Notification Center, Sandboxing, etc.).
- Provide a path for iterative releases: start with terminal/session core, then layer on advanced UI/automation features.

### 2. Current Architecture Snapshot
- **UI & App lifecycle** – Entirely AppKit/Cocoa: e.g. `Application`/`PseudoTerminal`/`PreferencePanel` rely on `NSApplication`, `NSWindow`, `NSView`, Interface Builder `.xib`s, Touch Bar APIs, and Apple-specific notifications.
- **Terminal/session core** – Objective-C classes (`PTYSession`, `PTYTextView`, `PseudoTerminal`, upstream controller classes) implement rendering, keyboard handling, tmux integration, triggers, scripting hooks, etc., with substantial platform-neutral logic but intertwined with AppKit types and categories.
- **Automation & integrations** – AppleScript dictionary from the upstream macOS app (`automation.sdef`), macOS security (Keychain, TCC prompts, entitlements), Sparkle updater, Touch ID, Accessibility listeners, Notification Center.
- **Build tooling** – Xcode projects, Mac frameworks, entitlements, sandbox helpers. No Meson/CMake or Linux CI artifacts yet.

### 3. Target Architecture (Linux)
| Layer | Responsibilities | Notes |
| --- | --- | --- |
| **liblterm-core** | Terminal emulation, rendering pipeline, tmux bridge, triggers, profile models, scripting hooks | Refactor Objective-C into platform-neutral C/C++ (or Swift w/ GNUStep) and wrap via C ABI for GTK UI. Separate rendering backend (e.g., expose draw buffer) from AppKit. |
| **Platform services** | Clipboard, notifications, secure storage, global hotkeys, URL handlers | Map to Linux equivalents: GTK clipboard, libnotify, libsecret/gnome-keyring, xdg-desktop-portal, DBus, libxkbcommon for hotkeys. |
| **GTK UI Shell** | Windows, tabs, splits, preferences, status bar, inspector panes | Recreate the upstream UI using GTK widgets. Provide CSS/themeing to mimic macOS styling. Implement layout manager for tabs/splits and profile editor UI. |
| **Automation layer** | DBus API and/or gRPC/gRPC-web for scripting; optional Python plug-ins | Replace AppleScript with DBus interface; expose same operations as current scripting bridge. Integrate existing Python helpers under `tools/` where possible. |
| **Packaging** | Meson/CMake build, Flatpak/AppImage packages, CI for Ubuntu/Fedora/Arch | Provide developer docs and scripts for local builds and tests. |

### 4. Workstreams & Milestones
1. **Codebase audit & extraction**
   - Inventory cross-platform logic in `sources/PTYSession*`, `sources/PTYTextView*`, `sources/PseudoTerminal*`, tmux integrations, triggers, scripting helpers.
   - Identify AppKit categories/mixins that must be replaced or shimmed.
   - Define new core module boundaries (headers, unit tests).
2. **Core refactor**
   - Strip direct `NS*` types from reusable code; introduce abstraction interfaces for font, color, clipboard, timers, run loops.
   - Introduce Meson/CMake project that builds `liblterm-core` on macOS/Linux to validate portability.
3. **Platform services layer**
   - Implement Linux glue (GTK clipboard, libnotify, libsecret, global hotkeys using X11/Wayland APIs).
   - Replace Keychain usage with libsecret backend; substitute macOS notifications with freedesktop notifications.
4. **GTK UI shell**
   - Build GTK app skeleton with session list, tabs, split panes, preference dialog.
   - Port profile editor UI, color picker, triggers, key mappings.
   - Implement rendering widget backed by liblterm-core’s drawing buffer (Cairo or OpenGL).
5. **Automation & scripting**
   - Design DBus or gRPC API mirroring the upstream AppleScript schema.
   - Rehost Python tools to call new API; ensure triggers/scripting console continue to work.
6. **Feature parity & polish**
   - Reintroduce advanced features: session restoration, search, instant replay, annotations, status bar components, AI integration as time allows.
   - Decide on replacements for Sparkle updates (Flatpak updates, built-in updater, or distro repos).
7. **Packaging & CI**
   - Add GitHub/GitLab CI for Linux builds.
   - Provide Flatpak manifest, AppImage recipe, and Debian/Fedora spec files.
   - Document developer setup (dependencies, build commands, tests).

### 5. Immediate Next Steps
1. Choose implementation language/bindings for GTK layer (C/gtk, C++/gtkmm, Swift/SwiftGTK, Rust/gtk-rs). This affects how much Objective-C code can be reused directly.
2. Produce a component map showing which Objective-C files migrate to liblterm-core vs. Linux-specific replacements.
3. Prototype minimal liblterm-core build: extract `VT100Terminal`, `Screen`, `PTYTask` equivalents and compile standalone on Linux to validate dependency surface.
4. Spike a GTK window hosting a Cairo/GL drawing area that can consume a dummy frame buffer from the core, ensuring rendering path is viable.

### 6. Risks & Considerations
- **Scope size**: the upstream macOS codebase is ~2.4k source files; expect a multi-quarter effort. Prioritize feature sets for incremental releases.
- **Objective-C reuse**: Running ObjC on Linux is possible via GNUStep, but many AppKit features aren’t available; a reimplementation in C++/Rust/Swift may be more sustainable.
- **Performance parity**: Metal-based rendering, Core Animation, and event tap hacks must be redesignated for X11/Wayland; benchmarking will be necessary.
- **Automation/security**: Replacing AppleScript/Keychain with DBus/libsecret needs careful security design.
- **Maintenance load**: Keep macOS and Linux builds in sync by sharing as much code as feasible and enforcing common tests.

### 7. Tracking
- Create GitLab/GitHub issues per workstream with acceptance criteria.
- Use feature parity checklist to track macOS vs. Linux functionality.
- Establish nightly Linux CI once core library compiles.

This document should evolve as we validate toolchain choices and discover additional dependencies. Update with concrete design decisions, interface definitions, and timelines as milestones progress.

