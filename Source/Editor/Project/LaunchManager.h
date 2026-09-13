// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Core/Object.h>

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

namespace Urho3D
{

/// Launch configuration.
struct LaunchConfiguration
{
    static const ea::string UnspecifiedName;

    ea::string name_;
    ea::string mainPlugin_;
    StringVariantMap engineParameters_;

    LaunchConfiguration() = default;
    LaunchConfiguration(const ea::string& name, const ea::string& mainPlugin);

    void SerializeInBlock(Archive& archive);
    unsigned ToHash() const;
};
using LaunchConfigurationVector = ea::vector<LaunchConfiguration>;

/// Manages launch configurations in the project.
class LaunchManager : public Object
{
    URHO3D_OBJECT(LaunchManager, Object);

public:
    explicit LaunchManager(Context* context);
    ~LaunchManager() override;
    void SerializeInBlock(Archive& archive) override;

    void AddConfiguration(const LaunchConfiguration& configuration);
    void RemoveConfiguration(unsigned index);
    const LaunchConfiguration* FindConfiguration(const ea::string& name) const;
    bool HasConfiguration(const ea::string& name) const;

    LaunchConfigurationVector& GetMutableConfigurations() { return configurations_; }
    const LaunchConfigurationVector& GetConfigurations() const { return configurations_; }
    StringVector GetSortedConfigurations() const;

private:
    LaunchConfigurationVector configurations_;
};

}
