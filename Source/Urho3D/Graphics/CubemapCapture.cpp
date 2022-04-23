//
// Copyright (c) 2017-2020 the rbfx project.
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

#include "../Graphics/Camera.h"
#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Graphics/CubemapCapture.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/GraphicsEvents.h"
#include "../IO/Log.h"
#include "../Scene/Node.h"
#include "../Graphics/RenderPath.h"
#include "../Graphics/RenderSurface.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Octree.h"
#include "../RenderPipeline/RenderPipeline.h"
#include "../Resource/ResourceCache.h"
#include "../Scene/Scene.h"
#include "../Graphics/TextureCube.h"
#include "../Graphics/View.h"
#include "../Graphics/Zone.h"

#ifdef URHO3D_COMPUTE
    #include "../Graphics/ComputeDevice.h"
#endif

#include "../DebugNew.h"

namespace Urho3D
{

extern const char* SCENE_CATEGORY;

CubemapCapture::CubemapCapture(Context* context) :
    Component(context)
{
    SubscribeToEvent(E_RENDERSURFACEUPDATE, [&](StringHash eventType, VariantMap& eventData)
    {
        CheckAndQueueUpdate();
    });

    // TODO(compute): Revisit me
    SubscribeToEvent(E_BEGINRENDERING, [&](StringHash h, VariantMap& eventData) { Render(); });
}

CubemapCapture::~CubemapCapture()
{

}

void CubemapCapture::RegisterObject(Context* context)
{
    context->RegisterFactory<CubemapCapture>(SCENE_CATEGORY);

    URHO3D_ACCESSOR_ATTRIBUTE("Face Size", GetFaceSize, SetFaceSize, unsigned, 64, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Far Distance", GetFarDist, SetFarDist, float, 10000.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Link To Zone", IsZoneLinked, SetZoneLinked, bool, false, AM_DEFAULT);
}

void CubemapCapture::SetFaceSize(unsigned dim)
{
    faceSize_ = Max(1, dim);

    if (target_.NotNull())
    {
        SetupTextures();
        SetupZone();
    }
}

void CubemapCapture::SetTarget(SharedPtr<TextureCube> texTarget)
{
    target_ = texTarget;
    SetupZone();
    MarkDirty();
}

void CubemapCapture::SetRenderPath(SharedPtr<RenderPath> rp)
{
    renderPath_ = rp;
    MarkDirty();
}

SharedPtr<TextureCube> CubemapCapture::GetTarget() const
{
    return target_;
}

SharedPtr<TextureCube> CubemapCapture::GetFiltered() const
{
    return filtered_;
}

SharedPtr<RenderPath> CubemapCapture::GetRenderPath() const
{
    return renderPath_;
}

void CubemapCapture::Render()
{
    if (target_.Null())
        SetupTextures();
    if (dirty_)
    {
        if (renderPath_.Null())
            renderPath_ = GetSubsystem<Renderer>()->GetDefaultRenderPath();

        CubemapCapture::Render(SharedPtr<Scene>(GetScene()), renderPath_, target_, GetNode(), farDist_);
        Filter();

        auto& dataMap = GetEventDataMap();
        dataMap[CubemapCaptureUpdate::P_NODE] = GetNode();
        dataMap[CubemapCaptureUpdate::P_CAPTURE] = this;
        dataMap[CubemapCaptureUpdate::P_TEXTURE] = target_;
        SendEvent(E_CUBEMAPCAPTUREUPDATE, dataMap);
    }

    SetupZone();
    dirty_ = false;
}

void CubemapCapture::Filter()
{
    if (faceSize_ == 128)
        FilterCubemaps_128({ target_ }, { filtered_ });
    else
        FilterCubemaps({ target_ }, { filtered_ }, { 1, 8, 16 });

    SetupZone();
    dirty_ = false;
}

void CubemapCapture::RenderAll(SharedPtr<Scene> scene, unsigned maxCt)
{
    // If a caller's calculations gave a zero we'd make a dead BeginFrame/EndFrame scope.
    if (maxCt == 0)
        return;

    ea::vector<CubemapCapture*> captures;
    scene->GetComponents<CubemapCapture>(captures, true);

    if (captures.empty())
        return;

    // identify who needs to be renderered
    bool anyDirty = false;
    for (auto& c : captures)
    {
        if (c->IsDirty())
        {
            anyDirty = true;
            break;
        }
    }

    if (!anyDirty)
        return;

    auto graphics = scene->GetSubsystem<Graphics>();
    if (!graphics->BeginFrame())
    {
        URHO3D_LOGERROR("CubemapCapture::RenderAll, failed to BeginFrame");
        return;
    }

    for (auto& cap : captures)
    {
        if (maxCt <= 0)
            break;

        if (cap->IsDirty())
        {
            CubemapCapture::Render(SharedPtr<Scene>(cap->GetScene()), cap->GetRenderPath(), cap->GetTarget(),
                cap->GetNode(), cap->GetFarDist(), false);

            // Send the event signaling this as having been updated, ie. so it can be queued for filtering.
            auto& dataMap = cap->GetEventDataMap();
            dataMap[CubemapCaptureUpdate::P_NODE] = cap->GetNode();
            dataMap[CubemapCaptureUpdate::P_CAPTURE] = cap;
            dataMap[CubemapCaptureUpdate::P_TEXTURE] = cap->GetTarget();
            cap->SendEvent(E_CUBEMAPCAPTUREUPDATE, dataMap);

            --maxCt;
        }
    }

    graphics->ResetRenderTargets();
    graphics->EndFrame();
}

void CubemapCapture::Render(SharedPtr<Scene> scene, SharedPtr<RenderPath> renderPath, SharedPtr<TextureCube> cubeTarget,
    Node* anchorNode, float farDist, bool needBeginEnd)
{
    needBeginEnd = false;

    auto context = scene->GetContext();
    auto renderer = scene->GetSubsystem<Renderer>();
    auto graphics = scene->GetSubsystem<Graphics>();

    auto renderPipeline = anchorNode->GetDerivedComponent<RenderPipeline>();
    if (!renderPipeline)
    {
        URHO3D_LOGERROR("CubemapCapture::Render, cannot find RenderPipeline");
        return;
    }

    if (needBeginEnd)
    {
        if (!graphics->BeginFrame())
        {
            URHO3D_LOGERROR("CubemapCapture::Render, failed to BeginFrame");
            return;
        }
    }

    Node cameraNode(context);
    cameraNode.SetWorldPosition(anchorNode->GetWorldPosition());

    Camera* camera = cameraNode.CreateComponent<Camera>(LOCAL, 1);
    camera->SetFov(90);
    camera->SetNearClip(0.0001f);
    camera->SetFarClip(farDist);
    camera->SetAspectRatio(1.0f);

    IntRect rect{0, 0, cubeTarget->GetWidth(), cubeTarget->GetHeight()};
    Viewport vpt(context, scene, camera, rect, renderPipeline);
    vpt.AllocateView();
    auto view = vpt.GetRenderPipelineView();

    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        auto renderTarget = cubeTarget->GetRenderSurface((CubeMapFace)i);

        auto octree = scene->GetComponent<Octree>();
        const FrameInfo frameInfo{1, 0.0f, rect.Size(), rect, &vpt, renderTarget, scene, camera, octree};
        octree->Update(frameInfo);

        view->Define(renderTarget, &vpt);
        cameraNode.SetWorldRotation(CubeFaceRotation((CubeMapFace)i));
        view->Update(frameInfo);
        view->Render();
    }

