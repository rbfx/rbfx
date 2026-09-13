// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Reflection;
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    [System.Security.SuppressUnmanagedCodeSecurity]
    public partial class Urho3D
    {
        public static void ParseArguments(Assembly program, string[] args)
        {
            int argc = args.Length + 1;                 // args + executable path
            var argv = new string[args.Length + 2];     // args + executable path + null
            argv[0] = new Uri(program.Location).LocalPath;
            args.CopyTo(argv, 1);
            Urho3D.ParseArgumentsInternal(argc, argv);
        }

        [DllImport(global::Urho3DNet.Urho3DPINVOKE.DllImportModule, EntryPoint="Urho3D_ParseArguments")]
        private static extern void ParseArgumentsInternal(int argc,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]string[] argv);
    }
}
