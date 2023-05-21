/*
 *  Copyright 2019-2022 Diligent Graphics LLC
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

#include <cstring>

#include "../../../Primitives/interface/BasicTypes.h"
#include "../../../Graphics/GraphicsEngine/interface/Shader.h"
#include "../../../Common/interface/StringTools.hpp"
#include "../../../Common/interface/HashUtils.hpp"

struct XXH3_state_s;

namespace Diligent
{

struct XXH128Hash
{
    Uint64 LowPart  = {};
    Uint64 HighPart = {};

    constexpr bool operator==(const XXH128Hash& RHS) const noexcept
    {
        return LowPart == RHS.LowPart && HighPart == RHS.HighPart;
    }
};

struct XXH128State final
{
    XXH128State();

    ~XXH128State();

    XXH128State(const XXH128State& RHS) = delete;
    XXH128State& operator=(const XXH128State& RHS) = delete;

    XXH128State(XXH128State&& RHS) noexcept :
        m_State{RHS.m_State}
    {
        RHS.m_State = nullptr;
    }

    XXH128State& operator=(XXH128State&& RHS) noexcept
    {
        this->m_State = RHS.m_State;
        RHS.m_State   = nullptr;

        return *this;
    }

    template <typename T>
    typename std::enable_if<std::is_fundamental<T>::value || std::is_enum<T>::value>::type Update(const T& Val) noexcept
    {
        UpdateRaw(&Val, sizeof(Val));
    }

    template <typename T>
    typename std::enable_if<(std::is_same<typename std::remove_cv<T>::type, char>::value ||
                             std::is_same<typename std::remove_cv<T>::type, wchar_t>::value),
                            void>::type
    Update(T* Str) noexcept
    {
        UpdateStr(Str);
    }

    template <typename CharType>
    void Update(const std::basic_string<CharType>& str) noexcept
    {
        UpdateStr(str.c_str(), str.length());
    }

    template <typename FirstArgType, typename... RestArgsType>
    void Update(const FirstArgType& FirstArg, const RestArgsType&... RestArgs) noexcept
    {
        Update(FirstArg);
        Update(RestArgs...);
    }

    void UpdateRaw(const void* pData, uint64_t Size) noexcept;

    template <typename T>
    typename std::enable_if<(std::is_same<typename std::remove_cv<T>::type, char>::value ||
                             std::is_same<typename std::remove_cv<T>::type, wchar_t>::value),
                            void>::type
    UpdateStr(T* pStr, size_t Len = 0) noexcept
    {
        if (pStr == nullptr)
            return;
        if (Len == 0)
            Len = StrLen(pStr);
        UpdateRaw(pStr, Len);
    }

    template <typename... ArgsType>
    void operator()(const ArgsType&... Args) noexcept
    {
        Update(Args...);
    }

    void Update(const ShaderCreateInfo& ShaderCI) noexcept;

    template <typename T>
    typename std::enable_if<(std::is_same<typename std::remove_cv<T>::type, SamplerDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, StencilOpDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, DepthStencilStateDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, RasterizerStateDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, BlendStateDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, TextureViewDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, SampleDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, ShaderResourceVariableDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, ImmutableSamplerDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, PipelineResourceDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, PipelineResourceLayoutDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, RenderPassAttachmentDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, AttachmentReference>::value ||
                             std::is_same<typename std::remove_cv<T>::type, ShadingRateAttachment>::value ||
                             std::is_same<typename std::remove_cv<T>::type, SubpassDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, SubpassDependencyDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, RenderPassDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, LayoutElement>::value ||
                             std::is_same<typename std::remove_cv<T>::type, InputLayoutDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, GraphicsPipelineDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, RayTracingPipelineDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, PipelineStateDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, PipelineResourceSignatureDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, Version>::value ||
                             std::is_same<typename std::remove_cv<T>::type, ShaderDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, TilePipelineDesc>::value ||
                             std::is_same<typename std::remove_cv<T>::type, PipelineStateCreateInfo>::value ||
                             std::is_same<typename std::remove_cv<T>::type, GraphicsPipelineStateCreateInfo>::value ||
                             std::is_same<typename std::remove_cv<T>::type, ComputePipelineStateCreateInfo>::value ||
                             std::is_same<typename std::remove_cv<T>::type, RayTracingPipelineStateCreateInfo>::value ||
                             std::is_same<typename std::remove_cv<T>::type, TilePipelineStateCreateInfo>::value ||
                             std::is_same<typename std::remove_cv<T>::type, VertexPoolElementDesc>::value),
                            void>::type
    Update(const T& Val) noexcept
    {
        HashCombiner<XXH128State, T> Combiner{*this};
        Combiner(Val);
    }


    XXH128Hash Digest() noexcept;

private:
    XXH3_state_s* m_State = nullptr;
};

} // namespace Diligent

namespace std
{

template <>
struct hash<Diligent::XXH128Hash>
{
    size_t operator()(const Diligent::XXH128Hash& Hash) const
    {
        auto h = Hash.LowPart ^ Hash.HighPart;
#if defined(DILIGENT_PLATFORM_64)
        return static_cast<size_t>(h);
#elif defined(DILIGENT_PLATFORM_32)
        return static_cast<size_t>((h & ~uint32_t{0u}) ^ (h >> uint64_t{32u}));
#endif
    }
};

} // namespace std
