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

#include "AssetInspector.h"
#include "Pipeline/Asset.h"
#include "Inspector/SoundInspector.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>

#include <Urho3D/SystemUI/SystemUI.h>

namespace Urho3D
{

SoundInspector::~SoundInspector()
{
    StopPlaying();
}

void SoundInspector::RenderInspector(const char* filter)
{
    if (auto self = inspected_.Lock())
    {
        if (Sound* asset = dynamic_cast<Sound*>(self.Get()))
        {
            if (!ui::CollapsingHeader(asset->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                return;

            ui::Separator();

            ui::Text("Frequency %u", (unsigned)asset->GetFrequency());
            ui::TextUnformatted(asset->IsStereo() ? "Stereo" : "Mono");
            if (asset->IsSixteenBit())
            {
                ui::SameLine();
                ui::TextUnformatted(", 16-bit");
            }
            if (asset->IsCompressed())
                ui::TextUnformatted("Compressed");
            if (asset->IsLooped())
                ui::Text("Loop Start: %ull", asset->GetRepeat() - asset->GetStart());

            if (ui::Button(ICON_FA_PLAY " Play"))
            {
                playingSource_.Reset(new SoundSource(context_));
                playingSource_->Play(asset);
            }
            ui::SameLine();
            if (ui::Button(ICON_FA_STOP " Stop"))
            {
                if (playingSource_)
                    playingSource_->Stop();
                playingSource_.Reset();
            }
            if (playingSource_)
            {
                ui::SameLine();

                float pos = playingSource_->GetTimePosition();
                if (ui::SliderFloat("##time", &pos, 0.0f, asset->GetLength()))
                    playingSource_->Seek(Max(Min(pos, asset->GetLength()), 0.0f));
                if (!playingSource_->IsPlaying())
                    playingSource_->Seek(0);
            }
        }
    }
}

void SoundInspector::ClearSelection()
{
    StopPlaying();
}

void SoundInspector::StopPlaying()
{
    if (playingSource_)
    {
        playingSource_->Stop();
        playingSource_.Reset();
    }
}

}