    if (needBeginEnd)
        graphics->EndFrame();
}

Quaternion CubemapCapture::CubeFaceRotation(CubeMapFace face)
{
    Quaternion result;
    switch (face)
    {
    case FACE_POSITIVE_X:
        result = Quaternion(0, 90, 0);
        break;
    case FACE_NEGATIVE_X:
        result = Quaternion(0, -90, 0);
        break;
    case FACE_POSITIVE_Y:
        result = Quaternion(-90, 0, 0);
        break;
    case FACE_NEGATIVE_Y:
        result = Quaternion(90, 0, 0);
        break;
    case FACE_POSITIVE_Z:
        result = Quaternion(0, 0, 0);
        break;
    case FACE_NEGATIVE_Z:
        result = Quaternion(0, 180, 0);
        break;
    }
    return result;
}

void CubemapCapture::CheckAndQueueUpdate()
{
    // TODO(compute): Revisit
    /*if (!target_)
        SetupTextures();

    auto renderer = GetSubsystem<Renderer>();
    for (unsigned i = 0; i < MAX_CUBEMAP_FACES; ++i)
    {
        auto renderTarget = target_->GetRenderSurface((CubeMapFace)i);
        if (!renderTarget->GetViewport(0))
        {
            auto viewport
            renderTarget->SetViewport(0, );
        }
    }*/
    //renderer->QueueRenderSurface();
}

