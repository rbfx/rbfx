/*
 *  Copyright 2019-2024 Diligent Graphics LLC
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

#include <vector>

#include "EngineGLImplTraits.hpp"
#include "BufferBase.hpp"
#include "BufferViewGLImpl.hpp" // Required by BufferBase
#include "GLObjectWrapper.hpp"
#include "AsyncWritableResource.hpp"
#include "GLContextState.hpp"

namespace Diligent
{

/// Buffer object implementation in OpenGL backend.
class BufferGLImpl final : public BufferBase<EngineGLImplTraits>, public AsyncWritableResource
{
public:
    using TBufferBase = BufferBase<EngineGLImplTraits>;

    BufferGLImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 RenderDeviceGLImpl*        pDeviceGL,
                 const BufferDesc&          BuffDesc,
                 GLContextState&            CtxState,
                 const BufferData*          pBuffData,
                 bool                       bIsDeviceInternal);

    BufferGLImpl(IReferenceCounters*        pRefCounters,
                 FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                 class RenderDeviceGLImpl*  pDeviceGL,
                 const BufferDesc&          BuffDesc,
                 GLContextState&            CtxState,
                 GLuint                     GLHandle,
                 bool                       bIsDeviceInternal);

    ~BufferGLImpl();

    /// Queries the specific interface, see IObject::QueryInterface() for details
    virtual void DILIGENT_CALL_TYPE QueryInterface(const INTERFACE_ID& IID, IObject** ppInterface) override;

    void UpdateData(GLContextState& CtxState, Uint64 Offset, Uint64 Size, const void* pData);
    void CopyData(GLContextState& CtxState, BufferGLImpl& SrcBufferGL, Uint64 SrcOffset, Uint64 DstOffset, Uint64 Size);
    void Map(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, PVoid& pMappedData);
    void MapRange(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, Uint64 Offset, Uint64 Length, PVoid& pMappedData);
    void Unmap(GLContextState& CtxState);

    __forceinline void BufferMemoryBarrier(MEMORY_BARRIER RequiredBarriers, GLContextState& GLContextState);

    const GLObjectWrappers::GLBufferObj& GetGLHandle() const { return m_GlBuffer; }

    /// Implementation of IBufferGL::GetGLBufferHandle().
    virtual GLuint DILIGENT_CALL_TYPE GetGLBufferHandle() const override final { return GetGLHandle(); }

    /// Implementation of IBuffer::GetNativeHandle() in OpenGL backend.
    virtual Uint64 DILIGENT_CALL_TYPE GetNativeHandle() override final { return BitCast<Uint64>(GetGLBufferHandle()); }

    /// Implementation of IBuffer::GetSparseProperties().
    virtual SparseBufferProperties DILIGENT_CALL_TYPE GetSparseProperties() const override final;

private:
    virtual void CreateViewInternal(const struct BufferViewDesc& ViewDesc, IBufferView** ppView, bool bIsDefaultView) override;

    friend class DeviceContextGLImpl;
    friend class VAOCache;

    GLObjectWrappers::GLBufferObj m_GlBuffer;
    const Uint32                  m_BindTarget;
    const GLenum                  m_GLUsageHint;

#if PLATFORM_EMSCRIPTEN
    struct MappedData
    {
        std::vector<Uint8> Data;
        MAP_TYPE           Type   = MAP_WRITE;
        Uint64             Offset = 0;
    } m_Mapped;
#endif
};

void BufferGLImpl::BufferMemoryBarrier(MEMORY_BARRIER RequiredBarriers, GLContextState& GLState)
{
#if GL_ARB_shader_image_load_store
#    ifdef DILIGENT_DEBUG
    {
        constexpr auto BufferBarriers = MEMORY_BARRIER_ALL_BUFFER_BARRIERS;
        VERIFY((RequiredBarriers & BufferBarriers) != 0, "At least one buffer memory barrier flag should be set");
        VERIFY((RequiredBarriers & ~BufferBarriers) == 0, "Inappropriate buffer memory barrier flag");
    }
#    endif

    GLState.EnsureMemoryBarrier(RequiredBarriers, this);
#endif
}

} // namespace Diligent
