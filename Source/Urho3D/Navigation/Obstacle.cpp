// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/DebugRenderer.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Navigation/DynamicNavigationMesh.h"
#include "Urho3D/Navigation/Obstacle.h"
#include "Urho3D/Navigation/NavigationEvents.h"
#include "Urho3D/Scene/Scene.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

Obstacle::Obstacle(Context* context) :
    Component(context),
    height_(5.0f),
    radius_(5.0f),
    obstacleId_(0)
{
}

Obstacle::~Obstacle()
{
    if (obstacleId_ > 0 && ownerMesh_)
        ownerMesh_->RemoveObstacle(this);
}

void Obstacle::RegisterObject(Context* context)
{
    context->AddFactoryReflection<Obstacle>(Category_Navigation);

    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 5.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Height", GetHeight, SetHeight, float, 5.0f, AM_DEFAULT);
}

void Obstacle::OnSetEnabled()
{
    if (ownerMesh_)
    {
        if (IsEnabledEffective())
            ownerMesh_->AddObstacle(this);
        else
            ownerMesh_->RemoveObstacle(this);
    }
}

void Obstacle::SetHeight(float newHeight)
{
    height_ = newHeight;
    if (IsEnabledEffective() && ownerMesh_)
        ownerMesh_->ObstacleChanged(this);
}

void Obstacle::SetRadius(float newRadius)
{
    radius_ = newRadius;
    if (IsEnabledEffective() && ownerMesh_)
        ownerMesh_->ObstacleChanged(this);
}

void Obstacle::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
        node_->AddListener(this);
}

void Obstacle::OnSceneSet(Scene* previousScene, Scene* scene)
{
    if (scene)
    {
        if (scene == node_)
        {
            URHO3D_LOGWARNING(GetTypeName() + " should not be created to the root scene node");
            return;
        }
        if (!ownerMesh_)
            ownerMesh_ = node_->FindComponent<DynamicNavigationMesh>(ComponentSearchFlag::ParentRecursive);
        if (ownerMesh_)
        {
            if (IsEnabledEffective())
                ownerMesh_->AddObstacle(this);
            SubscribeToEvent(ownerMesh_, E_NAVIGATION_TILE_ADDED, URHO3D_HANDLER(Obstacle, HandleNavigationTileAdded));
        }
    }
    else
    {
        if (obstacleId_ > 0 && ownerMesh_)
            ownerMesh_->RemoveObstacle(this);

        UnsubscribeFromEvent(E_NAVIGATION_TILE_ADDED);
        ownerMesh_.Reset();
    }
}

void Obstacle::OnMarkedDirty(Node* node)
{
    if (IsEnabledEffective() && ownerMesh_)
    {
        Scene* scene = GetScene();
        /// \hack If scene already unassigned, or if it's being destroyed, do nothing
        if (!scene || scene->Refs() == 0)
            return;

        // If within threaded update, update later
        if (scene->IsThreadedUpdate())
        {
            scene->DelayedMarkedDirty(this);
            return;
        }

        ownerMesh_->ObstacleChanged(this);
    }
}

void Obstacle::HandleNavigationTileAdded(StringHash eventType, VariantMap& eventData)
{
    // Re-add obstacle if it is intersected with newly added tile
    const IntVector2 tile = eventData[NavigationTileAdded::P_TILE].GetIntVector2();
    if (IsEnabledEffective() && ownerMesh_ && ownerMesh_->IsObstacleInTile(this, tile))
        ownerMesh_->ObstacleChanged(this);
}

void Obstacle::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
        debug->AddCylinder(node_->GetWorldPosition(), radius_, height_, Color(0.0f, 1.0f, 1.0f), depthTest);
}

void Obstacle::DrawDebugGeometry(bool depthTest)
{
    Scene* scene = GetScene();
    if (scene)
        DrawDebugGeometry(scene->GetComponent<DebugRenderer>(), depthTest);
}

}
