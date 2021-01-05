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

namespace Urho3D
{

/// A class responsible for serializing application settings. It will be used by editor to save initial settings for player application to load them.
class URHO3D_API ApplicationSettings : public Object
{
    URHO3D_OBJECT(ApplicationSettings, Object);
public:
    explicit ApplicationSettings(Context* context);

    ///
    bool Serialize(Archive& archive) override;

    /// Resource name of scene that player application will start.
    ea::string defaultScene_{};
    /// A map of settings which will be loaded into Application::engineParameters_ on player startup.
    StringVariantMap engineParameters_{};
    /// Plugins to be loaded by the player application.
    StringVector plugins_{};
    /// List on platforms this package is valid on.
    StringVector platforms_{};
};


}
