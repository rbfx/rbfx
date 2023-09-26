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

namespace Diligent
{

template <typename BufferViewImplType>
bool VerifyBufferViewModeD3D(BufferViewImplType* pViewD3D, const D3DShaderResourceAttribs& Attribs, const char* ShaderName)
{
    if (pViewD3D == nullptr)
        return true;

    const auto& ViewDesc = pViewD3D->GetDesc();
    const auto& BuffDesc = pViewD3D->GetBuffer()->GetDesc();

    auto LogBufferBindingError = [&](const char* Msg) //
    {
        LOG_ERROR_MESSAGE("Buffer view '", ViewDesc.Name, "' of buffer '", BuffDesc.Name,
                          "' bound to shader variable '", Attribs.Name, "' in shader '", ShaderName, "' is invalid: ", Msg);
    };

    switch (Attribs.GetInputType())
    {
        case D3D_SIT_TEXTURE:
        case D3D_SIT_UAV_RWTYPED:
            if (BuffDesc.Mode != BUFFER_MODE_FORMATTED || ViewDesc.Format.ValueType == VT_UNDEFINED)
            {
                LogBufferBindingError("formatted buffer view is expected.");
                return false;
            }
            break;

        case D3D_SIT_STRUCTURED:
        case D3D_SIT_UAV_RWSTRUCTURED:
            if (BuffDesc.Mode != BUFFER_MODE_STRUCTURED)
            {
                LogBufferBindingError("structured buffer view is expected.");
                return false;
            }
            break;

        case D3D_SIT_BYTEADDRESS:
        case D3D_SIT_UAV_RWBYTEADDRESS:
            if (BuffDesc.Mode != BUFFER_MODE_RAW)
            {
                LogBufferBindingError("raw buffer view is expected.");
                return false;
            }
            break;
    }

    return true;
}

} // namespace Diligent
