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

/// \file
/// Defines c++ graphics engine utilities

#include "ShaderSourceFactoryUtils.h"

#include <vector>
#include <unordered_set>
#include <string>

#include "../../../Common/interface/RefCntAutoPtr.hpp"

namespace Diligent
{

/// C++ wrapper over MemoryShaderSourceFactoryCreateInfo.
struct MemoryShaderSourceFactoryCreateInfoX
{
    MemoryShaderSourceFactoryCreateInfoX() noexcept
    {}

    MemoryShaderSourceFactoryCreateInfoX(const MemoryShaderSourceFactoryCreateInfo& _Desc) :
        Desc{_Desc}
    {
        if (Desc.NumSources != 0)
            Sources.assign(Desc.pSources, Desc.pSources + Desc.NumSources);

        SyncDesc(true);
    }

    MemoryShaderSourceFactoryCreateInfoX(const std::initializer_list<MemoryShaderSourceFileInfo>& _Sources, bool CopySources) :
        Sources{_Sources}
    {
        Desc.CopySources = CopySources;
        SyncDesc(true);
    }

    MemoryShaderSourceFactoryCreateInfoX(const MemoryShaderSourceFactoryCreateInfoX& _DescX) :
        MemoryShaderSourceFactoryCreateInfoX{static_cast<const MemoryShaderSourceFactoryCreateInfo&>(_DescX)}
    {}

    MemoryShaderSourceFactoryCreateInfoX& operator=(const MemoryShaderSourceFactoryCreateInfoX& _DescX)
    {
        MemoryShaderSourceFactoryCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    MemoryShaderSourceFactoryCreateInfoX(MemoryShaderSourceFactoryCreateInfoX&&) noexcept = default;
    MemoryShaderSourceFactoryCreateInfoX& operator=(MemoryShaderSourceFactoryCreateInfoX&&) noexcept = default;

    MemoryShaderSourceFactoryCreateInfoX& Add(const MemoryShaderSourceFileInfo& Elem)
    {
        Sources.push_back(Elem);
        auto& Name = Sources.back().Name;
        Name       = StringPool.emplace(Name).first->c_str();
        SyncDesc();
        return *this;
    }

    template <typename... ArgsType>
    MemoryShaderSourceFactoryCreateInfoX& Add(ArgsType&&... args)
    {
        const MemoryShaderSourceFileInfo Elem{std::forward<ArgsType>(args)...};
        return Add(Elem);
    }

    void Clear()
    {
        MemoryShaderSourceFactoryCreateInfoX EmptyDesc;
        std::swap(*this, EmptyDesc);
    }

    const MemoryShaderSourceFactoryCreateInfo& Get() const noexcept
    {
        return Desc;
    }

    Uint32 GetNumSources() const noexcept
    {
        return Desc.NumSources;
    }

    operator const MemoryShaderSourceFactoryCreateInfo&() const noexcept
    {
        return Desc;
    }

    const MemoryShaderSourceFileInfo& operator[](size_t Index) const noexcept
    {
        return Sources[Index];
    }

    MemoryShaderSourceFileInfo& operator[](size_t Index) noexcept
    {
        return Sources[Index];
    }

private:
    void SyncDesc(bool CopyStrings = false)
    {
        Desc.NumSources = static_cast<Uint32>(Sources.size());
        Desc.pSources   = Desc.NumSources > 0 ? Sources.data() : nullptr;

        if (CopyStrings)
        {
            for (auto& Source : Sources)
                Source.Name = StringPool.emplace(Source.Name).first->c_str();
        }
    }
    MemoryShaderSourceFactoryCreateInfo     Desc;
    std::vector<MemoryShaderSourceFileInfo> Sources;
    std::unordered_set<std::string>         StringPool;
};

inline RefCntAutoPtr<IShaderSourceInputStreamFactory> CreateMemoryShaderSourceFactory(const MemoryShaderSourceFactoryCreateInfo& CI)
{
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pFactory;
    CreateMemoryShaderSourceFactory(CI, &pFactory);
    return pFactory;
}

inline RefCntAutoPtr<IShaderSourceInputStreamFactory> CreateMemoryShaderSourceFactory(const std::initializer_list<MemoryShaderSourceFileInfo>& Sources, bool CopySources = false)
{
    MemoryShaderSourceFactoryCreateInfoX CI{Sources, CopySources};
    return CreateMemoryShaderSourceFactory(CI);
}


/// C++ wrapper over PipelineResourceSignatureDesc.
struct CompoundShaderSourceFactoryCreateInfoX : CompoundShaderSourceFactoryCreateInfo
{
    CompoundShaderSourceFactoryCreateInfoX() noexcept
    {}

