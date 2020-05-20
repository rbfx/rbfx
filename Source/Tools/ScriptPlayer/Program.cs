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
