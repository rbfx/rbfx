/*
 *  Copyright 2019-2023 Diligent Graphics LLC
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

/// \file
/// Definition of the Diligent::IResourceMapping interface and related data structures

#include "DeviceObject.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

// {6C1AC7A6-B429-4139-9433-9E54E93E384A}
static DILIGENT_CONSTEXPR INTERFACE_ID IID_ResourceMapping =
    {0x6c1ac7a6, 0xb429, 0x4139, {0x94, 0x33, 0x9e, 0x54, 0xe9, 0x3e, 0x38, 0x4a}};

/// Describes the resource mapping object entry
struct ResourceMappingEntry
{
    // clang-format off

    /// Object name
    const Char* Name        DEFAULT_INITIALIZER(nullptr);

    /// Pointer to the object's interface
    IDeviceObject* pObject  DEFAULT_INITIALIZER(nullptr);

    Uint32 ArrayIndex       DEFAULT_INITIALIZER(0);


#if DILIGENT_CPP_INTERFACE
    constexpr ResourceMappingEntry() noexcept {}

    /// Initializes the structure members

    /// \param [in] _Name       - Object name.
    /// \param [in] _pObject    - Pointer to the object.
    /// \param [in] _ArrayIndex - For array resources, index in the array
    constexpr ResourceMappingEntry (const Char* _Name, IDeviceObject* _pObject, Uint32 _ArrayIndex = 0)noexcept :
        Name      { _Name     },
        pObject   { _pObject  },
        ArrayIndex{_ArrayIndex}
    {}
    // clang-format on
#endif
};
typedef struct ResourceMappingEntry ResourceMappingEntry;


/// Resource mapping create information
struct ResourceMappingCreateInfo
{
    /// A pointer to the array of resource mapping entries.
    const ResourceMappingEntry* pEntries DEFAULT_INITIALIZER(nullptr);

    /// The number of entries in pEntries array.
    Uint32 NumEntries DEFAULT_INITIALIZER(0);

#if DILIGENT_CPP_INTERFACE
    constexpr ResourceMappingCreateInfo() noexcept
    {}

    constexpr ResourceMappingCreateInfo(const ResourceMappingEntry* _pEntries,
                                        Uint32                      _NumEntries) noexcept :
        pEntries{_pEntries},
        NumEntries{_NumEntries}
    {}
#endif
};
typedef struct ResourceMappingCreateInfo ResourceMappingCreateInfo;

#define DILIGENT_INTERFACE_NAME IResourceMapping
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IResourceMappingInclusiveMethods \
    IObjectInclusiveMethods;             \
    IResourceMappingMethods ResourceMapping

// clang-format off

/// Resource mapping

/// This interface provides mapping between literal names and resource pointers.
/// It is created by IRenderDevice::CreateResourceMapping().
/// \remarks Resource mapping holds strong references to all objects it keeps.
DILIGENT_BEGIN_INTERFACE(IResourceMapping, IObject)
{
    /// Adds a resource to the mapping.

    /// \param [in] Name - Resource name.
    /// \param [in] pObject - Pointer to the object.
    /// \param [in] bIsUnique - Flag indicating if a resource with the same name
    ///                         is allowed to be found in the mapping. In the latter
    ///                         case, the new resource replaces the existing one.
    ///
    /// \remarks Resource mapping increases the reference counter for referenced objects. So an
    ///          object will not be released as long as it is in the resource mapping.
    VIRTUAL void METHOD(AddResource)(THIS_
                                     const Char*    Name,
                                     IDeviceObject* pObject,
                                     Bool           bIsUnique) PURE;


    /// Adds resource array to the mapping.

    /// \param [in] Name - Resource array name.
    /// \param [in] StartIndex - First index in the array, where the first element will be inserted
    /// \param [in] ppObjects - Pointer to the array of objects.
    /// \param [in] NumElements - Number of elements to add
    /// \param [in] bIsUnique - Flag indicating if a resource with the same name
    ///                         is allowed to be found in the mapping. In the latter
    ///                         case, the new resource replaces the existing one.
    ///
    /// \remarks Resource mapping increases the reference counter for referenced objects. So an
    ///          object will not be released as long as it is in the resource mapping.
    VIRTUAL void METHOD(AddResourceArray)(THIS_
                                          const Char*           Name,
                                          Uint32                StartIndex,
                                          IDeviceObject* const* ppObjects,
                                          Uint32                NumElements,
                                          Bool                  bIsUnique) PURE;


    /// Removes a resource from the mapping using its literal name.

    /// \param [in] Name - Name of the resource to remove.
    /// \param [in] ArrayIndex - For array resources, index in the array
    VIRTUAL void METHOD(RemoveResourceByName)(THIS_
                                              const Char* Name,
                                              Uint32      ArrayIndex DEFAULT_VALUE(0)) PURE;

    /// Finds a resource in the mapping.

    /// \param [in] Name        - Resource name.
    /// \param [in] ArrayIndex  - for arrays, index of the array element.
    ///
    /// \return Pointer to the object with the given name and array index.
    ///
    /// \remarks The method does *NOT* increase the reference counter
    ///          of the returned object, so Release() must not be called.
    ///          The pointer is guaranteed to be valid until the object is removed
    ///          from the resource mapping, or the mapping is destroyed.
    VIRTUAL IDeviceObject* METHOD(GetResource)(THIS_
                                               const Char* Name,
                                               Uint32      ArrayIndex DEFAULT_VALUE(0)) PURE;

    /// Returns the size of the resource mapping, i.e. the number of objects.
    VIRTUAL size_t METHOD(GetSize)(THIS) PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IResourceMapping_AddResource(This, ...)          CALL_IFACE_METHOD(ResourceMapping, AddResource,          This, __VA_ARGS__)
#    define IResourceMapping_AddResourceArray(This, ...)     CALL_IFACE_METHOD(ResourceMapping, AddResourceArray,     This, __VA_ARGS__)
#    define IResourceMapping_RemoveResourceByName(This, ...) CALL_IFACE_METHOD(ResourceMapping, RemoveResourceByName, This, __VA_ARGS__)
#    define IResourceMapping_GetResource(This, ...)          CALL_IFACE_METHOD(ResourceMapping, GetResource,          This, __VA_ARGS__)
#    define IResourceMapping_GetSize(This)                   CALL_IFACE_METHOD(ResourceMapping, GetSize,              This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
