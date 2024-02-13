//
// Copyright (c) 2017-2021 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//


#if URHO3D_NAVIGATION
#if URHO3D_PHYSICS

#include "../CommonUtils.h"
#include "../SceneUtils.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Navigation/CrowdAgent.h>
#include <Urho3D/Navigation/DynamicNavigationMesh.h>
#include <Urho3D/Navigation/Navigable.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Navigation/NavigationEvents.h>
#include <Urho3D/Math/RandomEngine.h>

namespace
{

struct CrowdAgentTest
{

    SharedPtr<Node> crowdAgentNode;
    Vector3 startPosition;
    bool validAgent;

};

SharedPtr<Scene> CreateTestScene(Context* context, int numObjects)
{
    auto scene = MakeShared<Scene>(context);

    auto cache = context->GetSubsystem<ResourceCache>();

    scene->CreateComponent<Octree>();

    Node* planeNode = scene->CreateChild("Plane");
    planeNode->SetScale(Vector3(100.0f, 0.01f, 100.0f));
    auto* planeBody = planeNode->CreateComponent<RigidBody>();
    auto* planeObject = planeNode->CreateComponent<CollisionShape>();
    planeObject->SetShapeType(SHAPE_BOX);
    planeObject->SetBox(Vector3::ONE);

    Node* boxGroup = scene->CreateChild("Boxes");
    for (unsigned i = 0; i < numObjects; ++i)
    {
        Node* boxNode = boxGroup->CreateChild("Box");
        float size = 1.0f + Random(5.0f);
        boxNode->SetPosition(Vector3(Random(60.0f) - 30.0f, size * 0.5f, Random(60.0f) - 30.0f));
        boxNode->SetScale(size);
        auto* boxBody = boxNode->CreateComponent<RigidBody>();
        auto* boxObject = boxNode->CreateComponent<CollisionShape>();
        boxObject->SetShapeType(SHAPE_BOX);
        boxObject->SetBox(Vector3::ONE);
    }

    return scene;
}


CrowdAgentTest SpawnCrowdAgent(Vector3 pos, Node* agentsSceneNode, bool isValid)
{
    CrowdAgentTest return_agent;

    SharedPtr<Node> agentNode(agentsSceneNode->CreateChild("AgentNode"));
    agentNode->SetWorldPosition(pos);

    // Create a CrowdAgent component and set its height and realistic max speed/acceleration. Use default radius
    CrowdAgent* agent = agentNode->CreateComponent<CrowdAgent>();
    agent->SetHeight(2.0f);
    agent->SetMaxSpeed(3.0f);
    agent->SetMaxAccel(5.0f);

    return_agent.crowdAgentNode = agentNode;
    return_agent.startPosition = pos;
    return_agent.validAgent = isValid;

    return return_agent;
}

}


TEST_CASE("Recast/Detour Crowdmanager test with DynamicNavigationMesh")
{

    SetRandomSeed(Time::GetSystemTime());

    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);

    auto scene = CreateTestScene(context, 20);

    // Create a DynamicNavigationMesh component to the scene root
    auto* navMesh = scene->CreateComponent<DynamicNavigationMesh>();
    // Set small tiles to show navigation mesh streaming
    navMesh->SetTileSize(32);
    // Set the agent height large enough to exclude the layers under boxes
    navMesh->SetAgentHeight(10.0f);
    // Set nav mesh cell height to minimum (allows agents to be grounded)
    navMesh->SetCellHeight(0.05f);
    // Create a Navigable component to the scene root. This tags all of the geometry in the scene as being part of the
    // navigation mesh. By default this is recursive, but the recursion could be turned off from Navigable
    scene->CreateComponent<Navigable>();
    // Add padding to the navigation mesh in Y-direction so that we can add objects on top of the tallest boxes
    // in the scene and still update the mesh correctly
    navMesh->SetPadding(Vector3(0.0f, 10.0f, 0.0f));
    // Now build the navigation geometry. This will take some time. Note that the navigation mesh will prefer to use
    // physics geometry from the scene nodes, as it often is simpler, but if it can not find any (like in this example)
    // it will use renderable geometry instead
    navMesh->Rebuild();

    // Create a CrowdManager component to the scene root
    auto* crowdManager = scene->CreateComponent<CrowdManager>();
    CrowdObstacleAvoidanceParams params = crowdManager->GetObstacleAvoidanceParams(0);
    // Set the params to "High (66)" setting
    params.velBias = 0.5f;
    params.adaptiveDivs = 7;
    params.adaptiveRings = 3;
    params.adaptiveDepth = 3;
    crowdManager->SetObstacleAvoidanceParams(0, params);

    Node* agentsSceneNode = scene->CreateChild("AgentsSceneNode");

    ea::vector<CrowdAgentTest> testAgents;

    //Spawn valid agents outside of boxrange to be sure they are on the navigationmesh and not inside of a boxNode/corner
    for (unsigned i = 0; i < 10; ++i)
    {
        testAgents.push_back(SpawnCrowdAgent(Vector3(-40.0f, 0.0f, Random(40.0f) - 20.0f), agentsSceneNode, true));
    }

    //Spawn random agents
    for (unsigned i = 0; i < 100; ++i)
    {
        testAgents.push_back(SpawnCrowdAgent(Vector3(Random(80.0f) - 40.0f, 0.0f, Random(80.0f) - 40.0f), agentsSceneNode, false));
    }

    //Spawn invalid agents outside of navmesh
    for (unsigned i = 0; i < 10; ++i)
    {
        testAgents.push_back(SpawnCrowdAgent(Vector3(1000.0f, 0.0f, 0.0f), agentsSceneNode, false));
    }

    //Moving agents
    for (unsigned i = 0; i < testAgents.size(); ++i)
    {
        Vector3 randomTargetPos = Vector3(Random(60.0f) - 30.0f, 0.0f, Random(60.0f) - 30.0f);
        //find a new targetposition with range > boxNode size+
        Vector3 pathPos = navMesh->FindNearestPoint(randomTargetPos, Vector3(15.0f, 1.0f, 15.0f));
        crowdManager->SetCrowdTarget(pathPos, testAgents[i].crowdAgentNode);
    }

    Tests::RunFrame(context, 20.0f, 1.0f);

    //Test position of valid agents
    for (unsigned i = 0; i < testAgents.size(); ++i)
    {
        if (testAgents[i].validAgent)
            REQUIRE(testAgents[i].crowdAgentNode->GetWorldPosition() != testAgents[i].startPosition);
    }

}

#endif
#endif
