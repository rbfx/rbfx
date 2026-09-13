// Copyright (c) 2008-2020 the Urho3D project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
    enum class PageIndex : unsigned
    {
        MainMenu = 0,
        LoadGame,
    };

    AdvancedUI* GetSample() const;

    /// Implement RmlUIComponent
    /// @{
    void Update(float timeStep) override;
    void OnDataModelInitialized() override;
    void OnDocumentPostLoad() override;
    /// @}

    void OnContinue();
    void OnNewGame();
    void OnLoadGame();
    void OnDeleteGame();
    void OnSettings();
    void OnExit();

    void GoBack();

    /// UI state
    /// @{
    PageIndex currentPage_{};
    unsigned selectedGameIndex_{};
    StringVector savedGames_;
    bool isGamePlayed_{};
    /// @}

    unsigned nextGameIndex_{};
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


