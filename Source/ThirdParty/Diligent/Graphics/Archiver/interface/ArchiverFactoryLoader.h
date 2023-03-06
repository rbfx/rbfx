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

#include "ArchiverFactory.h"

#if PLATFORM_ANDROID || PLATFORM_LINUX || PLATFORM_MACOS || PLATFORM_IOS || PLATFORM_TVOS || (PLATFORM_WIN32 && !defined(_MSC_VER))
// https://gcc.gnu.org/wiki/Visibility
#    define API_QUALIFIER __attribute__((visibility("default")))
#elif PLATFORM_WIN32 || PLATFORM_UNIVERSAL_WINDOWS
#    define API_QUALIFIER
#else
#    error Unsupported platform
#endif

#if ENGINE_DLL && PLATFORM_WIN32 && defined(_MSC_VER)
#    include "../../GraphicsEngine/interface/LoadEngineDll.h"
#    define EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL 1
#endif

DILIGENT_BEGIN_NAMESPACE(Diligent)

#if EXPLICITLY_LOAD_ARCHIVER_FACTORY_DLL

typedef struct IArchiverFactory* (*GetArchiverFactoryType)();

inline GetArchiverFactoryType DILIGENT_GLOBAL_FUNCTION(LoadArchiverFactory)()
{
    return (GetArchiverFactoryType)LoadEngineDll("Archiver", "GetArchiverFactory");
}

#else

API_QUALIFIER
struct IArchiverFactory* DILIGENT_GLOBAL_FUNCTION(GetArchiverFactory)();

#endif

DILIGENT_END_NAMESPACE // namespace Diligent
