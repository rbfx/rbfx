/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

/// \file
/// Declaration of DynamicTextureAtlas interface and related data structures

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Texture.h"
#include "../../../Common/interface/BasicMath.hpp"

namespace Diligent
{

struct IDynamicTextureAtlas;

// {F7ACDCFB-074F-4E31-94C5-DD4C988E4532}
static const INTERFACE_ID IID_TextureAtlasSuballocation =
    {0xf7acdcfb, 0x74f, 0x4e31, {0x94, 0xc5, 0xdd, 0x4c, 0x98, 0x8e, 0x45, 0x32}};


// {00E1D6FA-47B4-4062-B96C-D3E191A02351}
static const INTERFACE_ID IID_DynamicTextureAtlas =
    {0xe1d6fa, 0x47b4, 0x4062, {0xb9, 0x6c, 0xd3, 0xe1, 0x91, 0xa0, 0x23, 0x51}};


/// Dynamic texture atlas suballocation.
struct ITextureAtlasSuballocation : public IObject
{
    /// Returns the suballocation origin.
    virtual uint2 GetOrigin() const = 0;

    /// Returns the suballocation slice.
    virtual Uint32 GetSlice() const = 0;

    /// Returns the suballocation size.
    virtual uint2 GetSize() const = 0;

    /// Returns the texture coordinate scale (xy) and bias (zw).
    virtual float4 GetUVScaleBias() const = 0;

    /// Returns the suballocation alignment.
    virtual Uint32 GetAlignment() const = 0;

    /// Returns the pointer to the parent texture atlas.
    virtual IDynamicTextureAtlas* GetAtlas() = 0;

    /// Stores a pointer to the user-provided data object, which
    /// may later be retrieved through GetUserData().
    ///
    /// \param [in] pUserData - Pointer to the user data object to store.
    ///
    /// \note   The method is not thread-safe and an application
    ///         must externally synchronize the access.
    virtual void SetUserData(IObject* pUserData) = 0;

    /// Returns the pointer to the user data object previously
    /// set with SetUserData() method.
    ///
    /// \return     Pointer to the user data object
    virtual IObject* GetUserData() const = 0;
};


/// Dynamic texture atlas usage stats.
struct DynamicTextureAtlasUsageStats
{
    /// The total committed memory size of the atlas, in bytes.
    Uint64 CommittedSize = 0;

    /// The total number of allocations in the atlas.
    Uint32 AllocationCount = 0;

    /// The total area of the texture atlas, e.g.
    /// the total number of texels in all slices.
    Uint64 TotalArea = 0;

    /// The total allocated area, e.g. the total
    /// number of texels in all allocations.
    Uint64 AllocatedArea = 0;

    /// The total used area, e.g. the total number
    /// of texels actually used by all allocations.

    /// Used area is always equal to or larger than the
    /// allocated area due to alignment requirements.
    Uint64 UsedArea = 0;
};


/// Dynamic texture atlas.
struct IDynamicTextureAtlas : public IObject
{
    /// Returns the pointer to the internal texture object.

    /// \param[in]  pDevice  - Pointer to the render device that will be used to
    ///                        create a new internal texture array, if necessary.
    /// \param[in]  pContext - Pointer to the device context that will be used to
    ///                        copy existing contents to the new texture array, if
    ///                        necessary.
    ///
    /// \remarks    If the internal texture needs to be resized, pDevice and pContext will
    ///             be used to create a new texture and copy existing contents to it.
    ///
    ///             The method is not thread safe. An application must externally synchronize
    ///             the access.
    virtual ITexture* GetTexture(IRenderDevice* pDevice, IDeviceContext* pContext) = 0;


    /// Performs suballocation from the atlas.

    /// \param[in]  Width           - Suballocation width.
    /// \param[in]  Height          - Suballocation height.
    /// \param[out] ppSuballocation - Memory location where pointer to the new suballocation will be
    ///                               stored.
    ///
    /// \remarks    The method is thread-safe and can be called from multiple threads simultaneously.
    ///
    ///             Internal texture array may need to be extended after the allocation happened.
    ///             An application may call GetTexture() to ensure that the texture is resized and old
    ///             contents is copied.
    virtual void Allocate(Uint32                       Width,
                          Uint32                       Height,
                          ITextureAtlasSuballocation** ppSuballocation) = 0;


    /// Returns the texture atlas description
    virtual const TextureDesc& GetAtlasDesc() const = 0;

