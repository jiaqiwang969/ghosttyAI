# Repository Guidelines

## Project Structure & Module Organization
- src: Zig core (`*.zig`) including terminal, renderer, CLI.
- macos: Swift/Metal app, Xcode project, entitlements.
- test: integration cases and helper scripts (`run.sh`, `run-host.sh`).
- scripts: local dev/test helpers (e.g., `test_terminal_communication.sh`).
- build.zig, build.zig.zon: Zig build config; Makefile: convenience tasks.
- zig-out, zig-cache: build outputs and cache (cleanable).
- docs, po, vendor, include, pkg: documentation, translations, third‑party, headers, packaging.

## Build, Test, and Development Commands
- Build (debug): `zig build` — fast dev build.
- Build (release): `zig build -Doptimize=ReleaseFast` — optimized binary.
- macOS app build + sign: `make build-ghostty` or `make build-ghostty-debug` — builds via Zig and applies ad‑hoc code signature.
- Run app (macOS): `make run` — launches `zig-out/Ghostty.app`.
- Unit tests: `zig build test` (filter: `zig build test -Dtest-filter=<substr>`).
- Valgrind (Linux): `zig build run-valgrind` — memory checks with suppressions.
- Clean: `make clean` — removes `zig-out`, `zig-cache`, and macOS build artifacts.

## Coding Style & Naming Conventions
- Zig: use `zig fmt` before commit; follow Zig idioms (types UpperCamelCase, functions/vars lowerCamelCase, constants lowerCamelCase).
- C/C++/ObjC: `.clang-format` (Chromium-like, 2‑space indent) governs style.
- Swift: 4‑space indent per `.editorconfig`; trim trailing whitespace.
- Shell: 2‑space indent per `.editorconfig`.
- File/dir names: kebab-case for tools/scripts; Zig files `TitleCase.zig` match types where practical.

## Testing Guidelines
- Prefer Zig unit tests co-located in modules; run with `zig build test`.
- Integration tests: see `test/` and scripts (`test/run.sh`).
- Name tests descriptively; use `-Dtest-filter` to iterate quickly.
- Before PR: run unit tests locally; validate key manual flows on your platform (see CONTRIBUTING for input/IME checks).

## Commit & Pull Request Guidelines
- Commits: imperative summary, ≤72 chars; optionally use type/scope (e.g., `fix(session): …`, `feat(renderer): …`).
- PRs: link an accepted issue; include clear description, reproduction or rationale, and screenshots/GIFs for UI changes.
- Disclose any AI assistance used in the PR (see CONTRIBUTING).
- Ensure builds/tests pass; run formatters (`zig fmt`, clang-format/Swift where applicable).

## Security & Configuration Tips
- Do not commit secrets; crash reports are stored locally under `$XDG_STATE_HOME/ghostty/crash`.
- macOS: ad‑hoc signing is applied by `make build-ghostty`; adjust if using hardened runtime or distributing.
