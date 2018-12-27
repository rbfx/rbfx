//
// Copyright (c) 2008-2018 the Urho3D project.
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
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Skybox.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/CollisionShapesDerived.h>
#include "Urho3D/Physics/PhysicsWorld.h"
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include "Physics.h"
#include "PhysicsSamplesUtils.h"

#include <Urho3D/DebugNew.h>
//#include "Urho3D/Graphics/VisualDebugger.h"
#include "Urho3D/Physics/FixedDistanceConstraint.h"
#include "Urho3D/Physics/BallAndSocketConstraint.h"
#include "Urho3D/Physics/NewtonKinematicsJoint.h"
#include "Urho3D/Physics/FullyFixedConstraint.h"
#include "Urho3D/Physics/HingeConstraint.h"
#include "Urho3D/Physics/SliderConstraint.h"
#include "Urho3D/Physics/PhysicsEvents.h"
#include "Urho3D/UI/Text3D.h"

#include "Urho3D/Graphics/Terrain.h"



URHO3D_DEFINE_APPLICATION_MAIN(Physics)

Physics::Physics(Context* context) :
    Sample(context)
{
}

void Physics::Start()
{

   // context_->RegisterSubsystem<VisualDebugger>();


    // Execute base class startup
    Sample::Start();

    // Create the scene content
    CreateScene();

    // Create the UI content
    CreateInstructions();

    // Setup the viewport for displaying the scene
    SetupViewport();

    // Hook up to the frame update and render post-update events
    SubscribeToEvents();

    // Set the mouse mode to use in the sample
    Sample::InitMouseMode(MM_RELATIVE);
}

void Physics::CreateScene()
{
    auto* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    scene_->SetTimeScale(1.0f);

    // Create octree, use default volume (-1000, -1000, -1000) to (1000, 1000, 1000)
    // Create a physics simulation world with default parameters, which will update at 60fps. the Octree must
    // exist before creating drawable components, the PhysicsWorld must exist before creating physics components.
    // Finally, create a DebugRenderer component so that we can draw physics debug geometry
    scene_->CreateComponent<Octree>();
    PhysicsWorld* newtonWorld = scene_->CreateComponent<PhysicsWorld>();
    newtonWorld->SetGravity(Vector3(0, -9.81f, 0));
    newtonWorld->SetPhysicsScale(1.0f);
    //scene_->CreateComponent<NewtonCollisionShape_SceneCollision>();
    scene_->CreateComponent<DebugRenderer>();

    // Create a Zone component for ambient lighting & fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    auto* zone = zoneNode->CreateComponent<Zone>();
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(1.0f, 1.0f, 1.0f));
    zone->SetFogStart(300.0f);
    zone->SetFogEnd(500.0f);

    // Create a directional light to the world. Enable cascaded shadows on it
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.6f, -1.0f, 0.8f));
    auto* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    // Set cascade splits at 10, 50 and 200 world units, fade shadows out at 80% of maximum shadow distance
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));

    // Create skybox. The Skybox component is used like StaticModel, but it will be always located at the camera, giving the
    // illusion of the box planes being far away. Use just the ordinary Box model and a suitable material, whose shader will
    // generate the necessary 3D texture coordinates for cube mapping
    Node* skyNode = scene_->CreateChild("Sky");
    skyNode->SetScale(500.0f); // The scale actually does not matter
    auto* skybox = skyNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/Skybox.xml"));



    //CreateScenery(Vector3(0,0,0));


    //SpawnMaterialsTest(Vector3(0,-25,100));


    //SpawnCompoundedRectTest2(Vector3(100, 100, 0));

    //SpawnBallSocketTest(Vector3(50, 10, 0));
    //SpawnHingeActuatorTest(Vector3(52, 10, 0));

    //CreatePyramids(Vector3(0,0,0));


    SpawnCompound(Vector3(-2, 10 , 10));
    //SpawnConvexHull(Vector3(-2, 3, 10));

    //SpawnVehicle(Vector3(0, 10, 0));
    //for(int i = 0; i < 50; i++)
    //SpawnTrialBike(Vector3(0, 10, i*4));


    //SpawnCollisionExceptionsTest(Vector3(0, 1, 0));
    //SpawnSliderTest(Vector3(0, 10, 0));
    //SpawnLinearJointedObject(1.0f, Vector3(10 , 2, 10));

    //////
    //SpawnNSquaredJointedObject(Vector3(-20, 10, 10));

    //SpawnCompoundedRectTest(Vector3(20, 10, 10));

    ////////create scale test
    //SpawnSceneCompoundTest(Vector3(-20, 10, 20), true);
    //SpawnSceneCompoundTest(Vector3(-20, 10, 30), false);

    //CreateTowerOfLiar(Vector3(40, 0, 20));


    // Create the camera. Set far clip to match the fog. Note: now we actually create the camera node outside the scene, because
    // we want it to be unaffected by scene load / save
    cameraNode_ = new Node(context_);
    auto* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(500.0f);

    // Set an initial position for the camera scene node above the floor
    cameraNode_->SetPosition(Vector3(0.0f, 5.0f, -15.0));
}
void Physics::CreateInstructions()
{
    auto* cache = GetSubsystem<ResourceCache>();
    auto* ui = GetSubsystem<UI>();

    // Construct new Text object, set string to display and font to use
    auto* instructionText = ui->GetRoot()->CreateChild<Text>();
    instructionText->SetText(
        "Use WASD keys and mouse/touch to move\n"
        "LMB to spawn physics objects\n"
        "F5 to save scene, F7 to load\n"
        "Space to toggle physics debug geometry"
    );
    instructionText->SetFont(cache->GetResource<Font>("Fonts/Anonymous Pro.ttf"), 15);
    // The text has multiple rows. Center them in relation to each other
    instructionText->SetTextAlignment(HA_CENTER);

    // Position the text relative to the screen center
    instructionText->SetHorizontalAlignment(HA_CENTER);
    instructionText->SetVerticalAlignment(VA_CENTER);
    instructionText->SetPosition(0, ui->GetRoot()->GetHeight() / 4);
}

void Physics::SetupViewport()
{
    auto* renderer = GetSubsystem<Renderer>();

    // Set up a viewport to the Renderer subsystem so that the 3D scene can be seen
    SharedPtr<Viewport> viewport(new Viewport(context_, scene_, cameraNode_->GetComponent<Camera>()));
    renderer->SetViewport(0, viewport);
}

