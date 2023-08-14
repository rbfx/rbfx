//
// Copyright (c) 2022 the RBFX project.
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

#include "Urho3D/XR/VRInterface.h"

#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/Graphics.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/StereoRenderPipeline.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

static ea::string VRLastTransform = "LastTransform";
static ea::string VRLastTransformWS = "LastTransformWS";

XRBinding::XRBinding(Context* ctx) : BaseClassName(ctx)
{

}

XRBinding::~XRBinding()
{

}

VRInterface::VRInterface(Context* ctx) : BaseClassName(ctx)
{

}

VRInterface::~VRInterface()
{
    viewport_.Reset();
}

void VRInterface::SetMSAALevel(int level)
{
    if (msaaLevel_ != level)
    {
        msaaLevel_ = Clamp(level, 1, 16);
        CreateEyeTextures();
    }
}

void VRInterface::SetRenderScale(float value)
{
    if (value != renderTargetScale_)
    {
        renderTargetScale_ = Clamp(renderTargetScale_, 0.25f, 2.0f);
        if (trueEyeTexWidth_ > 0)
        {
            eyeTexWidth_ = trueEyeTexWidth_ * renderTargetScale_;
            eyeTexHeight_ = trueEyeTexHeight_ * renderTargetScale_;
        }
        CreateEyeTextures();
    }
}

void VRInterface::CreateEyeTextures()
{
    sharedTexture_.Reset();
    leftTexture_.Reset();
    rightTexture_.Reset();

    sharedDS_.Reset();
    leftDS_.Reset();
    rightDS_.Reset();

    if (useSingleTexture_)
    {
        sharedTexture_ = new Texture2D(GetContext());
        sharedTexture_->SetNumLevels(1);
        sharedTexture_->SetSize(eyeTexWidth_ * 2, eyeTexHeight_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget, msaaLevel_);
        sharedTexture_->SetFilterMode(FILTER_BILINEAR);

        sharedDS_ = new Texture2D(GetContext());
        sharedDS_->SetNumLevels(1);
        sharedDS_->SetSize(eyeTexWidth_ * 2, eyeTexHeight_, TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT, TextureFlag::BindDepthStencil, msaaLevel_);
        sharedTexture_->GetRenderSurface()->SetLinkedDepthStencil(sharedDS_->GetRenderSurface());
    }
    else // TODO: are we going to make this path functional again? Maybe there is merit for ultra-wide FOV?
    {
        leftTexture_ = new Texture2D(GetContext());
        leftTexture_->SetNumLevels(1);
        leftTexture_->SetSize(eyeTexWidth_, eyeTexHeight_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget, msaaLevel_);
        leftTexture_->SetFilterMode(FILTER_BILINEAR);

        rightTexture_ = new Texture2D(GetContext());
        rightTexture_->SetNumLevels(1);
        rightTexture_->SetSize(eyeTexWidth_, eyeTexHeight_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget, msaaLevel_);
        rightTexture_->SetFilterMode(FILTER_BILINEAR);

        leftDS_ = new Texture2D(GetContext());
        leftDS_->SetNumLevels(1);
        leftDS_->SetSize(eyeTexWidth_, eyeTexHeight_, TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT, TextureFlag::BindDepthStencil, msaaLevel_);

        rightDS_ = new Texture2D(GetContext());
        rightDS_->SetNumLevels(1);
        rightDS_->SetSize(eyeTexWidth_, eyeTexHeight_, TextureFormat::TEX_FORMAT_D24_UNORM_S8_UINT, TextureFlag::BindDepthStencil, msaaLevel_);

        leftTexture_->GetRenderSurface()->SetLinkedDepthStencil(leftDS_->GetRenderSurface());
        rightTexture_->GetRenderSurface()->SetLinkedDepthStencil(rightDS_->GetRenderSurface());
    }
}

void VRInterface::PrepareRig(Node* headRoot)
{
    auto renderDevice = headRoot->GetContext()->GetSubsystem<RenderDevice>();

    headRoot->SetWorldPosition(Vector3(0, 0, 0));
    headRoot->SetWorldRotation(Quaternion::IDENTITY);
    auto head = headRoot->CreateChild("Head");

    auto leftEye = head->CreateChild("Left_Eye");
    auto rightEye = head->CreateChild("Right_Eye");

    auto leftCam = leftEye->GetOrCreateComponent<Camera>();
    auto rightCam = rightEye->GetOrCreateComponent<Camera>();

    if (renderDevice->GetBackend() == RenderBackend::OpenGL)
    {
        leftCam->SetFlipVertical(true);
        rightCam->SetFlipVertical(true);
    }

    auto leftHand = headRoot->CreateChild("Left_Hand");
    auto rightHand = headRoot->CreateChild("Right_Hand");
}

