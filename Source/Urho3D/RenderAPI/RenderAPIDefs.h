// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#include "Urho3D/Container/Hash.h"
#include "Urho3D/Math/Color.h"

// TODO(diligent): Remove this include
#include "Urho3D/Graphics/GraphicsDefs.h"

#include <Diligent/Graphics/GraphicsEngine/interface/InputLayout.h>

#include <EASTL/array.h>
#include <EASTL/fixed_vector.h>

namespace Urho3D
{

/// Description of the single input required by the vertex shader.
struct URHO3D_API VertexShaderAttribute
{
    VertexElementSemantic semantic_{};
    unsigned semanticIndex_{};
    unsigned inputIndex_{};
};

/// Description of vertex shader attributes.
using VertexShaderAttributeVector = ea::fixed_vector<VertexShaderAttribute, Diligent::MAX_LAYOUT_ELEMENTS>;

/// Description of immutable texture sampler bound to the pipeline.
struct URHO3D_API SamplerStateDesc
{
    Color borderColor_{};
    TextureFilterMode filterMode_{FILTER_DEFAULT};
    unsigned char anisotropy_{};
    bool shadowCompare_{};
    ea::array<TextureAddressMode, 3> addressMode_{ADDRESS_CLAMP, ADDRESS_CLAMP, ADDRESS_CLAMP};

    bool operator==(const SamplerStateDesc& rhs) const
    {
        return borderColor_ == rhs.borderColor_ //
            && filterMode_ == rhs.filterMode_ //
            && anisotropy_ == rhs.anisotropy_ //
            && shadowCompare_ == rhs.shadowCompare_ //
            && addressMode_ == rhs.addressMode_;
    }

    bool operator!=(const SamplerStateDesc& rhs) const { return !(*this == rhs); }

    unsigned ToHash() const
    {
        unsigned hash = 0;
        CombineHash(hash, MakeHash(borderColor_));
        CombineHash(hash, filterMode_);
        CombineHash(hash, anisotropy_);
        CombineHash(hash, shadowCompare_);
        CombineHash(hash, addressMode_[COORD_U]);
        CombineHash(hash, addressMode_[COORD_V]);
        CombineHash(hash, addressMode_[COORD_W]);
        return hash;
    }
};

} // namespace Urho3D