void Physics::SubscribeToEvents()
{
    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(Physics, HandleUpdate));

    // Subscribe HandlePostRenderUpdate() function for processing the post-render update event, during which we request
    // debug geometry
    SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(Physics, HandlePostRenderUpdate));

    SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(Physics, HandleMouseButtonUp));

    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(Physics, HandleMouseButtonDown));


    SubscribeToEvent(E_PHYSICSCOLLISIONSTART, URHO3D_HANDLER(Physics, HandleCollisionStart));


}

void Physics::MoveCamera(float timeStep)
{
    // Do not move if the UI has a focused element (the console)
    if (GetSubsystem<UI>()->GetFocusElement())
        return;

    auto* input = GetSubsystem<Input>();

    // Movement speed as world units per second
    const float MOVE_SPEED = 20.0f;
    // Mouse sensitivity as degrees per pixel
    const float MOUSE_SENSITIVITY = 0.1f;

    // Use this frame's mouse motion to adjust camera node yaw and pitch. Clamp the pitch between -90 and 90 degrees
    IntVector2 mouseMove = input->GetMouseMove();
    yaw_ += MOUSE_SENSITIVITY * mouseMove.x_;
    pitch_ += MOUSE_SENSITIVITY * mouseMove.y_;
    pitch_ = Clamp(pitch_, -90.0f, 90.0f);

    // Construct new orientation for the camera scene node from yaw and pitch. Roll is fixed to zero
    cameraNode_->SetRotation(Quaternion(pitch_, yaw_, 0.0f));

    float speedFactor = 1.0f;
    if (input->GetKeyDown(KEY_SHIFT))
        speedFactor *= 0.25f;


    // Read WASD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetKeyDown(KEY_W))
        cameraNode_->Translate(Vector3::FORWARD * MOVE_SPEED * speedFactor * timeStep);
    if (input->GetKeyDown(KEY_S))
        cameraNode_->Translate(Vector3::BACK * MOVE_SPEED * speedFactor *timeStep);
    if (input->GetKeyDown(KEY_A))
        cameraNode_->Translate(Vector3::LEFT * MOVE_SPEED * speedFactor * timeStep);
    if (input->GetKeyDown(KEY_D))
        cameraNode_->Translate(Vector3::RIGHT * MOVE_SPEED * speedFactor *timeStep);



    if (input->GetMouseButtonPress(MOUSEB_LEFT))
        CreatePickTargetNodeOnPhysics();




    if (input->GetKeyPress(KEY_R)) {

        //print stuff about the rigid body
        RayQueryResult res = GetCameraPickNode();
        if (res.node_)
        {
            RigidBody* rigBody = res.node_->GetComponent<RigidBody>();
            if (rigBody)
            {
                float mass = rigBody->GetEffectiveMass();
                URHO3D_LOGINFO("mass: " + String(mass));

            }

        }
    }

    if (input->GetKeyPress(KEY_TAB))
    {
        input->SetMouseMode(MM_ABSOLUTE);
        GetSubsystem<Input>()->SetMouseVisible(!GetSubsystem<Input>()->IsMouseVisible());
        GetSubsystem<Input>()->SetMouseGrabbed(!GetSubsystem<Input>()->IsMouseGrabbed());
       
    }


    if (input->GetMouseButtonPress(MOUSEB_RIGHT))
        DecomposePhysicsTree();

    if (input->GetMouseButtonPress(MOUSEB_MIDDLE))
    {
        FireSmallBall();
    }

    if (input->GetKeyPress(KEY_T))
        TransportNode();

    if (input->GetKeyPress(KEY_Y))
        RecomposePhysicsTree();

    if (input->GetKeyPress(KEY_DELETE))
    {
        //if()
        RemovePickNode(input->GetKeyDown(KEY_SHIFT));
    }

    if (input->GetKeyPress(KEY_PERIOD))
    {
        //do a raycast test
        PODVector<RigidBody*> bodies;

        float length = M_LARGE_VALUE;

        Ray ray = Ray(cameraNode_->GetWorldPosition(), cameraNode_->GetWorldDirection());
        //GSS<VisualDebugger>()->AddLine(ray.origin_, ray.origin_ + ray.direction_ * length, Color::RED)->SetLifeTimeMs(100000);
        PODVector<PhysicsRayCastIntersection> intersections;
        scene_->GetComponent<PhysicsWorld>()->RayCast(intersections, ray);

        for (PhysicsRayCastIntersection& intersections : intersections) {
            URHO3D_LOGINFO(String(intersections.rigBody->GetNode()->GetID()) + " " + String(intersections.rayIntersectWorldPosition));
            //GSS<VisualDebugger>()->AddCross(intersections.rayIntersectWorldPosition, 0.5f, Color::RED, false)->SetLifeTimeMs(100000);
        }
    }


    if (input->GetKeyPress(KEY_L))
    {
        //mark all physics things dirty
        PODVector<Node*> nodes;
        scene_->GetChildrenWithComponent<RigidBody>(nodes, true);

        for (Node* node : nodes)
        {
            node->GetComponent<RigidBody>()->MarkDirty();
        }
        nodes.Clear();

        scene_->GetChildren(nodes, true);

        for (Node* node : nodes)
        {
            if(node->GetDerivedComponent<CollisionShape>())
                node->GetDerivedComponent<CollisionShape>()->MarkDirty();
        }
    }

    // Check for loading/saving the scene. Save the scene to the file Data/Scenes/Physics.xml relative to the executable
    // directory
    if (input->GetKeyPress(KEY_F5))
    {
        String filePath = GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/PhysicsStressTest.xml";
        File saveFile(context_, filePath, FILE_WRITE);
        scene_->SaveXML(saveFile);


        scene_->GetComponent<PhysicsWorld>()->SerializeNewtonWorld("newtonWorldFile.ngd");


    }
    if (input->GetKeyPress(KEY_F7))
    {
        String filePath = GetSubsystem<FileSystem>()->GetProgramDir() + "Data/Scenes/PhysicsStressTest.xml";
        File loadFile(context_, filePath, FILE_READ);
        scene_->LoadXML(loadFile);
        scene_->GetComponent<DebugRenderer>()->SetView(cameraNode_->GetComponent<Camera>());
    }

    // Toggle physics debug geometry with space
    if (input->GetKeyPress(KEY_SPACE))
        drawDebug_ = !drawDebug_;


    if (0) {
        //do a collision cast around camera
        PODVector<RigidBody*> bodies;
        scene_->GetComponent<PhysicsWorld>()->GetRigidBodies(bodies, Sphere(cameraNode_->GetWorldPosition(), 2.0f));

        if (bodies.Size())
            URHO3D_LOGINFO(String(bodies.Size()));
    }

    if (1)
    {
        //test collision point on convex hull
        Node* convexHull = scene_->GetChild("convexhull", true);
        if (convexHull)
        {

            bool s = scene_->GetComponent<PhysicsWorld>()->RigidBodyContainsPoint(convexHull->GetComponent<RigidBody>(), cameraNode_->GetWorldPosition());


            if (s)
                URHO3D_LOGINFO("collision!");
        }

    }



}



