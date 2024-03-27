// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/Graphics/CameraAssistant.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/Scene/SceneEvents.h"

namespace Urho3D
{

namespace
{
static const StringVector boundaryNodesStructureElementNames = {"Boundary Count", "   NodeID"};
}

CameraAssistant::CameraAssistant(Context* context)
    : Component(context)
{
}

CameraAssistant::~CameraAssistant() = default;

void CameraAssistant::RegisterObject(Context* context)
{
    context->AddFactoryReflection<CameraAssistant>(Category_Scene);

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Boundary Nodes", GetNodeIDsAttr, SetNodeIDsAttr, VariantVector,
        Variant::emptyVariantVector, AM_DEFAULT | AM_NODEIDVECTOR)
        .SetMetadata(AttributeMetadata::VectorStructElements, boundaryNodesStructureElementNames);
    URHO3D_ACCESSOR_ATTRIBUTE("Easing factor", GetEasingFactor, SetEasingFactor, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Min FOV", GetMinFov, SetMinFov, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Min Orthographic Size", GetMinOrthoSize, SetMinOrthoSize, float, 0.0f, AM_DEFAULT);
}

void CameraAssistant::OnSceneSet(Scene* scene)
{
    BaseClassName::OnSceneSet(scene);

    UpdateSubscriptions();
}

    void CameraAssistant::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (previousNode)
        previousNode->RemoveListener(this);
    if (currentNode)
        currentNode->AddListener(this);

    UpdateSubscriptions();
}

void CameraAssistant::ApplyAttributes()
{
    if (!nodesDirty_)
        return;

    // Remove all old boundary nodes before searching for new
    boundaryNodes_.clear();

    Scene* scene = GetScene();
    if (scene)
    {
        // The first index stores the number of IDs redundantly. This is for editing
        for (unsigned i = 1; i < nodeIDsAttr_.size(); ++i)
        {
            Node* node = scene->GetNode(nodeIDsAttr_[i].GetUInt());
            if (node)
            {
                WeakPtr<Node> boundaryWeak(node);
                boundaryNodes_.push_back(boundaryWeak);
            }
        }
    }

    nodesDirty_ = false;
}

void CameraAssistant::AddBoundaryNode(Node* node)
{
    if (!node)
        return;

    WeakPtr<Node> boundaryWeak(node);
    if (boundaryNodes_.contains(boundaryWeak))
        return;

    // Add as a listener for the boundary node, so that we know to dirty the transforms when the node moves or is
    // enabled/disabled
    node->AddListener(this);
    boundaryNodes_.push_back(boundaryWeak);
    UpdateNumTransforms();
}

void CameraAssistant::RemoveBoundaryNode(Node* node)
{
    if (!node)
        return;

    WeakPtr<Node> boundaryWeak(node);
    auto i = boundaryNodes_.find(boundaryWeak);
    if (i == boundaryNodes_.end())
        return;

    boundaryNodes_.erase(i);
    UpdateNumTransforms();
}

void CameraAssistant::RemoveAllBoundaryNodes()
{
    for (unsigned i = 0; i < boundaryNodes_.size(); ++i)
    {
        Node* node = boundaryNodes_[i];
        if (node)
            node->RemoveListener(this);
    }

    boundaryNodes_.clear();
    UpdateNumTransforms();
}

Node* CameraAssistant::GetBoundaryNode(unsigned index) const
{
    return index < boundaryNodes_.size() ? boundaryNodes_[index].Get() : nullptr;
}

void CameraAssistant::SetNodeIDsAttr(const VariantVector& value)
{
    // Just remember the node IDs. They need to go through the SceneResolver, and we actually find the nodes during
    // ApplyAttributes()
    if (value.size())
    {
        nodeIDsAttr_.clear();

        unsigned index = 0;
        unsigned numBoundaries = value[index++].GetUInt();
        // Prevent crash on entering negative value in the editor
        if (numBoundaries > M_MAX_INT)
            numBoundaries = 0;

        nodeIDsAttr_.push_back(numBoundaries);
        while (numBoundaries--)
        {
            // If vector contains less IDs than should, fill the rest with zeroes
            if (index < value.size())
                nodeIDsAttr_.push_back(value[index++].GetUInt());
            else
                nodeIDsAttr_.push_back(0);
        }
    }
    else
    {
        nodeIDsAttr_.clear();
        nodeIDsAttr_.push_back(0);
    }

    nodesDirty_ = true;
    nodeIDsDirty_ = false;
}

const VariantVector& CameraAssistant::GetNodeIDsAttr() const
{
    if (nodeIDsDirty_)
        UpdateNodeIDs();

    return nodeIDsAttr_;
}

