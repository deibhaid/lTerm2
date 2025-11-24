## VT100 Parser Migration Plan

Purpose: capture the concrete steps needed to move the Objective-C parser/terminal stack into `libiterm-core` while keeping behavior parity with the macOS upstream terminal.

### 1. Modules to Extract
| Module | Source files | Notes |
| --- | --- | --- |
| Token data structures | `VT100Token.{h,m}`, `VT100CSIParam.{h,m}` | Convert Objective-C classes into C structs (`iterm_token`, `iterm_csi_param`). **Done:** `iterm_token_types.h`, `iterm_csi_param.h`, `iterm_screen_char.h`, `iterm_token.h/.c`.
| Parser logic | `VT100Parser.{h,m}`, `VT100CSIParser.{h,m}`, `VT100DCSParser.{h,m}`, `VT100StateMachine.{h,m}` | Port state machine and tokenization; placeholder parser currently emits basic ASCII/CSI/OSC tokens using new structs. Next: move real VT100 code over.
| Terminal driver | `VT100Terminal.{h,m}` | Implements interpretation of tokens and delegate callbacks (cursor movement, graphic rendition). Needs C delegate interface. |

### 2. Target Structure
```
core/
  include/
    iterm_token.h        # C structs/enums for tokens
    iterm_terminal.h     # Terminal instance + delegate vtable
  src/parser/
    vt100_token.c
    vt100_parser.c
    vt100_terminal.c
```

### 3. Abstractions Needed
- **Strings/Unicode**: replace `NSString`/`NSData` with UTF-8 byte arrays; limit conversions to portability-friendly helpers (`iterm_unicode_utils`).
- **Colors/Attributes**: move `VT100GraphicRendition` fields into plain structs (ints/floats) so rendering layers can interpret them without AppKit.
- **Delegates**: define `struct iterm_terminal_delegate` with callbacks:
  - `append_text`, `append_ascii`, `line_feed`, `cursor_move`, `bell`, `report`, `set_mode`, etc.
  - Provide context pointer passed through to each callback.
- **Byte stream**: `iterm_reader` (new) mirrors `VT100ByteStream` to feed the parser without Objective-C dependencies.

### 4. Migration Steps
1. **Token structs** *(in progress)* – shared enums/structs complete (see above). Remaining work: mixed ASCII CR/LF helpers, gang tokens, and unit tests mirroring `VT100TokenTests`.
2. **State machine** *(done)* – `iterm_state_machine_*` APIs now provide the transition framework formerly implemented by `VT100StateMachine`.
3. **Parser** – move `VT100Parser`/`VT100CSIParser` into the new C state machine, using the newly added `iterm_reader`, `iterm_parser_context`, and token structs. Replace categories (`NSData+iTerm`, `NSArray+iTerm`) with helper functions. Placeholder parser currently lives in `core/src/parser/iterm_parser.c`.
4. **Terminal delegate** – implement the methods that interpret tokens and call delegate callbacks; ensure feature flags (keypad mode, wraparound, moreFix, etc.) remain.
5. **Wire to GTK shell** – once the C API is stable, update macOS code (`PTYSession`) to use it via Objective-C wrappers for shared behavior, while the GTK shell uses it directly.

### 5. Testing
- Reuse existing Objective-C unit tests by building them against the C headers via bridging headers, ensuring no regressions.
- Add new C tests under `core/tests/` covering CSI parsing, OSC parsing, DCS sequences, and terminal actions (cursor movement, graphic rendition changes).

### 6. Risks
- Hidden dependencies on NSDictionary/NSArray categories for parsing; may need to re-implement utilities in C.
- Unicode normalization differences after dropping NSString APIs.
- Ensuring both macOS and Linux builds consume the same C API without duplicate logic.

This plan should be updated as each module migrates, documenting any deviations or new helper abstractions added along the way.

