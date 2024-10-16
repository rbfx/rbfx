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

#include <memory>
#include <unordered_map>
#include <mutex>
#include <vector>

#include "GraphicsTypesX.hpp"
#include "GLProgram.hpp"

namespace Diligent
{

class ShaderGLImpl;

/// Program cached contains linked programs for the given combination of shaders and resource layouts.
class GLProgramCache
{
public:
    GLProgramCache();
    ~GLProgramCache();

    // clang-format off
    GLProgramCache             (const GLProgramCache&)  = delete;
    GLProgramCache             (      GLProgramCache&&) = delete;
    GLProgramCache& operator = (const GLProgramCache&)  = delete;
    GLProgramCache& operator = (      GLProgramCache&&) = delete;
    // clang-format on

    using SharedGLProgramObjPtr = std::shared_ptr<GLProgram>;

    struct GetProgramAttribs
    {
        ShaderGLImpl* const*         ppShaders          = nullptr;
        Uint32                       NumShaders         = 0;
        bool                         IsSeparableProgram = false;
        PipelineResourceLayoutDesc*  pResourceLayout    = nullptr;
        IPipelineResourceSignature** ppSignatures       = nullptr;
        Uint32                       NumSignatures      = 0;
    };

    SharedGLProgramObjPtr GetProgram(const GetProgramAttribs& Attribs);

private:
    struct ProgramCacheKey
    {
    public:
        ProgramCacheKey(const GetProgramAttribs& Attribs);
        bool operator==(const ProgramCacheKey& Key) const noexcept;

        size_t GetHash() const noexcept { return Hash; }

    private:
        size_t Hash = 0;

        bool IsSeparableProgram = false;

        std::vector<UniqueIdentifier> ShaderUIDs;
        std::vector<UniqueIdentifier> SignaturUIDs;

        PipelineResourceLayoutDescX ResourceLayout;
    };

    struct ProgramCacheKeyHasher
    {
        std::size_t operator()(const ProgramCacheKey& Key) const noexcept
        {
            return Key.GetHash();
        }
    };

    std::mutex                                                                           m_CacheMtx;
    std::unordered_map<ProgramCacheKey, std::weak_ptr<GLProgram>, ProgramCacheKeyHasher> m_Cache;
};

} // namespace Diligent
