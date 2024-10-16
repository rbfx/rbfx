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

#include "pch.h"

#include "GLProgramCache.hpp"
#include "ShaderGLImpl.hpp"
#include "RenderDeviceGLImpl.hpp"
#include "PipelineResourceSignatureGLImpl.hpp"
#include "HashUtils.hpp"

namespace Diligent
{

GLProgramCache::GLProgramCache()
{
}

GLProgramCache::~GLProgramCache()
{
}

GLProgramCache::ProgramCacheKey::ProgramCacheKey(const GetProgramAttribs& Attribs) :
    IsSeparableProgram{Attribs.IsSeparableProgram}
{
    Hash = ComputeHash(Attribs.IsSeparableProgram, Attribs.NumShaders, Attribs.NumSignatures);

    ShaderUIDs.reserve(Attribs.NumShaders);
    for (Uint32 i = 0; i < Attribs.NumShaders; ++i)
    {
        auto ShaderUID = Attribs.ppShaders[i]->GetUniqueID();
        HashCombine(Hash, ShaderUID);
        ShaderUIDs.push_back(ShaderUID);
    }

    if (Attribs.NumSignatures != 0)
    {
        SignaturUIDs.reserve(Attribs.NumSignatures);
        for (Uint32 i = 0; i < Attribs.NumSignatures; ++i)
        {
            if (PipelineResourceSignatureGLImpl* pSignature = ClassPtrCast<PipelineResourceSignatureGLImpl>(Attribs.ppSignatures[i]))
            {
                auto SignaturUID = pSignature->GetUniqueID();
                HashCombine(Hash, SignaturUID);
                SignaturUIDs.push_back(SignaturUID);
            }
        }
    }
    else
    {
        VERIFY_EXPR(Attribs.pResourceLayout != nullptr);
        ResourceLayout = *Attribs.pResourceLayout;
        HashCombine(Hash, static_cast<const PipelineResourceLayoutDesc&>(ResourceLayout));
    }
}

bool GLProgramCache::ProgramCacheKey::operator==(const ProgramCacheKey& Key) const noexcept
{
    // clang-format off
    return (Hash               == Key.Hash               &&
            IsSeparableProgram == Key.IsSeparableProgram &&
            ShaderUIDs         == Key.ShaderUIDs         &&
            SignaturUIDs       == Key.SignaturUIDs       &&
            (!SignaturUIDs.empty() || ResourceLayout == Key.ResourceLayout));
    // clang-format on
}

GLProgramCache::SharedGLProgramObjPtr GLProgramCache::GetProgram(const GetProgramAttribs& Attribs)
{
    ProgramCacheKey Key{Attribs};

    // Try to find the program in the cache
    {
        std::lock_guard<std::mutex> Lock{m_CacheMtx};

        auto it = m_Cache.find(Key);
        if (it != m_Cache.end())
        {
            if (SharedGLProgramObjPtr Program = it->second.lock())
            {
                return Program;
            }
            else
            {
                m_Cache.erase(it);
            }
        }
    }

    // The program is not found. Create a new one.
    // Note that since the program is created outside the lock, it is possible that
    // multiple threads will create the same program. Only one program will be added to the cache
    // and the rest will be destroyed.

    // Linking the program may take a considerable amount of time.
    std::shared_ptr<GLProgram> NewProgram = std::make_shared<GLProgram>(Attribs.ppShaders, Attribs.NumShaders, Attribs.IsSeparableProgram);

    std::lock_guard<std::mutex> Lock{m_CacheMtx};

    auto it_inserted = m_Cache.emplace(Key, NewProgram);
    // Check if the proram is valid
    if (auto Program = it_inserted.first->second.lock())
    {
        return Program;
    }
    else
    {
        it_inserted.first->second = NewProgram;
        return NewProgram;
    }
}

} // namespace Diligent
