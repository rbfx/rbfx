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

#ifdef DILIGENT_ENABLE_D3D_NVAPI
#    include "nvapi.h"
#endif

namespace Diligent
{

class NVApiLoader final
{
public:
    bool Load()
    {
#ifdef DILIGENT_ENABLE_D3D_NVAPI
        m_NVApiLoaded = (NvAPI_Initialize() == NVAPI_OK);
#endif
        return IsLoaded();
    }

    void Unload()
    {
#ifdef DILIGENT_ENABLE_D3D_NVAPI
        if (m_NVApiLoaded)
        {
            // NB: NVApi must be unloaded only after the last reference to ID3D11Device has
            //     been released, otherwise ID3D11Device::Release will crash.
            NvAPI_Unload();
            m_NVApiLoaded = false;
        }
#else
        VERIFY_EXPR(!m_NVApiLoaded);
#endif
    }

    ~NVApiLoader()
    {
        Unload();
    }

    void Invalidate()
    {
        m_NVApiLoaded = false;
    }

    bool IsLoaded() const
    {
        return m_NVApiLoaded;
    }

    explicit operator bool() const
    {
        return IsLoaded();
    }

private:
    bool m_NVApiLoaded = false;
};

} // namespace Diligent
