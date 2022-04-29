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

#include "../Engine/SingleStateApplication.h"
#include "Sprite.h"

namespace Urho3D
{

/// Splash screen application state.
class URHO3D_API SplashScreen : public ApplicationState
{
    URHO3D_OBJECT(SplashScreen, ApplicationState)

public:
    /// Construct.
    explicit SplashScreen(Context* context);

    /// Activate game screen. Executed by Application.
    void Activate(SingleStateApplication* application) override;

    /// Handle the logic update event.
    void Update(float timeStep) override;

    bool FetchSceneResourcesAsync(const ea::string& fileName);

    void SetBackgroundImage(Texture* image);
    void SetForegroundImage(Texture* image);
    void SetProgressImage(Texture* image);
    void SetNextState(ApplicationState* state);

private:
    unsigned maxResourceCounter_ = 0;

    SharedPtr<Scene> scene_;

    SharedPtr<ApplicationState> nextState_;
    SharedPtr<Sprite> background_;
    SharedPtr<Sprite> foreground_;
    SharedPtr<Sprite> progressBar_;
};

} // namespace Urho3D
