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

using System;
using System.Collections.Generic;

namespace Diligent;

public static partial class Native
{
    private static readonly Dictionary<Type, Func<IntPtr>> Callbacks = new() {
                { typeof(IEngineFactoryD3D11),  GetEngineFactoryD3D11_},
                { typeof(IEngineFactoryD3D12),  GetEngineFactoryD3D12_ },
                { typeof(IEngineFactoryVk),     GetEngineFactoryVk_ },
                { typeof(IEngineFactoryOpenGL), GetEngineFactoryOpenGL_ }
            };

    public static T CreateEngineFactory<T>() where T : IEngineFactory
    {
        if (!Callbacks.TryGetValue(typeof(T), out var func))
            throw new ArgumentException($"Unexpected type: {typeof(T).Name}");

        var handle = func();
        return handle != IntPtr.Zero ? (T)Activator.CreateInstance(typeof(T), handle) : null;
    }
}
