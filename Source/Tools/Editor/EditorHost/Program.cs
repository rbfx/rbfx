//
// Copyright (c) 2017-2019 the rbfx project.
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
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using File = System.IO.File;
using Urho3DNet;

namespace EditorHost
{
    /// This class runs in a context of reloadable appdomain.
    internal class DomainManager : MarshalByRefObject, IDisposable
    {
        private Context _context;
        private readonly List<PluginApplication> _plugins = new List<PluginApplication>();
        private readonly int _version;

        public DomainManager(int version, IntPtr contextPtr)
        {
            _version = version;
            _context = Context.wrap(contextPtr, true);
        }

        public void Dispose()
        {
            _context.Dispose();
            _context = null;

            foreach (var plugin in _plugins)
            {
                plugin.Unload();
                plugin.Dispose();
            }

            _plugins.Clear();
            GC.Collect(GC.MaxGeneration);
            GC.WaitForPendingFinalizers();
        }

        /// Returns a versioned path.
        private string GetNewPluginPath(string path)
        {
            // Mimics PluginManager::GetTemporaryPluginPath().
            var tempPath = _context.GetFileSystem().GetTemporaryDir() + $"Urho3D-Editor-Plugins-{Process.GetCurrentProcess().Id}";
            var fileName = Path.GetFileNameWithoutExtension(path);
            path = $"{tempPath}/{fileName}{_version}.dll";
            return path;
        }

        private string VersionFile(string path)
        {
            if (path == null)
                throw new ArgumentException($"{nameof(path)} may not be null.");

            var pdbPath = Path.ChangeExtension(path, "pdb");
            var versionedPath = GetNewPluginPath(path);
            Directory.CreateDirectory(Path.GetDirectoryName(versionedPath));
            Debug.Assert(versionedPath != null, $"{nameof(versionedPath)} != null");
            File.Copy(path, versionedPath, true);

            if (File.Exists(pdbPath))
            {
                var versionedPdbPath = Path.ChangeExtension(versionedPath, "pdb");
                File.Copy(pdbPath, versionedPdbPath, true);
                versionedPdbPath = Path.GetFileName(versionedPdbPath);  // Do not include full path when patching dll

                // Update .pdb path in a newly copied file
                var pathBytes = Encoding.ASCII.GetBytes(Path.GetFileName(pdbPath)); // Does this work with i18n paths?
                var dllBytes = File.ReadAllBytes(versionedPath);
                int i;
                for (i = 0; i < dllBytes.Length; i++)
                {
                    if (dllBytes.Skip(i).Take(pathBytes.Length).SequenceEqual(pathBytes)) // I know its slow ¯\_(ツ)_/¯
                    {
                        if (dllBytes[i + pathBytes.Length] != 0) // Check for null terminator
                            continue;

                        while (dllBytes[--i] != 0)
                        {
                            // We found just a file name. Time to walk back to find a start of the string. This is
                            // required because dll is executing from non-original directory and pdb path points
                            // somewhere else we can not predict.
                        }

                        i++;

                        // Copy full pdb path
                        var newPathBytes = Encoding.ASCII.GetBytes(versionedPdbPath);
                        newPathBytes.CopyTo(dllBytes, i);
                        dllBytes[i + newPathBytes.Length] = 0;
                        break;
                    }
                }

                if (i == dllBytes.Length)
                    return null;

                File.WriteAllBytes(versionedPath, dllBytes);
            }

            return versionedPath;
        }

        public IntPtr LoadPlugin(string path)
        {
            if (!File.Exists(path) || !path.EndsWith(".dll"))
                return IntPtr.Zero;

            path = VersionFile(path);
            if (path == null)
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
            _plugins.Add(plugin);

            return PluginApplication.getCPtr(plugin).Handle;
        }
    }

    internal class Program
    {
        public static readonly string ProgramFile = new Uri(Assembly.GetExecutingAssembly().CodeBase).LocalPath;
        public static readonly string ProgramDirectory = Path.GetDirectoryName(ProgramFile);
        private Context _context;
        private int Version { get; set; } = -1;
        private DomainManager _manager;
        private AppDomain _pluginDomain;
        private static int _pluginCheckCounter;

        public Program()
        {
            InstallAssemblyLoader(AppDomain.CurrentDomain);
        }

        private static void InstallAssemblyLoader(AppDomain domain)
        {
            domain.ReflectionOnlyAssemblyResolve += (sender, args) => Assembly.ReflectionOnlyLoadFrom(
                Path.Combine(ProgramDirectory, args.Name.Substring(0, args.Name.IndexOf(',')) + ".dll"));
        }

        private void LoadRuntime()
        {
            Version++;
            var managerType = typeof(DomainManager);
            Debug.Assert(_pluginDomain == null && _manager == null && managerType.FullName != null);
            var domainName = AppDomain.CurrentDomain.FriendlyName.Replace(".", $"{Version}.");
            _pluginDomain = AppDomain.CreateDomain(domainName);
            _manager = _pluginDomain.CreateInstanceAndUnwrap(managerType.Assembly.FullName,
                managerType.FullName, false, BindingFlags.Default, null,
                new object[]{Version, Context.getCPtr(_context).Handle}, null, null) as DomainManager;
        }

        private void UnloadRuntime()
        {
            _manager.Dispose();
            _manager = null;
            AppDomain.Unload(_pluginDomain);    // TODO: This may fail.
            _pluginDomain = null;
        }

        private class PluginChecker : MarshalByRefObject
        {
            public bool IsPlugin(string path)
            {
                try
                {
                    var pluginBaseName = typeof(PluginApplication).FullName;
                    var assembly = Assembly.ReflectionOnlyLoadFrom(path);
                    var types = assembly.GetTypes();
                    return types.Any(type => type.BaseType?.FullName == pluginBaseName);
                }
                catch (Exception e)
                {
                    return false;
                }
            }
        }

        private static bool IsPlugin(string path)
        {
            try
            {
                var checkerType = typeof(PluginChecker);
                var tmpDomain = AppDomain.CreateDomain($"CheckPlugin{_pluginCheckCounter++}");
                InstallAssemblyLoader(tmpDomain);
                var checker = (PluginChecker)tmpDomain.CreateInstanceAndUnwrap(checkerType.Assembly.FullName, checkerType.FullName);
                var result = checker.IsPlugin(path);
                AppDomain.Unload(tmpDomain);
                return result;
            }
            catch (Exception e)
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
                case ScriptRuntimeCommand.LoadRuntime:
                    LoadRuntime();
                    break;
                case ScriptRuntimeCommand.UnloadRuntime:
                    UnloadRuntime();
                    break;
                case ScriptRuntimeCommand.LoadAssembly:
                {
                    var path = Marshal.PtrToStringAnsi(Marshal.ReadIntPtr(args, 0));    // TODO: utf-8
                    return _manager.LoadPlugin(path);
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

                using (var editor = Application.wrap(CreateEditorApplication(Context.getCPtr(_context).Handle), true))
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

        [DllImport("libEditor")]
        private static extern IntPtr CreateEditorApplication(IntPtr context);

        [DllImport("libEditor")]
        private static extern void ParseArgumentsC(int argc,
            [MarshalAs(UnmanagedType.LPArray, ArraySubType = UnmanagedType.LPStr)]string[] argv);
    }
}
