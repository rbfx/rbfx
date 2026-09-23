// Copyright (c) 2021-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    public partial class Application
    {
        public delegate IntPtr UserApplicationFactory(HandleRef context);
        public static Application CreateApplicationFromFactory(Context context, UserApplicationFactory factory)
        {
            return Application.wrap(factory(Context.getCPtr(context)), true);
        }

        internal IntPtr ExternalWindow
        {
            set
            {
                EngineParameters[Urho3D.EpExternalWindow] = value;
            }
        }
    }
}
