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

#include "pch.h"

#include "BufferGLImpl.hpp"

#include "RenderDeviceGLImpl.hpp"
#include "DeviceContextGLImpl.hpp"

#include "GLTypeConversions.hpp"
#include "EngineMemory.h"

namespace Diligent
{

static GLenum GetBufferBindTarget(const BufferDesc& Desc)
{
    GLenum Target = GL_ARRAY_BUFFER;
    if (Desc.BindFlags & BIND_VERTEX_BUFFER)
        Target = GL_ARRAY_BUFFER;
    else if (Desc.BindFlags & BIND_INDEX_BUFFER)
        Target = GL_ELEMENT_ARRAY_BUFFER;
    else if (Desc.BindFlags & BIND_UNIFORM_BUFFER)
        Target = GL_UNIFORM_BUFFER;
    else if (Desc.BindFlags & BIND_INDIRECT_DRAW_ARGS)
    {
#ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4127) // conditional expression is constant
#endif
        VERIFY(GL_DRAW_INDIRECT_BUFFER != 0, "Indirect draw is not supported");
#ifdef _MSC_VER
#    pragma warning(pop)
#endif
        Target = GL_DRAW_INDIRECT_BUFFER;
    }
    else if (Desc.Usage == USAGE_STAGING)
    {
        if (Desc.CPUAccessFlags == CPU_ACCESS_WRITE)
            Target = GL_PIXEL_UNPACK_BUFFER;
        else if (Desc.CPUAccessFlags == CPU_ACCESS_READ)
            Target = GL_PIXEL_PACK_BUFFER;
    }

    return Target;
}
BufferGLImpl::BufferGLImpl(IReferenceCounters*        pRefCounters,
                           FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                           RenderDeviceGLImpl*        pDeviceGL,
                           const BufferDesc&          BuffDesc,
                           GLContextState&            GLState,
                           const BufferData*          pBuffData /*= nullptr*/,
                           bool                       bIsDeviceInternal) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDeviceGL,
        BuffDesc,
        bIsDeviceInternal
    },
    m_GlBuffer    {true                          }, // Create buffer immediately
    m_BindTarget  {GetBufferBindTarget(BuffDesc) },
    m_GLUsageHint {UsageToGLUsage(BuffDesc)}
