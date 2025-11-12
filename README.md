# displaymode

Windows command-line tool (CLI) to list and change monitor resolution, refresh rate, and orientation.
Think "xrandr for Windows."

## Windows Refresh Rate CLI
- Example: `displaymode --display \\.\\DISPLAY1 --hz 144`

## Change Resolution via Command Line
- Example: `displaymode --display 0 --width 1920 --height 1080 --persist`

## List Display Modes (Windows)
- Example: `displaymode --list-modes --display "Dell" --json`

## Usage

- List displays
  - `displaymode --list`
  - `displaymode --list --json`
- List modes
  - `displaymode --list-modes --display \\.\\DISPLAY1`
  - `displaymode --list-modes --display 0`
- Apply
  - `displaymode --display \\.\\DISPLAY1 --hz 144`
  - `displaymode --display 0 --width 1920 --height 1080 --hz 120 --persist`
  - `displaymode --display "Dell" --orientation portrait --dry-run`

## Options

- `--list` List active displays (index, name, source).
- `--list-modes` List all modes for the selected display (use with `--display`).
- `--display <id|index|name>` Select display by Windows source (for example, `\\.\\DISPLAY1`), zero-based index from `--list`, or name substring (errors if ambiguous).
- `--hz <int|decimal>` Set refresh rate; decimals are rounded (for example, 59.94 -> 60).
- `--width <px>` `--height <px>` Set resolution in pixels.
- `--orientation <landscape|portrait|landscape_flipped|portrait_flipped>` Set rotation.
- `--persist` Save to registry (persistent across reboots). Omit for session-only.
- `--dry-run` Validate only (no change).
- `--json` JSON output for list and apply.
- `--quiet` Suppress non-error messages.
- `--verbose` More detail in human-readable output.

## Exit Codes

- `0` changed
- `2` no-op (already set)
- `3` not found or ambiguous
- `4` invalid arguments
- `5` query/list error
- `6` apply failed or verification mismatch

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```

