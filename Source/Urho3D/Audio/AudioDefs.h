// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
