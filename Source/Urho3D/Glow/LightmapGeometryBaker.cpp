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
#include "../Graphics/Graphics.h"
#include "../Graphics/Material.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/StaticModel.h"
#include "../Graphics/Texture2D.h"
#include "../Graphics/View.h"
#include "../Resource/ResourceCache.h"
#include "../Resource/XMLFile.h"
#include "../Scene/Node.h"

namespace Urho3D
{

namespace
{

/// Number of multi-tap samples.
const unsigned numMultiTapSamples = 25;

/// Multi-tap offsets.
const Vector2 multiTapOffsets[numMultiTapSamples] =
{
    {  1.0f,  1.0f },
    {  1.0f, -1.0f },
    { -1.0f,  1.0f },
    { -1.0f, -1.0f },

    {  1.0f,  0.5f },
    {  1.0f, -0.5f },
    { -1.0f,  0.5f },
    { -1.0f, -0.5f },
    {  0.5f,  1.0f },
    {  0.5f, -1.0f },
    { -0.5f,  1.0f },
    { -0.5f, -1.0f },

    {  1.0f,  0.0f },
    { -1.0f,  0.0f },
    {  0.0f,  1.0f },
    {  0.0f, -1.0f },

    {  0.5f,  0.5f },
    {  0.5f, -0.5f },
    { -0.5f,  0.5f },
    { -0.5f, -0.5f },

    {  0.5f,  0.0f },
    { -0.5f,  0.0f },
    {  0.0f,  0.5f },
    {  0.0f, -0.5f },

    {  0.0f,  0.0f },
};

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

/// Load render path.
SharedPtr<RenderPath> LoadRenderPath(Context* context, const ea::string& renderPathName)
{
    auto renderPath = MakeShared<RenderPath>();
    auto renderPathXml = context->GetCache()->GetResource<XMLFile>(renderPathName);
    if (!renderPath->Load(renderPathXml))
        return nullptr;
    return renderPath;
}

/// Read RGBA32 float texture to vector.
void ReadTextureRGBA32Float(Texture* texture, ea::vector<Vector4>& dest)
{
    auto texture2D = dynamic_cast<Texture2D*>(texture);
    const unsigned numElements = texture->GetDataSize(texture->GetWidth(), texture->GetHeight()) / sizeof(Vector4);
    dest.resize(numElements);
    texture2D->GetData(0, dest.data());
}

/// Extract Vector3 from Vector4.
Vector3 ExtractVector3FromVector4(const Vector4& data) { return { data.x_, data.y_, data.z_ }; }

/// Extract w-component as unsigned integer from Vector4.
unsigned ExtractUintFromVector4(const Vector4& data) { return static_cast<unsigned>(data.w_); }

}

LightmapGeometryBakingScene GenerateLightmapGeometryBakingScene(Context* context,
    const LightmapChart& chart, const LightmapGeometryBakingSettings& settings, SharedPtr<RenderPath> renderPath)
{
    Material* bakingMaterial = context->GetCache()->GetResource<Material>(settings.materialName_);

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
    unsigned geometryId = 1;
    for (const LightmapChartElement& element : chart.elements_)
    {
        if (element.staticModel_)
        {
            for (unsigned tap = 0; tap < numMultiTapSamples; ++tap)
            {
                const Vector2 tapOffset = multiTapOffsets[tap] * chart.GetTexelSize();
                const Vector4 tapOffset4{ 0.0f, 0.0f, tapOffset.x_, tapOffset.y_ };
                const float tapDepth = 1.0f - static_cast<float>(tap) / (numMultiTapSamples - 1);

                auto material = bakingMaterial->Clone();
                material->SetShaderParameter("LMOffset", element.region_.GetScaleOffset() + tapOffset4);
                material->SetShaderParameter("LightmapLayer", tapDepth);
                material->SetShaderParameter("LightmapGeometry", static_cast<float>(geometryId));

                Node* node = scene->CreateChild();
                node->SetPosition(element.node_->GetWorldPosition());
                node->SetRotation(element.node_->GetWorldRotation());
                node->SetScale(element.node_->GetWorldScale());

                StaticModel* staticModel = node->CreateComponent<StaticModel>();
                staticModel->SetModel(element.staticModel_->GetModel());
                staticModel->SetMaterial(material);
            }

            ++geometryId;
        }
    }

    return { context, chart.width_, chart.height_, chart.size_, scene, camera, renderPath };
}

ea::vector<LightmapGeometryBakingScene> GenerateLightmapGeometryBakingScenes(
    Context* context, const ea::vector<LightmapChart>& charts, const LightmapGeometryBakingSettings& settings)
{
    SharedPtr<RenderPath> renderPath = LoadRenderPath(context, settings.renderPathName_);

    ea::vector<LightmapGeometryBakingScene> result;
    for (const LightmapChart& chart : charts)
        result.push_back(GenerateLightmapGeometryBakingScene(context, chart, settings, renderPath));

    return result;
}

LightmapChartGeometryBuffer BakeLightmapGeometryBuffer(const LightmapGeometryBakingScene& bakingScene)
{
    Context* context = bakingScene.context_;
    Graphics* graphics = context->GetGraphics();
    Renderer* renderer = context->GetRenderer();

    static thread_local ea::vector<Vector4> buffer;

    if (!graphics->BeginFrame())
        return {};

    LightmapChartGeometryBuffer geometryBuffer{ bakingScene.width_, bakingScene.height_ };

    // Get render surface
    Texture* renderTexture = renderer->GetScreenBuffer(
        bakingScene.size_.x_, bakingScene.size_.y_,Graphics::GetRGBAFormat(), 1, true, false, false, false);
    RenderSurface* renderSurface = static_cast<Texture2D*>(renderTexture)->GetRenderSurface();

    // Setup viewport
    Viewport viewport(context);
    viewport.SetCamera(bakingScene.camera_);
    viewport.SetRect(IntRect::ZERO);
    viewport.SetRenderPath(bakingScene.renderPath_);
    viewport.SetScene(bakingScene.scene_);

    // Render scene
    View view(context);
    view.Define(renderSurface, &viewport);
    view.Update(FrameInfo());
    view.Render();

    // Store results
    ReadTextureRGBA32Float(view.GetExtraRenderTarget("position"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.geometryPositions_.begin(), ExtractVector3FromVector4);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.geometryIds_.begin(), ExtractUintFromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothposition"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.smoothPositions_.begin(), ExtractVector3FromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("facenormal"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.faceNormals_.begin(), ExtractVector3FromVector4);

    ReadTextureRGBA32Float(view.GetExtraRenderTarget("smoothnormal"), buffer);
    ea::transform(buffer.begin(), buffer.end(), geometryBuffer.smoothNormals_.begin(), ExtractVector3FromVector4);

    graphics->EndFrame();
    return geometryBuffer;
}

LightmapChartGeometryBufferVector BakeLightmapGeometryBuffers(const ea::vector<LightmapGeometryBakingScene>& bakingScenes)
{
    LightmapChartGeometryBufferVector geometryBuffers;
    for (const LightmapGeometryBakingScene& bakingScene : bakingScenes)
        geometryBuffers.push_back(BakeLightmapGeometryBuffer(bakingScene));
    return geometryBuffers;
}

}
