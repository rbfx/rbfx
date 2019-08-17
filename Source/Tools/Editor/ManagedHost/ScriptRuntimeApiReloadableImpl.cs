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

namespace Editor
{
    public class ScriptRuntimeApiReloadableImpl : ScriptRuntimeApiImpl
    {
        public override bool VerifyAssembly(string path)
        {
            string expectName = typeof(PluginApplication).FullName;
            AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path);
            return plugin.MainModule.Types.Any(t => t.IsClass && t.BaseType != null && t.BaseType.FullName == expectName);
        }

        public override bool SetAssemblyVersion(string path, uint version)
        {
            if (path == null)
                throw new ArgumentException($"{nameof(path)} may not be null.");

            try
            {
                AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path,
                    new ReaderParameters {SymbolReaderProvider = new PdbReaderProvider(), ReadSymbols = true});
                plugin.Name.Version = new Version(plugin.Name.Version.Major, plugin.Name.Version.Minor,
                    plugin.Name.Version.Build, (int) version);

                plugin.Write(path,
                    new WriterParameters {SymbolWriterProvider = new PdbWriterProvider(), WriteSymbols = true});
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }
    }
}
