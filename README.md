# lTerm2

Linux-first continuation of the upstream macOS terminal project. This repo starts empty and only
includes components that have been evaluated or rebuilt for Linux:

- `core/` – platform-neutral terminal engine (Meson project that produces
  `libiterm-core`).
- `gtk-shell/` – GTK prototype for the new UI shell.
- `docs/` – planning notes and migration guides for the port.
- `current_work.md` – feature tracking ledger.

## Getting Started

### Build the shared core
```bash
cd core
meson setup builddir
meson compile -C builddir
meson test -C builddir
```

### Run the GTK shell prototype
The GTK shell links against `libiterm_core` built in the previous step.
```bash
cd gtk-shell
meson setup builddir
meson compile -C builddir
./builddir/src/iterm2-gtk
```

## Porting workflow

1. Keep macOS-specific code in the original upstream macOS repo that lives alongside this workspace.
2. When a component becomes portable/rewritten for Linux, copy or recreate it
   here under `lTerm2/`.
3. Track parity progress in `current_work.md`.
4. Extend the migration docs in `docs/` as abstractions evolve.

