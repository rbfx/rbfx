using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;

namespace Urho3DNet
{
    /// This class runs in a context of reloadable appdomain.
    internal class DomainManager : MarshalByRefObject, IDisposable
    {
        private Context _context;
        private readonly int _version;
        private readonly List<PluginApplication> _plugins = new List<PluginApplication>();

        public DomainManager(int version, IntPtr contextPtr)
        {
            _context = Context.wrap(contextPtr, false);
            _version = version;
        }

        public void Dispose()
        {
            foreach (PluginApplication plugin in _plugins)
            {
                plugin.Unload();
                plugin.Dispose();
            }
            _plugins.Clear();

            _context.Dispose();
            _context = null;

            GC.Collect(GC.MaxGeneration);
            GC.WaitForPendingFinalizers();
        }

        public IntPtr LoadAssembly(string path)
        {
            if (!System.IO.File.Exists(path) || !path.EndsWith(".dll"))
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

            Type pluginType = assembly.GetTypes().First(t => t.IsClass && t.BaseType == typeof(PluginApplication));
            if (pluginType == null)
                return IntPtr.Zero;

            var plugin = Activator.CreateInstance(pluginType, _context) as PluginApplication;
            if (plugin is null)
                return IntPtr.Zero;

            plugin.Load();
            _plugins.Add(plugin);

            return PluginApplication.getCPtr(plugin).Handle;
        }

        /// Returns a versioned path.
        private string GetNewPluginPath(string path)
        {
            // Mimics PluginManager::GetTemporaryPluginPath().
            string tempPath = _context.GetFileSystem().GetTemporaryDir() +
                              $"Urho3D-Editor-Plugins-{Process.GetCurrentProcess().Id}";
            string fileName = Path.GetFileNameWithoutExtension(path);
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
            System.IO.File.Copy(path, versionedPath, true);

            if (System.IO.File.Exists(pdbPath))
            {
                var versionedPdbPath = Path.ChangeExtension(versionedPath, "pdb");
                System.IO.File.Copy(pdbPath, versionedPdbPath, true);
                versionedPdbPath = Path.GetFileName(versionedPdbPath);  // Do not include full path when patching dll

                // Update .pdb path in a newly copied file
                var pathBytes = Encoding.ASCII.GetBytes(Path.GetFileName(pdbPath)); // Does this work with i18n paths?
                var dllBytes = System.IO.File.ReadAllBytes(versionedPath);
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

                System.IO.File.WriteAllBytes(versionedPath, dllBytes);
            }

            return versionedPath;
        }
    }

    public class ScriptRuntimeApiReloadableImpl : ScriptRuntimeApiImpl
    {
        private int Version { get; set; } = -1;
        internal DomainManager _manager;
        protected AppDomain _pluginDomain;

        public ScriptRuntimeApiReloadableImpl(Context context) : base(context)
        {
        }

        public override bool LoadRuntime()
        {
            Type managerType = typeof(DomainManager);

            if (_pluginDomain != null || _manager != null || managerType.FullName == null)
                return false;

            Version++;
            string domainName = AppDomain.CurrentDomain.FriendlyName.Replace(".", $"{Version}.");
            _pluginDomain = AppDomain.CreateDomain(domainName);
            _manager = _pluginDomain.CreateInstanceAndUnwrap(managerType.Assembly.FullName,
                managerType.FullName, false, BindingFlags.Default, null,
                new object[]{Version, Context.getCPtr(GetContext()).Handle}, null, null) as DomainManager;

            return true;
        }

        public override bool UnloadRuntime()
        {
            if (_pluginDomain == null || _manager == null)
                return false;

            _manager.Dispose();
            _manager = null;
            AppDomain.Unload(_pluginDomain);    // TODO: This may fail.
            _pluginDomain = null;
            return true;
        }

        public override PluginApplication LoadAssembly(string path)
        {
            if (_manager == null)
                return null;

            IntPtr pluginInstance = _manager.LoadAssembly(path);
            return PluginApplication.wrap(pluginInstance, false);
        }
    }
}
