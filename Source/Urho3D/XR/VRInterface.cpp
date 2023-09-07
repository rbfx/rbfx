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
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Graphics/Model.h"
#include "Urho3D/Graphics/Octree.h"
#include "Urho3D/Graphics/Renderer.h"
#include "Urho3D/Graphics/Skybox.h"
#include "Urho3D/Graphics/StaticModel.h"
#include "Urho3D/Graphics/Technique.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/PipelineState.h"
#include "Urho3D/RenderAPI/RenderDevice.h"
#include "Urho3D/RenderPipeline/ShaderConsts.h"
#include "Urho3D/RenderPipeline/StereoRenderPipeline.h"
#include "Urho3D/Resource/ResourceCache.h"
#include "Urho3D/Resource/XMLFile.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"
#include "Urho3D/UI/UI.h"
#include "Urho3D/XR/VRRig.h"

#if URHO3D_RMLUI
    #include "Urho3D/RmlUI/RmlUI.h"
#endif

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

bool VRRigDesc::IsValid() const
{
    return scene_ && head_ && leftEye_ && rightEye_ && leftHand_ && rightHand_;
}

XRBinding::XRBinding(Context* context, const ea::string& name, const ea::string& localizedName, VRHand hand,
    VariantType dataType, bool isPose, bool isAimPose)
    : BaseClassName(context)
    , name_(name)
    , localizedName_(localizedName)
    , hand_(hand)
    , dataType_(dataType)
    , haptic_(dataType_ == VAR_NONE)
    , isPose_(isPose)
    , isAimPose_(isAimPose)
{
}

XRActionGroup::XRActionGroup(Context* context, const ea::string& name, const ea::string& localizedName)
    : BaseClassName(context)
    , name_(name)
    , localizedName_(localizedName)
{
}

XRBinding* XRActionGroup::FindBinding(const ea::string& name, VRHand hand) const
{
    for (XRBinding* binding : bindings_)
    {
        if (binding->GetName().comparei(name) == 0 && (hand == VR_HAND_NONE || hand == binding->Hand()))
            return binding;
    }
    return nullptr;
}

VRInterface::VRInterface(Context* ctx) : BaseClassName(ctx)
{
}

VRInterface::~VRInterface()
{
}

void VRInterface::ShutdownSession()
{
    if (flatScreenTexture_)
    {
        auto cache = GetSubsystem<ResourceCache>();
        auto renderer = GetSubsystem<Renderer>();
        auto legacyUI = GetSubsystem<UI>();
#if URHO3D_RMLUI
        auto rmlUI = GetSubsystem<RmlUI>();
#endif

        cache->ReleaseResource(flatScreenTexture_->GetName(), true);
        cache->ReleaseResource(flatScreenMaterial_->GetName(), true);

        flatScreenTexture_ = nullptr;
        flatScreenMaterial_ = nullptr;

        renderer->SetBackbufferRenderSurface(nullptr);
        legacyUI->SetRenderTarget(nullptr);
#if URHO3D_RMLUI
        rmlUI->SetRenderTarget(nullptr);
#endif
    }
}

void VRInterface::ConnectToRig(const VRRigDesc& rig)
{
    if (!rig.IsValid())
    {
        URHO3D_LOGERROR("Invalid VR rig description");
        return;
    }

    rig_ = rig;

    if (!rig_.pipeline_)
        rig_.pipeline_ = MakeShared<StereoRenderPipeline>(context_);
    if (!rig_.viewport_)
        rig_.viewport_ = new Viewport(context_, rig_.scene_, rig_.leftEye_, nullptr, rig_.pipeline_);

    rig_.viewport_->SetEye(rig_.leftEye_, 0);
    rig_.viewport_->SetEye(rig_.rightEye_, 1);
}

