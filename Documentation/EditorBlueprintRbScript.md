# Blueprint and rbscript editor workflow

## Blueprint interactions

The Blueprint canvas now defers context-menu opening to the next UI frame. This prevents an immediate redraw or selection update from closing the menu after a right-click. The existing delete action remains available from the context menu and operates on the selected Blueprint asset after confirmation.

Blueprint assets can be selected from the resource browser, opened in the Blueprint tab, and removed through the context menu. The delete action is intentionally routed through the project resource request system so the browser, active editor tab, and filesystem state remain synchronized.

## rbscript files

The resource browser registers the `rbscript` resource factory. From the current folder, use **New rbscript** to create a source file under `Data/Scripts/`. The editor generates a unique name such as `Scripts/NewScript.rbscript`, `Scripts/NewScript1.rbscript`, and so on, without overwriting an existing file.

The integrated **rbscript** tab provides the following workflow:

| Command | Behavior |
|---|---|
| New rbscript | Creates a source file from the built-in typed gameplay template and opens it. |
| Open Browser | Activates the resource browser so an existing `.rbscript` file can be selected. |
| Compile | Runs the current rbscript document through the editor compiler path. |
| Save | Writes the active source to the project `Data/` directory. |
| Auto compile | Recompiles the active document after edits. |
| Preview | Shows lexical/token information for the current source. |
| Diagnostics | Shows compiler and runtime diagnostics associated with the document. |

The source editor uses a multiline text control with tab support and keeps an in-memory document per open resource. Saving is performed through the editor resource request system, and external resource reloads update the active document.

## Validation

The editor configuration was compiled successfully on Linux with `URHO3D_EDITOR=ON`. The complete engine test suite remains green: **305/305 tests passed**.

## Remaining production work

The current workflow is functional, but the following items are still appropriate for a production-grade editor iteration:

1. Add a confirmation dialog and undo entry for destructive Blueprint deletion.
2. Add file-system watcher conflict resolution when a script is modified both externally and inside the editor.
3. Add syntax highlighting, symbol navigation, rename refactoring, and code completion backed by the rbscript type registry.
4. Add a dedicated rbscript debugger with breakpoints, call stack, locals, watches, and hot reload state migration.
5. Add Blueprint asset recovery, version history, graph diff, and merge tools for collaborative editing.
6. Add automated UI tests for right-click menus, deletion confirmation, resource creation, save/reload, and editor restart persistence.
7. Rebuild and smoke-test the Windows distribution after each editor source update; the Linux build verifies compilation but cannot execute the Windows GUI in this environment.

These items are extensions of the current editor workflow, not prerequisites for creating or editing a basic Blueprint or rbscript file.
