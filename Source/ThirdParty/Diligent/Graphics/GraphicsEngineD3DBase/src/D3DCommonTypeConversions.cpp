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

#include "ShaderResources.hpp"

#include <array>

#include "WinHPreface.h"
#include <d3dcommon.h>
#include "WinHPostface.h"

namespace Diligent
{

RESOURCE_DIMENSION D3DSrvDimensionToResourceDimension(D3D_SRV_DIMENSION SrvDim)
{
    switch (SrvDim)
    {
        // clang-format off
        case D3D_SRV_DIMENSION_BUFFER:           return RESOURCE_DIM_BUFFER;
        case D3D_SRV_DIMENSION_TEXTURE1D:        return RESOURCE_DIM_TEX_1D;
        case D3D_SRV_DIMENSION_TEXTURE1DARRAY:   return RESOURCE_DIM_TEX_1D_ARRAY;
        case D3D_SRV_DIMENSION_TEXTURE2D:        return RESOURCE_DIM_TEX_2D;
        case D3D_SRV_DIMENSION_TEXTURE2DARRAY:   return RESOURCE_DIM_TEX_2D_ARRAY;
        case D3D_SRV_DIMENSION_TEXTURE2DMS:      return RESOURCE_DIM_TEX_2D;
        case D3D_SRV_DIMENSION_TEXTURE2DMSARRAY: return RESOURCE_DIM_TEX_2D_ARRAY;
        case D3D_SRV_DIMENSION_TEXTURE3D:        return RESOURCE_DIM_TEX_3D;
        case D3D_SRV_DIMENSION_TEXTURECUBE:      return RESOURCE_DIM_TEX_CUBE;
        case D3D_SRV_DIMENSION_TEXTURECUBEARRAY: return RESOURCE_DIM_TEX_CUBE_ARRAY;
        // clang-format on
        default:
            return RESOURCE_DIM_BUFFER;
    }
}

SHADER_CODE_BASIC_TYPE D3DShaderVariableTypeToShaderCodeBasicType(D3D_SHADER_VARIABLE_TYPE D3DVarType)
{
    static_assert(SHADER_CODE_BASIC_TYPE_COUNT == 21, "Did you add a new type? You may need to handle it here.");

    struct D3DShaderVarTypeToShaderCodeBasicTypeMapping
    {
        D3DShaderVarTypeToShaderCodeBasicTypeMapping()
        {
            Data[D3D_SVT_VOID]                      = SHADER_CODE_BASIC_TYPE_VOID;
            Data[D3D_SVT_BOOL]                      = SHADER_CODE_BASIC_TYPE_BOOL;
            Data[D3D_SVT_INT]                       = SHADER_CODE_BASIC_TYPE_INT;
            Data[D3D_SVT_FLOAT]                     = SHADER_CODE_BASIC_TYPE_FLOAT;
            Data[D3D_SVT_STRING]                    = SHADER_CODE_BASIC_TYPE_STRING;
            Data[D3D_SVT_TEXTURE]                   = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE1D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE2D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE3D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURECUBE]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_SAMPLER]                   = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_SAMPLER1D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_SAMPLER2D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_SAMPLER3D]                 = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_SAMPLERCUBE]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_PIXELSHADER]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_VERTEXSHADER]              = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_PIXELFRAGMENT]             = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_VERTEXFRAGMENT]            = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_UINT]                      = SHADER_CODE_BASIC_TYPE_UINT;
            Data[D3D_SVT_UINT8]                     = SHADER_CODE_BASIC_TYPE_UINT8;
            Data[D3D_SVT_GEOMETRYSHADER]            = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RASTERIZER]                = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_DEPTHSTENCIL]              = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_BLEND]                     = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_BUFFER]                    = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_CBUFFER]                   = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TBUFFER]                   = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE1DARRAY]            = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE2DARRAY]            = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RENDERTARGETVIEW]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_DEPTHSTENCILVIEW]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE2DMS]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURE2DMSARRAY]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_TEXTURECUBEARRAY]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_HULLSHADER]                = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_DOMAINSHADER]              = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_INTERFACE_POINTER]         = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_COMPUTESHADER]             = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_DOUBLE]                    = SHADER_CODE_BASIC_TYPE_DOUBLE;
            Data[D3D_SVT_RWTEXTURE1D]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWTEXTURE1DARRAY]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWTEXTURE2D]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWTEXTURE2DARRAY]          = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWTEXTURE3D]               = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWBUFFER]                  = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_BYTEADDRESS_BUFFER]        = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWBYTEADDRESS_BUFFER]      = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_STRUCTURED_BUFFER]         = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_RWSTRUCTURED_BUFFER]       = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_APPEND_STRUCTURED_BUFFER]  = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_CONSUME_STRUCTURED_BUFFER] = SHADER_CODE_BASIC_TYPE_UNKNOWN;
            Data[D3D_SVT_MIN8FLOAT]                 = SHADER_CODE_BASIC_TYPE_MIN8FLOAT;
            Data[D3D_SVT_MIN10FLOAT]                = SHADER_CODE_BASIC_TYPE_MIN10FLOAT;
            Data[D3D_SVT_MIN16FLOAT]                = SHADER_CODE_BASIC_TYPE_MIN16FLOAT;
            Data[D3D_SVT_MIN12INT]                  = SHADER_CODE_BASIC_TYPE_MIN12INT;
            Data[D3D_SVT_MIN16INT]                  = SHADER_CODE_BASIC_TYPE_MIN16INT;
            Data[D3D_SVT_MIN16UINT]                 = SHADER_CODE_BASIC_TYPE_MIN16UINT;
        }

        std::array<SHADER_CODE_BASIC_TYPE, D3D_SVT_MIN16UINT + 1> Data;
    };

    static const D3DShaderVarTypeToShaderCodeBasicTypeMapping Mapping;
    return Mapping.Data[D3DVarType];
}

SHADER_CODE_VARIABLE_CLASS D3DShaderVariableClassToShaderCodeVaraibleClass(D3D_SHADER_VARIABLE_CLASS D3DVariableClass)
{
    static_assert(SHADER_CODE_VARIABLE_CLASS_COUNT == 6, "Did you add a new variable class? You may need to handle it here.");

    // clang-format off
    switch (D3DVariableClass)
    {
        case D3D_SVC_SCALAR:            return SHADER_CODE_VARIABLE_CLASS_SCALAR;
        case D3D_SVC_VECTOR:            return SHADER_CODE_VARIABLE_CLASS_VECTOR;
        case D3D_SVC_MATRIX_ROWS:       return SHADER_CODE_VARIABLE_CLASS_MATRIX_ROWS;
        case D3D_SVC_MATRIX_COLUMNS:    return SHADER_CODE_VARIABLE_CLASS_MATRIX_COLUMNS;
        case D3D_SVC_OBJECT:            return SHADER_CODE_VARIABLE_CLASS_UNKNOWN;
        case D3D_SVC_STRUCT:            return SHADER_CODE_VARIABLE_CLASS_STRUCT;
        case D3D_SVC_INTERFACE_CLASS:   return SHADER_CODE_VARIABLE_CLASS_UNKNOWN;
        case D3D_SVC_INTERFACE_POINTER: return SHADER_CODE_VARIABLE_CLASS_UNKNOWN;

        default:
            UNEXPECTED("Unknown variable class");
            return SHADER_CODE_VARIABLE_CLASS_UNKNOWN;
    }
    // clang-format on
}

} // namespace Diligent