// clang-format on
{
    ValidateBufferInitData(BuffDesc, pBuffData);

    if (m_Desc.Usage == USAGE_UNIFIED)
    {
        LOG_ERROR_AND_THROW("Unified resources are not supported in OpenGL/GLES");
    }

    // TODO: find out if it affects performance if the buffer is originally bound to one target
    // and then bound to another (such as first to GL_ARRAY_BUFFER and then to GL_UNIFORM_BUFFER)

    // We must unbind VAO because otherwise we will break the bindings
    constexpr bool ResetVAO = true;
    GLState.BindBuffer(m_BindTarget, m_GlBuffer, ResetVAO);
    const GLvoid* pData = pBuffData != nullptr ? pBuffData->pData : nullptr;

    // Create and initialize a buffer object's data store

    // Target must be one of GL_ARRAY_BUFFER, GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER,
    // GL_ELEMENT_ARRAY_BUFFER, GL_PIXEL_PACK_BUFFER, GL_PIXEL_UNPACK_BUFFER, GL_TEXTURE_BUFFER,
    // GL_TRANSFORM_FEEDBACK_BUFFER, or GL_UNIFORM_BUFFER.

    // Usage must be one of GL_STREAM_DRAW, GL_STREAM_READ, GL_STREAM_COPY, GL_STATIC_DRAW,
    // GL_STATIC_READ, GL_STATIC_COPY, GL_DYNAMIC_DRAW, GL_DYNAMIC_READ, or GL_DYNAMIC_COPY.

    //The frequency of access may be one of these:
    //
    //STREAM
    //  The data store contents will be modified once and used at most a few times.
    //
    //STATIC
    //  The data store contents will be modified once and used many times.
    //
    //DYNAMIC
    //  The data store contents will be modified repeatedly and used many times.
    //

    //The nature of access may be one of these:
    //
    //DRAW
    //  The data store contents are modified by the application, and used as the source for GL
    //  drawing and image specification commands.
    //
    //READ
    //  The data store contents are modified by reading data from the GL, and used to return that
    //  data when queried by the application.
    //
    //COPY
    //  The data store contents are modified by reading data from the GL, and used as the source
    //  for GL drawing and image specification commands.

    // See also http://www.informit.com/articles/article.aspx?p=2033340&seqNum=2

    // All buffer bind targets (GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER etc.) relate to the same
    // kind of objects. As a result they are all equivalent from a transfer point of view.
    glBufferData(m_BindTarget, StaticCast<GLsizeiptr>(BuffDesc.Size), pData, m_GLUsageHint);
    CHECK_GL_ERROR_AND_THROW("glBufferData() failed");
    GLState.BindBuffer(m_BindTarget, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;

    m_GlBuffer.SetName(m_Desc.Name);
}

static BufferDesc GetBufferDescFromGLHandle(GLContextState& GLState, BufferDesc BuffDesc, GLuint BufferHandle)
{
    // NOTE: the operations in this function are merely for debug purposes.
    // If binding a buffer to a target does not work, these operations can be skipped
    GLenum BindTarget = GetBufferBindTarget(BuffDesc);

    // We must unbind VAO because otherwise we will break the bindings
    constexpr bool ResetVAO = true;
    GLState.BindBuffer(BindTarget, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);

    // Note that any glBindBufferBase, glBindBufferRange etc. also bind the buffer to the generic buffer binding point.
    glBindBuffer(BindTarget, BufferHandle);
    CHECK_GL_ERROR("Failed to bind GL buffer to ", BindTarget, " target");

    GLint BufferSize = 0;
    glGetBufferParameteriv(BindTarget, GL_BUFFER_SIZE, &BufferSize);
    CHECK_GL_ERROR("glGetBufferParameteriv() failed");
    VERIFY_EXPR(BufferSize > 0);

    VERIFY(BuffDesc.Size == 0 || BuffDesc.Size == static_cast<Uint32>(BufferSize), "Buffer size specified by the BufferDesc (", BuffDesc.Size, ") does not match the size recovered from gl buffer object (", BufferSize, ")");
    if (BufferSize > 0)
        BuffDesc.Size = static_cast<Uint32>(BufferSize);

    glBindBuffer(BindTarget, 0);
    CHECK_GL_ERROR("Failed to unbind GL buffer");

    return BuffDesc;
}

BufferGLImpl::BufferGLImpl(IReferenceCounters*        pRefCounters,
                           FixedBlockMemoryAllocator& BuffViewObjMemAllocator,
                           RenderDeviceGLImpl*        pDeviceGL,
                           const BufferDesc&          BuffDesc,
                           GLContextState&            CtxState,
                           GLuint                     GLHandle,
                           bool                       bIsDeviceInternal) :
    // clang-format off
    TBufferBase
    {
        pRefCounters,
        BuffViewObjMemAllocator,
        pDeviceGL,
        GetBufferDescFromGLHandle(CtxState, BuffDesc, GLHandle),
        bIsDeviceInternal
    },
    // Attach to external buffer handle
    m_GlBuffer    {true, GLObjectWrappers::GLBufferObjCreateReleaseHelper(GLHandle)},
    m_BindTarget  {GetBufferBindTarget(m_Desc)},
    m_GLUsageHint {UsageToGLUsage(BuffDesc)   }
// clang-format on
{
    m_MemoryProperties = MEMORY_PROPERTY_HOST_COHERENT;
}

BufferGLImpl::~BufferGLImpl()
{
    GetDevice()->OnDestroyBuffer(*this);
}

IMPLEMENT_QUERY_INTERFACE(BufferGLImpl, IID_BufferGL, TBufferBase)

void BufferGLImpl::UpdateData(GLContextState& CtxState, Uint64 Offset, Uint64 Size, const void* pData)
{
    BufferMemoryBarrier(
        MEMORY_BARRIER_BUFFER_UPDATE, // Reads or writes to buffer objects via any OpenGL API functions that allow
                                      // modifying their contents will reflect data written by shaders prior to the barrier.
                                      // Additionally, writes via these commands issued after the barrier will wait on
                                      // the completion of any shader writes to the same memory initiated prior to the barrier.
        CtxState);

    // We must unbind VAO because otherwise we will break the bindings
    const auto ResetVAO = m_BindTarget == GL_ARRAY_BUFFER || m_BindTarget == GL_ELEMENT_ARRAY_BUFFER;
    CtxState.BindBuffer(m_BindTarget, m_GlBuffer, ResetVAO);
    // All buffer bind targets (GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER etc.) relate to the same
    // kind of objects. As a result they are all equivalent from a transfer point of view.
    glBufferSubData(m_BindTarget, StaticCast<GLintptr>(Offset), StaticCast<GLsizeiptr>(Size), pData);
    CHECK_GL_ERROR("glBufferSubData() failed");
    CtxState.BindBuffer(m_BindTarget, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);
}


void BufferGLImpl::CopyData(GLContextState& CtxState, BufferGLImpl& SrcBufferGL, Uint64 SrcOffset, Uint64 DstOffset, Uint64 Size)
{
    BufferMemoryBarrier(
        MEMORY_BARRIER_BUFFER_UPDATE, // Reads or writes to buffer objects via any OpenGL API functions that allow
                                      // modifying their contents will reflect data written by shaders prior to the barrier.
                                      // Additionally, writes via these commands issued after the barrier will wait on
                                      // the completion of any shader writes to the same memory initiated prior to the barrier.
        CtxState);
    SrcBufferGL.BufferMemoryBarrier(
        MEMORY_BARRIER_BUFFER_UPDATE,
        CtxState);

    // Whilst glCopyBufferSubData() can be used to copy data between buffers bound to any two targets,
    // the targets GL_COPY_READ_BUFFER and GL_COPY_WRITE_BUFFER are provided specifically for this purpose.
    // Neither target is used for anything else by OpenGL, and so you can safely bind buffers to them for
    // the purposes of copying or staging data without disturbing OpenGL state or needing to keep track of
    // what was bound to the target before your copy.
    constexpr bool ResetVAO = false; // No need to reset VAO for READ/WRITE targets
    CtxState.BindBuffer(GL_COPY_WRITE_BUFFER, m_GlBuffer, ResetVAO);
    CtxState.BindBuffer(GL_COPY_READ_BUFFER, SrcBufferGL.m_GlBuffer, ResetVAO);
    glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, StaticCast<GLintptr>(SrcOffset), StaticCast<GLintptr>(DstOffset), StaticCast<GLsizeiptr>(Size));
    CHECK_GL_ERROR("glCopyBufferSubData() failed");
    CtxState.BindBuffer(GL_COPY_READ_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);
    CtxState.BindBuffer(GL_COPY_WRITE_BUFFER, GLObjectWrappers::GLBufferObj::Null(), ResetVAO);
}