void CameraAssistant::SetEasingFactor(float factor)
{
    easingFactor_ = Urho3D::Min(Urho3D::Max(0.0f, factor), 1.0f);
}

void CameraAssistant::SetMinFov(float fov)
{
    minFov_ = Urho3D::Min(0.0f, fov);
}

void CameraAssistant::SetMinOrthoSize(float orthoSize)
{
    minOrthoSize_ = Urho3D::Min(0.0f, orthoSize);
}

void CameraAssistant::SetWorldSpacePadding(float padding)
{
    padding_ = padding;
}

void CameraAssistant::UpdateSubscriptions()
{
    const auto subscribe = GetScene() && IsEnabledEffective();

    if (subscribe == subscribed_)
    {
        return;
    }

    subscribed_ = subscribe;
    if (subscribe)
    {
        SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, &CameraAssistant::UpdateCameraParameters);
    }
    else
    {
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
    }
}

void CameraAssistant::OnSetEnabled()
{
    UpdateSubscriptions();
}

void CameraAssistant::UpdateCameraParameters(VariantMap& args)
{
    using namespace  ScenePostUpdate;
    const float timestep = args[P_TIMESTEP].GetFloat();
    const float timescale = timestep * 60.0f;
    const float factor = (timestep == 0.0f) ? easingFactor_ : 1.0f - Urho3D::Pow(1.0f - easingFactor_, timescale);

    auto* camera = GetNode()->GetComponent<Camera>();
    if (!camera)
    {
        // We don't need to update world transforms if nothings changed.
        return;
    }
    const float aspectRatio = camera->GetAspectRatio();
    const Matrix3x4 viewMatrix = camera->GetView();

    // Update transforms and update camera.
    unsigned index = 0;

    if (camera->IsOrthographic())
    {
        float orthoSize = 0.0f;
        for (unsigned i = 0; i < boundaryNodes_.size(); ++i)
        {
            const Node* node = boundaryNodes_[i];
            if (!node || !node->IsEnabled())
                continue;

            const Vector3 cameraSpaceTransform = viewMatrix * node->GetWorldPosition();

            if (cameraSpaceTransform.z_ > camera->GetNearClip())
            {
                const auto size = 2.0f
                    * Urho3D::Max((Urho3D::Abs(cameraSpaceTransform.x_) + padding_) / aspectRatio,
                        Urho3D::Abs(cameraSpaceTransform.y_) + padding_);
                orthoSize = Urho3D::Max(orthoSize, size);
            }
        }
        if (orthoSize > 0.0f)
        {
            orthoSize = Urho3D::Max(minOrthoSize_, orthoSize);
            const float currentSize = camera->GetOrthoSize();
            if (!Urho3D::Equals(currentSize, orthoSize) && factor < 1.0f)
                orthoSize = Urho3D::Lerp(currentSize, orthoSize, factor);
            camera->SetOrthoSize(orthoSize);
        }
    }
    else
    {
        float fov = 0.0f;
        for (unsigned i = 0; i < boundaryNodes_.size(); ++i)
        {
            Node* node = boundaryNodes_[i];
            if (!node || !node->IsEnabled())
                continue;

            const Vector3 cameraSpaceTransform = viewMatrix * node->GetWorldPosition();

            if (cameraSpaceTransform.z_ > camera->GetNearClip())
            {
                const auto yFov =
                    2.0f * Urho3D::Atan2(Urho3D::Abs(cameraSpaceTransform.y_) + padding_, cameraSpaceTransform.z_);
                const auto xFov = 2.0f
                    * Urho3D::Atan2(
                        (Urho3D::Abs(cameraSpaceTransform.x_) + padding_) / aspectRatio, cameraSpaceTransform.z_);
                fov = Urho3D::Max(fov, Urho3D::Max(yFov, xFov));
            }
        }
        if (fov > 0.0f)
        {
            fov = Urho3D::Min(Urho3D::Max(minFov_, fov), M_MAX_FOV);
            const float currentFov = camera->GetFov();
            if (!Urho3D::Equals(currentFov, fov) && factor < 1.0f)
                fov = Urho3D::Lerp(currentFov, fov, factor);
            camera->SetFov(fov);
        }
    }
}


void CameraAssistant::UpdateNumTransforms()
{
    nodeIDsDirty_ = true;
}

void CameraAssistant::UpdateNodeIDs() const
{
    unsigned numBoundaries = boundaryNodes_.size();

    nodeIDsAttr_.clear();
    nodeIDsAttr_.push_back(numBoundaries);

    for (unsigned i = 0; i < numBoundaries; ++i)
    {
        Node* node = boundaryNodes_[i];
        nodeIDsAttr_.push_back(node ? node->GetID() : 0);
    }

    nodeIDsDirty_ = false;
}

} // namespace Urho3D