void Physics::SpawnSceneCompoundTest(const Vector3& worldPos, bool oneBody)
{
    Node* root = scene_->CreateChild();
    root->SetPosition(worldPos);
    const int levelCount = 10;
    const int breadth = 2;
    Node* curNode = root;


    for (int i = 0; i < levelCount; i++)
    {

        curNode = curNode->CreateChild();
        curNode->SetName("SpawnSceneCompoundTest:" + String(i));
        curNode->AddTag("scaleTestCube");
        float rotDelta = 10.0f;

        curNode->SetScale(Vector3(Random(0.8f, 1.2f), Random(0.8f, 1.2f), Random(0.8f, 1.2f)));
        curNode->Rotate(Quaternion(Random(-rotDelta, rotDelta), Random(-rotDelta, rotDelta), Random(-rotDelta, rotDelta)));
        curNode->Translate(Vector3(Random(0.5f, 2.0f), Random(0.5f,2.0f), Random(0.5f, 2.0f)));

        StaticModel* stMdl = curNode->CreateComponent<StaticModel>();
        stMdl->SetModel(GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Cone.mdl"));
        stMdl->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/Stone.xml"));
        stMdl->SetCastShadows(true);
        if (i == 0 || !oneBody) {
            RigidBody* rigBody = curNode->CreateComponent<RigidBody>();
            rigBody->SetMassScale(1.0f);
            //rigBody->SetAngularDamping(1.0f);
        }
        CollisionShape* colShape = curNode->CreateComponent<CollisionShape_Cone>();
        colShape->SetRotationOffset(Quaternion(0, 0, 90));

        //URHO3D_LOGINFO(String(curNode->GetWorldTransform().HasSkew()));

    }
}



void Physics::SpawnObject()
{
    auto* cache = GetSubsystem<ResourceCache>();
    Node* firstNode = nullptr;
    Node* prevNode = nullptr;
    bool isFirstNode = true;
    for (int i = 0; i < 2; i++) {


        // Create a smaller box at camera position
        Node* boxNode;
        if (prevNode)
            boxNode = prevNode->CreateChild();
        else {
            boxNode = scene_->CreateChild();
            firstNode = boxNode;
        }
        prevNode = boxNode;
        const float range = 3.0f;


        boxNode->SetWorldPosition(cameraNode_->GetWorldPosition() + Vector3(Random(-1.0f,1.0f) * range,Random(-1.0f, 1.0f)* range, Random(-1.0f, 1.0f)* range));
        boxNode->SetRotation(cameraNode_->GetRotation());
        boxNode->SetScale(1.0f);

        auto* boxObject = boxNode->CreateComponent<StaticModel>();
        boxObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        boxObject->SetMaterial(cache->GetResource<Material>("Materials/StoneEnvMapSmall.xml"));
        boxObject->SetCastShadows(true);


         // Create physics components, use a smaller mass also
         auto* body = boxNode->CreateComponent<RigidBody>();
         body->SetMassScale(0.1f);
        
        

        isFirstNode = false;
        //body->SetContinuousCollision(true);
        auto* shape = boxNode->CreateComponent<CollisionShape_Box>();

        const float OBJECT_VELOCITY = 20.0f;

        // Set initial velocity for the RigidBody based on camera forward vector. Add also a slight up component
        // to overcome gravity better
       // body->SetLinearVelocity(cameraNode_->GetRotation() * Vector3(0.0f, 0.25f, 1.0f) * OBJECT_VELOCITY);

    }

}


void Physics::CreatePyramids(Vector3 position)
{
    int size = 8;
    float horizontalSeperation = 2.0f;
    //create pyramids
    const int numIslands = 0;
    for (int x2 = -numIslands; x2 <= numIslands; x2++)
        for (int y2 = -numIslands; y2 <= numIslands; y2++)
        {
            for (int y = 0; y < size; ++y)
            {
                for (int x = -y; x <= y; ++x)
                {
                    Node* node = SpawnSamplePhysicsBox(scene_, Vector3((float)x*horizontalSeperation, -(float)y + float(size), 0.0f) + Vector3(x2, 0, y2)*50.0f + position, Vector3::ONE);
                }
            }
        }
}


void Physics::CreateTowerOfLiar(Vector3 position)
{
    float length = 10.0f;
    float width = 5.0f;
    int numBoxes = 16;

    float thickness = 10.0f / (float(numBoxes));
    float fudgeFactor = 0.04f;
    Vector3 curPosition = position - Vector3(0,thickness*0.5f,0);
    for (int i = 0; i < numBoxes; i++) {
        float delta = length / (2.0f*(numBoxes - i));


        curPosition = curPosition + Vector3(delta - delta*fudgeFactor, thickness, 0);

        Node* box = SpawnSamplePhysicsBox(scene_, curPosition, Vector3(length, thickness, width));
        //box->GetComponent<RigidBody>()->SetAutoSleep(false);

  
    }




}





void Physics::SpawnConvexHull(const Vector3& worldPos)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Create a smaller box at camera position
    Node* boxNode = scene_->CreateChild("convexhull");

    boxNode->SetWorldPosition(worldPos);
    boxNode->SetScale(1.0f);

    auto* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    boxObject->SetCastShadows(true);


    // Create physics components, use a smaller mass also
    auto* body = boxNode->CreateComponent<RigidBody>();
    body->SetMassScale(1.0f);

    auto* shape = boxNode->CreateComponent<CollisionShape_ConvexHull>();


}



void Physics::SpawnCompound(const Vector3& worldPos)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Create a smaller box at camera position
    Node* boxNode = scene_->CreateChild();

    boxNode->SetWorldPosition(worldPos);
    boxNode->SetScale(1.0f);

    auto* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    boxObject->SetCastShadows(true);


    // Create physics components, use a smaller mass also
    auto* body = boxNode->CreateComponent<RigidBody>();
    body->SetMassScale(1.0f);

    auto* shape = boxNode->CreateComponent<CollisionShape_ConvexHullCompound>();

}




