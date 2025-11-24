## Terminal Core Scope & Dependencies

Goal: identify the Objective-C/C components that form the upstream terminal engine so we can extract them into a platform-neutral library for the GTK shell.

### 1. Major Functional Areas
| Area | Key Files | Responsibility | Direct AppKit Usage? | Notes |
| --- | --- | --- | --- | --- |
| Parser & state machine | `VT100Terminal.{h,m}`, `VT100Parser.{h,m}`, `VT100CSIParser.{h,m}`, `VT100DCSParser.{h,m}`, `VT100StateMachine.{h,m}`, `VT100Token.{h,m}` | Parse bytes from PTY into tokens/escape sequences, manage CSI/DCS/OSC logic, delegate actions to screen | Minimal (imports `<Cocoa/Cocoa.h>` for `NSColor`, NSString utilities) | Replace AppKit types (NSColor, NSString categories) with Core equivalents |
| Screen & buffer | `Screen.{h,m}`, `ScreenChar.{h,m}`, `ScreenCharArray.{h,m}`, `VT100Grid.{h,m}`, `TerminalCursor.{h,m}` | Maintain scrollback/grid, attributes, cursor, selection | Uses `NSColor`, `NSFont`, `NSAttributedString`, `NSImage` in some helpers | Need neutral color/font structs and image hooks |
| Rendering | `PTYTextView.{h,m}`, `TerminalTextExtractor.{h,m}`, `TerminalColorMap.{h,m}`, `TerminalColorPreset.*`, `TerminalSelection.{h,m}` | Draw glyphs, handle fonts, ligatures, selection, pointer events | Heavy AppKit dependency (NSView subclass, Core Text, Metal/OpenGL switches) | Will be rewritten for GTK; reuse logical pieces only |
| PTY & process mgmt | `PTYTask.{h,m}`, `TerminalTaskQueue`, `TerminalProcessCache`, `PTYSession+Task*` | Launch shells, handle IO, watch processes | Mostly CoreFoundation + BSD APIs; minimal AppKit | Straightforward to port |
| Session orchestration | `PTYSession.{h,m}`, `PseudoTerminal.{h,m}`, `TerminalController.{h,m}` | Manage tabs/windows, session lifecycle, UI actions | Deep AppKit ties (NSWindow, NSResponder) | For core extraction, only reuse the non-UI helpers (e.g., session state, tmux bridge) |
| Tmux bridge | `TmuxController.{h,m}`, `TmuxGateway`, `TmuxLayoutParser` | Communicate with tmux server, sync panes | Minor AppKit (logging, colors) | Can live in core with small adjustments |
| Triggers & automation | `CaptureTrigger`, `TerminalTriggerController`, `TerminalScriptFunctionCall` | Regex triggers, script execution | Relies on Foundation mostly | Candidate for later extraction |

### 2. AppKit/Platform Dependencies to Remove or Abstract
- `NSColor`, `NSFont`, `NSImage`, `NSAttributedString` used in rendering and some screen helpers → introduce platform-neutral structs (`it_color`, `it_font_desc`), move conversions to UI layer.
- `NSView`, `NSResponder`, `NSTimer`, `NSEvent` references inside `PTYTextView`/`PseudoTerminal` → GTK shell will provide replacements; core should only expose callbacks.
- `NSNotificationCenter`, `NSUserDefaults` usage in core classes (e.g., `Screen`, `VT100Terminal`) → replace with lightweight observer/config injection.
- Swift/Objective-C bridging headers referenced by `PTYSession` (e.g., the upstream `SharedARC-Swift.h`) → ensure core avoids Swift-only helpers.

### 3. Extraction Strategy
1. **Create `core/` (working name `liblterm-core`)** with Meson target building on macOS/Linux.
2. **Move parser/state machine files first** – they have limited AppKit surface, so porting means replacing `NSColor`/`NSString` helpers and removing `<Cocoa/Cocoa.h>` includes.
3. **Introduce abstract interfaces** for rendering hooks:
   - `it_terminal_delegate` struct of function pointers (append glyphs, scroll, set cursor, ring bell, send data). This replaces Objective-C delegate protocols such as `VT100TerminalDelegate`.
   - `it_screen_observer` for scrollback changes, selection updates, etc.
4. **Port PTY/process layer** (`PTYTask` equivalents) using POSIX APIs and expose as C functions safe for Linux.
5. **Keep UI-heavy classes out of the core** (`PTYTextView`, `PseudoTerminal`, `TerminalController`). The GTK shell will reimplement their behavior using signals/actions while calling into the core.

### 4. Immediate Tasks
1. **Parser bundle** *(complete for MVP)*: tokenizer/state machine/token structs are now in `core/src/parser`. Ongoing work is focused on coverage (full CSI/OSC/SGR semantics) rather than scaffolding.
2. **Screen bundle** *(in progress)*: `lterm_screen` provides a basic grid/cursor/scrollback; next step is attribute handling, margins, and scroll regions pulled from `Screen.*`/`VT100Grid.*`.
3. **PTY/process layer** *(new)*: `lterm_pty` wraps `openpty`/`fork` and powers the GTK shell’s login session. Upcoming work involves session management (pty lifecycle, resizing, flow control, send data API).
4. **Tracking**: log progress in `current_work.md` and add TODOs per bundle (e.g., parser coverage, screen attributes, PTY lifecycle, tmux bridge).

### 5. References
- Core entry points today: `PTYSession` owns `VT100Terminal`, `Screen`, `PTYTextView`, `PTYTask`.
- Rendering call flow: `VT100Terminal` parses → delegate (`PTYSession`) mutates `Screen` → `PTYTextView` observes `Screen` changes → draws via Metal/Core Text.

This document should be updated as each bundle moves into the new core library or when additional dependencies surface.

