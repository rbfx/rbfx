//
// Copyright 2017-2023 Valve Corporation.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include "virtual_surround_effect.h"
using namespace ipl;

#include "phonon.h"
#include "util.h"

#define STEAMAUDIO_SKIP_API_FUNCTIONS
#include "phonon_interfaces.h"
#include "api_context.h"
#include "api_hrtf.h"
#include "api_virtual_surround_effect.h"

namespace api {

// --------------------------------------------------------------------------------------------------------------------
// CVirtualSurroundEffect
// --------------------------------------------------------------------------------------------------------------------

IPLint32 CVirtualSurroundEffect::getTailSize()
{
    auto _effect = mHandle.get();
    if (!_effect)
        return 0;

    return _effect->numTailSamplesRemaining();
}

IPLAudioEffectState CVirtualSurroundEffect::getTail(IPLAudioBuffer* out)
{
    auto _effect = mHandle.get();
    if (!_effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    AudioBuffer _out(out->numChannels, out->numSamples, out->data);

    return static_cast<IPLAudioEffectState>(_effect->tail(_out));
}

CVirtualSurroundEffect::CVirtualSurroundEffect(CContext* context,
                                               IPLAudioSettings* audioSettings,
                                               IPLVirtualSurroundEffectSettings* effectSettings)
{
    auto _context = context->mHandle.get();
    if (!_context)
        throw Exception(Status::Failure);

    auto _speakerLayout = SpeakerLayout(static_cast<SpeakerLayoutType>(effectSettings->speakerLayout.type),
                                        effectSettings->speakerLayout.numSpeakers,
                                        reinterpret_cast<Vector3f*>(effectSettings->speakerLayout.speakers));

    auto _hrtf = reinterpret_cast<CHRTF*>(effectSettings->hrtf)->mHandle.get();
    if (!_hrtf)
        throw Exception(Status::Failure);

    AudioSettings _audioSettings{};
    _audioSettings.samplingRate = audioSettings->samplingRate;
    _audioSettings.frameSize = audioSettings->frameSize;

    VirtualSurroundEffectSettings _effectSettings{};
    _effectSettings.speakerLayout = &_speakerLayout;
    _effectSettings.hrtf = _hrtf.get();

    new (&mHandle) Handle<VirtualSurroundEffect>(ipl::make_shared<VirtualSurroundEffect>(_audioSettings, _effectSettings), _context);
}

IVirtualSurroundEffect* CVirtualSurroundEffect::retain()
{
    mHandle.retain();
    return this;
}

void CVirtualSurroundEffect::release()
{
    if (mHandle.release())
    {
        this->~CVirtualSurroundEffect();
        gMemory().free(this);
    }
}

void CVirtualSurroundEffect::reset()
{
    auto _effect = mHandle.get();
    if (!_effect)
        return;

    _effect->reset();
}

IPLAudioEffectState CVirtualSurroundEffect::apply(IPLVirtualSurroundEffectParams* params,
                                                  IPLAudioBuffer* in,
                                                  IPLAudioBuffer* out)
{
    auto _effect = mHandle.get();
    if (!_effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    auto _hrtf = reinterpret_cast<CHRTF*>(params->hrtf)->mHandle.get();
    if (!_hrtf)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    AudioBuffer _in(in->numChannels, in->numSamples, in->data);
    AudioBuffer _out(out->numChannels, out->numSamples, out->data);

    VirtualSurroundEffectParams _params{};
    _params.hrtf = _hrtf.get();

    return static_cast<IPLAudioEffectState>(_effect->apply(_params, _in, _out));
}


// --------------------------------------------------------------------------------------------------------------------
// CContext
// --------------------------------------------------------------------------------------------------------------------

IPLerror CContext::createVirtualSurroundEffect(IPLAudioSettings* audioSettings,
                                               IPLVirtualSurroundEffectSettings* effectSettings,
                                               IVirtualSurroundEffect** effect)
{
    if (!audioSettings || !effectSettings || !effect)
        return IPL_STATUS_FAILURE;

    if (audioSettings->samplingRate <= 0 || audioSettings->frameSize <= 0)
        return IPL_STATUS_FAILURE;

    try
    {
        auto _effect = reinterpret_cast<CVirtualSurroundEffect*>(gMemory().allocate(sizeof(CVirtualSurroundEffect), Memory::kDefaultAlignment));
        new (_effect) CVirtualSurroundEffect(this, audioSettings, effectSettings);
        *effect = _effect;
    }
    catch (Exception e)
    {
        return static_cast<IPLerror>(e.status());
    }

    return IPL_STATUS_SUCCESS;
}

// --------------------------------------------------------------------------------------------------------------------
// API Functions
// --------------------------------------------------------------------------------------------------------------------

IPLint32 IPLCALL iplVirtualSurroundEffectGetTailSize(IPLVirtualSurroundEffect effect)
{
    if (!effect)
        return 0;

    auto _effect = reinterpret_cast<api::CVirtualSurroundEffect*>(effect);

    return _effect->getTailSize();
}

IPLAudioEffectState IPLCALL iplVirtualSurroundEffectGetTail(IPLVirtualSurroundEffect effect, IPLAudioBuffer* out)
{
    if (!effect)
        return IPL_AUDIOEFFECTSTATE_TAILCOMPLETE;

    auto _effect = reinterpret_cast<api::CVirtualSurroundEffect*>(effect);

    return _effect->getTail(out);
}

}
