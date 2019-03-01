//
// Copyright (c) 2008-2019 the Urho3D project.
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

namespace Urho3D
{

class Node;
class Scene;
class HingeConstraint;

}

/// PhysicsTests example.
/// This sample demonstrates different types of physics configurations and provides a testing ground for physics functionality.
class PhysicsTests : public Sample
{
    URHO3D_OBJECT(PhysicsTests, Sample);

public:
    /// Construct.
    explicit PhysicsTests(Context* context);

    /// Setup after engine initialization and before running the main loop.
    void Start() override;

protected:
    /// Return XML patch instructions for screen joystick layout for a specific sample app, if any.
    String GetScreenJoystickPatchString() const override { return
        "<patch>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Spawn</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button0']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"MouseButtonBinding\" />"
        "            <attribute name=\"Text\" value=\"LEFT\" />"
        "        </element>"
        "    </add>"
        "    <remove sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/attribute[@name='Is Visible']\" />"
        "    <replace sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]/element[./attribute[@name='Name' and @value='Label']]/attribute[@name='Text']/@value\">Debug</replace>"
        "    <add sel=\"/element/element[./attribute[@name='Name' and @value='Button1']]\">"
        "        <element type=\"Text\">"
        "            <attribute name=\"Name\" value=\"KeyBinding\" />"
        "            <attribute name=\"Text\" value=\"SPACE\" />"
        "        </element>"
        "    </add>"
        "</patch>";
    }

private:
    /// Construct the scene content.
    void CreateScene();

    void SpawnSceneCompoundTest(const Vector3& worldPos, bool oneBody);
    void CreatePyramids(Vector3 position);


    void CreateTowerOfLiar(Vector3 position);
    /// Construct an instruction text to the UI.
    void CreateInstructions();
    /// Set up a viewport for displaying the scene.
    void SetupViewport();
    /// Subscribe to application-wide logic update and post-render update events.
    void SubscribeToEvents();





    /// Read input and moves the camera.
    void MoveCamera(float timeStep);





    /// Spawn a physics object from the camera position.
    void SpawnObject();
    void SpawnConvexHull(const Vector3& worldPos);
    void SpawnCompound(const Vector3& worldPos);
    void SpawnDecompCompound(const Vector3& worldPos);
    void SpawnNSquaredJointedObject(Vector3 worldPosition);
    void SpawnGlueJointedObject(Vector3 worldPosition);
    void SpawnLinearJointedObject(float size, Vector3 worldPosition);
    void SpawnMaterialsTest(Vector3 worldPosition);
    void SpawnBallSocketTest(Vector3 worldPosition);
    void SpawnHingeActuatorTest(Vector3 worldPosition);
    void SpawnCollisionExceptionsTest(Vector3 worldPosition);
    void SpawnSliderTest(Vector3 worldPosition);
    void SpawnRandomObjects();
    void SpawnCompoundedRectTest(Vector3 worldPosition);
    void SpawnCompoundedRectTest2(Vector3 worldPosition);
    void SpawnTrialBike(Vector3 worldPosition, Quaternion orientation, bool enableGyroOnWheels);
    void SpawnHingeSpringTest(const Vector3 ZERO, const Quaternion IDENTITY);
    void SpawnKinematicBodyTest(Vector3 worldPosition, Quaternion worldRotation);

    /// Handle the logic update event.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle the post-render update event.
    void HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle physics pre-step
    void HandlePhysicsPreStep(StringHash eventType, VariantMap& eventData);

    /// Handle physics post-step
    void HandlePhysicsPostStep(StringHash eventType, VariantMap& eventData);

    /// Flag for drawing debug geometry.
    bool drawDebug_ = false;
    void DecomposePhysicsTree();
    void RecomposePhysicsTree();
    void  TransportNode();



    void HandleMouseButtonUp(StringHash eventType, VariantMap& eventData);
    void HandleMouseButtonDown(StringHash eventType, VariantMap& eventData);


    void HandleCollisionStart(StringHash eventType, VariantMap& eventData);
    void HandleCollisionEnd(StringHash eventType, VariantMap& eventData);


    Node* pickPullNode = nullptr;
    Quaternion pickPullCameraStartOrientation;
    Vector3 pickPullStartPositionWorld;
    Vector3 pickPullEndPositionWorld;
    HingeConstraint* hingeActuatorTest = nullptr;
    float timeAccum = 0.0f;




    void CreatePickTargetNodeOnPhysics();
    void ReleasePickTargetOnPhysics();
    void UpdatePickPull();
    RayQueryResult GetCameraPickNode();

    //temp variable to tracking world position across updates.
    PODVector<float> worldPosHistory_;
    PODVector<float> worldPosHistory2_;


    PODVector<Constraint*> forceCalculationConstraints_;

    WeakPtr<Node> kinematicNode_;

    void CreateScenery(Vector3 worldPosition);
    void RemovePickNode(bool removeRigidBodyOnly = false);
   
   
};