void Physics::SpawnDecompCompound(const Vector3& worldPos)
{
    auto* cache = GetSubsystem<ResourceCache>();

    // Create a smaller box at camera position
    Node* boxNode = scene_->CreateChild();

    boxNode->SetWorldPosition(worldPos);
    boxNode->SetScale(1.0f);

    auto* boxObject = boxNode->CreateComponent<StaticModel>();
    boxObject->SetModel(cache->GetResource<Model>("Models/Mushroom.mdl"));
    boxObject->SetMaterial(cache->GetResource<Material>("Materials/Mushroom.xml"));
    boxObject->SetCastShadows(true);


    // Create physics components, use a smaller mass also
    auto* body = boxNode->CreateComponent<RigidBody>();
    body->SetMassScale(1.0f);

    auto* shape = boxNode->CreateComponent<CollisionShape_ConvexDecompositionCompound>();
}

void Physics::SpawnNSquaredJointedObject(Vector3 worldPosition)
{
    //lets joint spheres together with a distance limiting joint.
    const float dist = 5.0f;

    const int numSpheres = 25;

    PODVector<Node*> nodes;
    //make lots of spheres
    for (int i = 0; i < numSpheres; i++)
    {
        Node* node = SpawnSamplePhysicsSphere(scene_, worldPosition  + Vector3(0,dist*0.5f,0) - Quaternion(Random()*360.0f, Random()*360.0f, Random()*360.0f) * (Vector3::FORWARD*dist));
      
        nodes += node;
        
    }


    //connect them all O(n*n) joints
    for (Node* node : nodes)
    {
        for (Node* node2 : nodes)
        {
            if (node == node2)
                continue;

            FixedDistanceConstraint* constraint = node->CreateComponent<FixedDistanceConstraint>();
            constraint->SetOwnRotation(Quaternion(45, 45, 45));
            //constraint->SetOtherRotation(Quaternion(45, 0, 0));
            constraint->SetOtherBody(node2->GetComponent<RigidBody>());
            constraint->SetOtherPosition(Vector3(0.0, 0, 0));
        }
    }
}

void Physics::SpawnGlueJointedObject(Vector3 worldPosition)
{
    //lets joint spheres together with a distance limiting joint.
    const float dist = 10.0f;

    const int numSpheres = 25;

    PODVector<Node*> nodes;
    //make lots of spheres
    for (int i = 0; i < numSpheres; i++)
    {
        Node* node = SpawnSamplePhysicsSphere(scene_, worldPosition + Vector3(0, dist*0.5f, 0) - Quaternion(Random()*360.0f, Random()*360.0f, Random()*360.0f) * (Vector3::FORWARD*dist));

        nodes += node;

    }


    //connect them all O(n*n) joints
    for (Node* node : nodes)
    {
        if (node == nodes[0])
            continue;

        FullyFixedConstraint* constraint = node->CreateComponent<FullyFixedConstraint>();
        constraint->SetOtherBody(nodes[0]->GetComponent<RigidBody>());
        constraint->SetOtherPosition(Vector3(0.0, 0, 0));

    }
}



void Physics::SpawnLinearJointedObject(float size, Vector3 worldPosition)
{
    //lets joint spheres together with a distance limiting joint.
    const float dist = size;

    const int numSpheres = 20;

    PODVector<Node*> nodes;
    //make lots of spheres
    for (int i = 0; i < numSpheres; i++)
    {
        nodes += SpawnSamplePhysicsSphere(scene_, worldPosition + Vector3(0,i*dist,0), dist*0.5f);

        if (i > 0) {
            HingeConstraint* constraint = nodes[i - 1]->CreateComponent<HingeConstraint>();
            constraint->SetOtherBody(nodes[i]->GetComponent<RigidBody>());
            constraint->SetWorldPosition(worldPosition + Vector3(0, i*dist, 0) - Vector3(0, dist, 0)*0.5f);
            //constraint->SetOwnRotation(Quaternion(0, 0, -90));
           // constraint->SetOtherRotation(Quaternion(0,0,-90));
           // constraint->SetTwistLimitsEnabled(true);
            
        }
    }
}



void Physics::SpawnMaterialsTest(Vector3 worldPosition)
{

    Node* ramp = SpawnSamplePhysicsBox(scene_, worldPosition, Vector3(100, 1, 100));
    ramp->Rotate(Quaternion(-20.0f, 0, 0));
    ramp->Translate(Vector3(0, 50, 0), TS_WORLD);
    ramp->GetComponent<RigidBody>()->SetMassScale(0);


    for (int i = 0; i < 5; i++)
    {
        Node* box = SpawnSamplePhysicsBox(scene_, ramp->GetWorldPosition() + Vector3(-2.5 + float(i)*1.1f, 2, 0), Vector3::ONE);


        CollisionShape* collisionShape = box->GetDerivedComponent<CollisionShape>();

        collisionShape->SetStaticFriction(0.1f*i - 0.05f);
        collisionShape->SetKineticFriction(0.1f*i);
      
    }

    SpawnCompoundedRectTest(ramp->GetWorldPosition() + Vector3(-5, 8, 10));

}



void Physics::SpawnBallSocketTest(Vector3 worldPosition)
{
    //lets joint spheres together with a distance limiting joint.

    Node* sphere1 =  SpawnSamplePhysicsSphere(scene_, worldPosition);
    Node* sphere2 = SpawnSamplePhysicsSphere(scene_, worldPosition + Vector3(0,2.0, 0));
    //sphere1->GetComponent<RigidBody>()->SetMassScale(0);
    BallAndSocketConstraint* constraint = sphere1->CreateComponent<BallAndSocketConstraint>();


    constraint->SetOtherWorldPosition(sphere2->GetWorldPosition() - Vector3(0, 2.0, 0));
    constraint->SetOtherBody(sphere2->GetComponent<RigidBody>());

    //sphere2->GetComponent<RigidBody>()->SetMassScale(0.0f);
    //sphere1->GetComponent<RigidBody>()->SetMassScale(0.0f);


}
void Physics::SpawnHingeActuatorTest(Vector3 worldPosition)
{
    //lets joint spheres together with a distance limiting joint.

    Node* box1 = SpawnSamplePhysicsBox(scene_, worldPosition, Vector3(10,1,10));
    Node* box2 = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(10, 0, 0), Vector3(10, 1, 10));

    //box1->GetComponent<RigidBody>()->SetAutoSleep(false);
   // box2->GetComponent<RigidBody>()->SetAutoSleep(false);


    //sphere1->GetComponent<RigidBody>()->SetMassScale(0);
    HingeConstraint* constraint = box1->CreateComponent<HingeConstraint>();
    constraint->SetWorldPosition(worldPosition + Vector3(10, 1, 0)*0.5f);
    constraint->SetWorldRotation(Quaternion(0, 90, 0));
    //constraint->SetOtherWorldPosition(sphere2->GetWorldPosition() - Vector3(0, 2.0, 0));
    constraint->SetOtherBody(box2->GetComponent<RigidBody>());


    constraint->SetPowerMode(HingeConstraint::ACTUATOR);
    constraint->SetMaxTorque(10000.0f);
   // constraint->SetEnableLimits(false);
    constraint->SetActuatorMaxAngularRate(1000.0f);
    constraint->SetActuatorTargetAngle(0.0f);
    hingeActuatorTest = constraint;
    //sphere2->GetComponent<RigidBody>()->SetMassScale(0.0f);
    //sphere1->GetComponent<RigidBody>()->SetMassScale(0.0f);


}