void VRInterface::UpdateRig(Node* vrRig, float nearDist, float farDist, bool forSinglePass)
{
    auto head = vrRig->GetChild("Head");
    auto leftEye = head->GetChild("Left_Eye");
    auto rightEye = head->GetChild("Right_Eye");

    UpdateRig(head->GetScene(), head, leftEye, rightEye, nearDist, farDist, forSinglePass);

    // Is below what we really want to do? Or do we have some sort of mode?
    //      ie. HeadTracking_Free, HeadTracking_LockedXY, HeadTracking_LockedY
    //      leave on caller to integrate velocity as required?
    //// track the stage directly under the head, we'll never change our Y
    //auto headPos = head->GetWorldPosition();
    //auto headLocal = head->GetPosition();
    //auto selfTrans = head->GetWorldPosition();
    //
    //// track head in the XZ plane
    //vrRig->SetWorldPosition(Vector3(headPos.x_, selfTrans.y_, headPos.z_));
    //
    //// neutralize the head position locally, keeping vertical, because the rig moved to track the OldTransformWS is still valid
    //head->SetPosition(Vector3(0, headLocal.y_, 0));
}

void VRInterface::UpdateRig(Scene* scene, Node* head, Node* leftEye, Node* rightEye, float nearDist, float farDist, bool forSinglePass)
{
    // always update these, they get used going into depth layers for time-warp, best assume that ever feeding bad values means bad things to eyeballs.
    lastNearDist_ = nearDist;
    lastFarDist_ = farDist;

    if (!IsLive())
        return;

    if (head == nullptr)
    {
        auto headRoot = scene->CreateChild("VRRig");
        head = headRoot->CreateChild("Head");
    }

    // no textures? create them now?
    if (sharedTexture_ == nullptr)
        CreateEyeTextures();

    head->SetVar(VRLastTransform, head->GetTransformMatrix());
    head->SetVar(VRLastTransformWS, head->GetWorldTransform());
    head->SetTransformMatrix(GetHeadTransform());

    if (leftEye == nullptr)
        leftEye = head->CreateChild("Left_Eye");
    if (rightEye == nullptr)
        rightEye = head->CreateChild("Right_Eye");

    auto leftCam = leftEye->GetOrCreateComponent<Camera>();
    auto rightCam = rightEye->GetOrCreateComponent<Camera>();

    leftCam->SetUseClipping(true); // need to set this so shader-construction grabs a version with clipping planes
    leftCam->SetFov(100.0f);  // junk mostly, the eye matrices will be overriden
    leftCam->SetNearClip(nearDist);
    leftCam->SetFarClip(farDist);

    rightCam->SetUseClipping(true);
    rightCam->SetFov(100.0f); // junk mostly, the eye matrices will be overriden
    rightCam->SetNearClip(nearDist);
    rightCam->SetFarClip(farDist);

    leftCam->SetProjection(GetProjection(VR_EYE_LEFT, nearDist, farDist));
    rightCam->SetProjection(GetProjection(VR_EYE_RIGHT, nearDist, farDist));

    if (GetRuntime() == VRRuntime::OPENVR)
    {
        leftEye->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_LEFT));
        rightEye->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_RIGHT));

        // uhhh ... what, and it's only the eyes, everyone else's transforms are good
        // buddha ... I hope this isn't backend specific
        leftEye->Rotate(Quaternion(0, 0, 180), TS_LOCAL);
        rightEye->Rotate(Quaternion(0, 0, 180), TS_LOCAL);
    }
    else if (GetRuntime() == VRRuntime::OPENXR)
    {
        leftEye->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_LEFT));
        rightEye->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_RIGHT));
    }
    else
        URHO3D_LOGERROR("Unknown VR runtime specified");

    float ipdAdjust = ipdCorrection_ * 0.5f * 0.001f;
    leftEye->Translate({ ipdAdjust, 0, 0 }, TS_LOCAL);
    rightEye->Translate({ -ipdAdjust, 0, 0 }, TS_LOCAL);

    if (viewport_ == nullptr)
        URHO3D_LOGWARNING("VRInterface requires a Viewport to be specified, not specifying one will cause a default to be constructed");

    if (sharedTexture_ && forSinglePass)
    {
        auto surface = sharedTexture_->GetRenderSurface();
        if (surface == nullptr) // no surface then we're not actually read
            return;

        if (surface->GetViewport(0) == nullptr)
        {
            // Cover the case someone's messed up
            if (viewport_ == nullptr)
                viewport_ = new Viewport(GetContext(), scene, leftCam, nullptr, pipeline_ = MakeShared<StereoRenderPipeline>(GetContext()));

            viewport_->SetEye(leftCam, 0);
            viewport_->SetEye(rightCam, 1);
            viewport_->SetRect({ 0, 0, sharedTexture_->GetWidth(), sharedTexture_->GetHeight() });
            surface->SetViewport(0, viewport_);
        }
        else
        {
            auto view = surface->GetViewport(0);
            view->SetScene(scene);
            view->SetEye(leftCam, 0);
            view->SetEye(rightCam, 1);
        }

        // we need to queue the update ourselves so things can get properly shutdown
        surface->QueueUpdate();
    }
    else
    {
        // TODO: why is this still here?
        auto leftSurface = useSingleTexture_ ? sharedTexture_->GetRenderSurface() : leftTexture_->GetRenderSurface();
        auto rightSurface = useSingleTexture_ ? sharedTexture_->GetRenderSurface() : rightTexture_->GetRenderSurface();

        if (leftSurface->GetViewport(0) == nullptr)
        {
            SharedPtr<Viewport> leftView(new Viewport(GetContext(), scene, leftCam));
            SharedPtr<Viewport> rightView(new Viewport(GetContext(), scene, rightCam));

            leftView->SetRect(GetLeftEyeRect());
            rightView->SetRect(GetRightEyeRect());

            leftSurface->SetViewport(0, leftView);
            rightSurface->SetViewport(1, rightView);
        }
        else
        {
            auto leftView = leftSurface->GetViewport(0);
            leftView->SetScene(scene);
            leftView->SetCamera(leftCam);

            auto rightView = rightSurface->GetViewport(1);
            rightView->SetScene(scene);
            rightView->SetCamera(rightCam);
        }

        leftSurface->SetUpdateMode(SURFACE_UPDATEALWAYS);
        rightSurface->SetUpdateMode(SURFACE_UPDATEALWAYS);
    }
}

