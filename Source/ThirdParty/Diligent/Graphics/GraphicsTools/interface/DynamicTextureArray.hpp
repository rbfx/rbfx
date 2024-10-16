/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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
/// Declaration of DynamicTextureArray class

#include <atomic>

#include "../../GraphicsEngine/interface/RenderDevice.h"
#include "../../GraphicsEngine/interface/DeviceContext.h"
#include "../../GraphicsEngine/interface/Texture.h"
#include "../../GraphicsEngine/interface/Fence.h"
#include "../../GraphicsEngine/interface/DeviceMemory.h"
#include "../../../Common/interface/RefCntAutoPtr.hpp"

namespace Diligent
{

/// Dynamic texture array create information.
struct DynamicTextureArrayCreateInfo
{
    /// Texture array description.

    /// \remarks
    ///     - Desc.Type must be RESOURCE_DIM_TEX_2D_ARRAY
    ///     - Desc.Format must not be TEX_FORMAT_UNKNOWN
    ///     - Desc.Width and Desc.Height must not be zero
    TextureDesc Desc;

    /// The number of slices in memory page.

    /// \remarks
    ///     This value is only relevant when Desc.Usage == USAGE_SPARSE and
    ///     defines the number of texture array slices in one memory page.
    Uint32 NumSlicesInMemoryPage = 1;
};

/// Dynamically resizable texture 2D array
class DynamicTextureArray
{
public:
    /// Initializes the dynamic texture array.

    /// \param[in] pDevice    - Render device that will be used to create the texture array.
    ///                         This parameter may be null (see remarks).
    /// \param[in] CreateInfo - Texture array create information, see Diligent::DynamicTextureArrayCreateInfo.
    ///
    /// \remarks            If pDevice is null, internal texture creation will be postponed
    ///                     until Update() or Resize() is called.
    DynamicTextureArray(IRenderDevice* pDevice, const DynamicTextureArrayCreateInfo& CreateInfo);

    // clang-format off
    DynamicTextureArray           (const DynamicTextureArray&)  = delete;
    DynamicTextureArray& operator=(const DynamicTextureArray&)  = delete;
    DynamicTextureArray           (      DynamicTextureArray&&) = delete;
    DynamicTextureArray& operator=(      DynamicTextureArray&&) = delete;
    // clang-format on


    /// Resizes the texture array to the specified number of slices.

    /// \param[in] pDevice        - Render device that will be used create new internal texture.
    ///                             This parameter may be null (see remarks).
    /// \param[in] pContext       - Device context that will be used to copy existing contents
    ///                             to the new texture (when using non-sparse texture), or commit
    ///                             new memory tiles (when using sparse texture).
    ///                             This parameter may be null (see remarks).
    /// \param[in] NewArraySize   - The new number of slices in the texture array.
    /// \param[in] DiscardContent - Whether to discard previous texture content (for non-sparse textures).
    ///
    /// \return     Pointer to the new texture object after resize.
    ///
    /// \remarks    The method operation depends on which of pDevice and pContext parameters
    ///             are not null:
    ///             - Both pDevice and pContext are not null: internal texture is created (if necessary)
    ///               and existing contents is copied (for non-sparse textures). Update() may be called with
    ///               both pDevice and pContext being null.
    ///             - pDevice is not null, pContext is null: internal texture or additional memory pages
    ///               are created, but existing contents is not copied and memory tiles are not bound.
    ///               An application must provide non-null device context when calling Update().
    ///             - Both pDevice and pContext are null: internal texture or memory pages are not created.
    ///               An application must provide non-null device and device context when calling
    ///               Update().
    ///
    ///             Typically pContext is null when the method is called from a worker thread.
    ///
    ///             If NewArraySize is zero, internal buffer will be released.
    ITexture* Resize(IRenderDevice*  pDevice,
                     IDeviceContext* pContext,
                     Uint32          NewArraySize,
                     bool            DiscardContent = false);


    /// Updates the internal texture object.

    /// \param[in] pDevice  - Render device that will be used to create a new texture,
    ///                       if necessary (see remarks).
    /// \param[in] pContext - Device context that will be used to copy existing
    ///                       texture contents (when using non-sparse texture), or bind
    ///                       memory tiles (when using sparse textures), if necessary
    ///                       (see remarks).
    /// \return               A pointer to the texture object.
    ///
    /// \remarks    If the texture has been resized, but internal texture object has not been
    ///             initialized, pDevice and pContext must not be null.
    ///
    ///             If the texture does not need to be updated (PendingUpdate() returns false),
    ///             both pDevice and pContext may be null.
    ITexture* Update(IRenderDevice*  pDevice,
                     IDeviceContext* pContext);


    /// Returns a pointer to the texture object.

    /// \remarks    If the texture has not be initialized, the method returns null.
    ///             If the texture may need to be updated (initialized or resized),
    ///             use the Update() method.
    ITexture* GetTexture() const
    {
        return m_pTexture;
    }

    /// Returns true if the texture must be updated before use (e.g. it has been resized,
    /// but internal texture has not been initialized or updated).
    /// When update is not pending, Update() may be called with null device and context.
    bool PendingUpdate() const
    {
        return m_PendingSize != m_Desc.ArraySize;
    }

    /// Returns the texture description.
    const TextureDesc& GetDesc() const
    {
        return m_Desc;
    }

    /// Returns dynamic texture version.
    /// The version is incremented every time a new internal texture is created.
    Uint32 GetVersion() const
    {
        return m_Version;
    }

    /// Returns the amount of memory currently used by the dynamic array, in bytes.
    Uint64 GetMemoryUsage() const;

private:
    void CommitResize(IRenderDevice*  pDevice,
                      IDeviceContext* pContext,
                      bool            AllowNull);

    void ResizeSparseTexture(IDeviceContext* pContext);
    void ResizeDefaultTexture(IDeviceContext* pContext);

    void CreateSparseTexture(IRenderDevice* pDevice);
    void CreateResources(IRenderDevice* pDevice);

    const std::string m_Name;
    TextureDesc       m_Desc;
    const Uint32      m_NumSlicesInPage;

    std::atomic<Uint32> m_Version{0};

    Uint32 m_PendingSize = 0;

    RefCntAutoPtr<ITexture>      m_pTexture;
    RefCntAutoPtr<ITexture>      m_pStaleTexture;
    RefCntAutoPtr<IDeviceMemory> m_pMemory;

    Uint64 m_MemoryPageSize = 0;

    Uint64 m_NextBeforeResizeFenceValue = 1;
    Uint64 m_NextAfterResizeFenceValue  = 1;
    Uint64 m_LastAfterResizeFenceValue  = 0;

    RefCntAutoPtr<IFence> m_pBeforeResizeFence;
    RefCntAutoPtr<IFence> m_pAfterResizeFence;
};

} // namespace Diligent
