// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Reflection;
using System.Runtime.InteropServices;
using Urho3DNet;

namespace Editor
{
    internal class Program
    {
        private void Run(string[] args)
        {
            Urho3D.ParseArguments(Assembly.GetExecutingAssembly(), args);
            Context.SetRuntimeApi(new ScriptRuntimeApiReloadableImpl());
            using (var context = new Context())
            {
                context.AddRef();
                using (Application editor = Application.CreateApplicationFromFactory(context, CreateApplication))
                {
                    editor.AddRef();
                    Environment.ExitCode = editor.Run();
                    editor.ReleaseRef();
                }
                context.ReleaseRef();
            }
        }

        [STAThread]
        public static void Main(string[] args)
        {
            new Program().Run(args);
        }

        [DllImport("libEditorWrapper")]
        private static extern IntPtr CreateApplication(HandleRef context);
    }
}
