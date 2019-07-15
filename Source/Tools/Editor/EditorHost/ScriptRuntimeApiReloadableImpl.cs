using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using Urho3DNet;
using Mono.Cecil;
using Mono.Cecil.Cil;
using Mono.Cecil.Pdb;

namespace EditorHost
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
            string version = _version.ToString();
            string dirName = Path.GetDirectoryName(path);
            string extension = Path.GetExtension(path);
            string baseName = Path.GetFileNameWithoutExtension(path);
            baseName = baseName.Substring(0, baseName.Length - version.Length);
            string newPath = $"{dirName}/{baseName}{version}{extension}";

            if (baseName.Length == 0 || Path.GetDirectoryName(path) != Path.GetDirectoryName(newPath))
                throw new ArgumentException($"Project name {Path.GetFileNameWithoutExtension(path)} is too short.");

            return newPath;
        }

        private string VersionFile(string path)
        {
            if (path == null)
                throw new ArgumentException($"{nameof(path)} may not be null.");

            AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path,
                new ReaderParameters { SymbolReaderProvider = new PdbReaderProvider(), ReadSymbols = true});
            plugin.Name.Version = new Version(plugin.Name.Version.Major, plugin.Name.Version.Minor,
                plugin.Name.Version.Build, _version);

            ImageDebugHeaderEntry debugHeader = plugin.MainModule.GetDebugHeader().Entries
                .First(h => h.Directory.Type == ImageDebugType.CodeView);

            // 000 uint magic;            // 0x53445352
            // 004 uint[16] signature;
            // 014 uint age;
            // 018 string pdbPath;
            uint magic = BitConverter.ToUInt32(debugHeader.Data, 0);
            if (magic != 0x53445352)
                return null;

            var pdbEnd = 0x018;
            while (debugHeader.Data[++pdbEnd] != 0){}

            string pdbPath = Encoding.UTF8.GetString(debugHeader.Data, 0x018, pdbEnd - 0x018);
            string newFilePath = GetNewPluginPath(path);
            string newPdbPath = GetNewPluginPath(pdbPath);
            byte[] newPdbPathBytes = Encoding.UTF8.GetBytes(newPdbPath);
            Array.Copy(newPdbPathBytes, 0, debugHeader.Data, 0x018, newPdbPathBytes.Length);
            debugHeader.Data[0x018 + newPdbPathBytes.Length] = 0;

            plugin.Write(newFilePath,
                new WriterParameters { SymbolWriterProvider = new PdbWriterProvider(), WriteSymbols = true});
            System.IO.File.Copy(pdbPath, newPdbPath, true);

            return newFilePath;
        }
    }

    public class ScriptRuntimeApiReloadableImpl : ScriptRuntimeApiImpl
    {
        private int Version { get; set; } = -1;
        private DomainManager _manager;
        private AppDomain _pluginDomain;

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
