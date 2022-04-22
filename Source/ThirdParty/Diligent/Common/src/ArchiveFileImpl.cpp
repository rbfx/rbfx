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

#include "ArchiveFileImpl.hpp"

#include <algorithm>

namespace Diligent
{

ArchiveFileImpl::ArchiveFileImpl(IReferenceCounters* pRefCounters, const Char* Path) :
    TObjectBase{pRefCounters},
    m_File{Path, EFileAccessMode::Read},
    m_FileSize{m_File ? m_File->GetSize() : 0}
{
    if (!m_File)
        LOG_ERROR_AND_THROW("Failed to open file '", Path, "'");
}

Bool ArchiveFileImpl::Read(Uint64 Offset, Uint64 Size, void* pData)
{
    if (Size == 0)
        return True;

    if (Offset >= m_FileSize)
        return False;

    DEV_CHECK_ERR(pData != nullptr, "pData must not be null");

    std::unique_lock<std::mutex> Lock{m_Mtx};

    if (!m_File->SetPos(StaticCast<size_t>(Offset), FilePosOrigin::Start))
        return False;

    const auto RemainingSize = m_FileSize - Offset;
    return m_File->Read(pData, StaticCast<size_t>(std::min(Size, RemainingSize))) && Size <= RemainingSize;
}

RefCntAutoPtr<IArchive> ArchiveFileImpl::Create(const Char* Path)
{
    return RefCntAutoPtr<IArchive>{MakeNewRCObj<ArchiveFileImpl>()(Path)};
}

} // namespace Diligent