void Physics::SpawnCollisionExceptionsTest(Vector3 worldPosition)
{

    Node* a = SpawnSamplePhysicsBox(scene_, worldPosition, Vector3(1, 1, 1));
    Node* b = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(0,1,0), Vector3(1, 1, 1)*0.9f);
    Node* c = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(0, 1, 0)*2, Vector3(1, 1, 1)*0.8f);
    Node* d = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(0, 1, 0)*3, Vector3(1, 1, 1)*0.7f);
    Node* e = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(0, 1, 0)*4, Vector3(1, 1, 1)*0.5f);

    RigidBody* a_b = a->GetComponent<RigidBody>();
    RigidBody* b_b = b->GetComponent<RigidBody>();
    RigidBody* c_b = c->GetComponent<RigidBody>();
    RigidBody* d_b = d->GetComponent<RigidBody>();
    RigidBody* e_b = e->GetComponent<RigidBody>();


    URHO3D_LOGINFO(String(a_b->GetID()));
    URHO3D_LOGINFO(String(b_b->GetID()));
    URHO3D_LOGINFO(String(c_b->GetID()));

    a_b->SetCollisionOverride(e_b, false);

    a_b->SetCollisionOverride(d_b, false);

    a_b->SetCollisionOverride(c_b, false);

    c_b->SetNoCollideOverride(true);

}

void Physics::SpawnSliderTest(Vector3 worldPosition)
{
    Node* a = SpawnSamplePhysicsBox(scene_, worldPosition, Vector3::ONE);
    Node* b = SpawnSamplePhysicsBox(scene_, worldPosition+Vector3(1,0,0), Vector3::ONE);
    //a->GetComponent<RigidBody>()->SetMassScale(0.0f);
    

    SliderConstraint* constraint = a->CreateComponent<SliderConstraint>();
    constraint->SetOtherBody(b->GetComponent<RigidBody>());

    constraint->SetEnableSliderLimits(true, true);
    constraint->SetSliderLimits(-2, 2);

    constraint->SetEnableTwistLimits(true, true);
    constraint->SetTwistLimits(-180, 180);




    constraint->SetEnableSliderSpringDamper(true);
    constraint->SetEnableTwistSpringDamper(true);

}

void Physics::FireSmallBall()
{
    float range = 10.0f;



    for (int i = 0; i < 100; i++) {

        Vector3 posOffset = Vector3(Random(-range, range), Random(-range, range), Random(-range, range));
        int ran = Random(4);
        Node* node = nullptr;
        if (ran == 0)
            node = SpawnSamplePhysicsSphere(scene_, cameraNode_->GetWorldPosition() + posOffset);
        else if (ran == 1)
            node = SpawnSamplePhysicsBox(scene_, cameraNode_->GetWorldPosition() + posOffset, Vector3::ONE);
        else if (ran == 2)
            node = SpawnSamplePhysicsCone(scene_, cameraNode_->GetWorldPosition() + posOffset, 0.5f);
        else
            node = SpawnSamplePhysicsCylinder(scene_, cameraNode_->GetWorldPosition() + posOffset, 0.5f);


        node->GetComponent<RigidBody>()->SetLinearVelocity(cameraNode_->GetWorldDirection() * 10.0f);
        node->GetComponent<RigidBody>()->SetContinuousCollision(false);
        node->GetComponent<RigidBody>()->SetLinearDamping(0.01f);
        node->GetComponent<RigidBody>()->SetMassScale(Random(1.0f, 10.0f));
        node->GetComponent<RigidBody>()->SetGenerateContacts(false);

    }

    //if (Random(2) == 0)
    //{
    //    sphere->AddTag("bulletball_linearDamp");
    //    sphere->GetComponent<RigidBody>()->SetLinearDamping(0.5);
    //}
    //else
    //{
    //    sphere->AddTag("bulletball_customDamp");
    //    sphere->GetComponent<RigidBody>()->SetLinearDamping(0);

    //}
}

void Physics::SpawnCompoundedRectTest(Vector3 worldPosition)
{
    //make 2 1x1x1 physics rectangles. 1 with just one shape and 1 with 2 smaller compounds.

    Node* regularRect = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(-2, 0, 0), Vector3(1, 1, 2));

    Node* compoundRootRect = scene_->CreateChild();

    Model* sphereMdl = GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
    Material* sphereMat = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    Node* visualNode = compoundRootRect->CreateChild();

    visualNode->SetPosition(Vector3(0, 0, 0.5));
    visualNode->SetScale(Vector3(1, 1, 2));
    StaticModel* sphere1StMdl = visualNode->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);

    compoundRootRect->SetWorldPosition(worldPosition + Vector3(2, 0, 0));
    compoundRootRect->CreateComponent<RigidBody>();
    CollisionShape_Box* box1 = compoundRootRect->CreateComponent<CollisionShape_Box>();
    CollisionShape_Box* box2 = compoundRootRect->CreateComponent<CollisionShape_Box>();

    //Test different collision parts having different physical properties:
    box1->SetElasticity(1.0f);
    box2->SetElasticity(0.0f);


    box1->SetPositionOffset(Vector3(0, 0, 1));

}

