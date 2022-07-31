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

#include "../Core/Object.h"
#include "../Core/Variant.h"
#include "../IO/Archive.h"
#include "../Engine/ApplicationFlavor.h"

namespace Urho3D
{

/// A class responsible for serializing application settings needed to launch player.
class URHO3D_API ApplicationSettings : public Object
{
    URHO3D_OBJECT(ApplicationSettings, Object);

public:
    struct FlavoredSettings
    {
        ApplicationFlavorPattern flavor_;
        StringVariantMap engineParameters_;

        void SerializeInBlock(Archive& archive);
    };
    using FlavoredSettingsVector = ea::vector<FlavoredSettings>;

    explicit ApplicationSettings(Context* context);

    /// Try to load settings for uninitialized engine.
    static SharedPtr<ApplicationSettings> LoadForCurrentApplication(Context* context);
    /// Serialize the settings.
    void SerializeInBlock(Archive& archive) override;

    /// Evaluate parameters for specifed flavor.
    StringVariantMap GetParameters(const ApplicationFlavor& flavor) const;
    /// Evaluate parameters for the flavor of the current platform.
    StringVariantMap GetParametersForCurrentFlavor() const;

    FlavoredSettingsVector& GetParametersPerFlavor() { return settings_; }

private:
    FlavoredSettingsVector settings_;
};


}
