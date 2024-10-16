/*
 *  Copyright 2019-2023 Diligent Graphics LLC
 *  Copyright 2015-2019 Egor Yusov
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *  In no event and under no legal theory, whether in tort (including negligence),
 *  contract, or otherwise, unless required by applicable law (such as deliberate
 *  and grossly negligent acts) or agreed to in writing, shall any Contributor be
 *  liable for any damages, including any direct, indirect, special, incidental,
 *  or consequential damages of any character arising as a result of this License or
 *  out of the use or inability to use the software (including but not limited to damages
 *  for loss of goodwill, work stoppage, computer failure or malfunction, or any and
 *  all other commercial damages or losses), even if such Contributor has been advised
 *  of the possibility of such damages.
 */

#pragma once

// clang-format off

/// \file
/// Definition of the Diligent::ITextureView interface and related data structures

#include "../../../Primitives/interface/FlagEnum.h"
#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

struct ISampler;

// {5B2EA04E-8128-45E4-AA4D-6DC7E70DC424}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_TextureView =
    {0x5b2ea04e, 0x8128, 0x45e4,{0xaa, 0x4d, 0x6d, 0xc7, 0xe7, 0xd, 0xc4, 0x24}};

// clang-format off

/// Describes allowed unordered access view mode
DILIGENT_TYPED_ENUM(UAV_ACCESS_FLAG, Uint8)
{
    /// Access mode is unspecified
    UAV_ACCESS_UNSPECIFIED = 0x00,

    /// Allow read operations on the UAV
    UAV_ACCESS_FLAG_READ   = 0x01,

    /// Allow write operations on the UAV
    UAV_ACCESS_FLAG_WRITE  = 0x02,

    /// Allow read and write operations on the UAV
    UAV_ACCESS_FLAG_READ_WRITE = UAV_ACCESS_FLAG_READ | UAV_ACCESS_FLAG_WRITE,

    UAV_ACCESS_FLAG_LAST = UAV_ACCESS_FLAG_READ_WRITE
};
DEFINE_FLAG_ENUM_OPERATORS(UAV_ACCESS_FLAG)

/// Texture view flags
DILIGENT_TYPED_ENUM(TEXTURE_VIEW_FLAGS, Uint8)
{
    /// No flags
    TEXTURE_VIEW_FLAG_NONE                     = 0,

    /// Allow automatic mipmap generation for this view.
    /// This flag is only allowed for TEXTURE_VIEW_SHADER_RESOURCE view type.
    /// The texture must be created with MISC_TEXTURE_FLAG_GENERATE_MIPS flag.
    TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION = 1u << 0,

    TEXTURE_VIEW_FLAG_LAST = TEXTURE_VIEW_FLAG_ALLOW_MIP_MAP_GENERATION
};
DEFINE_FLAG_ENUM_OPERATORS(TEXTURE_VIEW_FLAGS)


/// Texture component swizzle
DILIGENT_TYPED_ENUM(TEXTURE_COMPONENT_SWIZZLE, Uint8)
{
    /// Identity swizzle (e.g. R->R, G->G, B->B, A->A).
    TEXTURE_COMPONENT_SWIZZLE_IDENTITY = 0,

    /// The component is set to zero.
    TEXTURE_COMPONENT_SWIZZLE_ZERO,

    /// The component is set to one.
    TEXTURE_COMPONENT_SWIZZLE_ONE,

    /// The component is set to the value of the red channel of the texture.
    TEXTURE_COMPONENT_SWIZZLE_R,

    /// The component is set to the value of the green channel of the texture.
    TEXTURE_COMPONENT_SWIZZLE_G,

    /// The component is set to the value of the blue channel of the texture.
    TEXTURE_COMPONENT_SWIZZLE_B,

    /// The component is set to the value of the alpha channel of the texture.
    TEXTURE_COMPONENT_SWIZZLE_A,

    TEXTURE_COMPONENT_SWIZZLE_COUNT
};


