// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{

class Plugin;

/// Enumeration describing plugin file path status.
enum ModuleType
{
    /// Not a valid plugin.
    MODULE_INVALID,
    /// A native plugin.
    MODULE_NATIVE,
    /// A managed plugin.
    MODULE_MANAGED,
};

/// A class managing lifetime of dynamically loaded library module.
class URHO3D_API DynamicLibrary : public Object
{
    URHO3D_OBJECT(DynamicLibrary, Object);

public:
    explicit DynamicLibrary(Context* context);
    ~DynamicLibrary() override;

    /// Load a specified dynamic library and return true on success.
    bool Load(const ea::string& path);
    /// Unload currently loaded dynamic library. Returns true only if library was previously loaded and unloading succeeded.
    bool Unload();
    /// Instantiate plugin interface from DLL.
    Plugin* InstantiatePlugin();

    /// Looks up exported symbol in current loaded dynamic library and returns it. Works only for native modules.
    void* GetSymbol(const ea::string& symbol);
    /// Return a type of current loaded module.
    ModuleType GetModuleType() const { return moduleType_; }
    /// Return a path to loaded module.
    const ea::string& GetPath() const { return path_; }

    /// Inspects a specified file and detects its type.
    static ModuleType ReadModuleInformation(Context* context, const ea::string& path, unsigned* pdbPathOffset=nullptr,
        unsigned* pdbPathLength=nullptr);

private:
    /// A path of current loaded module.
    ea::string path_;
    /// A platform-specific handle to current loaded module.
    uintptr_t handle_ = 0;
    /// A type of current loaded module.
    ModuleType moduleType_ = MODULE_INVALID;
};

}
