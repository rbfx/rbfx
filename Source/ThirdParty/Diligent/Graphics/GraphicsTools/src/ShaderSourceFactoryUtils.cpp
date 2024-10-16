/*
 *  Copyright 2023 Diligent Graphics LLC
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

#include "ShaderSourceFactoryUtils.h"

#include <vector>
#include <unordered_map>
#include <string>

#include "ObjectBase.hpp"
#include "HashUtils.hpp"
#include "RefCntAutoPtr.hpp"
#include "StringDataBlobImpl.hpp"
#include "MemoryFileStream.hpp"

namespace Diligent
{

class CompoundShaderSourceFactory : public ObjectBase<IShaderSourceInputStreamFactory>
{
public:
    using TBase = ObjectBase<IShaderSourceInputStreamFactory>;

    static RefCntAutoPtr<IShaderSourceInputStreamFactory> Create(const CompoundShaderSourceFactoryCreateInfo& CreateInfo)
    {
        return RefCntAutoPtr<IShaderSourceInputStreamFactory>{MakeNewRCObj<CompoundShaderSourceFactory>()(CreateInfo)};
    }

    CompoundShaderSourceFactory(IReferenceCounters*                          pRefCounters,
                                const CompoundShaderSourceFactoryCreateInfo& CI) :
        TBase{pRefCounters}
    {
        if (CI.ppFactories != nullptr)
        {
            m_pFactories.reserve(CI.NumFactories);
            for (Uint32 i = 0; i < CI.NumFactories; ++i)
            {
                m_pFactories.emplace_back(CI.ppFactories[i]);
            }
        }

        if (CI.pFileSubstitutes != nullptr)
        {
            for (Uint32 i = 0; i < CI.NumFileSubstitutes; ++i)
            {
                m_FileSubstituteMap.emplace(HashMapStringKey{CI.pFileSubstitutes[i].Name, true}, CI.pFileSubstitutes[i].Substitute);
            }
        }
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_IShaderSourceInputStreamFactory, TBase)

    virtual void DILIGENT_CALL_TYPE CreateInputStream(const Char*   Name,
                                                      IFileStream** ppStream) override final
    {
        CreateInputStream2(Name, CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_NONE, ppStream);
    }

    virtual void DILIGENT_CALL_TYPE CreateInputStream2(const Char*                             Name,
                                                       CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                       IFileStream**                           ppStream) override final
    {
        VERIFY_EXPR(ppStream != nullptr && *ppStream == nullptr);
        if (!m_FileSubstituteMap.empty())
        {
            auto it = m_FileSubstituteMap.find(Name);
            if (it != m_FileSubstituteMap.end())
                Name = it->second.c_str();
        }

        for (size_t i = 0; i < m_pFactories.size() && *ppStream == nullptr; ++i)
        {
            if (m_pFactories[i])
                m_pFactories[i]->CreateInputStream2(Name, CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_SILENT, ppStream);
        }

        if (*ppStream == nullptr && (Flags & CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_SILENT) != 0)
        {
            LOG_ERROR("Failed to create input stream for source file ", Name);
        }
    }

private:
    std::vector<RefCntAutoPtr<IShaderSourceInputStreamFactory>> m_pFactories;

    std::unordered_map<HashMapStringKey, std::string> m_FileSubstituteMap;
};

void CreateCompoundShaderSourceFactory(const CompoundShaderSourceFactoryCreateInfo& CreateInfo, IShaderSourceInputStreamFactory** ppFactory)
{
    auto pFactory = CompoundShaderSourceFactory::Create(CreateInfo);
    pFactory->QueryInterface(IID_IShaderSourceInputStreamFactory, reinterpret_cast<IObject**>(ppFactory));
}




class MemoryShaderSourceFactory final : public ObjectBase<IShaderSourceInputStreamFactory>
{
public:
    using TBase = ObjectBase<IShaderSourceInputStreamFactory>;

    static RefCntAutoPtr<IShaderSourceInputStreamFactory> Create(const MemoryShaderSourceFactoryCreateInfo& CreateInfo)
    {
        return RefCntAutoPtr<IShaderSourceInputStreamFactory>{MakeNewRCObj<MemoryShaderSourceFactory>()(CreateInfo)};
    }

    MemoryShaderSourceFactory(IReferenceCounters*                        pRefCounters,
                              const MemoryShaderSourceFactoryCreateInfo& CI) :
        TBase{pRefCounters}
    {
        if (CI.CopySources)
        {
            m_Sources.resize(CI.NumSources);
            for (Uint32 i = 0; i < CI.NumSources; ++i)
            {
                const auto& Source = CI.pSources[i];
                if (Source.Length > 0)
                {
                    m_Sources[i].assign(Source.pData, Source.Length);
                }
                else
                {
                    m_Sources[i] = Source.pData;
                }
            }
        }

        for (Uint32 i = 0; i < CI.NumSources; ++i)
        {
            const auto& Source = CI.pSources[i];
            DEV_CHECK_ERR(Source.Name != nullptr && Source.Name[0] != '\0', "Source name must not be null or empty");
            DEV_CHECK_ERR(Source.pData != nullptr, "Source data must not be null");
            m_NameToSourceMap.emplace(HashMapStringKey{Source.Name, true}, CI.CopySources ? m_Sources[i].c_str() : Source.pData);
        }
    }

    virtual void DILIGENT_CALL_TYPE CreateInputStream(const Char* Name, IFileStream** ppStream) override final
    {
        CreateInputStream2(Name, CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_NONE, ppStream);
    }

    virtual void DILIGENT_CALL_TYPE CreateInputStream2(const Char*                             Name,
                                                       CREATE_SHADER_SOURCE_INPUT_STREAM_FLAGS Flags,
                                                       IFileStream**                           ppStream) override final
    {
        auto SourceIt = m_NameToSourceMap.find(Name);
        if (SourceIt != m_NameToSourceMap.end())
        {
            RefCntAutoPtr<StringDataBlobImpl> pDataBlob{MakeNewRCObj<StringDataBlobImpl>()(SourceIt->second)};
            RefCntAutoPtr<MemoryFileStream>   pMemStream{MakeNewRCObj<MemoryFileStream>()(pDataBlob)};

            pMemStream->QueryInterface(IID_FileStream, reinterpret_cast<IObject**>(ppStream));
        }
        else
        {
            *ppStream = nullptr;
            if ((Flags & CREATE_SHADER_SOURCE_INPUT_STREAM_FLAG_SILENT) == 0)
            {
                LOG_ERROR("Failed to create input stream for source file ", Name);
            }
        }
    }

    IMPLEMENT_QUERY_INTERFACE_IN_PLACE(IID_IShaderSourceInputStreamFactory, TBase)

private:
    std::vector<std::string> m_Sources;

    std::unordered_map<HashMapStringKey, const Char*> m_NameToSourceMap;
};

void CreateMemoryShaderSourceFactory(const MemoryShaderSourceFactoryCreateInfo& CreateInfo, IShaderSourceInputStreamFactory** ppFactory)
{
    auto pFactory = MemoryShaderSourceFactory::Create(CreateInfo);
    pFactory->QueryInterface(IID_IShaderSourceInputStreamFactory, reinterpret_cast<IObject**>(ppFactory));
}


} // namespace Diligent

extern "C"
{
    void Diligent_CreateCompoundShaderSourceFactory(const Diligent::CompoundShaderSourceFactoryCreateInfo& CreateInfo,
                                                    Diligent::IShaderSourceInputStreamFactory**            ppFactory)
    {
        Diligent::CreateCompoundShaderSourceFactory(CreateInfo, ppFactory);
    }

    void Diligent_CreateMemoryShaderSourceFactory(const Diligent::MemoryShaderSourceFactoryCreateInfo& CreateInfo,
                                                  Diligent::IShaderSourceInputStreamFactory**          ppFactory)
    {
        Diligent::CreateMemoryShaderSourceFactory(CreateInfo, ppFactory);
    }
}