void VRInterface::CreateDefaultRig(const VRFlatScreenParameters& params)
{
    auto cache = GetSubsystem<ResourceCache>();
    auto renderer = GetSubsystem<Renderer>();
    auto legacyUI = GetSubsystem<UI>();
#if URHO3D_RMLUI
    auto rmlUI = GetSubsystem<RmlUI>();
#endif

    defaultScene_ = MakeShared<Scene>(context_);
    defaultScene_->CreateComponent<Octree>();

    Node* skyboxNode = defaultScene_->CreateChild("Skybox");
    auto skybox = skyboxNode->CreateComponent<Skybox>();
    skybox->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    skybox->SetMaterial(cache->GetResource<Material>("Materials/DefaultSkybox.xml"));

    Node* rigNode = defaultScene_->CreateChild("VRRig");
    defaultRig_ = rigNode->CreateComponent<VRRig>();

    if (params.enable_)
    {
        flatScreenTexture_ = MakeShared<Texture2D>(context_);
        flatScreenTexture_->SetName("manual://Textures/FlatScreen.raw");
        flatScreenTexture_->SetSize(
            params.size_.x_, params.size_.y_, TextureFormat::TEX_FORMAT_RGBA8_UNORM, TextureFlag::BindRenderTarget);
        cache->AddManualResource(flatScreenTexture_);

        flatScreenMaterial_ = MakeShared<Material>(context_);
        flatScreenMaterial_->SetName("manual://Materials/FlatScreen.material");
        flatScreenMaterial_->SetTexture(ShaderResources::Albedo, flatScreenTexture_);
        flatScreenMaterial_->SetCullMode(CULL_NONE);
        auto technique = cache->GetResource<Technique>("Techniques/UnlitTransparent.xml");
        flatScreenMaterial_->SetTechnique(0, technique);

        Node* flatScreenNode = defaultScene_->CreateChild("FlatScreen");
        flatScreenNode->SetPosition({0, 2.0, params.distance_});
        flatScreenNode->SetRotation({-90.0f, Vector3::RIGHT});
        const float aspectRatio = static_cast<float>(params.size_.x_) / params.size_.y_;
        flatScreenNode->SetScale(Vector3{aspectRatio, 1.0f, 1.0f} * params.height_);

        auto flatScreenModel = flatScreenNode->CreateComponent<StaticModel>();
        flatScreenModel->SetModel(cache->GetResource<Model>("Models/Plane.mdl"));
        flatScreenModel->SetMaterial(0, flatScreenMaterial_);

        renderer->SetBackbufferRenderSurface(flatScreenTexture_->GetRenderSurface());
        legacyUI->SetRenderTarget(flatScreenTexture_);
#if URHO3D_RMLUI
        rmlUI->SetRenderTarget(flatScreenTexture_);
#endif
    }
}

void VRInterface::ValidateCurrentRig()
{
    if (!rig_.IsValid())
        defaultRig_->Activate();
}

void VRInterface::UpdateCurrentRig()
{
    URHO3D_ASSERT(GetRuntime() == VRRuntime::OPENXR, "Only OpenXR is supported at this time");

    // Skip update if we are not ready
    RenderSurface* currentSurface = currentBackBufferColor_ ? currentBackBufferColor_->GetRenderSurface() : nullptr;
    if (!rig_.IsValid() || !currentSurface)
        return;

    // Update transforms and cameras
    Node* head = rig_.head_;
    head->SetVar("PreviousTransformLocal", head->GetTransformMatrix());
    head->SetVar("PreviousTransformWorld", head->GetWorldTransform());
    head->SetTransformMatrix(GetHeadTransform());

    Node* leftEyeNode = rig_.leftEye_->GetNode();
    Node* rightEyeNode = rig_.rightEye_->GetNode();

    Camera* leftEyeCamera = rig_.leftEye_;
    Camera* rightEyeCamera = rig_.rightEye_;

    for (Camera* camera : {leftEyeCamera, rightEyeCamera})
    {
        // TODO(xr): Revisit how clipping is handled
        camera->SetUseClipping(true); // need to set this so shader-construction grabs a version with clipping planes
        camera->SetFov(100.0f); // junk mostly, the eye matrices will be overriden
        camera->SetNearClip(rig_.nearDistance_);
        camera->SetFarClip(rig_.farDistance_);
    }

    leftEyeCamera->SetProjection(GetProjection(VR_EYE_LEFT, rig_.nearDistance_, rig_.farDistance_));
    rightEyeCamera->SetProjection(GetProjection(VR_EYE_RIGHT, rig_.nearDistance_, rig_.farDistance_));

    leftEyeNode->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_LEFT));
    rightEyeNode->SetTransformMatrix(GetEyeLocalTransform(VR_EYE_RIGHT));

    const float ipdAdjust = ipdCorrection_ * 0.5f * 0.001f;
    leftEyeNode->Translate({ipdAdjust, 0, 0}, TS_LOCAL);
    rightEyeNode->Translate({-ipdAdjust, 0, 0}, TS_LOCAL);

    // Connect to the current surface in the swap chain
    if (currentSurface->GetViewport(0) != rig_.viewport_)
        currentSurface->SetViewport(0, rig_.viewport_);

    rig_.viewport_->SetRect({0, 0, currentBackBufferColor_->GetWidth(), currentBackBufferColor_->GetHeight()});
    currentSurface->QueueUpdate();
}

XRBinding* VRInterface::GetInputBinding(const ea::string& path) const
{
    if (activeActionSet_)
    {
        if (XRBinding* binding = activeActionSet_->FindBinding(path, VR_HAND_NONE))
            return binding;
    }
    return nullptr;
}

XRBinding* VRInterface::GetInputBinding(const ea::string& path, VRHand hand) const
{
    if (activeActionSet_)
    {
        if (XRBinding* binding = activeActionSet_->FindBinding(path, hand))
            return binding;
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

void RegisterVRLibrary(Context* context)
{
    VRRig::RegisterObject(context);
}

}