SharedPtr<XRBinding> VRInterface::GetInputBinding(const ea::string& path)
{
    if (activeActionSet_)
    {
        for (auto b : activeActionSet_->bindings_)
            if (b->path_.comparei(path) == 0)
                return b;
    }
    return nullptr;
}

SharedPtr<XRBinding> VRInterface::GetInputBinding(const ea::string& path, VRHand hand)
{
    if (activeActionSet_)
    {
        for (auto b : activeActionSet_->bindings_)
            if (hand == b->hand_ && b->path_.comparei(path) == 0)
                return b;
    }
    return nullptr;
}

void VRInterface::DrawEyeMask()
{
    // TODO(xr): Fix me
#if 0
    // TODO: is there a meaningful difference between going with z-fail or stencil? Both have to do VS to find clip-space?
    if (hiddenAreaMesh_[0] && hiddenAreaMesh_[1])
    {
        auto renderer = GetSubsystem<Renderer>();
        auto gfx = GetSubsystem<Graphics>();

        if (!eyeMaskPipelineState_)
        {
            GraphicsPipelineStateDesc stateDesc = { };
            stateDesc.InitializeInputLayout(GeometryBufferArray{ { hiddenAreaMesh_[0]->GetVertexBuffer(0) }, hiddenAreaMesh_[0]->GetIndexBuffer(), nullptr });
            stateDesc.indexType_ = IBT_UINT32;
            stateDesc.primitiveType_ = TRIANGLE_LIST;
            stateDesc.depthWriteEnabled_ = true;
            stateDesc.colorWriteEnabled_ = true; // so we can troubleshoot testing neon colors, but also so we get black so there won't be bleed artifacts or glow over.
            stateDesc.cullMode_ = CULL_NONE;
            stateDesc.blendMode_ = BLEND_REPLACE;
            stateDesc.fillMode_ = FILL_SOLID;

            ea::string defs = ea::string("URHO3D_USE_CBUFFERS ") + GetRuntimeName();
            stateDesc.vertexShader_ = gfx->GetShader(VS, "v2/VR_EyeMask", defs.c_str());
            stateDesc.pixelShader_ = gfx->GetShader(PS, "v2/VR_EyeMask", defs.c_str());

            eyeMaskPipelineState_ = renderer->GetOrCreatePipelineState(stateDesc);
        }

        if (!eyeMaskPipelineState_)
            return;

        IntRect oldViewport = gfx->GetViewport();
        IntRect vpts[] = {
            GetLeftEyeRect(),
            GetRightEyeRect()
        };

        for (int i = 0; i < 2; ++i)
        {
            gfx->SetViewport(vpts[i]);

            auto drawQueue = renderer->GetDefaultDrawQueue();
            drawQueue->Reset(false);

            drawQueue->SetBuffers({ { hiddenAreaMesh_[i]->GetVertexBuffer(0) }, hiddenAreaMesh_[i]->GetIndexBuffer(), nullptr });
            drawQueue->SetPipelineState(eyeMaskPipelineState_);

            if (drawQueue->BeginShaderParameterGroup(SP_CUSTOM, true))
            {
                drawQueue->AddShaderParameter(StringHash("Projection"), GetProjection((VREye)i, 0, 1));
                drawQueue->CommitShaderParameterGroup(SP_CUSTOM);
            }

            drawQueue->CommitShaderResources();
            drawQueue->DrawIndexed(hiddenAreaMesh_[i]->GetIndexStart(), hiddenAreaMesh_[i]->GetIndexCount());

            drawQueue->Execute();
        }

        gfx->SetViewport(oldViewport);
    }
#endif
}