void BufferGLImpl::Map(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, PVoid& pMappedData)
{
    MapRange(CtxState, MapType, MapFlags, 0, m_Desc.Size, pMappedData);
}

void BufferGLImpl::MapRange(GLContextState& CtxState, MAP_TYPE MapType, Uint32 MapFlags, Uint64 Offset, Uint64 Length, PVoid& pMappedData)
{
    BufferMemoryBarrier(
        MEMORY_BARRIER_CLIENT_MAPPED_BUFFER, // Access by the client to persistent mapped regions of buffer
                                             // objects will reflect data written by shaders prior to the barrier.
                                             // Note that this may cause additional synchronization operations.
        CtxState);

    // We must unbind VAO because otherwise we will break the bindings
    constexpr bool ResetVAO = true;
    CtxState.BindBuffer(m_BindTarget, m_GlBuffer, ResetVAO);

    // !!!WARNING!!! GL_MAP_UNSYNCHRONIZED_BIT is not the same thing as MAP_FLAG_DO_NOT_WAIT.
    // If GL_MAP_UNSYNCHRONIZED_BIT flag is set, OpenGL will not attempt to synchronize operations
    // on the buffer. This does not mean that map will fail if the buffer still in use. It is thus
    // what WRITE_NO_OVERWRITE does

    GLbitfield Access = 0;
    switch (MapType)
    {
        case MAP_READ:
            Access |= GL_MAP_READ_BIT;
            break;

        case MAP_WRITE:
            Access |= GL_MAP_WRITE_BIT;

            if (MapFlags & MAP_FLAG_DISCARD)
            {
                // Use GL_MAP_INVALIDATE_BUFFER_BIT flag to discard previous buffer contents

                // If GL_MAP_INVALIDATE_BUFFER_BIT is specified, the entire contents of the buffer may
                // be discarded and considered invalid, regardless of the specified range. Any data
                // lying outside the mapped range of the buffer object becomes undefined, as does any
                // data within the range but not subsequently written by the application.This flag may
                // not be used with GL_MAP_READ_BIT.
                Access |= GL_MAP_INVALIDATE_BUFFER_BIT;
            }

            if (MapFlags & MAP_FLAG_NO_OVERWRITE)
            {
                // If GL_MAP_UNSYNCHRONIZED_BIT flag is set, OpenGL will not attempt to synchronize
                // operations on the buffer.
                Access |= GL_MAP_UNSYNCHRONIZED_BIT;
            }
            break;


        case MAP_READ_WRITE:
            Access |= GL_MAP_WRITE_BIT | GL_MAP_READ_BIT;
            break;


        default: UNEXPECTED("Unknown map type");
    }

    pMappedData = glMapBufferRange(m_BindTarget, StaticCast<GLintptr>(Offset), StaticCast<GLsizeiptr>(Length), Access);
    CHECK_GL_ERROR("glMapBufferRange() failed");
    VERIFY(pMappedData, "Map failed");
}

void BufferGLImpl::Unmap(GLContextState& CtxState)
{
    constexpr bool ResetVAO = true;
    CtxState.BindBuffer(m_BindTarget, m_GlBuffer, ResetVAO);
    auto Result = glUnmapBuffer(m_BindTarget);
    // glUnmapBuffer() returns TRUE unless data values in the buffer's data store have
    // become corrupted during the period that the buffer was mapped. Such corruption
    // can be the result of a screen resolution change or other window system - dependent
    // event that causes system heaps such as those for high - performance graphics memory
    // to be discarded. GL implementations must guarantee that such corruption can
    // occur only during the periods that a buffer's data store is mapped. If such corruption
    // has occurred, glUnmapBuffer() returns FALSE, and the contents of the buffer's
    // data store become undefined.
    VERIFY(Result != GL_FALSE, "Failed to unmap buffer. The data may have been corrupted");
    (void)Result;
}

void BufferGLImpl::CreateViewInternal(const BufferViewDesc& OrigViewDesc, IBufferView** ppView, bool bIsDefaultView)
{
    VERIFY(ppView != nullptr, "Buffer view pointer address is null");
    if (!ppView) return;
    VERIFY(*ppView == nullptr, "Overwriting reference to existing object may cause memory leaks");

    *ppView = nullptr;

    try
    {
        auto* const pDeviceGLImpl = GetDevice();

        auto ViewDesc = OrigViewDesc;
        ValidateAndCorrectBufferViewDesc(m_Desc, ViewDesc, pDeviceGLImpl->GetAdapterInfo().Buffer.StructuredBufferOffsetAlignment);

        auto& BuffViewAllocator = pDeviceGLImpl->GetBuffViewObjAllocator();
        VERIFY(&BuffViewAllocator == &m_dbgBuffViewAllocator, "Buff view allocator does not match allocator provided at buffer initialization");

        auto pContext = pDeviceGLImpl->GetImmediateContext(0);
        VERIFY(pContext, "Immediate context has been released");
        auto& CtxState = pContext->GetContextState();

        *ppView = NEW_RC_OBJ(BuffViewAllocator, "BufferViewGLImpl instance", BufferViewGLImpl, bIsDefaultView ? this : nullptr)(pDeviceGLImpl, CtxState, ViewDesc, this, bIsDefaultView);

        if (!bIsDefaultView)
            (*ppView)->AddRef();
    }
    catch (const std::runtime_error&)
    {
        const auto* ViewTypeName = GetBufferViewTypeLiteralName(OrigViewDesc.ViewType);
        LOG_ERROR("Failed to create view '", (OrigViewDesc.Name ? OrigViewDesc.Name : ""), "' (", ViewTypeName, ") for buffer '", (m_Desc.Name ? m_Desc.Name : ""), "'");
    }
}

SparseBufferProperties BufferGLImpl::GetSparseProperties() const
{
    DEV_ERROR("IBuffer::GetSparseProperties() is not supported in OpenGL");
    return {};
}

} // namespace Diligent
