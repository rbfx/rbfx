//
// Copyright (c) 2017-2020 the rbfx project.
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
