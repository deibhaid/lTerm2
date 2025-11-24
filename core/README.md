# iterm-core (Work in Progress)

Portable terminal engine extracted from the upstream macOS terminal project. This Meson project will eventually build the parser, screen, PTY, and tmux logic used by the GTK shell.

## Building
```bash
cd core
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

Currently the library exports:
- `iterm_core_get_version()` – placeholder version metadata.
- Minimal parser API (`iterm_parser_*`) with basic ASCII/CSI/OSC/DCS tokenization.
- State machine primitives (`iterm_state_machine_*`) mirroring `VT100StateMachine`, used by the upcoming VT100 parser port.
- Token representation helpers (`iterm_token_types.h`, `iterm_csi_param.h`, `iterm_token.h/.c`, `iterm_screen_char.h`) defining shared enums, CSI params, ASCII buffers, screen-char storage, saved-data handling, key/value payloads, CR/LF counters, and subtokens.
- Byte-stream and parser-context abstractions (`iterm_reader.h/.c`, `iterm_parser_context.h`) that replace `VT100ByteStream`/`iTermParserContext` with portable equivalents.

Unit tests live under `core/tests/` (`parser_test`, `state_machine_test`). This scaffolding will be replaced with the actual VT100 implementation as files migrate from `sources/`.

