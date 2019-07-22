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
    public class ScriptRuntimeApiReloadableImpl : ScriptRuntimeApiImpl
    {
        public override PluginApplication LoadAssembly(string path, uint version)
        {
            if (!System.IO.File.Exists(path) || !path.EndsWith(".dll"))
                return null;

            path = VersionFile(path, version);

            if (path == null)
                return null;

            Assembly assembly;
            try
            {
                assembly = Assembly.LoadFile(path);
            }
            catch (Exception)
            {
                return null;
            }

            Type pluginType = assembly.GetTypes().First(t => t.IsClass && t.BaseType == typeof(PluginApplication));
            if (pluginType == null)
                return null;

            return Activator.CreateInstance(pluginType, Context.Instance) as PluginApplication;
        }

        public override bool VerifyAssembly(string path)
        {
            string expectName = typeof(PluginApplication).FullName;
            AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path);
            return plugin.MainModule.Types.Any(t => t.IsClass && t.BaseType != null && t.BaseType.FullName == expectName);
        }

        /// Returns a versioned path.
        private static string GetNewPluginPath(string path, uint version)
        {
            string dirName = Path.GetDirectoryName(path);
            string extension = Path.GetExtension(path);
            string baseName = Path.GetFileNameWithoutExtension(path);
            baseName = baseName.Substring(0, baseName.Length - version.ToString().Length);
            string newPath = $"{dirName}/{baseName}{version}{extension}";

            if (baseName.Length == 0 || Path.GetDirectoryName(path) != Path.GetDirectoryName(newPath))
                throw new ArgumentException($"Project name {Path.GetFileNameWithoutExtension(path)} is too short.");

            return newPath;
        }

        private string VersionFile(string path, uint version)
        {
            if (path == null)
                throw new ArgumentException($"{nameof(path)} may not be null.");

            AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path,
                new ReaderParameters { SymbolReaderProvider = new PdbReaderProvider(), ReadSymbols = true});
            plugin.Name.Version = new Version(plugin.Name.Version.Major, plugin.Name.Version.Minor,
                plugin.Name.Version.Build, (int)version);

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
            string newFilePath = GetNewPluginPath(path, version);
            string newPdbPath = GetNewPluginPath(pdbPath, version);
            byte[] newPdbPathBytes = Encoding.UTF8.GetBytes(newPdbPath);
            Array.Copy(newPdbPathBytes, 0, debugHeader.Data, 0x018, newPdbPathBytes.Length);
            debugHeader.Data[0x018 + newPdbPathBytes.Length] = 0;

            plugin.Write(newFilePath,
                new WriterParameters { SymbolWriterProvider = new PdbWriterProvider(), WriteSymbols = true});
            System.IO.File.Copy(pdbPath, newPdbPath, true);

            return newFilePath;
        }
    }
}
