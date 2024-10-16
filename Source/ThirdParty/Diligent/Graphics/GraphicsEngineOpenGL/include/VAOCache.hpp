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

#include <cstring>
#include <vector>
#include <unordered_map>

#include "GraphicsTypes.h"
#include "Buffer.h"
#include "InputLayout.h"
#include "SpinLock.hpp"
#include "HashUtils.hpp"
#include "DeviceContextBase.hpp"

namespace Diligent
{

class PipelineStateGLImpl;
class BufferGLImpl;

class VAOCache
{
public:
    VAOCache();
    ~VAOCache();

    // clang-format off
    VAOCache             (const VAOCache&)  = delete;
    VAOCache             (      VAOCache&&) = delete;
    VAOCache& operator = (const VAOCache&)  = delete;
    VAOCache& operator = (      VAOCache&&) = delete;
    // clang-format on

    struct VAOAttribs
    {
        const PipelineStateGLImpl&            PSO;
        BufferGLImpl* const                   pIndexBuffer;
        VertexStreamInfo<BufferGLImpl>* const VertexStreams;
        const Uint32                          NumVertexStreams;
    };
    const GLObjectWrappers::GLVertexArrayObj& GetVAO(const VAOAttribs&     Attribs,
                                                     class GLContextState& GLContextState);
    const GLObjectWrappers::GLVertexArrayObj& GetEmptyVAO();

    void OnDestroyBuffer(const BufferGLImpl& Buffer);
    void OnDestroyPSO(const PipelineStateGLImpl& PSO);

private:
    // This structure is used as the key to find VAO
    struct VAOHashKey
    {
        VAOHashKey(const VAOAttribs& Attribs);

        // Note that using pointers is unsafe as they may (and will) be reused:
        // pBuffer->Release();
        // pDevice->CreateBuffer(&pBuffer); // Returns same pointer

        // VAO encapsulates both input layout and all bound buffers.
        // PSO uniquely defines the layout (attrib pointers, divisors, etc.),
        // so we do not need to add individual layout elements to the key.
        // The key needs to contain all bound buffers.
        UniqueIdentifier PsoUId         = 0;
        UniqueIdentifier IndexBufferUId = 0;

        Uint32 UsedSlotsMask = 0;
        static_assert(MAX_BUFFER_SLOTS <= sizeof(UsedSlotsMask) * 8, "Use more bits for UsedSlotsMask");

        struct StreamAttribs
        {
            UniqueIdentifier BufferUId;
            Uint64           Offset;
            // Note that buffer stride is defined by the PSO, so no need to keep it here as
            // it is already handled by the PsoUId

            bool operator!=(const StreamAttribs& rhs) const
            {
                return BufferUId != rhs.BufferUId || Offset != rhs.Offset;
            }
        } Streams[MAX_BUFFER_SLOTS]; // Do not zero-out

        size_t Hash = 0;

        bool operator==(const VAOHashKey& Key) const noexcept;

        struct Hasher
        {
            std::size_t operator()(const VAOHashKey& Key) const noexcept
            {
                return Key.Hash;
            }
        };
    };

    // Clears stale entries from m_PSOToKey and m_BuffToKey when a VAO is removed from m_Cache
    void ClearStaleKeys(const std::vector<VAOHashKey>& StaleKeys);

    Threading::SpinLock                                                                    m_CacheLock;
    std::unordered_map<VAOHashKey, GLObjectWrappers::GLVertexArrayObj, VAOHashKey::Hasher> m_Cache;

    std::unordered_map<UniqueIdentifier, std::vector<VAOHashKey>> m_PSOToKey;
    std::unordered_map<UniqueIdentifier, std::vector<VAOHashKey>> m_BuffToKey;

    // Any draw command fails if no VAO is bound. We will use this empty
    // VAO for draw commands with null input layout, such as these that
    // only use VertexID as input.
    GLObjectWrappers::GLVertexArrayObj m_EmptyVAO;
};

} // namespace Diligent
