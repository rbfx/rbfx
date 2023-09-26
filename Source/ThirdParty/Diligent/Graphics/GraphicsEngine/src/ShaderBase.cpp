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

#include "ShaderBase.hpp"
#include "FixedLinearAllocator.hpp"

namespace Diligent
{

ShaderCreateInfoWrapper::ShaderCreateInfoWrapper(const ShaderCreateInfo& ShaderCI,
                                                 IMemoryAllocator&       RawAllocator) noexcept(false) :
    m_CreateInfo{ShaderCI},
    m_SourceFactory{ShaderCI.pShaderSourceStreamFactory}
{
    m_CreateInfo.ppConversionStream = nullptr;
    m_CreateInfo.ppCompilerOutput   = nullptr;

    FixedLinearAllocator Allocator{RawAllocator};

    Allocator.AddSpaceForString(ShaderCI.EntryPoint);
    Allocator.AddSpaceForString(ShaderCI.Desc.Name);
    Allocator.AddSpaceForString(ShaderCI.Desc.CombinedSamplerSuffix);

    if (ShaderCI.ByteCode && ShaderCI.ByteCodeSize > 0)
    {
        Allocator.AddSpace(ShaderCI.ByteCodeSize, alignof(Uint32));
    }
    else if (ShaderCI.Source != nullptr)
    {
        Allocator.AddSpaceForString(ShaderCI.Source, ShaderCI.SourceLength);
    }
    else if (ShaderCI.FilePath != nullptr && ShaderCI.pShaderSourceStreamFactory != nullptr)
    {
        Allocator.AddSpaceForString(ShaderCI.FilePath);
    }
    else
    {
        LOG_ERROR_AND_THROW("Shader create info must contain Source, Bytecode or FilePath with pShaderSourceStreamFactory");
    }

    size_t MacroCount = 0;
    if (ShaderCI.Macros)
    {
        for (auto* Macro = ShaderCI.Macros; Macro->Name != nullptr && Macro->Definition != nullptr; ++Macro, ++MacroCount)
        {}
        Allocator.AddSpace<ShaderMacro>(MacroCount + 1);

        for (size_t i = 0; i < MacroCount; ++i)
        {
            Allocator.AddSpaceForString(ShaderCI.Macros[i].Name);
            Allocator.AddSpaceForString(ShaderCI.Macros[i].Definition);
        }
    }

    Allocator.Reserve();

    m_pRawMemory = decltype(m_pRawMemory){Allocator.ReleaseOwnership(), STDDeleterRawMem<void>{RawAllocator}};

    m_CreateInfo.EntryPoint                 = Allocator.CopyString(ShaderCI.EntryPoint);
    m_CreateInfo.Desc.Name                  = Allocator.CopyString(ShaderCI.Desc.Name);
    m_CreateInfo.Desc.CombinedSamplerSuffix = Allocator.CopyString(ShaderCI.Desc.CombinedSamplerSuffix);

    if (m_CreateInfo.Desc.Name == nullptr)
        m_CreateInfo.Desc.Name = "";

    if (ShaderCI.ByteCode && ShaderCI.ByteCodeSize > 0)
    {
        m_CreateInfo.ByteCode = Allocator.Copy(ShaderCI.ByteCode, ShaderCI.ByteCodeSize, alignof(Uint32));
    }
    else if (ShaderCI.Source != nullptr)
    {
        m_CreateInfo.Source       = Allocator.CopyString(ShaderCI.Source, ShaderCI.SourceLength);
        m_CreateInfo.SourceLength = ShaderCI.SourceLength;
    }
    else
    {
        VERIFY_EXPR(ShaderCI.FilePath != nullptr && ShaderCI.pShaderSourceStreamFactory != nullptr);
        m_CreateInfo.FilePath = Allocator.CopyString(ShaderCI.FilePath);
    }

    if (MacroCount > 0)
    {
        auto* pMacros       = Allocator.ConstructArray<ShaderMacro>(MacroCount + 1);
        m_CreateInfo.Macros = pMacros;
        for (auto* Macro = ShaderCI.Macros; Macro->Name != nullptr && Macro->Definition != nullptr; ++Macro, ++pMacros)
        {
            pMacros->Name       = Allocator.CopyString(Macro->Name);
            pMacros->Definition = Allocator.CopyString(Macro->Definition);
        }
        pMacros->Name       = nullptr;
        pMacros->Definition = nullptr;
    }
}

} // namespace Diligent
