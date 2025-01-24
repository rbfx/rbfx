/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include "XXH128Hasher.hpp"

#include "xxhash.h"

#include "DebugUtilities.hpp"
#include "Cast.hpp"
#include "ShaderToolsCommon.hpp"

namespace Diligent
{

XXH128State::XXH128State() :
    m_State{XXH3_createState()}
{
    VERIFY_EXPR(m_State != nullptr);
    XXH3_128bits_reset(m_State);
}

XXH128State::~XXH128State()
{
    XXH3_freeState(m_State);
}

void XXH128State::UpdateRaw(const void* pData, uint64_t Size) noexcept
{
    VERIFY_EXPR(pData != nullptr);
    VERIFY_EXPR(Size != 0);
    XXH3_128bits_update(m_State, pData, StaticCast<size_t>(Size));
}

XXH128Hash XXH128State::Digest() noexcept
{
    XXH128_hash_t Hash = XXH3_128bits_digest(m_State);
    return {Hash.low64, Hash.high64};
}

void XXH128State::Update(const ShaderCreateInfo& ShaderCI) noexcept
{
    ASSERT_SIZEOF64(ShaderCI, 152, "Did you add new members to ShaderCreateInfo? Please handle them here.");

    Update(ShaderCI.SourceLength, // Aka ByteCodeSize
           ShaderCI.EntryPoint,
           ShaderCI.Desc,
           ShaderCI.SourceLanguage,
           ShaderCI.ShaderCompiler,
           ShaderCI.HLSLVersion,
           ShaderCI.GLSLVersion,
           ShaderCI.GLESSLVersion,
           ShaderCI.MSLVersion,
           ShaderCI.CompileFlags,
           ShaderCI.LoadConstantBufferReflection);

    if (ShaderCI.Source != nullptr || ShaderCI.FilePath != nullptr)
    {
        DEV_CHECK_ERR(ShaderCI.ByteCode == nullptr, "ShaderCI.ByteCode must be null when either Source or FilePath is specified");
        ProcessShaderIncludes(ShaderCI, [this](const ShaderIncludePreprocessInfo& ProcessInfo) {
            UpdateStr(ProcessInfo.Source, ProcessInfo.SourceLength);
        });
    }
    else if (ShaderCI.ByteCode != nullptr && ShaderCI.ByteCodeSize != 0)
    {
        UpdateRaw(ShaderCI.ByteCode, ShaderCI.ByteCodeSize);
    }

    if (ShaderCI.Macros)
    {
        for (size_t i = 0; i < ShaderCI.Macros.Count; ++i)
        {
            const auto& Macro = ShaderCI.Macros[i];
            Update(Macro.Name, Macro.Definition);
        }
    }

    if (ShaderCI.GLSLExtensions != nullptr)
    {
        UpdateStr(ShaderCI.GLSLExtensions);
    }

    if (ShaderCI.WebGPUEmulatedArrayIndexSuffix != nullptr)
    {
        UpdateStr(ShaderCI.WebGPUEmulatedArrayIndexSuffix);
    }
}

} // namespace Diligent
