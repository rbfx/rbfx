// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using Urho3DNet;
using Mono.Cecil;
using Mono.Cecil.Pdb;

namespace Editor
{
    public class ScriptRuntimeApiReloadableImpl : CompiledScriptRuntimeApiImpl
    {
        public override bool VerifyAssembly(string path)
        {
            string expectName = typeof(Plugin).FullName;
            AssemblyDefinition plugin = AssemblyDefinition.ReadAssembly(path);
            foreach (var pair in GetTypesWithAttribute<LoadablePluginAttribute>(plugin))
            {
                if (pair.Item1.BaseType != null && pair.Item1.BaseType.FullName == expectName)
                    return true;
            }
            return false;
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

                plugin.Write(path, new WriterParameters {SymbolWriterProvider = new PdbWriterProvider(), WriteSymbols = true});
            }
            catch (Exception)
            {
                return false;
            }

            return true;
        }

        private static IEnumerable<Tuple<TypeDefinition, CustomAttribute>> GetTypesWithAttribute<T>(AssemblyDefinition assembly)
        {
            foreach (var type in assembly.MainModule.Types)
            {
                foreach (var attribute in type.CustomAttributes)
                {
                    if (attribute.AttributeType.FullName == typeof(T).FullName)
                        yield return new Tuple<TypeDefinition, CustomAttribute>(type, attribute);
                }
            }
        }

    }
}
