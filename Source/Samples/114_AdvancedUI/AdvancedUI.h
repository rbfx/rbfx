//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include "Sample.h"

#include <Urho3D/UI/Text3D.h>
#include <Urho3D/RmlUI/RmlUIComponent.h>

#include <RmlUi/Core.h>

namespace Urho3D { class RmlUI; }

class AdvancedUI;

/// A 2D UI window, managed by main UI instance returned by GetSubsystem<RmlUI>().
class AdvancedUIWindow : public RmlUIComponent
{
    URHO3D_OBJECT(AdvancedUIWindow, RmlUIComponent);

public:
    explicit AdvancedUIWindow(Context* context);

private:
    AdvancedUI* GetSample() const;

    /// Implement RmlUIComponent
    /// @{
    void Update(float timeStep) override;
    void OnDataModelInitialized() override;
    /// @}

    void OnContinue();
    void OnNewGame();
    void OnLoadGame();
    void OnSettings();
    void OnExit();

    int newGameIndex_{1};

    StringVector savedGames_;
    ea::string gameToLoad_;

    bool isGamePlayed_{};
    ea::string playedGameName_;
};

/// A RmlUI demonstration.
class AdvancedUI : public Sample
{
    URHO3D_OBJECT(AdvancedUI, Sample);

public:
    /// Construct.
    explicit AdvancedUI(Context* context);
    /// Setup after engine initialization and before running the main loop.
    void Start() override;
    /// Disable exit on Esc.
    bool IsEscapeEnabled() override { return false; }

private:
    friend class AdvancedUIWindow;

    /// Initialize 3D scene.
    void InitScene();
    /// Initialize UI subsystems, backbuffer and cube windows.
    void InitWindow();
    /// Initialize currently played game.
    void InitGame(bool gamePlayed, const ea::string& text = EMPTY_STRING);
    /// Handle keys.
    void OnUpdate(StringHash, VariantMap&);

    /// Window which will be rendered into backbuffer.
    WeakPtr<AdvancedUIWindow> window_;
    /// 3D text that acts as indicator of played game.
    WeakPtr<Text3D> text3D_;
};