void CubemapCapture::SetupZone()
{
    if (GetNode() == nullptr)
        return;

    Zone* zone = GetNode()->GetComponent<Zone>();

    // check the parent too so we can have a capture position independent of the Zone centroid.
    if (zone == nullptr && GetNode()->GetParent())
        zone = GetNode()->GetParent()->GetComponent<Zone>();

    if (zone)
    {
        auto tgt = filtered_.NotNull() ? filtered_ : target_;

        if (matchToZone_)
            zone->SetZoneTexture(tgt);
        else if (zone->GetZoneTexture() == tgt) // if it's the same as us then clear it
            zone->SetZoneTexture(nullptr);
    }
}

void CubemapCapture::SetupTextures()
{
    target_.Reset(new TextureCube(GetContext()));
    target_->SetNumLevels(1);
    target_->SetSize(faceSize_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET);
    target_->SetFilterMode(FILTER_BILINEAR);

    filtered_.Reset(new TextureCube(GetContext()));
    filtered_->SetSize(faceSize_, Graphics::GetRGBAFormat(), TEXTURE_RENDERTARGET); // render-target usage is to make GL happy mip-map wise, otherwise have to refactor for glTexImage2D for each level
    filtered_->SetFilterMode(FILTER_BILINEAR);
}

SharedPtr<TextureCube> CubemapCapture::FilterCubemap(SharedPtr<TextureCube> cubeMap, unsigned rayCt)
{
    eastl::vector< SharedPtr<TextureCube> > dest;
    SharedPtr<TextureCube> copy(new TextureCube(cubeMap->GetContext()));
    copy->SetSize(cubeMap->GetWidth(), cubeMap->GetFormat());
    dest.push_back(copy);

    FilterCubemaps({ cubeMap }, dest, eastl::vector<unsigned> { rayCt });

    return dest.back();
}

void CubemapCapture::FilterCubemaps(const eastl::vector< SharedPtr<TextureCube> >& cubemaps, const eastl::vector< SharedPtr<TextureCube> >& destCubemaps, const eastl::vector<unsigned>& rayCounts)
{
#if !defined(URHO3D_COMPUTE)
    URHO3D_LOGERROR("CubemapCapture::FilterCubemaps, cannot be executed without URHO3D_COMPUTE enabled");
#else
    if (cubemaps.empty())
        return;

    unsigned firstDim = cubemaps[0]->GetWidth();
    for (auto& c : cubemaps)
    {
        if (c->GetWidth() != firstDim)
        {
            URHO3D_LOGERROR("CubemapCapture::FilterCubemaps, all cubemaps must have the same dimensions");
            return;
        }
    }

    unsigned levelCt = destCubemaps[0]->GetLevels();

    auto graphics = cubemaps[0]->GetSubsystem<Graphics>();
    auto cache = graphics->GetSubsystem<ResourceCache>();

    float roughStep = 1.0f / (float)(levelCt - 1);

    ea::vector< SharedPtr<ShaderVariation> > shaders;
    for (unsigned i = 0; i < levelCt; ++i)
    {
        const unsigned w = destCubemaps[0]->GetLevelWidth(i);
        const unsigned rayCt = rayCounts[Clamp<unsigned>(i, 0, rayCounts.size() - 1)]; // clip it to the first or last value if needed

        shaders.push_back(SharedPtr<ShaderVariation>(graphics->GetShader(CS, "v2/C_FilterCubemap", Format("RAY_COUNT={} FILTER_RES={} FILTER_INV_RES={} ROUGHNESS={}",
            rayCt,
            w,
            1.0f / (float)w,
            roughStep * i)
        )));
    }

    // go through them cubemap -> level
    auto computeDevice = graphics->GetSubsystem<ComputeDevice>();
    for (unsigned c = 0; c < cubemaps.size(); ++c)
    {
        auto& cube = cubemaps[c];
        auto& destCube = destCubemaps[c];
        computeDevice->SetReadTexture(cube, 0);
        for (unsigned i = 0; i < levelCt; ++i)
        {
            computeDevice->SetWriteTexture(destCube, 1, UINT_MAX, i);
            computeDevice->SetProgram(shaders[i]);
            computeDevice->Dispatch(destCube->GetLevelWidth(i), destCube->GetLevelHeight(i), 6);
        }
    }
    computeDevice->SetWriteTexture(nullptr, 1, 0, 0);
    computeDevice->ApplyBindings();
#endif
}

}
