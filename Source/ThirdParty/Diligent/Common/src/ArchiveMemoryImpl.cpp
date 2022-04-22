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

#include "ArchiveMemoryImpl.hpp"

#include <cstring>
#include <algorithm>

namespace Diligent
{

ArchiveMemoryImpl::ArchiveMemoryImpl(IReferenceCounters* pRefCounters, IDataBlob* pBlob) :
    TObjectBase{pRefCounters},
    m_pBlob{pBlob}
{
    if (!m_pBlob)
        LOG_ERROR_AND_THROW("pBlob must not be null");
}

ArchiveMemoryImpl::~ArchiveMemoryImpl()
{
}

Bool ArchiveMemoryImpl::Read(Uint64 Offset, Uint64 Size, void* pData)
{
    if (Size == 0)
        return True;

    const auto BlobSize = m_pBlob->GetSize();
    if (Offset >= BlobSize)
        return False;

    DEV_CHECK_ERR(pData != nullptr, "pData must not be null");
    const auto* pSrcData = reinterpret_cast<const Uint8*>(m_pBlob->GetConstDataPtr());

    const auto RemainingSize = BlobSize - Offset;
    std::memcpy(pData, pSrcData + StaticCast<size_t>(Offset), StaticCast<size_t>(std::min(Size, RemainingSize)));
    return Size <= RemainingSize;
}

RefCntAutoPtr<IArchive> ArchiveMemoryImpl::Create(IDataBlob* pBlob)
{
    return RefCntAutoPtr<IArchive>{MakeNewRCObj<ArchiveMemoryImpl>()(pBlob)};
}

} // namespace Diligent