void Physics::SpawnCompoundedRectTest2(Vector3 worldPosition)
{
    //make 2 1x1x1 physics rectangles. 1 with just one shape and 1 with 2 smaller compounds.

   // Node* regularRect = SpawnSamplePhysicsBox(scene_, worldPosition + Vector3(-2, 0, 0), Vector3(1, 1, 2));

    Node* compoundRootRect = scene_->CreateChild();
    compoundRootRect->SetWorldPosition(worldPosition + Vector3(2, 0, 0));
    compoundRootRect->CreateComponent<RigidBody>();
    


    for (int i = 0; i < 2; i++)
    {


        Node* subNode = compoundRootRect->CreateChild();
        CollisionShape_Box* box = subNode->CreateComponent<CollisionShape_Box>();

        //box->SetDensity(0.1f);

        
        subNode->SetPosition(Vector3(5.0f*i,0,0));

        Text3D* text = subNode->CreateComponent<Text3D>();
        text->SetText(String("Density: " + String(box->GetDensity()) + "\n\n\n\n\n\n\n\n\n"));
        text->SetFont(GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf"));
        text->SetFaceCameraMode(FC_LOOKAT_XYZ);
        text->SetVerticalAlignment(VA_BOTTOM);
        


        Model* sphereMdl = GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
        Material* sphereMat = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");



        StaticModel* sphere1StMdl = subNode->CreateComponent<StaticModel>();
        sphere1StMdl->SetCastShadows(true);
        sphere1StMdl->SetModel(sphereMdl);
        sphere1StMdl->SetMaterial(sphereMat);


    }










    Node* first = nullptr;
    Node* second = nullptr;

    for (int i = 0; i < 2; i++)
    {


        Node* subNode = scene_->CreateChild();
        CollisionShape_Box* box = subNode->CreateComponent<CollisionShape_Box>();
        RigidBody* rigBody = subNode->CreateComponent<RigidBody>();
        //box->SetDensity(0.1f);


        if (i == 0)
            first = subNode;
        else
            second = subNode;


        subNode->SetPosition(Vector3(5.0f*i, 0, 0) + worldPosition + Vector3(0,0,5));

        Text3D* text = subNode->CreateComponent<Text3D>();
        text->SetText(String("Density: " + String(box->GetDensity()) + "\n\n\n\n\n\n\n\n\n"));
        text->SetFont(GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf"));
        text->SetFaceCameraMode(FC_LOOKAT_XYZ);
        text->SetVerticalAlignment(VA_BOTTOM);



        Model* sphereMdl = GetSubsystem<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
        Material* sphereMat = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");



        StaticModel* sphere1StMdl = subNode->CreateComponent<StaticModel>();
        sphere1StMdl->SetCastShadows(true);
        sphere1StMdl->SetModel(sphereMdl);
        sphere1StMdl->SetMaterial(sphereMat);


    }


    FullyFixedConstraint* fixedConstriant = first->CreateComponent<FullyFixedConstraint>();
    fixedConstriant->SetOtherBody(second->GetComponent<RigidBody>());









}



void Physics::SpawnTrialBike(Vector3 worldPosition)
{
    Node* root = scene_->CreateChild("TrialBike");

    //A (Engine Body)
    Node* A = SpawnSamplePhysicsBox(root, worldPosition, Vector3(1, 1, 0.5f));

    Node* B = SpawnSamplePhysicsBox(A, worldPosition + Vector3(-1,0.7,0), Vector3(2, 0.3, 0.5));
    B->RemoveComponent<RigidBody>();
    B->SetWorldRotation(Quaternion(0, 0, -30));

    Node* C = SpawnSamplePhysicsBox(root, worldPosition + Vector3(-1, -0.5, 0), Vector3(2, 0.3, 0.5));
    C->SetWorldRotation(Quaternion(0, 0, 0));

    C->GetComponent<RigidBody>()->SetCollisionOverride(A->GetComponent<RigidBody>(), false);

    //make back spring.
    HingeConstraint* hingeConstraint = A->CreateComponent<HingeConstraint>();
    hingeConstraint->SetOtherBody(C->GetComponent<RigidBody>());
    hingeConstraint->SetNoPowerSpringDamper(true);
    hingeConstraint->SetNoPowerSpringCoefficient(1000.0f);
    hingeConstraint->SetWorldRotation(Quaternion(90, 0, 90));
    hingeConstraint->SetWorldPosition(A->GetWorldPosition() + Vector3(0,-0.5,0));



    Node* D = SpawnSamplePhysicsBox(A, worldPosition + Vector3(0.7,0.5,0), Vector3(1,0.5,0.5));
    D->RemoveComponent<RigidBody>();
    D->SetWorldRotation(Quaternion(0, 0, 45));


    Node* E = SpawnSamplePhysicsBox(root, worldPosition + Vector3(1.5, 0, 0), Vector3(0.2, 2.5, 0.5));
    E->GetComponent<RigidBody>()->SetCollisionOverride(A->GetComponent<RigidBody>(), false);
    E->SetWorldRotation(Quaternion(0, 0, 20));


    HingeConstraint* hinge = E->CreateComponent<HingeConstraint>();
    hinge->SetOtherBody(A->GetComponent<RigidBody>());
    hinge->SetWorldPosition(worldPosition + Vector3(1.2, 0.8, 0));
    hinge->SetWorldRotation(Quaternion(0,0,-90 + 20));


    Node* F = SpawnSamplePhysicsBox(root, worldPosition + Vector3(1.5, 0, 0), Vector3(0.2, 2.5, 0.5));
    F->SetWorldRotation(Quaternion(0, 0, 20));
    F->GetComponent<RigidBody>()->SetCollisionOverride(E->GetComponent<RigidBody>(), false);
    F->GetComponent<RigidBody>()->SetCollisionOverride(A->GetComponent<RigidBody>(), false);

    SliderConstraint* frontSuspension = F->CreateComponent<SliderConstraint>();
    frontSuspension->SetOtherBody(E->GetComponent<RigidBody>());
    frontSuspension->SetWorldRotation(F->GetWorldRotation() * Quaternion(0,0,90));
    frontSuspension->SetEnableSliderSpringDamper(true);
    frontSuspension->SetSliderSpringCoefficient(1000.0f);
    frontSuspension->SetSliderDamperCoefficient(50.0f);
    frontSuspension->SetEnableTwistLimits(true, true);
    frontSuspension->SetTwistLimits(0, 0);
    frontSuspension->SetEnableSliderLimits(true, true);
    frontSuspension->SetSliderLimits(-0.5, 0.5);



    float wheelFriction = 2.0f;

    //backwheel
    Vector3 backWheelOffset = Vector3(-2.0, -0.5, 0);
    Node* backWheel = SpawnSamplePhysicsChamferCylinder(root, worldPosition + backWheelOffset, 0.8f,0.2f);
    backWheel->SetWorldRotation(Quaternion(90,0,0));
    backWheel->GetComponent<RigidBody>()->SetCollisionOverride(C->GetComponent<RigidBody>(), false);
    backWheel->GetDerivedComponent<CollisionShape>()->SetFriction(wheelFriction);


    HingeConstraint* motor = backWheel->CreateComponent<HingeConstraint>();
    motor->SetPowerMode(HingeConstraint::MOTOR);
    motor->SetOtherBody(C->GetComponent<RigidBody>());
    motor->SetWorldPosition(worldPosition + backWheelOffset);
    motor->SetWorldRotation(Quaternion(0, 90, 0));
    motor->SetMotorTargetAngularRate(30);
    motor->SetMaxTorque(motor->GetMaxTorque()*0.00125f);





    Vector3 frontWheelOffset = Vector3(1.8, -1, 0);
    Node* frontWheel = SpawnSamplePhysicsChamferCylinder(root, worldPosition + frontWheelOffset, 0.8f, 0.2f);
    frontWheel->SetWorldRotation(Quaternion(90, 0, 0));
    frontWheel->GetComponent<RigidBody>()->SetCollisionOverride(E->GetComponent<RigidBody>(), false);
    frontWheel->GetComponent<RigidBody>()->SetCollisionOverride(F->GetComponent<RigidBody>(), false);
    frontWheel->GetDerivedComponent<CollisionShape>()->SetFriction(wheelFriction);


    HingeConstraint* frontAxle = frontWheel->CreateComponent<HingeConstraint>();
    //frontAxle->SetPowerMode(HingeConstraint::MOTOR);
    frontAxle->SetOtherBody(F->GetComponent<RigidBody>());
    frontAxle->SetWorldPosition(worldPosition + frontWheelOffset);
    frontAxle->SetWorldRotation(Quaternion(0, 90, 0));
    frontAxle->SetEnableLimits(false);
    //frontAxle->SetMotorTargetAngularRate(10);



}

void Physics::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Move the camera, scale movement with time step
    MoveCamera(timeStep);

    UpdatePickPull();



    //move the scene node as a rebuild scene collision test.
    Node* movingNode = scene_->GetChild("MovingSceneNode");


    if (hingeActuatorTest) {
        float angle = Sin(timeAccum*10.0f)*45.0f;

        //ui::Value("Angle: ", angle);
        URHO3D_LOGINFO(String(angle));
        hingeActuatorTest->SetActuatorTargetAngle(angle);

        timeAccum += timeStep;
    }
}

void Physics::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    // If draw debug mode is enabled, draw physics debug geometry. Use depth test to make the result easier to interpret
    if (drawDebug_) {
        scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(scene_->GetComponent<DebugRenderer>(), false);
        //GetSubsystem<VisualDebugger>()->DrawDebugGeometry(scene_->GetComponent<DebugRenderer>());
    }
}

