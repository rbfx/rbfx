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
