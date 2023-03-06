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
/// Defines graphics engine utilities

#include "../../GraphicsEngine/interface/Texture.h"
#include "../../GraphicsEngine/interface/Buffer.h"
#include "../../GraphicsEngine/interface/RenderDevice.h"

#if DILIGENT_C_INTERFACE
#    define REF *
#else
#    define REF &
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

void DILIGENT_GLOBAL_FUNCTION(CreateUniformBuffer)(IRenderDevice*                  pDevice,
                                                   Uint64                          Size,
                                                   const Char*                     Name,
                                                   IBuffer**                       ppBuffer,
                                                   USAGE Usage                     DEFAULT_VALUE(USAGE_DYNAMIC),
                                                   BIND_FLAGS BindFlags            DEFAULT_VALUE(BIND_UNIFORM_BUFFER),
                                                   CPU_ACCESS_FLAGS CPUAccessFlags DEFAULT_VALUE(CPU_ACCESS_WRITE),
                                                   void* pInitialData              DEFAULT_VALUE(nullptr));

void DILIGENT_GLOBAL_FUNCTION(GenerateCheckerBoardPattern)(Uint32         Width,
                                                           Uint32         Height,
                                                           TEXTURE_FORMAT Fmt,
                                                           Uint32         HorzCells,
                                                           Uint32         VertCells,
                                                           Uint8*         pData,
                                                           Uint64         StrideInBytes);

// clang-format off

/// Coarse mip filter type
DILIGENT_TYPED_ENUM(MIP_FILTER_TYPE, Uint8)
{
    /// Default filter type: BOX_AVERAGE for UNORM/SNORM and FP formats, and
    /// MOST_FREQUENT for UINT/SINT formats.
    MIP_FILTER_TYPE_DEFAULT = 0,

    /// 2x2 box average.
    MIP_FILTER_TYPE_BOX_AVERAGE,

    /// Use the most frequent element from the 2x2 box.
    /// This filter does not introduce new values and should be used
    /// for integer textures that contain non-filterable data (e.g. indices).
    MIP_FILTER_TYPE_MOST_FREQUENT
};


/// ComputeMipLevel function attributes
struct ComputeMipLevelAttribs
{
    /// Texture format.
    TEXTURE_FORMAT Format     DEFAULT_INITIALIZER(TEX_FORMAT_UNKNOWN);

    /// Fine mip level width.
    Uint32 FineMipWidth       DEFAULT_INITIALIZER(0);

    /// Fine mip level height.
    Uint32 FineMipHeight      DEFAULT_INITIALIZER(0);

    /// Pointer to the fine mip level data
    const void* pFineMipData  DEFAULT_INITIALIZER(nullptr);

    /// Fine mip level data stride, in bytes.
    size_t FineMipStride      DEFAULT_INITIALIZER(0);

    /// Pointer to the coarse mip level data
    void* pCoarseMipData      DEFAULT_INITIALIZER(nullptr);

    /// Coarse mip level data stride, in bytes.
    size_t CoarseMipStride    DEFAULT_INITIALIZER(0);

    /// Filter type.
    MIP_FILTER_TYPE FilterType DEFAULT_INITIALIZER(MIP_FILTER_TYPE_DEFAULT);

    /// Alpha cutoff value.
    ///
    /// \remarks
    ///     When AlphaCutoff is not 0, alpha channel is remapped as follows:
    ///         A_new = max(A_old; 1/3 * A_old + 2/3 * AlphaCutoff)
    float AlphaCutoff          DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    constexpr ComputeMipLevelAttribs() noexcept {}

    constexpr ComputeMipLevelAttribs(TEXTURE_FORMAT   _Format,
                                     Uint32           _FineMipWidth,
                                     Uint32           _FineMipHeight,
                                     const void*      _pFineMipData,
                                     size_t           _FineMipStride,
                                     void*            _pCoarseMipData,
                                     size_t           _CoarseMipStride,
                                     MIP_FILTER_TYPE _FilterType  = ComputeMipLevelAttribs{}.FilterType,
                                     float            _AlphaCutoff = ComputeMipLevelAttribs{}.AlphaCutoff) noexcept :
        Format          {_Format},
        FineMipWidth    {_FineMipWidth},
        FineMipHeight   {_FineMipHeight},
        pFineMipData    {_pFineMipData},
        FineMipStride   {_FineMipStride},
        pCoarseMipData  {_pCoarseMipData},
        CoarseMipStride {_CoarseMipStride},
        FilterType      {_FilterType},
        AlphaCutoff     {_AlphaCutoff}
    {} 
#endif
};
typedef struct ComputeMipLevelAttribs ComputeMipLevelAttribs;
// clang-format on

void DILIGENT_GLOBAL_FUNCTION(ComputeMipLevel)(const ComputeMipLevelAttribs REF Attribs);

/// For a Direct3D12 render device, returns the maximum supported shader version. For any other device type, returns 0.

/// \param [in]  pDevice - a pointer to the render device object.
/// \param [out] Version - maximum shader version supported by this device.
///
/// \return     true if the maximum shader version was determined successfully, and false otherwise.
bool DILIGENT_GLOBAL_FUNCTION(GetRenderDeviceD3D12MaxShaderVersion)(IRenderDevice* pDevice, ShaderVersion REF Version);

DILIGENT_END_NAMESPACE // namespace Diligent