void Physics::DecomposePhysicsTree()
{
    PODVector<RayQueryResult> res;
    Ray ray(cameraNode_->GetWorldPosition(), cameraNode_->GetWorldDirection());
    RayOctreeQuery querry(res, ray);


    scene_->GetComponent<Octree>()->Raycast(querry);

    if (res.Size() > 1) {

        PODVector<Node*> children;
        res[1].node_->GetChildren(children, true);

        //GSS<VisualDebugger>()->AddOrb(res[1].node_->GetWorldPosition(), 1.0f, Color::RED);

        //for (auto* child : children) {
        //    GSS<VisualDebugger>()->AddOrb(child->GetWorldPosition(), 1.0f, Color(Random(), Random(), Random()));
        //    child->SetParent(scene_);
        //}


        res[1].node_->SetParent(scene_);
    }
}

void Physics::RecomposePhysicsTree()
{

    PODVector<Node*> nodes = scene_->GetChildrenWithTag("scaleTestCube", true);

    for (int i = 1; i < nodes.Size(); i++) {
        nodes[i]->SetParent(nodes[0]);
    }

}


void Physics::TransportNode()
{
    RayQueryResult res = GetCameraPickNode();

    if (res.node_) {
        PODVector<Node*> children;
        if (res.node_->GetName() == "Floor")
            return;

        res.node_->SetWorldPosition(res.node_->GetWorldPosition() + Vector3(Random(), Random()+1.0f, Random())*1.0f);
        //res[1].node_->SetWorldRotation(Quaternion(Random()*360.0f, Random()*360.0f, Random()*360.0f));
    }
}

void Physics::HandleMouseButtonUp(StringHash eventType, VariantMap& eventData)
{
    ReleasePickTargetOnPhysics();
}

void Physics::HandleMouseButtonDown(StringHash eventType, VariantMap& eventData)
{

}


void Physics::HandleCollisionStart(StringHash eventType, VariantMap& eventData)
{
    RigidBody* bodyA = static_cast<RigidBody*>(eventData[PhysicsCollisionStart::P_BODYA].GetPtr());
    RigidBody* bodyB = static_cast<RigidBody*>(eventData[PhysicsCollisionStart::P_BODYB].GetPtr());

    RigidBodyContactEntry* contactData = static_cast<RigidBodyContactEntry*>(eventData[PhysicsCollisionStart::P_CONTACT_DATA].GetPtr());
    for (int i = 0; i < contactData->numContacts; i++) {
        //GetSubsystem<VisualDebugger>()->AddCross(contactData->contactPositions[i], 0.2f, Color::RED, true);

    }


}

RayQueryResult Physics::GetCameraPickNode()
{
    PODVector<RayQueryResult> res;
    Ray ray(cameraNode_->GetWorldPosition(), cameraNode_->GetWorldDirection());
    RayOctreeQuery querry(res, ray);
    scene_->GetComponent<Octree>()->Raycast(querry);

    if (res.Size() > 1) {
        return res[1];
    }
    return RayQueryResult();
}



