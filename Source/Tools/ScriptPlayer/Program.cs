// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Collections.Generic;
using System.CodeDom.Compiler;
using System.IO;
using System.Linq;
using System.Reflection;
using Microsoft.CSharp;
using Urho3DNet;
using Console = System.Console;
using File = System.IO.File;

public static class Program
{
    public static void Main(string[] args)
    {
        if (args.Length < 1)
        {
            Console.WriteLine("Usage: ScriptPlayer.exe /path/to/script.cs --extra --parameters");
            Console.WriteLine("  Script must contain static void Main(string[] args) method.");
            Console.WriteLine("  Extra parameters will be passed to compiled script.");
        }
        var sourceCode = new List<string>();
        string scriptDirectory;
        if (File.Exists(args[0]))
            scriptDirectory = Path.GetDirectoryName(Path.GetFullPath(args[0]));
        else if (Directory.Exists(args[0]))
            scriptDirectory = Path.GetFullPath(args[0]);
        else
        {
            Console.WriteLine($"File or directory '{args[0]}' does not exist.");
            return;
        }
        var scriptDirInfo = new DirectoryInfo(scriptDirectory);
        foreach (FileInfo fi in scriptDirInfo.GetFiles("*.cs", SearchOption.AllDirectories))
            sourceCode.Add(File.ReadAllText(fi.FullName));

        Assembly assembly = CsCompiler.Compile(sourceCode, null, null);
        if (assembly != null)
        {
            foreach (Type type in assembly.GetTypes())
            {
                foreach (MethodInfo method in type.GetMethods())
                {
                    if (!method.IsStatic || method.Name != "Main" || method.ReturnType != typeof(void))
                        continue;

                    ParameterInfo[] parameters = method.GetParameters();
                    if (parameters.Length != 1 || parameters[0].ParameterType != typeof(string[]))
                        continue;

                    var scriptArgs = new string[args.Length - 1];
                    Array.Copy(args, 1, scriptArgs, 0, args.Length - 1);
                    method.Invoke(null, new object[] {scriptArgs});
                    return;
                }
            }
        }
    }
}