void VRInterface::DrawRadialMask(BlendMode blendMode, Color inside, Color outside, float power)
{
    // TODO(xr): Fix me
#if 0
    if (radialAreaMesh_[0] && radialAreaMesh_[1])
    {
        auto renderer = GetSubsystem<Renderer>();
        auto gfx = GetSubsystem<Graphics>();

        if (!simpleVignettePipelineState_[blendMode])
        {
            GraphicsPipelineStateDesc stateDesc = { };
            stateDesc.InitializeInputLayout(GeometryBufferArray{ { radialAreaMesh_[0]->GetVertexBuffer(0) }, radialAreaMesh_[0]->GetIndexBuffer(), nullptr });
            stateDesc.indexType_ = IBT_UINT32;
            stateDesc.primitiveType_ = TRIANGLE_LIST;
            stateDesc.depthWriteEnabled_ = false;
            stateDesc.blendMode_ = blendMode;
            stateDesc.colorWriteEnabled_ = true;
            stateDesc.cullMode_ = CULL_NONE;

            ea::string defs = ea::string("URHO3D_USE_CBUFFERS ") + GetRuntimeName();
            stateDesc.vertexShader_ = gfx->GetShader(VS, "v2/VR_SimpleVignette", defs);
            stateDesc.pixelShader_ = gfx->GetShader(PS, "v2/VR_SimpleVignette", defs);

            simpleVignettePipelineState_[blendMode] = renderer->GetOrCreatePipelineState(stateDesc);
        }

        if (!simpleVignettePipelineState_[blendMode])
            return;

        IntRect oldViewport = gfx->GetViewport();
        IntRect vpts[] = {
            GetLeftEyeRect(),
            GetRightEyeRect()
        };

        for (int i = 0; i < 2; ++i)
        {
            gfx->SetViewport(vpts[i]);

            auto drawQueue = renderer->GetDefaultDrawQueue();
            drawQueue->Reset();

            drawQueue->SetBuffers({ { radialAreaMesh_[i]->GetVertexBuffer(0) }, radialAreaMesh_[i]->GetIndexBuffer(), nullptr });
            drawQueue->SetPipelineState(simpleVignettePipelineState_[blendMode]);

            if (drawQueue->BeginShaderParameterGroup(SP_CUSTOM, true))
            {
                drawQueue->AddShaderParameter(StringHash("Projection"), GetProjection((VREye)i, 0, 1));
                drawQueue->AddShaderParameter(StringHash("InsideColor"), inside);
                drawQueue->AddShaderParameter(StringHash("OutsideColor"), outside);
                drawQueue->AddShaderParameter(StringHash("BlendPower"), power);
                drawQueue->CommitShaderParameterGroup(SP_CUSTOM);
            }

            drawQueue->DrawIndexed(radialAreaMesh_[i]->GetIndexStart(), radialAreaMesh_[i]->GetIndexCount());

            drawQueue->Execute();
        }

        gfx->SetViewport(oldViewport);
    }
#endif
}

void VRInterface::SetCurrentActionSet(const ea::string& setName)
{
    auto found = actionSets_.find(setName);
    if (found != actionSets_.end())
        SetCurrentActionSet(found->second);
}

void VRInterface::SetVignette(bool enabled, Color insideColor, Color outsideColor, float power)
{
    vignetteEnabled_ = enabled;
    vignetteInsideColor_ = insideColor;
    vignetteOutsideColor_ = outsideColor;
    vignettePower_ = power;
}

void RegisterVR(Context* context)
{
#if 0
    VRRigWalker::Register(context);
#endif
}

}
