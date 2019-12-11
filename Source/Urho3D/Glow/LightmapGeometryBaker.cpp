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

#include "../Glow/LightmapGeometryBaker.h"

#include "../Core/Context.h"
#include "../Math/BoundingBox.h"
#include "../Graphics/Camera.h"
#include "../Graphics/Material.h"
#include "../Graphics/Octree.h"
#include "../Graphics/StaticModel.h"
#include "../Scene/Node.h"
#include "../Resource/ResourceCache.h"

namespace Urho3D
{

/// Set camera bounding box.
void SetCameraBoundingBox(Camera* camera, const BoundingBox& boundingBox)
{
    Node* node = camera->GetNode();

    const float zNear = 1.0f;
    const float zFar = boundingBox.Size().z_ + zNear;
    Vector3 position = boundingBox.Center();
    position.z_ = boundingBox.min_.z_ - zNear;

    node->SetPosition(position);
    node->SetDirection(Vector3::FORWARD);

    camera->SetOrthographic(true);
    camera->SetOrthoSize(Vector2(boundingBox.Size().x_, boundingBox.Size().y_));
    camera->SetNearClip(zNear);
    camera->SetFarClip(zFar);
}

URHO3D_API LightmapGeometryBakingScene GenerateLightmapGeometryBakingScene(
    Context* context, const LightmapChart& chart, const LightmapGeometryBakingSettings& settings)
{
    Material* bakingMaterial = context->GetCache()->GetResource<Material>(settings.material_);

    // Calculate bounding box
    BoundingBox boundingBox;
    for (const LightmapChartElement& element : chart.elements_)
    {
        if (element.staticModel_)
            boundingBox.Merge(element.staticModel_->GetWorldBoundingBox());
    }

    // Create scene and camera
    auto scene = MakeShared<Scene>(context);
    scene->CreateComponent<Octree>();

    auto camera = scene->CreateComponent<Camera>();
    SetCameraBoundingBox(camera, boundingBox);

    // Replicate all elements in the scene
    for (const LightmapChartElement& element : chart.elements_)
    {
        if (element.staticModel_)
        {
            auto material = bakingMaterial->Clone();
            material->SetShaderParameter("LMOffset", element.region_.GetScaleOffset());

            Node* node = scene->CreateChild();
            node->SetPosition(element.node_->GetWorldPosition());
            node->SetRotation(element.node_->GetWorldRotation());
            node->SetScale(element.node_->GetWorldScale());

            StaticModel* staticModel = node->CreateComponent<StaticModel>();
            staticModel->SetModel(element.staticModel_->GetModel());
            staticModel->SetMaterial(material);
        }
    }

    return { scene, camera };
}

}
