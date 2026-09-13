// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/ArchiveSerialization.h"
#include "../Math/SphericalHarmonics.h"

#include "../DebugNew.h"

namespace Urho3D
{

const SphericalHarmonics9 SphericalHarmonics9::ZERO;
const SphericalHarmonicsColor9 SphericalHarmonicsColor9::ZERO;
const SphericalHarmonicsDot9 SphericalHarmonicsDot9::ZERO;

void SerializeValue(Archive& archive, const char* name, SphericalHarmonicsDot9& value)
{
    ArchiveBlock block = archive.OpenUnorderedBlock(name);
    SerializeValue(archive, "Ar", value.Ar_);
    SerializeValue(archive, "Ag", value.Ag_);
    SerializeValue(archive, "Ab", value.Ab_);
    SerializeValue(archive, "Br", value.Br_);
    SerializeValue(archive, "Bg", value.Bg_);
    SerializeValue(archive, "Bb", value.Bb_);
    SerializeValue(archive, "C", value.C_);
}

}