    /// Returns internal texture array version. The version is incremented every time
    /// the array is expanded.
    virtual Uint32 GetVersion() const = 0;

    /// Returns the usage stats, see Diligent::DynamicTextureAtlasUsageStats.
    virtual void GetUsageStats(DynamicTextureAtlasUsageStats& Stats) const = 0;


    /// Computes the allocation alignment for the region of a given size.

    /// \param [in] Width  - Region width.
    /// \param [in] Height - Region height.
    /// \return            - Allocation alignment.
    virtual Uint32 GetAllocationAlignment(Uint32 Width, Uint32 Height) const = 0;
};


/// Dynamic texture atlas create information.
struct DynamicTextureAtlasCreateInfo
{
    /// Texture description

    /// Texture type must be 2D or 2D array. When the type is
    /// texture 2D, resizes will be disabled.
    TextureDesc Desc;


    /// Minimum region placement alignment.

    /// The minimum alignment must be zero or a power of two.
    /// When alignment is zero, the atlas may allocate the region in any suitable location.
    ///
    /// When alignment is non-zero, the region placement is aligned as follows:
    /// - If min(Width, Height) <= MinAlignment, the region placement is aligned by MinAlignment
    /// - If min(Width, Height) > MinAlignment, the alignment is doubled until it satisfies
    ///   the requirement above.
    ///
    /// Examples (when MinAlignment equals 64):
    /// - A 16x32 region will be aligned by 64 (it may be placed at e.g. (64, 128))
    /// - A 48x96 region will be aligned by 64 (it may be placed at e.g. (64, 0))
    /// - A 96x192 region will be aligned by 128 (it may be placed at e.g. (128, 256))
    /// - A 2048x1024 region will be aligned by 1024 (it may be placed at e.g. (1024, 0))
    ///
    /// Note that if minimum alignment is zero, the region placement will not be aligned at all,
    /// which may result in biasing issues in coarser mip levels. For example, if 128x128
    /// region is placed at (4, 12) coordinates in the atlas (i.e. R = [4, 132] x [12, 140]), all
    /// mip levels of R starting with level 3 will not be aligned with the mip 0.
    ///
    /// The atlas uses the minimum dimension of the region to align it. This is done to reduce
    /// the space waste. A 256 x 1024 region will be aligned by 256, so all mip levels up to 8
    /// will be properly aligned with mip 0. The last two mip levels however, may not be properly
    /// aligned.
    ///
    /// Compressed texture considerations.
    /// Every mip level of a compressed texture may only be updated at block granularity (typically, 4x4).
    /// Be aware that coarse mip levels may not be 4-aligned (even though their placement is properly aligned with mip 0).
    /// Consider a [256, 512] x [512, 768] region. Its mip levels 7, 8 will be [2, 4] x [4, 6] and [1, 2] x [2, 3].
    /// These mip-levels are not block-aligned. Moreover, they are smaller than the block.
    Uint32 MinAlignment = 64;

    /// The number of extra slices.

    /// When non-zero, the array will be expanded by the specified number of slices every time
    /// there is insufficient space. If zero, the array size will be doubled when
    /// more space is needed.
    Uint32 ExtraSliceCount = 0;


    /// Maximum number of slices in texture array.
    Uint32 MaxSliceCount = 2048;

    /// Silence allocation errors.
    bool Silent = false;
};


/// Computes the texture atlas suballocation alignment for the region of a given size.

/// \param [in] Width        - Region width.
/// \param [in] Height       - Region height.
/// \param [in] MinAlignment - Minimum required alignment, see DynamicTextureAtlasCreateInfo::MinAlignment.
/// \return                  - Allocation alignment.
Uint32 ComputeTextureAtlasSuballocationAlignment(Uint32 Width, Uint32 Height, Uint32 MinAlignment);


/// Creates a new dynamic texture atlas.

/// \param[in] pDevice    - Pointer to the render device that will be used to create internal
///                         texture array. If this parameter is null, the texture will be created
///                         when GetTexture() is called.
/// \param[in] CreateInfo - Atlas create info, see Diligent::DynamicTextureAtlasCreateInfo.
/// \param[in] ppAtlas    - Memory location where pointer to the texture atlas object will be written.
void CreateDynamicTextureAtlas(IRenderDevice*                       pDevice,
                               const DynamicTextureAtlasCreateInfo& CreateInfo,
                               IDynamicTextureAtlas**               ppAtlas);

} // namespace Diligent
