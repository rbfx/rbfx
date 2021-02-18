using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
using System.IO;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Emit;

namespace Urho3DNet
{
    public class CsCompiler
    {
        public static Assembly FindAndCompile(string scanPath, IEnumerable<string> extraReferences)
        {
            var scriptRsrcs = new StringList();
            var scriptCodes = new List<string>();
            var sourceFiles = new List<string>();
            Context.Instance.ResourceCache.Scan(scriptRsrcs, scanPath, "*.cs", Urho3D.ScanFiles, true);
            foreach (string fileName in scriptRsrcs)
            {
                using (File file = Context.Instance.ResourceCache.GetFile(fileName))
                {
                    // Gather both paths and code text here. If scripts are packaged we must compile them from
                    // text form. However if we are running a development version of application we prefer to
                    // compile scripts directly from file because then we get proper error locations.
                    if (file.IsPackaged)
                        scriptCodes.Add(file.ReadString());
                    else
                    {
                        string path = Context.Instance.ResourceCache.GetResourceFileName(fileName);
                        path = Urho3D.GetAbsolutePath(path);
                        path = Urho3D.GetNativePath(path);
                        sourceFiles.Add(path);
                    }
                }
            }
            return Compile(sourceFiles, scriptCodes, extraReferences);
        }

        public static Assembly Compile(IEnumerable<string> sourceFiles, IEnumerable<string> scriptCodes, IEnumerable<string> extraReferences)
        {
            var syntaxTrees = new List<SyntaxTree>();

            if (scriptCodes != null)
            {
                foreach (string code in scriptCodes)
                    syntaxTrees.Add(CSharpSyntaxTree.ParseText(code));
            }

            if (sourceFiles != null)
            {
                foreach (string file in sourceFiles)
                    syntaxTrees.Add(CSharpSyntaxTree.ParseText(System.IO.File.ReadAllText(file), path: file));
            }

            var metadataReferences = new List<MetadataReference>();
            // Reference all current referenced modules.
            metadataReferences.AddRange(AppDomain.CurrentDomain.GetAssemblies().Select(assembly => MetadataReference.CreateFromFile(assembly.Location)));
            // Add custom references if any.
            if (extraReferences != null)
                metadataReferences.AddRange(extraReferences.Select(r => MetadataReference.CreateFromFile(r)).ToArray());

            var compilation = CSharpCompilation.Create(Path.GetRandomFileName(), syntaxTrees.ToArray(),
                metadataReferences, new CSharpCompilationOptions(OutputKind.DynamicallyLinkedLibrary));

            using (var stream = new MemoryStream())
            {
                EmitResult result = compilation.Emit(stream);
                if (result.Success)
                {
                    stream.Seek(0, SeekOrigin.Begin);
                    return Assembly.Load(stream.GetBuffer());
                }

                IEnumerable<Diagnostic> failures = result.Diagnostics.Where(diagnostic => diagnostic.IsWarningAsError || diagnostic.Severity == DiagnosticSeverity.Error);
                foreach (Diagnostic diagnostic in failures)
                {
                    string resourceName = diagnostic.Location.SourceTree?.FilePath;
                    if (resourceName != null)
                    {
                        foreach (string resourceDir in Context.Instance.ResourceCache.ResourceDirs)
                        {
                            if (resourceName.StartsWith(resourceDir))
                            {
                                resourceName = resourceName.Substring(resourceDir.Length);
                                break;
                            }
                        }
                    }
                    else
                        resourceName = "?";

                    string message = $"{resourceName}:{diagnostic.Location.GetLineSpan().StartLinePosition.Line}:{diagnostic.Location.GetLineSpan().StartLinePosition.Character}: {diagnostic.GetMessage()}";
                    switch (diagnostic.Severity)
                    {
                        case DiagnosticSeverity.Info:
                            Log.Info(message);
                            break;
                        case DiagnosticSeverity.Warning:
                            Log.Warning(message);
                            break;
                        case DiagnosticSeverity.Error:
                            Log.Error(message);
                            break;
                        default:
                            break;
                    }
                }
            }
            return null;
        }
    }
}