    explicit CompoundShaderSourceFactoryCreateInfoX(const CompoundShaderSourceFactoryCreateInfo& _Desc) :
        CompoundShaderSourceFactoryCreateInfo{_Desc}
    {
        if (NumFactories != 0)
            Factories.assign(ppFactories, ppFactories + NumFactories);

        if (NumFileSubstitutes != 0)
            FileSubstitutes.assign(pFileSubstitutes, pFileSubstitutes + NumFileSubstitutes);

        SyncDesc(true);
    }

    explicit CompoundShaderSourceFactoryCreateInfoX(const std::initializer_list<IShaderSourceInputStreamFactory*>& _Factories,
                                                    const std::initializer_list<ShaderSourceFileSubstitueInfo>&    _FileSubstitutes = {}) :
        Factories{_Factories},
        FileSubstitutes{_FileSubstitutes}
    {
        SyncDesc(true);
    }

    CompoundShaderSourceFactoryCreateInfoX(const CompoundShaderSourceFactoryCreateInfoX& _DescX) :
        CompoundShaderSourceFactoryCreateInfoX{static_cast<const CompoundShaderSourceFactoryCreateInfo&>(_DescX)}
    {}

    CompoundShaderSourceFactoryCreateInfoX& operator=(const CompoundShaderSourceFactoryCreateInfoX& _DescX)
    {
        CompoundShaderSourceFactoryCreateInfoX Copy{_DescX};
        std::swap(*this, Copy);
        return *this;
    }

    CompoundShaderSourceFactoryCreateInfoX(CompoundShaderSourceFactoryCreateInfoX&&) noexcept = default;
    CompoundShaderSourceFactoryCreateInfoX& operator=(CompoundShaderSourceFactoryCreateInfoX&&) noexcept = default;

    CompoundShaderSourceFactoryCreateInfoX& AddFactory(IShaderSourceInputStreamFactory* pFactory)
    {
        Factories.push_back(pFactory);
        return SyncDesc();
    }

    CompoundShaderSourceFactoryCreateInfoX& AddFileSusbtitute(const ShaderSourceFileSubstitueInfo& Substitute)
    {
        FileSubstitutes.push_back(Substitute);
        FileSubstitutes.back().Name       = StringPool.emplace(Substitute.Name).first->c_str();
        FileSubstitutes.back().Substitute = StringPool.emplace(Substitute.Substitute).first->c_str();
        return SyncDesc();
    }

    template <typename... ArgsType>
    CompoundShaderSourceFactoryCreateInfoX& AddFileSusbtitute(ArgsType&&... args)
    {
        const ShaderSourceFileSubstitueInfo Sam{std::forward<ArgsType>(args)...};
        return AddFileSusbtitute(Sam);
    }

    CompoundShaderSourceFactoryCreateInfoX& ClearFactories()
    {
        Factories.clear();
        return SyncDesc();
    }

    CompoundShaderSourceFactoryCreateInfoX& ClearFileSubstitutes()
    {
        FileSubstitutes.clear();
        return SyncDesc();
    }

    CompoundShaderSourceFactoryCreateInfoX& Clear()
    {
        CompoundShaderSourceFactoryCreateInfoX CleanDesc;
        std::swap(*this, CleanDesc);
        return *this;
    }

private:
    CompoundShaderSourceFactoryCreateInfoX& SyncDesc(bool UpdateStrings = false)
    {
        NumFactories = static_cast<Uint32>(Factories.size());
        ppFactories  = NumFactories > 0 ? Factories.data() : nullptr;

        NumFileSubstitutes = static_cast<Uint32>(FileSubstitutes.size());
        pFileSubstitutes   = NumFileSubstitutes > 0 ? FileSubstitutes.data() : nullptr;

        if (UpdateStrings)
        {
            for (auto& FileSubs : FileSubstitutes)
            {
                FileSubs.Name       = StringPool.emplace(FileSubs.Name).first->c_str();
                FileSubs.Substitute = StringPool.emplace(FileSubs.Substitute).first->c_str();
            }
        }

        return *this;
    }

    std::vector<IShaderSourceInputStreamFactory*> Factories;
    std::vector<ShaderSourceFileSubstitueInfo>    FileSubstitutes;
    std::unordered_set<std::string>               StringPool;
};

inline RefCntAutoPtr<IShaderSourceInputStreamFactory> CreateCompoundShaderSourceFactory(const CompoundShaderSourceFactoryCreateInfo& CI)
{
    RefCntAutoPtr<IShaderSourceInputStreamFactory> pFactory;
    CreateCompoundShaderSourceFactory(CI, &pFactory);
    return pFactory;
}

inline RefCntAutoPtr<IShaderSourceInputStreamFactory> CreateCompoundShaderSourceFactory(const std::initializer_list<IShaderSourceInputStreamFactory*>& Factories,
                                                                                        const std::initializer_list<ShaderSourceFileSubstitueInfo>&    FileSubstitutes = {})
{
    CompoundShaderSourceFactoryCreateInfoX CI{Factories, FileSubstitutes};
    return CreateCompoundShaderSourceFactory(CI);
}

} // namespace Diligent
