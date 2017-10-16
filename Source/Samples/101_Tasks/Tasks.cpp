//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Tasks.h>
#include <Urho3D/Engine/Engine.h>
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
#include <Urho3D/Scene/Node.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>

#include "Tasks.h"

#include <Urho3D/DebugNew.h>

URHO3D_DEFINE_APPLICATION_MAIN(TasksSample);

TasksSample::TasksSample(Context* context) :
    Sample(context)
{
}

void TasksSample::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void TasksSample::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

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
    StaticModel* planeObject = planeNode->CreateComponent<StaticModel>();
    planeObject->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
    planeObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

    // Create a directional light to the world so that we can see something. The light scene node's orientation controls the
    // light direction; we will use the SetDirection() function which calculates the orientation from a forward direction vector.
    // The light will use default settings (white light, no shadows)
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f)); // The direction vector does not need to be normalized
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);

    Node* mushroomNode = scene_->CreateChild("Mushroom");
    StaticModel* mushroomObject = mushroomNode->CreateComponent<StaticModel>();
    mushroomObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
    mushroomObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));

    Node* mushroomTitleNode = mushroomNode->CreateChild("MushroomTitle");
    mushroomTitleNode->SetPosition(Vector3(0.0f, 1.2f, 0.0f));
    Text3D* mushroomTitleText = mushroomTitleNode->CreateComponent<Text3D>();
    mushroomTitleText->SetText("Mushroom");
    mushroomTitleText->SetFont(cache->GetResource<Font>("Fonts/BlueHighway.sdf"), 24);
    mushroomTitleText->SetColor(Color::RED);
    mushroomTitleText->SetAlignment(HA_CENTER, VA_CENTER);

    // Create a scene node for the camera, which we will move around
    // The camera will use default settings (1000 far clip distance, 45 degrees FOV, set aspect ratio automatically)
    cameraNode_ = scene_->CreateChild("Camera");
    cameraNode_->CreateComponent<Camera>();

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 3.0f, -8.f));
    cameraNode_->LookAt(mushroomNode->GetPosition());

    SetRandomSeed(static_cast<unsigned int>(time(0)));
}

void TasksSample::MushroomAI()
{
    // Implement mushroom logic.
    const char* mushroomText[] = {
        "Q: Mummy, why do all the other kids call me a hairy werewolf?",
        "A: Now stop talking about that and brush your face!",
        "Q: What did one thirsty vampire say to the other as they were passing the morgue?",
        "A: Let’s stop in for a cool one!",
        "Q: How can you tell if a vampire has a horrible cold?",
        "A: By his deep loud coffin!",
        "Q: What do skeletons say before eating?",
        "A: Bone Appetit!",
        "Q: Why did the vampire get fired from the blood bank?",
        "A: He was caught drinking on the job!",
        "Q: What is a vampire’s pet peeve?",
        "A: A Tourniquet!",
    };

    // This task runs as long as title node exists in a scene.
    WeakPtr<Node> titleNode(scene_->GetChild("MushroomTitle", true));
    for (;!titleNode.Expired();)
    {
        auto index = Random(0, SDL_arraysize(mushroomText) / 2);
        auto text3D = titleNode->GetComponent<Text3D>();

        // Mushroom says a joke question
        text3D->SetText(mushroomText[index * 2]);
        // And waits for 5 seconds. This does not block rendering.
        SuspendTask(5.f);

        // After 5 seconds mushroom tells an answer.
        text3D->SetText(mushroomText[index * 2 + 1]);
        SuspendTask(3.f);

        // And after 3 more seconds laughs.
        text3D->SetText("Hahahahaha!!!");
        // Next joke comes after 3 seconds.
        SuspendTask(3.f);

        // SuspendTask() may be called without arguments. Execution will be resumed on the next frame.
        SuspendTask();
    }
}

void TasksSample::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen. We need to define the scene and the camera
    // at minimum. Additionally we could configure the viewport screen size and the rendering path (eg. forward / deferred) to
    // use, but now we just use full screen and default render path configured in the engine command line options
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void TasksSample::SubscribeToEvents()
{
    // Create a task that will be scheduled each time E_UPDATE event is fired.
    GetTasks()->Create(E_UPDATE, std::bind(&TasksSample::MushroomAI, this));
}
