// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Foundation/AnimationViewTab.h"

#include "../Core/CommonEditorActions.h"
#include "../Core/IniHelpers.h"

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Texture3D.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/Widgets.h>

namespace Urho3D
{

namespace
{
}

void Foundation_AnimationViewTab(Context* context, Project* project)
{
    project->AddTab(MakeShared<AnimationViewTab>(context));
}

AnimationViewTab::AnimationViewTab(Context* context)
    : CustomSceneViewTab(context, "Animation", "a8e49ac3-8edb-493c-ac7e-0d42530c62fb",
        EditorTabFlag::NoContentPadding | EditorTabFlag::OpenByDefault, EditorTabPlacement::DockCenter)
{
    modelNode_ = GetScene()->CreateChild("Model");
    animatedModel_ = modelNode_->CreateComponent<AnimatedModel>();
    animatedModel_->SetCastShadows(true);
    animationController_ = modelNode_->CreateComponent<AnimationController>();
}

AnimationViewTab::~AnimationViewTab()
{
}

bool AnimationViewTab::CanOpenResource(const ResourceFileDescriptor& desc)
{
    auto name = desc.typeNames_.begin()->c_str();
    return desc.HasObjectType<Animation>();
}

void AnimationViewTab::RenderTitle()
{
    CustomSceneViewTab::RenderTitle();

    auto* cache = GetSubsystem<ResourceCache>();

    static const StringVector allowedTextureTypes{
        Model::GetTypeNameStatic(),
    };

    StringHash textureType = model_ ? model_->GetType() : Texture2D::GetTypeStatic();
    ea::string textureName = model_ ? model_->GetName() : "";
    if (Widgets::EditResourceRef(textureType, textureName, &allowedTextureTypes))
    {
        model_ = cache->GetResource<Model>(textureName);
        animatedModel_->SetModel(model_);
        if (model_)
        {
            ResetCamera();
        }
    }
}

void AnimationViewTab::ResetCamera()
{
    if (model_)
    {
        state_.LookAt(model_->GetBoundingBox());
    }
}


void AnimationViewTab::RenderContent()
{
    if (!animation_)
        return;

    CustomSceneViewTab::RenderContent();
}

void AnimationViewTab::OnResourceLoaded(const ea::string& resourceName)
{
    auto cache = GetSubsystem<ResourceCache>();
    animation_ = cache->GetResource<Animation>(resourceName, false);

    auto params = AnimationParameters{animation_}.Layer(0);
    params.looped_ = true;
    animationController_->StopAll();
    animationController_->PlayNewExclusive(params);
}

void AnimationViewTab::OnResourceUnloaded(const ea::string& resourceName)
{
    animation_.Reset();
}

void AnimationViewTab::OnActiveResourceChanged(const ea::string& oldResourceName, const ea::string& newResourceName)
{
}

void AnimationViewTab::OnResourceSaved(const ea::string& resourceName)
{
}

void AnimationViewTab::OnResourceShallowSaved(const ea::string& resourceName)
{
}

} // namespace Urho3D
