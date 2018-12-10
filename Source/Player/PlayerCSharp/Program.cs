//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

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
        private static readonly string ProgramFile = new Uri(Assembly.GetExecutingAssembly().CodeBase).LocalPath;
        public static readonly string ProgramDirectory = Path.GetDirectoryName(ProgramFile);
        private Context _context;

        private Program()
        {
            AppDomain.CurrentDomain.ReflectionOnlyAssemblyResolve += (sender, args) => Assembly.ReflectionOnlyLoadFrom(
                Path.Combine(ProgramDirectory, args.Name.Substring(0, args.Name.IndexOf(',')) + ".dll"));
        }

        private IntPtr LoadPlugin(string path)
        {
            if (!File.Exists(path) || !path.EndsWith(".dll"))
                return IntPtr.Zero;

            Assembly assembly;
            try
            {
                assembly = Assembly.LoadFile(path);
            }
            catch (Exception)
            {
                return IntPtr.Zero;
            }

            Type pluginType = null;
            foreach (var type in assembly.GetTypes())
            {
                if (!type.IsClass)
                    continue;

                if (type.BaseType == typeof(PluginApplication))
                {
                    pluginType = type;
                    break;
                }
            }

            if (pluginType == null)
                return IntPtr.Zero;

            var plugin = Activator.CreateInstance(pluginType, _context) as PluginApplication;
            if (plugin is null)
                return IntPtr.Zero;

            plugin.Load();

            return PluginApplication.getCPtr(plugin).Handle;
        }
        private static bool IsPlugin(string path)
        {
            var pluginBaseName = typeof(PluginApplication).FullName;
            try
            {
                var assembly = Assembly.ReflectionOnlyLoadFrom(path);
                var types = assembly.GetTypes();
                return types.Any(type => type.BaseType?.FullName == pluginBaseName);
            }
            catch (Exception)
            {
                return false;
            }
        }

        /// Keeps delegate pointer alive.
        private CommandHandlerDelegate _commandHandlerRef;
        private IntPtr CommandHandler(ScriptRuntimeCommand command, IntPtr args)
        {
            switch (command)
            {
                case ScriptRuntimeCommand.LoadAssembly:
                {
                    var path = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(args, 0));    // TODO: utf-8
                    return LoadPlugin(path);
                }
                case ScriptRuntimeCommand.VerifyAssembly:
                {
                    var path = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(args, 0));    // TODO: utf-8
                    if (IsPlugin(path))
                        return Urho3D.ScriptCommandSuccess;
                    break;
                }
            }
            return Urho3D.ScriptCommandFailed;
        }

        private void Run(string[] args)
        {
            var argc = args.Length + 1;                 // args + executable path
            var argv = new string[args.Length + 2];     // args + executable path + null
            argv[0] = ProgramFile;
            args.CopyTo(argv, 1);
            ParseArgumentsC(argc, argv);

            using (_context = new Context())
            {
                _commandHandlerRef = CommandHandler;
                _context.RegisterSubsystem(new Script(_context));
                _context.GetSubsystem<Script>().RegisterCommandHandler(
                    (int) ScriptRuntimeCommand.LoadRuntime, (int) ScriptRuntimeCommand.VerifyAssembly,
                    Marshal.GetFunctionPointerForDelegate(_commandHandlerRef));

                using (var editor = Application.wrap(CreateApplication(Context.getCPtr(_context).Handle), true))
                {
                    Environment.ExitCode = editor.Run();
                }
            }
        }

        [STAThread]
        public static void Main(string[] args)
        {
            new Program().Run(args);
        }

        [DllImport("libPlayer", CallingConvention = CallingConvention.Cdecl)]
        private static extern IntPtr CreateApplication(IntPtr context);

        [DllImport("libPlayer", CallingConvention = CallingConvention.Cdecl)]
        private static extern void ParseArgumentsC(int argc,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]string[] argv);
    }
}
