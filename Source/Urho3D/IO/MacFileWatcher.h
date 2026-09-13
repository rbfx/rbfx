// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#ifdef __cplusplus
extern "C" {
#endif

/// Return true when the running OS has the specified version number or later.
bool CheckMinimalVersion(int major, int minor);

/// Return true when individual file watcher is supported by the running Mac OS X.
bool IsFileWatcherSupported();

/// Create and start the file watcher.
void* CreateFileWatcher(const char* pathname, bool watchSubDirs);

/// Stop and release the file watcher.
void CloseFileWatcher(void* watcher);

/// Read changes queued by the file watcher.
const char* ReadFileWatcher(void* watcher);

#ifdef __cplusplus
}
#endif
