/*
 *  Copyright 2024 Diligent Graphics LLC
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

#include <string>
#include <vector>
#include <unordered_map>

namespace tint
{
class Program;
namespace inspector
{
struct ResourceBinding;
}
} // namespace tint

namespace Diligent
{

std::string ConvertSPIRVtoWGSL(const std::vector<uint32_t>& SPIRV);

struct WGSLResourceBindingInfo
{
    uint32_t Group     = 0;
    uint32_t Index     = 0;
    uint32_t ArraySize = 1;
};
using WGSLResourceMapping = std::unordered_map<std::string, WGSLResourceBindingInfo>;

std::string RamapWGSLResourceBindings(const std::string&         WGSL,
                                      const WGSLResourceMapping& ResMapping,
                                      const char*                EmulatedArrayIndexSuffix);


/// When WGSL is generated from SPIR-V, the names of resources may be mangled
///
/// Constant buffers:
///   HLSL:
///      cbuffer CB0
///      {
///          float4 g_Data0;
///      }
///   WGSL:
///      struct CB0 {
///        g_Data0 : vec4f,
///      }
///      @group(0) @binding(0) var<uniform> x_13 : CB0;
///
///
/// Structured buffers:
///   HLSL:
///      struct BufferData0
///      {
///          float4 data;
///      };
///      StructuredBuffer<BufferData0> g_Buff0;
///      StructuredBuffer<BufferData0> g_Buff1;
///   WGSL:
///      struct g_Buff0 {
///        x_data : RTArr,
///      }
///      @group(0) @binding(0) var<storage, read> g_Buff0_1 : g_Buff0;
///      @group(0) @binding(1) var<storage, read> g_Buff1   : g_Buff0;
///
/// This function returns the alternative name of the resource binding.
std::string GetWGSLResourceAlternativeName(const tint::Program& Program, const tint::inspector::ResourceBinding& Binding);

struct WGSLEmulatedResourceArrayElement
{
    std::string Name;
    int         Index = -1;

    WGSLEmulatedResourceArrayElement() noexcept {}

    explicit WGSLEmulatedResourceArrayElement(std::string _Name) :
        Name{std::move(_Name)}
    {}

    WGSLEmulatedResourceArrayElement(std::string _Name, int _Index) :
        Name{std::move(_Name)},
        Index{_Index}
    {}

    constexpr bool     IsValid() const { return Index >= 0; }
    constexpr explicit operator bool() const { return IsValid(); }

    bool operator==(const WGSLEmulatedResourceArrayElement& rhs) const
    {
        return Name == rhs.Name && Index == rhs.Index;
    }
    bool operator!=(const WGSLEmulatedResourceArrayElement& rhs) const
    {
        return !(*this == rhs);
    }
};
/// Returns WGSL emulated resource array element info, for example
/// g_Tex2D_7 -> {Name = "g_Tex2D", Index =  7}
/// g_Tex2D   -> {Name = "g_Tex2D", Index = -1}
WGSLEmulatedResourceArrayElement GetWGSLEmulatedArrayElement(const std::string& Name, const std::string& Suffix);

} // namespace Diligent
