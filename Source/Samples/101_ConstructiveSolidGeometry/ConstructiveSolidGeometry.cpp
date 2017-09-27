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
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/DecalSet.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/SystemUI/SystemUI.h>
#include <Urho3D/SystemUI/SystemUIEvents.h>
#include <Urho3D/Scene/ConstructiveSolidGeometry.h>
#include <IconFontCppHeaders/IconsFontAwesome.h>

#include "ConstructiveSolidGeometry.h"

#include <Urho3D/DebugNew.h>


URHO3D_DEFINE_APPLICATION_MAIN(ConstructiveSolidGeometry)

ConstructiveSolidGeometry::ConstructiveSolidGeometry(Context* context) :
    Sample(context),
    drawDebug_(false),
    gizmo_(context),
    inspector_(context)
{
}

void ConstructiveSolidGeometry::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateUI();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_FREE);
}

void ConstructiveSolidGeometry::CreateScene()
{
    scene_ = new Scene(context_);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Also create a DebugRenderer component so that we can draw debug geometry
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<DebugRenderer>();
    // Set default background color.
    Zone* zone = scene_->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create the camera. Limit far clip distance and add light shining forward.
    cameraNode_ = scene_->CreateChild("Camera");
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    cameraNode_->AddChild(lightNode);

    // Set an initial position for the camera scene node above the plane
    cameraNode_->SetPosition(Vector3(0.0f, 0.0f, -5.0f));
}

void ConstructiveSolidGeometry::CreateUI()
{
    // Load font-awesome. Required for system ui icons.
    GetSubsystem<SystemUI>()->AddFont("Fonts/fontawesome-webfont.ttf", 0, {ICON_MIN_FA, ICON_MAX_FA, 0}, true);

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();

    // Create a Cursor UI element because we want to be able to hide and show it at will. When hidden, the mouse cursor will
    // control the camera, and when visible, it will point the raycast target
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");
    SharedPtr<Cursor> cursor(new Cursor(context_));
    cursor->SetStyleAuto(style);
    ui->SetCursor(cursor);
    // Set starting position of the cursor at the rendering window center
    Graphics* graphics = GetSubsystem<Graphics>();
    cursor->SetPosition(graphics->GetWidth() / 2, graphics->GetHeight() / 2);

    // Construct new Text object, set string to display and font to use
    Text* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys to move\n"
        "LMB to select object, RMB to rotate view\n"
        "Space to toggle debug geometry\n"
        "7 to toggle occlusion culling"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void ConstructiveSolidGeometry::SetupViewport()
{
    Renderer* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void ConstructiveSolidGeometry::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(ConstructiveSolidGeometry, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(ConstructiveSolidGeometry, HandlePostRenderUpdate));

    // Subscribe to system ui event for rendering debug ui and manipulation gizmo.
    SubscribeToEvent(E_SYSTEMUIFRAME, URHO3D_HANDLER(ConstructiveSolidGeometry, RenderSystemUI));
}

void ConstructiveSolidGeometry::MoveCamera(float timeStep)
{
    // Right mouse button controls mouse cursor visibility: hide when pressed
    UI* ui = GetSubsystem<UI>();
    Input* input = GetSubsystem<Input>();
    ui->GetCursor()->SetVisible(!input->GetMouseButtonDown(MOUSEB_RIGHT));

    // Do not move if the UI has a focused element (the console)
    if (ui->GetFocusElement())
        return;

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    // Only move the camera when the cursor is hidden
    if (!ui->GetCursor()->IsVisible())
    {
        input->SetMouseMode(MM_RELATIVE, true);

        IntVector2 mouseMove = input->GetMouseMove();
        yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
        pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
        pitch_ = Clamp(pitch_, -90.0f, 90.0f);

        // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
        cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));
    }
    else
        input->SetMouseMode(MM_FREE, true);

    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);

    // Toggle debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;
}

Node* ConstructiveSolidGeometry::Raycast(float maxDistance)
{
    UI* ui = GetSubsystem<UI>();
    IntVector2 pos = ui->GetCursorPosition();
    // Check the cursor is visible and there is no UI element in front of the cursor
    if (!ui->GetCursor()->IsVisible() || ui->GetElementAt(pos, true))
        return nullptr;

    Graphics* graphics = GetSubsystem<Graphics>();
    Camera* camera = cameraNode_->GetComponent<Camera>();
    Ray cameraRay = camera->GetScreenRay((float)pos.x_ / graphics->GetWidth(), (float)pos.y_ / graphics->GetHeight());
    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit
    PODVector<RayQueryResult> results;
    RayOctreeQuery query(results, cameraRay, RAY_TRIANGLE, maxDistance, DRAWABLE_GEOMETRY);
    scene_->GetComponent<Octree>()->RaycastSingle(query);
    if (results.Size())
    {
        RayQueryResult& result = results[0];
        return result.drawable_->GetNode();
    }

    return nullptr;
}

void ConstructiveSolidGeometry::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);
}

void ConstructiveSolidGeometry::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw viewport debug geometry. Disable depth test so that we can see the effect of occlusion
    if (drawDebug_)
        GetSubsystem<Renderer>()->DrawDebugGeometry(false);
}

