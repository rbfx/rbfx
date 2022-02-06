//
// Copyright (c) 2008-2022 the Urho3D project.
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

#pragma once

#include "../Container/Str.h"

namespace Urho3D
{

// SoundSource type defaults
static const ea::string SOUND_MASTER = "Master";
static const ea::string SOUND_EFFECT = "Effect";
static const ea::string SOUND_AMBIENT = "Ambient";
static const ea::string SOUND_VOICE = "Voice";
static const ea::string SOUND_MUSIC = "Music";

// Audio channel configuration, WAV ordered.
enum SpeakerMode
{
    SPK_AUTO,
    SPK_MONO,           // Single channel
    SPK_STEREO,         // Stereo, L-R
    SPK_QUADROPHONIC,   // Surround 4, FL-FR-RL-RR
    SPK_SURROUND_5_1,   // 5.1 Surround, FL-FR-RL-RR-C-S (again WAV order)
};

}
