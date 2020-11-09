//
// Copyright (c) 2017-2020 the rbfx project.
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
using Urho3DNet;
using Mono.Cecil;
using Mono.Cecil.Pdb;

namespace Editor
{
    public class ScriptRuntimeApiReloadableImpl : CompiledScriptRuntimeApiImpl
    {
        public override bool VerifyAssembly(string path)
        {
            string expectName = typeof(PluginApplication).FullName;
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