void ConstructiveSolidGeometry::RenderSystemUI(StringHash eventType, VariantMap& eventData)
{
    // Test
//    Node* newNode = scene_->CreateChild();
//    auto model = newNode->CreateComponent<StaticModel>();
//    model->SetModel(GetCache()->GetResource<Model>("Models/Pyramid.mdl"));
//    selection_.Push(newNode);
//
//    newNode = scene_->CreateChild();
//    newNode->SetScale({0.2f, 2.f, 2.f});
//    model = newNode->CreateComponent<StaticModel>();
//    model->SetModel(GetCache()->GetResource<Model>("Models/Box.mdl"));
//    selection_.Push(newNode);
//
//    CSGManipulator csg(selection_.Front());
//    csg.Subtract(selection_.Back());
//    csg.BakeSingle();
//    selection_.Back()->Remove();
//    selection_.Pop();
//
//    engine_->Exit();


    // Render manipulation gizmo when node is selected.
    if (!selection_.Empty())
        gizmo_.Manipulate(cameraNode_->GetComponent<Camera>(), selection_);

    // Select node with the left mousebutton; cursor must be visible; must not click gizmo.
    // Selection is done after manipulation, because otherwise clicking gizmo may unselect current node.
    if (GetUI()->GetCursor()->IsVisible() && GetInput()->GetMouseButtonPress(MOUSEB_LEFT) && !gizmo_.IsActive())
    {
        if (!GetInput()->GetKeyDown(KEY_CTRL))
            selection_.Clear();

        if (auto selected = Raycast(300.f))
            selection_.Push(selected);
    }

    // Render utility window
    ui::SetNextWindowPos({0, 0}, ImGuiCond_Once);
    ui::SetNextWindowSize({420, 300}, ImGuiCond_Once);
    if (ui::Begin("ConstructiveSolidGeometry"))
    {
        if (ui::RadioButton("Translate", gizmo_.GetOperation() == GIZMOOP_TRANSLATE))
            gizmo_.SetOperation(GIZMOOP_TRANSLATE);
        ui::SameLine();
        if (ui::RadioButton("Rotate", gizmo_.GetOperation() == GIZMOOP_ROTATE))
            gizmo_.SetOperation(GIZMOOP_ROTATE);
        ui::SameLine();
        if (ui::RadioButton("Scale", gizmo_.GetOperation() == GIZMOOP_SCALE))
            gizmo_.SetOperation(GIZMOOP_SCALE);

        auto transformSpace = gizmo_.GetTransformSpace();
        // Scaling operations are done in local space of each node.
        if (gizmo_.GetOperation() == GIZMOOP_SCALE)
            transformSpace = TS_LOCAL;
        // Selection and translation are done in world space of each node when there is more than one node selected.
        else if (selection_.Size() > 1)
            transformSpace = TS_WORLD;

        if (ui::RadioButton("World", transformSpace == TS_WORLD))
            gizmo_.SetTransformSpace(TS_WORLD);
        ui::SameLine();
        if (ui::RadioButton("Local", transformSpace == TS_LOCAL))
            gizmo_.SetTransformSpace(TS_LOCAL);

        ui::TextUnformatted("Create:");
        ui::SameLine();
        String resource;
        if (ui::Button("Cube"))
            resource = "Models/Box.mdl";
        ui::SameLine();
        if (ui::Button("Cylinder"))
            resource = "Models/Cylinder.mdl";
        ui::SameLine();
        if (ui::Button("Pyramid"))
            resource = "Models/Pyramid.mdl";
        ui::SameLine();
        if (ui::Button("Sphere"))
            resource = "Models/Sphere.mdl";
        ui::SameLine();
        if (ui::Button("Torus"))
            resource = "Models/Torus.mdl";
        ui::SameLine();
        if (ui::Button("TeaPot"))
            resource = "Models/TeaPot.mdl";

        if (!resource.Empty())
        {
            selection_.Clear();
            Node* newNode = scene_->CreateChild();
            auto model = newNode->CreateComponent<StaticModel>();
            model->SetModel(GetCache()->GetResource<Model>(resource));
            selection_.Push(newNode);
        }

        ui::TextUnformatted("Operations:");
        ui::SameLine();
        if (selection_.Size() == 2)
        {
            if (ui::Button("Add"))
            {
                CSGManipulator csg(selection_.Front());
                csg.Union(selection_.Back());
                csg.BakeSingle();
                selection_.Back()->Remove();
                selection_.Pop();
            }
            ui::SameLine();
            if (ui::Button("Subtract"))
            {
                CSGManipulator csg(selection_.Front());
                csg.Subtract(selection_.Back());
                csg.BakeSingle();
                selection_.Back()->Remove();
                selection_.Pop();
            }
            ui::SameLine();
            if (ui::Button("Intersect"))
            {
                CSGManipulator csg(selection_.Front());
                csg.Intersection(selection_.Back());
                csg.BakeSingle();
                selection_.Back()->Remove();
                selection_.Pop();
            }
        }
        else
            ui::TextUnformatted("Please select two nodes (use CTRL)");

        ui::Columns(2);
        // Set width of the first column once on start.
        static bool widthSet = false;
        if (!widthSet)
        {
            ui::SetColumnWidth(0, 100);
            widthSet = true;
        }
        if (!selection_.Empty())
            inspector_.RenderAttributes(selection_.Back());
    }
    ui::End();
}
