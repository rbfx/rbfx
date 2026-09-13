// Copyright (c) 2008-2020 the Urho3D project.
// Copyright (c) 2020-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Sample.h"

#include <Urho3D/RmlUI/RmlUIComponent.h>

#include <RmlUi/Core.h>

namespace Urho3D { class RmlUI; }

/// A 2D UI window, managed by main UI instance returned by GetSubsystem<RmlUI>().
class SimpleWindow : public RmlUIComponent
{
    URHO3D_OBJECT(SimpleWindow, RmlUIComponent);
public:
    /// Construct.
    explicit SimpleWindow(Context* context);
    /// Callback function invoked from rml template.
    void CountClicks(Rml::DataModelHandle modelHandle, Rml::Event& ev, const Rml::VariantList& arguments);
    /// Process 'CloseWindow' event.
    void OnCloseWindow(StringHash, VariantMap& args);

protected:
    /// Update model and animate progressbars.
    void Update(float timeStep) override;
    /// Initialize document model.
    void OnDataModelInitialized() override;

    /// Value of UI slider.
    int sliderValue_ = 0;
    /// Value of button click counter.
    int counter_ = 0;
    /// Value of progressbar progress.
    float progress_ = 0;
    /// Value of Urho3D::Variant type.
    Variant variant_;
    /// Value of Urho3D::VariantVector type.
    VariantVector variantVector_;
    /// Value of Urho3D::VariantMap type.
    VariantMap variantMap_;
};


/// A RmlUI demonstration.
class HelloRmlUI : public Sample
{
    URHO3D_OBJECT(HelloRmlUI, Sample);
public:
    /// Construct.
    explicit HelloRmlUI(Context* context);
    /// Setup after engine initialization and before running the main loop.
    void Start() override;

private:
    /// Initialize 3D scene.
    void InitScene();
    /// Initialize UI subsystems, backbuffer and cube windows.
    void InitWindow();
    /// Animate cube, handle keys.
    void OnUpdate(StringHash, VariantMap&);

    /// Window which will be rendered into backbuffer.
    WeakPtr<SimpleWindow> window_;
    /// Window which will be rendered onto a side of a cube.
    WeakPtr<SimpleWindow> windowOnCube_;
    /// Texture to which windowOnCube_ will render.
    SharedPtr<Texture2D> texture_;
    /// Material which will apply windowOnCube_ on to a cube.
    SharedPtr<Material> material_;
};


