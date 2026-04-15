# GDB pretty-printers (rbfx)

This folder contains GDB Python pretty-printers for rbfx types:

- EASTL containers: `eastl_printers.py`
- Urho3D/rbfx engine types: `rbfx_printers.py`

## Quick use

In a GDB session:

- `source /path/to/rbfx/script/gdb/eastl_printers.py`
- `source /path/to/rbfx/script/gdb/rbfx_printers.py`

Both scripts auto-register printers on load.

## Permanent setup

Add to your `~/.gdbinit`:

- `source /path/to/rbfx/script/gdb/eastl_printers.py`
- `source /path/to/rbfx/script/gdb/rbfx_printers.py`

## VS Code `launch.json` (cppdbg + GDB)

If you debug with the VS Code C++ extension (`cppdbg` using `MIMode: gdb`), you can auto-load the printers via `setupCommands`.

Example (single-folder workspace):

```json
{
    "type": "cppdbg",
    "MIMode": "gdb",
    "setupCommands": [
        {
            "description": "Enable pretty-printing for gdb",
            "text": "-enable-pretty-printing",
            "ignoreFailures": true
        },
        {
            "description": "Load rbfx pretty-printers (EASTL)",
            "text": "source ${workspaceFolder}/script/gdb/eastl_printers.py",
            "ignoreFailures": true
        },
        {
            "description": "Load rbfx pretty-printers (Urho3D/rbfx)",
            "text": "source ${workspaceFolder}/script/gdb/rbfx_printers.py",
            "ignoreFailures": true
        }
    ]
}
```

Multi-root workspaces should use `${workspaceFolder:name}` (replace `name` with your workspace name)

If GDB complains about auto-load safe-path, you may need to allow this repo path:

- `set auto-load safe-path /`

(or a more restrictive path covering your source directory).
