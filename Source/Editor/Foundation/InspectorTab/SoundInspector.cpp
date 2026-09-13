// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../../Foundation/InspectorTab/SoundInspector.h"

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/SystemUI/SystemUI.h>

#include <IconFontCppHeaders/IconsFontAwesome6.h>

namespace Urho3D
{

void Foundation_SoundInspector(Context* context, InspectorTab* inspectorTab)
{
    inspectorTab->RegisterAddon<SoundInspector_>(inspectorTab->GetProject());
}

SoundInspector_::SoundInspector_(Project* project)
    : Object(project->GetContext())
    , project_(project)
    , soundSource_(MakeShared<SoundSource>(context_))
{
    soundSource_->SetIgnoreSceneTimeScale(true);
    project_->OnRequest.Subscribe(this, &SoundInspector_::OnProjectRequest);
}

void SoundInspector_::RenderContent()
{
    for (unsigned index = 0; index < sounds_.size(); ++index)
    {
        Sound* sound = sounds_[index];
        const IdScopeGuard guard(sound->GetName().c_str());
        if (index != 0)
            ui::Separator();

        ui::Text("%s", sound->GetName().c_str());
        RenderSound(sound);
    }
}

void SoundInspector_::RenderSound(Sound* sound)
{
    ea::string info;

    info += Format("Duration: {} s, Frequency: {} Hz\n",
        sound->GetLength(),
        static_cast<unsigned>(sound->GetFrequency()));

    info += Format("{}{}{}\n",
        sound->IsStereo() ? "Stereo" : "Mono",
        sound->IsSixteenBit() ? ", 16-bit" : "",
        sound->IsCompressed() ? ", Compressed" : "");

    if (sound->IsLooped())
        info += Format("Loop Start: {}\n", static_cast<unsigned>(sound->GetRepeat() - sound->GetStart()));

    ui::Text("%s", info.c_str());

    if (ui::Button(ICON_FA_PLAY " Play"))
        soundSource_->Play(sound);

    ui::SameLine();
    if (ui::Button(ICON_FA_STOP " Stop"))
        soundSource_->Stop();

    if (soundSource_->IsPlaying() && soundSource_->GetSound() == sound)
    {
        ui::SameLine();

        float pos = soundSource_->GetTimePosition();
        if (ui::SliderFloat("##time", &pos, 0.0f, sound->GetLength()))
            soundSource_->Seek(Max(Min(pos, sound->GetLength()), 0.0f));
        if (!soundSource_->IsPlaying())
            soundSource_->Seek(0);
    }
}

void SoundInspector_::RenderContextMenuItems()
{
}

void SoundInspector_::RenderMenu()
{
}

void SoundInspector_::ApplyHotkeys(HotkeyManager* hotkeyManager)
{
}

void SoundInspector_::OnProjectRequest(ProjectRequest* request)
{
    auto inspectResourceRequest = dynamic_cast<InspectResourceRequest*>(request);
    if (!inspectResourceRequest || inspectResourceRequest->GetResources().empty())
        return;

    const auto& resources = inspectResourceRequest->GetResources();

    const bool areAllSounds = ea::all_of(resources.begin(), resources.end(),
        [](const ResourceFileDescriptor& desc) { return desc.HasObjectType<Sound>(); });
    if (!areAllSounds)
        return;

    request->QueueProcessCallback([=]()
    {
        const auto resourceNames = inspectResourceRequest->GetSortedResourceNames();
        if (resourceNames_ != resourceNames)
        {
            resourceNames_ = resourceNames;
            InspectResources();
        }
        OnActivated(this);
    });
}

void SoundInspector_::InspectResources()
{
    auto cache = GetSubsystem<ResourceCache>();

    sounds_.clear();
    for (const auto& resourceName : resourceNames_)
    {
        if (auto sound = cache->GetResource<Sound>(resourceName))
            sounds_.emplace_back(sound);
    }
}

}