void Physics::CreateScenery(Vector3 worldPosition)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    if (0) {
        // Create a floor object, 1000 x 1000 world units. Adjust position so that the ground is at zero Y
        Node* floorNode = scene_->CreateChild("Floor");
        floorNode->SetPosition(worldPosition - Vector3(0, 0.5f, 0));
        floorNode->SetScale(Vector3(10000.0f, 1.0f, 10000.0f));
        auto* floorObject = floorNode->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));

        //// Make the floor physical by adding NewtonCollisionShape component. 

        auto* shape = floorNode->CreateComponent<CollisionShape_Box>();

    }

    if (1) {
        //Create heightmap terrain with collision
        Node* terrainNode = scene_->CreateChild("Terrain");
        terrainNode->SetPosition(worldPosition);
        auto* terrain = terrainNode->CreateComponent<Terrain>();
        terrain->SetPatchSize(64);
        terrain->SetSpacing(Vector3(2.0f, 0.2f, 2.0f)); // Spacing between vertices and vertical resolution of the height map
        terrain->SetSmoothing(true);
        terrain->SetHeightMap(cache->GetResource<Image>("Textures/HeightMap.png"));
        terrain->SetMaterial(cache->GetResource<Material>("Materials/Terrain.xml"));
        // The terrain consists of large triangles, which fits well for occlusion rendering, as a hill can occlude all
        // terrain patches and other objects behind it
        terrain->SetOccluder(true);

        terrainNode->CreateComponent<CollisionShape_HeightmapTerrain>();
    }



    //ramps
    if (0) {

        for (int i = 0; i < 10; i++) {
      
        Node* ramp = scene_->CreateChild("ramp");
        ramp->SetPosition(worldPosition + Vector3(300*float(i) + 100, 0, 100*(i%2)));
        ramp->SetScale(Vector3(100.0f, 1.0f, 100.0f));
        auto* floorObject = ramp->CreateComponent<StaticModel>();
        floorObject->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
        floorObject->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
        floorObject->SetCastShadows(true);
        ramp->SetWorldRotation(Quaternion(0, 0, 20));

        //// Make the floor physical by adding NewtonCollisionShape component. 

        auto* shape = ramp->CreateComponent<CollisionShape_Box>();
        }
    }



    float range = 200;
    float objectScale = 100;

    for (int i = 0; i < 0; i++)
    {
        Node* scenePart = scene_->CreateChild("ScenePart" + String(i));
        auto* stMdl = scenePart->CreateComponent<StaticModel>();
        stMdl->SetCastShadows(true);
        scenePart->SetPosition(Vector3(Random(-range, range), 0, Random(-range, range)) + worldPosition);
        scenePart->SetRotation(Quaternion(Random(-360, 0), Random(-360, 0), Random(-360, 0)));
        scenePart->SetScale(Vector3(Random(1.0f, objectScale), Random(1.0f, objectScale), Random(1.0f, objectScale)));

        if (i % 2) {
            stMdl->SetModel(cache->GetResource<Model>("Models/Cylinder.mdl"));
            stMdl->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
            CollisionShape* colShape = scenePart->CreateComponent<CollisionShape_Cylinder>();
            colShape->SetRotationOffset(Quaternion(0, 0, 90));
        }
        else if (i % 3) {
            stMdl->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
            stMdl->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
            CollisionShape* colShape = scenePart->CreateComponent<CollisionShape_Box>();
        }
        else {
            stMdl->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
            stMdl->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
            CollisionShape* colShape = scenePart->CreateComponent<CollisionShape_Sphere>();
        }
    }






    ////finally create a moving node for testing scene collision rebuilding.
    //Node* movingSceneNode = scene_->CreateChild("MovingSceneNode");
    //auto* stmdl = movingSceneNode->CreateComponent<StaticModel>();
    //stmdl->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    //stmdl->SetMaterial(cache->GetResource<Material>("Materials/StoneTiled.xml"));
    //movingSceneNode->CreateComponent<NewtonCollisionShape_Box>();




    //create rotation test.






}

void Physics::RemovePickNode(bool removeRigidBodyOnly /*= false*/)
{
    RayQueryResult res = GetCameraPickNode();
    if (res.node_) {
        if (removeRigidBodyOnly)
        {
            PODVector<RigidBody*> bodies;
            GetRootRigidBodies(bodies, res.node_, false);
            if(bodies.Size())
                bodies.Back()->GetNode()->Remove();


            //RigidBody* rigBody = res.node_->GetComponent<RigidBody>();
            //if (rigBody)
            //    rigBody->Remove();
        }
        else
        {
            res.node_->Remove();
        }
    }
}

void Physics::CreatePickTargetNodeOnPhysics()
{
    RayQueryResult res = GetCameraPickNode();
    if (res.node_) {
        if (res.node_->GetName() == "Floor")
            return;

        //get the most root rigid body
        RigidBody* candidateBody = GetRigidBody(res.node_, false);
        if (!candidateBody)
            return;



        //remember the node
        pickPullNode = candidateBody->GetNode();


        //create "PickTarget" on the hit surface, parented to the camera.
        Node* pickTarget = cameraNode_->CreateChild("CameraPullPoint");
        pickTarget->SetWorldPosition(res.position_);
        
        //create/update node that is on the surface of the node.
        if (pickPullNode->GetChild("PickPullSurfaceNode"))
        {
            pickPullNode->GetChild("PickPullSurfaceNode")->SetWorldPosition(res.position_);
        }
        else
        {
            pickPullNode->CreateChild("PickPullSurfaceNode");
            pickPullNode->GetChild("PickPullSurfaceNode")->SetWorldPosition(res.position_);
        }

        pickPullCameraStartOrientation = cameraNode_->GetWorldRotation();


        //make a kinematics joint
        KinematicsControllerConstraint* constraint = pickPullNode->CreateComponent<KinematicsControllerConstraint>();
        constraint->SetWorldPosition(pickPullNode->GetChild("PickPullSurfaceNode")->GetWorldPosition());
        constraint->SetWorldRotation(cameraNode_->GetWorldRotation());
        constraint->SetConstrainRotation(false);
    }
}




void Physics::ReleasePickTargetOnPhysics()
{
    if (pickPullNode)
    {
        pickPullNode->RemoveChild(pickPullNode->GetChild("PickPullSurfaceNode"));
        RigidBody* rigBody = pickPullNode->GetComponent<RigidBody>();
        if (rigBody)
        {
            rigBody->ResetForces();
        }
        pickPullNode->RemoveComponent<KinematicsControllerConstraint>();
        pickPullNode = nullptr;
    }

    cameraNode_->RemoveChild(cameraNode_->GetChild("CameraPullPoint"));
}
void Physics::UpdatePickPull()
{
    Node* pickTarget = cameraNode_->GetChild("CameraPullPoint");

    if (!pickTarget)
        return;
    if (!pickPullNode)
        return;


    Node* pickSource = pickPullNode->GetChild("PickPullSurfaceNode");

    if (!pickSource)
        return;

    pickPullNode->GetComponent<KinematicsControllerConstraint>()->SetOtherPosition(pickTarget->GetWorldPosition());
    pickPullNode->GetComponent<KinematicsControllerConstraint>()->SetOtherRotation(cameraNode_->GetWorldRotation() );

}
