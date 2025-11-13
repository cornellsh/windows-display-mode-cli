# displaymode "xrandr for Windows"

Windows command-line tool (CLI) to list and change monitor resolution, refresh rate, and orientation.

## Example
#### Set Windows Refresh Rate
- `displaymode --display \\.\\DISPLAY1 --hz 144`

#### Change Resolution
- `displaymode --display 0 --width 1920 --height 1080 --persist`

Windows CLI to list displays and change resolution, refresh rate, and rotation. Safe to script with `--dry-run`, post-apply verification, and `--json`.

Supported: Windows 7/8/10/11 (x64)

## Quick start

```cmd
displaymode --list
displaymode --list --json
displaymode --list-modes --display \\.\DISPLAY1
displaymode --display \\.\DISPLAY1 --hz 144
displaymode --display 0 --width 1920 --height 1080 --hz 120 --persist
displaymode --display "Dell" --orientation portrait --dry-run
```

#### List Display Modes
- `displaymode --list-modes --display "Dell" --json`
  
## Usage

```text
displaymode [--list | --list-modes]
            [--display <id|index|name>]
            [--width <px>] [--height <px>] [--hz <number>]
            [--orientation <landscape|portrait|landscape_flipped|portrait_flipped>]
            [--persist] [--dry-run]
            [--json] [--quiet | --verbose]
```

### Select a display

-   Device path: `\\.\DISPLAY1`
-   Index: zero-based index from `--list` (0, 1, ...)
-   Name: case-insensitive substring; fails if ambiguous

### Options

-   `--list` List active displays (index, name, source, current mode)
-   `--list-modes` List all supported modes for the selected display
-   `--width <px>`, `--height <px>` Resolution in pixels
-   `--hz <int|decimal>` Refresh rate; decimals rounded to a supported value (e.g., 59.94 -> 60)
-   `--orientation <...>` `landscape | portrait | landscape_flipped | portrait_flipped`
-   `--persist` Save across reboots; omit for session-only
-   `--dry-run` Validate only; no change
-   `--json` Structured output for list and apply
-   `--quiet | --verbose` Control human-readable verbosity

Notes: Use `--list-modes` to discover exact width/height/Hz/orientation supported by the driver and display. Prefer device path or index for scripting.

## Exit codes

```
0  changed
2  no-op (already set)
3  display not found or selection ambiguous
4  invalid arguments
5  query/list error
6  apply failed or verification mismatch
```

## Build

```bash
cmake -S . -B build
cmake --build build --config Release
```
