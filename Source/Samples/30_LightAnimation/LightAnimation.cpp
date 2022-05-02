//
// Copyright (c) 2008-2022 the Urho3D project.
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

#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/Input/FreeFlyController.h>

#include "LightAnimation.h"

#include <Urho3D/DebugNew.h>

LightAnimation::LightAnimation(Context* context)
    : Sample(context)
{
}

void LightAnimation::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the UI content
    CreateInstructions();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Set the mouse mode to use in the sample
    SetMouseMode(MM_RELATIVE);
    SetMouseVisible(false);
}

void LightAnimation::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create the Octree component to the scene. This is required before adding any drawable components, or else nothing will
    // show up. The default octree volume will be from (-1000, -1000, -1000) to (1000, 1000, 1000) in world coordinates; it
    // is also legal to place objects outside the volume but their visibility can then not be checked in a hierarchically
    // optimizing manner
    scene_->CreateComponent<Octree>();

    // Create a child scene node (at world origin) and a StaticModel component into it. Set the StaticModel to show a simple
    // plane mesh with a "stone" material. Note that naming the scene nodes is optional. Scale the scene node larger
    // (100 x 100 world units)
    Node* planeNode = scene_->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 1.0f, 100.0f));
    auto* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a point light to the world so that we can see something.
    Node* lightNode = scene_->CreateChild("PointLight");
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_POINT);
    light->SetRange(10.0f);

    // Apply light animation to light node
    auto* animationController = lightNode->CreateComponent<AnimationController>();
    animationController->PlayNew(AnimationParameters{context_, "Animations/LightAnimation.xml"}.Looped());

    // Create text animation
    SharedPtr<ValueAnimation> textAnimation(new ValueAnimation(context_));
    textAnimation->SetKeyFrame(0.0f, "WHITE");
    textAnimation->SetKeyFrame(1.0f, "RED");
    textAnimation->SetKeyFrame(2.0f, "YELLOW");
    textAnimation->SetKeyFrame(3.0f, "GREEN");
    textAnimation->SetKeyFrame(4.0f, "WHITE");
    GetUIRoot()->GetChild(ea::string("animatingText"))->SetAttributeAnimation("Text", textAnimation);

    // Create UI element animation
    // (note: a spritesheet and "Image Rect" attribute should be used in real use cases for better performance)
    SharedPtr<ValueAnimation> spriteAnimation(new ValueAnimation(context_));
    spriteAnimation->SetKeyFrame(0.0f, ResourceRef("Texture2D", "Urho2D/GoldIcon/1.png"));
    spriteAnimation->SetKeyFrame(0.1f, ResourceRef("Texture2D", "Urho2D/GoldIcon/2.png"));
    spriteAnimation->SetKeyFrame(0.2f, ResourceRef("Texture2D", "Urho2D/GoldIcon/3.png"));
    spriteAnimation->SetKeyFrame(0.3f, ResourceRef("Texture2D", "Urho2D/GoldIcon/4.png"));
    spriteAnimation->SetKeyFrame(0.4f, ResourceRef("Texture2D", "Urho2D/GoldIcon/5.png"));
    spriteAnimation->SetKeyFrame(0.5f, ResourceRef("Texture2D", "Urho2D/GoldIcon/1.png"));
    GetUIRoot()->GetChild(ea::string("animatingSprite"))->SetAttributeAnimation("Texture", spriteAnimation);

    // Create more StaticModel objects to the scene, randomly positioned, rotated and scaled. For rotation, we construct a
    // quaternion from Euler angles where the Y angle (rotation about the Y axis) is randomized. The mushroom model contains
    // LOD levels, so the StaticModel component will automatically select the LOD level according to the view distance (you'll
    // see the model get simpler as it moves further away). Finally, rendering a large number of the same object with the
    // same material allows instancing to be used, if the GPU supports it. This reduces the amount of CPU work in rendering the
    // scene.
    const unsigned NUM_OBJECTS = 200;
    for (unsigned i = 0; i < NUM_OBJECTS; ++i)
    {
        Node* mushroomNode = scene_->CreateChild("Mushroom");
        mushroomNode->SetPosition(Vector3(Random(90.0f) - 45.0f, 0.0f, Random(90.0f) - 45.0f));
        mushroomNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        mushroomNode->SetScale(0.5f + Random(2.0f));
        auto* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
        mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
        mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    }

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<FreeFlyController>();
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, 0.0f));
}

void LightAnimation::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = GetUIRoot()->CreateChild<Text>();
    instructionText->SetText("Use WASD keys and mouse/touch to move");
    auto* font = cache->GetResource<Font>("Fonts/Anonymous Pro.ttf");
    instructionText->SetFont(font, 15);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, GetUIRoot()->GetHeight() / 4);

    // Animating text
    auto* text = GetUIRoot()->CreateChild<Text>("animatingText");
    text->SetFont(font, 15);
    text->SetHorizontalAlignment(HA_CENTER);
    text->SetVerticalAlignment(VA_CENTER);
    text->SetPosition(0, GetUIRoot()->GetHeight() / 4 + 20);

    // Animating sprite in the top left corner
    auto* sprite = GetUIRoot()->CreateChild<Sprite>("animatingSprite");
    sprite->SetPosition(8, 8);
    sprite->SetSize(64, 64);
}

void LightAnimation::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    SetViewport(0, viewport);
}
