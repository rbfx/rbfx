// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using File = System.IO.File;
using Urho3DNet;

namespace Player
{
    internal class Program
    {
        private Program()
        {
        }

        private void Run(string[] args)
        {
            Urho3D.ParseArguments(Assembly.GetExecutingAssembly(), args);

            // TODO: iOS does not allow runtime-compiled code.
            Context.SetRuntimeApi(new CompiledScriptRuntimeApiImpl());
            using (var context = new Context())
            {
                using (var player = Application.CreateApplicationFromFactory(context, CreateApplication))
                {
                    Environment.ExitCode = player.Run();
                }
            }
        }

        [STAThread]
        public static void Main(string[] args)
        {
            new Program().Run(args);
        }

        [DllImport("libPlayer")]
        private static extern IntPtr CreateApplication(HandleRef context);
    }
}
