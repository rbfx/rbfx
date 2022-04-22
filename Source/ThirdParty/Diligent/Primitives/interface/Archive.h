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

#pragma once

// clang-format off

/// \file
/// Definition of the Diligent::IArchive interface

#include "Object.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// clang-format on

// {49C98F50-CD7D-4F3D-9432-42DD531A7B1D}
static const INTERFACE_ID IID_Archive =
    {0x49c98f50, 0xcd7d, 0x4f3d, {0x94, 0x32, 0x42, 0xdd, 0x53, 0x1a, 0x7b, 0x1d}};

#define DILIGENT_INTERFACE_NAME IArchive
#include "DefineInterfaceHelperMacros.h"

#define IArchiveInclusiveMethods \
    IObjectInclusiveMethods;     \
    IArchiveMethods Archive

// clang-format off

/// Binary archive
DILIGENT_BEGIN_INTERFACE(IArchive, IObject)
{
    /// Reads the data from the archive

    /// \param[in]  Offset - Offset, in bytes, from the beginning of the archive
    ///                      where to start reading data.
    /// \param[in]  Size   - Size of the data to read, in bytes.
    /// \param[in]  pData  - Pointer to the memory where to write the data.
    /// \return     true if the data have been read successfully, and false otherwise.
    ///
    /// \remarks    The method is thread-safe
    VIRTUAL Bool METHOD(Read)(THIS_
                              Uint64 Offset,
                              Uint64 Size,
                              void*  pData) PURE;

    /// Returns the archive size, in bytes
    ///
    /// \remarks    The method is thread-safe
    VIRTUAL Uint64 METHOD(GetSize)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IArchive_Read(This, ...)  CALL_IFACE_METHOD(Archive, Read,    This, __VA_ARGS__)
#    define IArchive_GetSize(This)    CALL_IFACE_METHOD(Archive, GetSize, This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE
