//
// Copyright (c) 2022-2022 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Core/Object.h"
#include "../Scene/Serializable.h"

namespace Urho3D
{

/// Configuration file.
class URHO3D_API ConfigFile : public Object
{
    URHO3D_OBJECT(ConfigFile, Object)

public:
    /// Construct.
    ConfigFile(Context* context);
    /// Return whether the serialization is needed.
    virtual bool IsSerializable();
    /// Return whether to show "reset to default" button.
    virtual bool CanResetToDefault();

    /// Serialization must be provided for configuration file.
    virtual void SerializeInBlock(Archive& archive) override = 0;
    /// Reset settings to default.
    virtual void ResetToDefaults();

     /// Load configuration file.
    bool Load();
    /// Save configuration file.
    bool Save() const;
};

/// Configuration files manager.
class URHO3D_API ConfigManager : public Object
{
    URHO3D_OBJECT(ConfigManager, Object)

public:
    /// Construct.
    ConfigManager(Context* context);
    /// Destruct.
    ~ConfigManager() override;

    /// Set configuration directory. Executed by engine during initialization.
    void SetConfigDir(const ea::string& dir);

    /// Get configuration file.
    ConfigFile* Get(StringHash type);

    /// Get configuration file.
    template <typename T> T* Get() { return static_cast<T*>(Get(T::GetTypeStatic())); }

    /// Load configuration file.
    bool Load(ConfigFile* configFile);

    /// Save configuration file.
    bool Save(const ConfigFile* configFile);

    /// Save all configuration files.
    bool SaveAll();

private:

    ea::unordered_map<StringHash, SharedPtr<ConfigFile>> files_;

    ea::string configurationDir_;
};

} // namespace Urho3D