/// Defines the per-channel texutre component mapping.
struct TextureComponentMapping
{
    /// Defines the component placed in the red component of the output vector.
    TEXTURE_COMPONENT_SWIZZLE R DEFAULT_INITIALIZER(TEXTURE_COMPONENT_SWIZZLE_IDENTITY);

    /// Defines the component placed in the green component of the output vector.
    TEXTURE_COMPONENT_SWIZZLE G DEFAULT_INITIALIZER(TEXTURE_COMPONENT_SWIZZLE_IDENTITY);

    /// Defines the component placed in the blue component of the output vector.
    TEXTURE_COMPONENT_SWIZZLE B DEFAULT_INITIALIZER(TEXTURE_COMPONENT_SWIZZLE_IDENTITY);

    /// Defines the component placed in the alpha component of the output vector.
    TEXTURE_COMPONENT_SWIZZLE A DEFAULT_INITIALIZER(TEXTURE_COMPONENT_SWIZZLE_IDENTITY);

#if DILIGENT_CPP_INTERFACE
    constexpr TextureComponentMapping() noexcept {}

    constexpr TextureComponentMapping(TEXTURE_COMPONENT_SWIZZLE _R,
                                      TEXTURE_COMPONENT_SWIZZLE _G,
                                      TEXTURE_COMPONENT_SWIZZLE _B,
                                      TEXTURE_COMPONENT_SWIZZLE _A) noexcept :
        R{_R},
        G{_G},
        B{_B},
        A{_A}
    {}

    constexpr Uint32 AsUint32() const
    {
        return (static_cast<Uint32>(R) <<  0u) |
               (static_cast<Uint32>(G) <<  8u) |
               (static_cast<Uint32>(B) << 16u) |
               (static_cast<Uint32>(A) << 24u);
    }

    constexpr bool operator==(const TextureComponentMapping& RHS) const
    {
        return (R == RHS.R || (R == TEXTURE_COMPONENT_SWIZZLE_IDENTITY && RHS.R == TEXTURE_COMPONENT_SWIZZLE_R) || (R == TEXTURE_COMPONENT_SWIZZLE_R && RHS.R == TEXTURE_COMPONENT_SWIZZLE_IDENTITY)) &&
               (G == RHS.G || (G == TEXTURE_COMPONENT_SWIZZLE_IDENTITY && RHS.G == TEXTURE_COMPONENT_SWIZZLE_G) || (G == TEXTURE_COMPONENT_SWIZZLE_G && RHS.G == TEXTURE_COMPONENT_SWIZZLE_IDENTITY)) &&
			   (B == RHS.B || (B == TEXTURE_COMPONENT_SWIZZLE_IDENTITY && RHS.B == TEXTURE_COMPONENT_SWIZZLE_B) || (B == TEXTURE_COMPONENT_SWIZZLE_B && RHS.B == TEXTURE_COMPONENT_SWIZZLE_IDENTITY)) &&
			   (A == RHS.A || (A == TEXTURE_COMPONENT_SWIZZLE_IDENTITY && RHS.A == TEXTURE_COMPONENT_SWIZZLE_A) || (A == TEXTURE_COMPONENT_SWIZZLE_A && RHS.A == TEXTURE_COMPONENT_SWIZZLE_IDENTITY));
    }
    constexpr bool operator!=(const TextureComponentMapping& RHS) const
    {
        return !(*this == RHS);
    }

    constexpr TEXTURE_COMPONENT_SWIZZLE operator[](size_t Component) const
	{
		return (&R)[Component];
	}

    constexpr TEXTURE_COMPONENT_SWIZZLE& operator[](size_t Component)
	{
		return (&R)[Component];
	}

