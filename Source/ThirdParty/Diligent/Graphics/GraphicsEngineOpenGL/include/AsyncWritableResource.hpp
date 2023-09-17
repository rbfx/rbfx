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

namespace Diligent
{

enum MEMORY_BARRIER : Uint32
{
    MEMORY_BARRIER_NONE = 0,
    MEMORY_BARRIER_ALL  = GL_ALL_BARRIER_BITS,

    // Buffer barriers.
    // Driver does not handle buffer write access in shader and write/read access to a persistent mapped memory.
    MEMORY_BARRIER_VERTEX_BUFFER        = GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT,  // map/storage -> vertex
    MEMORY_BARRIER_INDEX_BUFFER         = GL_ELEMENT_ARRAY_BARRIER_BIT,        // map/storage -> index
    MEMORY_BARRIER_UNIFORM_BUFFER       = GL_UNIFORM_BARRIER_BIT,              // map/storage -> uniform
    MEMORY_BARRIER_BUFFER_UPDATE        = GL_BUFFER_UPDATE_BARRIER_BIT,        // map/storage -> host read/write/map or copy
    MEMORY_BARRIER_CLIENT_MAPPED_BUFFER = GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT, // map/storage -> map, only for persistent mapped memory without GL_MAP_COHERENT_BIT
    MEMORY_BARRIER_STORAGE_BUFFER       = GL_SHADER_STORAGE_BARRIER_BIT,       // map/storage -> storage
    MEMORY_BARRIER_INDIRECT_BUFFER      = GL_COMMAND_BARRIER_BIT,              // map/storage -> indirect
    MEMORY_BARRIER_TEXEL_BUFFER         = GL_TEXTURE_FETCH_BARRIER_BIT,        // map/storage -> texel buffer fetch
    MEMORY_BARRIER_PIXEL_BUFFER         = GL_PIXEL_BUFFER_BARRIER_BIT,         // map/storage -> copy to/from texture
    MEMORY_BARRIER_IMAGE_BUFFER         = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT,  // map/storage -> image buffer
    MEMORY_BARRIER_ALL_BUFFER_BARRIERS =
        MEMORY_BARRIER_VERTEX_BUFFER |
        MEMORY_BARRIER_INDEX_BUFFER |
        MEMORY_BARRIER_UNIFORM_BUFFER |
        MEMORY_BARRIER_BUFFER_UPDATE |
        MEMORY_BARRIER_CLIENT_MAPPED_BUFFER |
        MEMORY_BARRIER_STORAGE_BUFFER |
        MEMORY_BARRIER_INDIRECT_BUFFER |
        MEMORY_BARRIER_TEXEL_BUFFER |
        MEMORY_BARRIER_IMAGE_BUFFER,


    // Texture barriers
    MEMORY_BARRIER_TEXTURE_FETCH  = GL_TEXTURE_FETCH_BARRIER_BIT,       // storage -> fetch
    MEMORY_BARRIER_STORAGE_IMAGE  = GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, // storage -> storage
    MEMORY_BARRIER_TEXTURE_UPDATE = GL_TEXTURE_UPDATE_BARRIER_BIT,      // storage -> host read/write or copy
    MEMORY_BARRIER_FRAMEBUFFER    = GL_FRAMEBUFFER_BARRIER_BIT,         // storage -> framebuffer
    MEMORY_BARRIER_ALL_TEXTURE_BARRIERS =
        MEMORY_BARRIER_TEXTURE_FETCH |
        MEMORY_BARRIER_STORAGE_IMAGE |
        MEMORY_BARRIER_TEXTURE_UPDATE |
        MEMORY_BARRIER_FRAMEBUFFER,
};
DEFINE_FLAG_ENUM_OPERATORS(MEMORY_BARRIER);


class AsyncWritableResource
{
public:
    AsyncWritableResource() noexcept {}

    void           SetPendingMemoryBarriers(MEMORY_BARRIER Barriers) { m_PendingMemoryBarriers |= Barriers; }
    MEMORY_BARRIER GetPendingMemoryBarriers() { return m_PendingMemoryBarriers; }

private:
    friend class GLContextState;
    void ResetPendingMemoryBarriers(MEMORY_BARRIER Barriers) { m_PendingMemoryBarriers = Barriers; }
    void ClearPendingMemoryBarriers(MEMORY_BARRIER Barriers) { m_PendingMemoryBarriers &= ~Barriers; }

    MEMORY_BARRIER m_PendingMemoryBarriers = MEMORY_BARRIER_NONE;
};

} // namespace Diligent
