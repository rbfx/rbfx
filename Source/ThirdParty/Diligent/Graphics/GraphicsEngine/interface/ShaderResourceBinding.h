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

/// \file
/// Definition of the Diligent::IShaderResourceBinding interface and related data structures

#include "../../../Primitives/interface/Object.h"
#include "Shader.h"
#include "ShaderResourceVariable.h"
#include "ResourceMapping.h"

DILIGENT_BEGIN_NAMESPACE(Diligent)

struct IPipelineState;
struct IPipelineResourceSignature;

// {061F8774-9A09-48E8-8411-B5BD20560104}
static const INTERFACE_ID IID_ShaderResourceBinding =
    {0x61f8774, 0x9a09, 0x48e8, {0x84, 0x11, 0xb5, 0xbd, 0x20, 0x56, 0x1, 0x4}};


#define DILIGENT_INTERFACE_NAME IShaderResourceBinding
#include "../../../Primitives/interface/DefineInterfaceHelperMacros.h"

#define IShaderResourceBindingInclusiveMethods \
    IObjectInclusiveMethods;                   \
    IShaderResourceBindingMethods ShaderResourceBinding

// clang-format off

/// Shader resource binding interface
DILIGENT_BEGIN_INTERFACE(IShaderResourceBinding, IObject)
{
    /// Returns a pointer to the pipeline resource signature object that
    /// defines the layout of this shader resource binding object.

    /// The method does *NOT* increment the reference counter of the returned object,
    /// so Release() must not be called.
    VIRTUAL struct IPipelineResourceSignature* METHOD(GetPipelineResourceSignature)(THIS) CONST PURE;


    /// Binds SRB resources using the resource mapping

    /// \param [in] ShaderStages - Flags that specify shader stages, for which resources will be bound.
    ///                            Any combination of Diligent::SHADER_TYPE may be used.
    /// \param [in] pResMapping  - Shader resource mapping where required resources will be looked up.
    /// \param [in] Flags        - Additional flags. See Diligent::BIND_SHADER_RESOURCES_FLAGS.
    VIRTUAL void METHOD(BindResources)(THIS_
                                       SHADER_TYPE                 ShaderStages,
                                       IResourceMapping*           pResMapping,
                                       BIND_SHADER_RESOURCES_FLAGS Flags) PURE;


    /// Checks currently bound resources, see remarks.

    /// \param [in] ShaderStages - Flags that specify shader stages, for which to check resources.
    ///                            Any combination of Diligent::SHADER_TYPE may be used.
    /// \param [in] pResMapping  - Optional shader resource mapping where resources will be looked up.
    ///                            May be null.
    /// \param [in] Flags        - Additional flags, see remarks.
    ///
    /// \return     Variable type flags that did not pass the checks and thus may need to be updated.
    ///
    /// \remarks    This method may be used to perform various checks of the currently bound resources:
    ///
    ///             - BIND_SHADER_RESOURCES_UPDATE_MUTABLE and BIND_SHADER_RESOURCES_UPDATE_DYNAMIC flags
    ///               define which variable types to examine. Note that BIND_SHADER_RESOURCES_UPDATE_STATIC
    ///               has no effect as static resources are accessed through the PSO.
    ///
    ///             - If BIND_SHADER_RESOURCES_KEEP_EXISTING flag is not set and pResMapping is not null,
    ///               the method will compare currently bound resources with the ones in the resource mapping.
    ///               If any mismatch is found, the method will return the types of the variables that
    ///               contain mismatching resources.
    ///               Note that the situation when non-null object is bound to the variable, but the resource
    ///               mapping does not contain an object corresponding to the variable name, does not count as
    ///               mismatch.
    ///
    ///             - If BIND_SHADER_RESOURCES_VERIFY_ALL_RESOLVED flag is set, the method will check that
    ///               all resources of the specified variable types are bound and return the types of the variables
    ///               that are not bound.
    VIRTUAL SHADER_RESOURCE_VARIABLE_TYPE_FLAGS METHOD(CheckResources)(
                                        THIS_
                                        SHADER_TYPE                 ShaderStages,
                                        IResourceMapping*           pResMapping,
                                        BIND_SHADER_RESOURCES_FLAGS Flags) CONST PURE;


    /// Returns the variable by its name.

    /// \param [in] ShaderType - Type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Name       - Variable name.
    ///
    /// \note  This operation may potentially be expensive. If the variable will be used often, it is
    ///        recommended to store and reuse the pointer as it never changes.
    VIRTUAL IShaderResourceVariable* METHOD(GetVariableByName)(THIS_
                                                               SHADER_TYPE ShaderType,
                                                               const Char* Name) PURE;


    /// Returns the total variable count for the specific shader stage.

    /// \param [in] ShaderType - Type of the shader.
    /// \remark The method only counts mutable and dynamic variables that can be accessed through
    ///         the Shader Resource Binding object. Static variables are accessed through the Shader
    ///         object.
    VIRTUAL Uint32 METHOD(GetVariableCount)(THIS_
                                            SHADER_TYPE ShaderType) CONST PURE;

    /// Returns the variable by its index.

    /// \param [in] ShaderType - Type of the shader to look up the variable.
    ///                          Must be one of Diligent::SHADER_TYPE.
    /// \param [in] Index      - Variable index. The index must be between 0 and the total number
    ///                          of variables in this shader stage as returned by
    ///                          IShaderResourceBinding::GetVariableCount().
    /// \remark Only mutable and dynamic variables can be accessed through this method.
    ///         Static variables are accessed through the Shader object.
    ///
    /// \note   This operation may potentially be expensive. If the variable will be used often, it is
    ///         recommended to store and reuse the pointer as it never changes.
    VIRTUAL IShaderResourceVariable* METHOD(GetVariableByIndex)(THIS_
                                                                SHADER_TYPE ShaderType,
                                                                Uint32      Index) PURE;

    /// Returns true if static resources have been initialized in this SRB.
    VIRTUAL Bool METHOD(StaticResourcesInitialized)(THIS) CONST PURE;
};
DILIGENT_END_INTERFACE

#include "../../../Primitives/interface/UndefInterfaceHelperMacros.h"

#if DILIGENT_C_INTERFACE

// clang-format off

#    define IShaderResourceBinding_GetPipelineResourceSignature(This) CALL_IFACE_METHOD(ShaderResourceBinding, GetPipelineResourceSignature, This)
#    define IShaderResourceBinding_BindResources(This, ...)           CALL_IFACE_METHOD(ShaderResourceBinding, BindResources,                This, __VA_ARGS__)
#    define IShaderResourceBinding_CheckResources(This, ...)          CALL_IFACE_METHOD(ShaderResourceBinding, CheckResources,               This, __VA_ARGS__)
#    define IShaderResourceBinding_GetVariableByName(This, ...)       CALL_IFACE_METHOD(ShaderResourceBinding, GetVariableByName,            This, __VA_ARGS__)
#    define IShaderResourceBinding_GetVariableCount(This, ...)        CALL_IFACE_METHOD(ShaderResourceBinding, GetVariableCount,             This, __VA_ARGS__)
#    define IShaderResourceBinding_GetVariableByIndex(This, ...)      CALL_IFACE_METHOD(ShaderResourceBinding, GetVariableByIndex,           This, __VA_ARGS__)
#    define IShaderResourceBinding_StaticResourcesInitialized(This)   CALL_IFACE_METHOD(ShaderResourceBinding, StaticResourcesInitialized,   This)

// clang-format on

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