    static constexpr TextureComponentMapping Identity()
	{
		return {
            TEXTURE_COMPONENT_SWIZZLE_IDENTITY,
            TEXTURE_COMPONENT_SWIZZLE_IDENTITY,
            TEXTURE_COMPONENT_SWIZZLE_IDENTITY,
            TEXTURE_COMPONENT_SWIZZLE_IDENTITY
        };
    }

    // Combines two component mappings into one.
    // The resulting mapping is equivalent to first applying the first (lhs) mapping,
    // then applying the second (rhs) mapping.
    TextureComponentMapping operator*(const TextureComponentMapping& Rhs) const
	{
        TextureComponentMapping CombinedMapping;
        for (size_t c = 0; c < 4; ++c)
        {
        	TEXTURE_COMPONENT_SWIZZLE  RhsCompSwizzle = Rhs[c];
            TEXTURE_COMPONENT_SWIZZLE& DstCompSwizzle = CombinedMapping[c];
            switch (RhsCompSwizzle)
            {
                case TEXTURE_COMPONENT_SWIZZLE_IDENTITY: DstCompSwizzle = (*this)[c]; break;
                case TEXTURE_COMPONENT_SWIZZLE_ZERO:     DstCompSwizzle = TEXTURE_COMPONENT_SWIZZLE_ZERO; break;
                case TEXTURE_COMPONENT_SWIZZLE_ONE:      DstCompSwizzle = TEXTURE_COMPONENT_SWIZZLE_ONE; break;
                case TEXTURE_COMPONENT_SWIZZLE_R:        DstCompSwizzle = (R == TEXTURE_COMPONENT_SWIZZLE_IDENTITY) ? TEXTURE_COMPONENT_SWIZZLE_R : R; break;
                case TEXTURE_COMPONENT_SWIZZLE_G:        DstCompSwizzle = (G == TEXTURE_COMPONENT_SWIZZLE_IDENTITY) ? TEXTURE_COMPONENT_SWIZZLE_G : G; break;
                case TEXTURE_COMPONENT_SWIZZLE_B:        DstCompSwizzle = (B == TEXTURE_COMPONENT_SWIZZLE_IDENTITY) ? TEXTURE_COMPONENT_SWIZZLE_B : B; break;
                case TEXTURE_COMPONENT_SWIZZLE_A:        DstCompSwizzle = (A == TEXTURE_COMPONENT_SWIZZLE_IDENTITY) ? TEXTURE_COMPONENT_SWIZZLE_A : A; break;
                default: DstCompSwizzle = (*this)[c]; break;
            }

            if ((DstCompSwizzle == TEXTURE_COMPONENT_SWIZZLE_R && c == 0) ||
                (DstCompSwizzle == TEXTURE_COMPONENT_SWIZZLE_G && c == 1) ||
				(DstCompSwizzle == TEXTURE_COMPONENT_SWIZZLE_B && c == 2) ||
				(DstCompSwizzle == TEXTURE_COMPONENT_SWIZZLE_A && c == 3))
			{
				DstCompSwizzle = TEXTURE_COMPONENT_SWIZZLE_IDENTITY;
			}
        }
        static_assert(TEXTURE_COMPONENT_SWIZZLE_COUNT == 7, "Please handle the new component swizzle");
        return CombinedMapping;
	}

    TextureComponentMapping& operator*=(const TextureComponentMapping& rhs)
    {
        *this = *this * rhs;
        return *this;
    }
#endif
};
typedef struct TextureComponentMapping TextureComponentMapping;


/// Texture view description
struct TextureViewDesc DILIGENT_DERIVE(DeviceObjectAttribs)

    /// Describes the texture view type, see Diligent::TEXTURE_VIEW_TYPE for details.
    TEXTURE_VIEW_TYPE ViewType     DEFAULT_INITIALIZER(TEXTURE_VIEW_UNDEFINED);

    /// View interpretation of the original texture. For instance,
    /// one slice of a 2D texture array can be viewed as a 2D texture.
    /// See Diligent::RESOURCE_DIMENSION for a list of texture types.
    /// If default value Diligent::RESOURCE_DIM_UNDEFINED is provided,
    /// the view type will match the type of the referenced texture.
    RESOURCE_DIMENSION TextureDim  DEFAULT_INITIALIZER(RESOURCE_DIM_UNDEFINED);

    /// View format. If default value Diligent::TEX_FORMAT_UNKNOWN is provided,
    /// the view format will match the referenced texture format.
    TEXTURE_FORMAT Format          DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Most detailed mip level to use
    Uint32 MostDetailedMip         DEFAULT_INITIALIZER(0);

    /// Total number of mip levels for the view of the texture.
    /// Render target and depth stencil views can address only one mip level.
    /// If 0 is provided, then for a shader resource view all mip levels will be
    /// referenced, and for a render target or a depth stencil view, one mip level
    /// will be referenced.
    Uint32 NumMipLevels            DEFAULT_INITIALIZER(0);

#if defined(DILIGENT_SHARP_GEN)
    Uint32 FirstSlice DEFAULT_INITIALIZER(0);
#else
    union
    {
        /// For a texture array, first array slice to address in the view
        Uint32 FirstArraySlice DEFAULT_INITIALIZER(0);

        /// For a 3D texture, first depth slice to address the view
        Uint32 FirstDepthSlice;
    };
#endif

#if defined(DILIGENT_SHARP_GEN)
    Uint32 NumSlices DEFAULT_INITIALIZER(0);
#else
    union
    {
        /// For a texture array, number of array slices to address in the view.
        /// Set to 0 to address all array slices.
        Uint32 NumArraySlices DEFAULT_INITIALIZER(0);

        /// For a 3D texture, number of depth slices to address in the view
        /// Set to 0 to address all depth slices.
        Uint32 NumDepthSlices;
    };
#endif

    /// For an unordered access view, allowed access flags. See Diligent::UAV_ACCESS_FLAG
    /// for details.
    UAV_ACCESS_FLAG    AccessFlags DEFAULT_INITIALIZER(UAV_ACCESS_UNSPECIFIED);

    /// Texture view flags, see Diligent::TEXTURE_VIEW_FLAGS.
    TEXTURE_VIEW_FLAGS Flags       DEFAULT_INITIALIZER(TEXTURE_VIEW_FLAG_NONE);

    /// Texture component swizzle, see Diligent::TextureComponentMapping.
    TextureComponentMapping Swizzle;

    // 
    // NB: when adding new members, don't forget to update std::hash<Diligent::TextureViewDesc>
    //

#if DILIGENT_CPP_INTERFACE && !defined(DILIGENT_SHARP_GEN)

    constexpr TextureViewDesc() noexcept {}

    constexpr TextureViewDesc(const Char*        _Name,
                              TEXTURE_VIEW_TYPE  _ViewType,
                              RESOURCE_DIMENSION _TextureDim,
                              TEXTURE_FORMAT     _Format                 = TextureViewDesc{}.Format,
                              Uint32             _MostDetailedMip        = TextureViewDesc{}.MostDetailedMip,
                              Uint32             _NumMipLevels           = TextureViewDesc{}.NumMipLevels,
                              Uint32             _FirstArrayOrDepthSlice = TextureViewDesc{}.FirstArraySlice,
                              Uint32             _NumArrayOrDepthSlices  = TextureViewDesc{}.NumArraySlices,
                              UAV_ACCESS_FLAG    _AccessFlags            = TextureViewDesc{}.AccessFlags,
                              TEXTURE_VIEW_FLAGS _Flags                  = TextureViewDesc{}.Flags) noexcept :
        DeviceObjectAttribs{_Name                  },
        ViewType           {_ViewType              },
        TextureDim         {_TextureDim            },
        Format             {_Format                },
        MostDetailedMip    {_MostDetailedMip       },
        NumMipLevels       {_NumMipLevels          },
        FirstArraySlice    {_FirstArrayOrDepthSlice},
        NumArraySlices     {_NumArrayOrDepthSlices },
        AccessFlags        {_AccessFlags           },
        Flags              {_Flags                 }
    {}

    constexpr Uint32 FirstArrayOrDepthSlice() const { return FirstArraySlice; }
    constexpr Uint32 NumArrayOrDepthSlices()  const { return NumArraySlices; }

    /// Tests if two texture view descriptions are equal.

    /// \param [in] RHS - reference to the structure to compare with.
    ///
    /// \return     true if all members of the two structures *except for the Name* are equal,
    ///             and false otherwise.
    ///
    /// \note   The operator ignores the Name field as it is used for debug purposes and
    ///         doesn't affect the texture view properties.
    constexpr bool operator==(const TextureViewDesc& RHS) const
    {
        // Ignore Name. This is consistent with the hasher (HashCombiner<HasherType, TextureViewDesc>).
        return //strcmp(Name, RHS.Name) == 0            &&
            ViewType                 == RHS.ViewType                 &&
            TextureDim               == RHS.TextureDim               &&
            Format                   == RHS.Format                   &&
            MostDetailedMip          == RHS.MostDetailedMip          &&
            NumMipLevels             == RHS.NumMipLevels             &&
            FirstArrayOrDepthSlice() == RHS.FirstArrayOrDepthSlice() &&
            NumArrayOrDepthSlices()  == RHS.NumArrayOrDepthSlices()  &&
            AccessFlags              == RHS.AccessFlags              &&
            Flags                    == RHS.Flags                    &&
            Swizzle                  == RHS.Swizzle;
    }
    constexpr bool operator!=(const TextureViewDesc& RHS) const
    {
        return !(*this == RHS);
    }
#else

#endif
};
typedef struct TextureViewDesc TextureViewDesc;

// clang-format on

#define DILIGENT_INTERFACE_NAME ITextureView
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define ITextureViewInclusiveMethods \
    IDeviceObjectInclusiveMethods;   \
    ITextureViewMethods TextureView

/// Texture view interface

/// \remarks
/// To create a texture view, call ITexture::CreateView().
/// Texture view holds strong references to the texture. The texture
/// will not be destroyed until all views are released.
/// The texture view will also keep a strong reference to the texture sampler,
/// if any is set.
DILIGENT_BEGIN_INTERFACE(ITextureView, IDeviceObject)
{
#if DILIGENT_CPP_INTERFACE
    /// Returns the texture view description used to create the object
    virtual const TextureViewDesc& METHOD(GetDesc)() const override = 0;
#endif

    /// Sets the texture sampler to use for filtering operations
    /// when accessing a texture from shaders. Only
    /// shader resource views can be assigned a sampler.
    /// The view will keep strong reference to the sampler.
    VIRTUAL void METHOD(SetSampler)(THIS_ struct ISampler * pSampler) PURE;

    /// Returns the pointer to the sampler object set by the ITextureView::SetSampler().

    /// The method does *NOT* increment the reference counter of the returned object,
    /// so Release() must not be called.
    VIRTUAL struct ISampler* METHOD(GetSampler)(THIS) PURE;


    /// Returns the pointer to the referenced texture object.

    /// The method does *NOT* increment the reference counter of the returned object,
    /// so Release() must not be called.
    VIRTUAL struct ITexture* METHOD(GetTexture)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

#    define ITextureView_GetDesc(This) (const struct TextureViewDesc*)IDeviceObject_GetDesc(This)

// clang-format off

#    define ITextureView_SetSampler(This, ...) CALL_IFACE_METHOD(TextureView, SetSampler, This, __VA_ARGS__)
#    define ITextureView_GetSampler(This)      CALL_IFACE_METHOD(TextureView, GetSampler, This)
#    define ITextureView_GetTexture(This)      CALL_IFACE_METHOD(TextureView, GetTexture, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE
