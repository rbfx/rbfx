using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;
#if NETFRAMEWORK
using Microsoft.CSharp;
#else
using System.IO;
using Microsoft.CodeAnalysis;
using Microsoft.CodeAnalysis.CSharp;
using Microsoft.CodeAnalysis.Emit;
#endif

namespace Urho3DNet
{
    public class CsCompiler
    {
        public static Assembly FindAndCompile(string scanPath, IEnumerable<string> extraReferences)
        {
            var scriptRsrcs = new StringList();
            var scriptCodes = new List<string>();
            var sourceFiles = new List<string>();
            Context.Instance.Cache.Scan(scriptRsrcs, scanPath, "*.cs", Urho3D.ScanFiles, true);
            foreach (string fileName in scriptRsrcs)
            {
                using (File file = Context.Instance.Cache.GetFile(fileName))
                {
                    // Gather both paths and code text here. If scripts are packaged we must compile them from
                    // text form. However if we are running a development version of application we prefer to
                    // compile scripts directly from file because then we get proper error locations.
                    if (file.IsPackaged())
                        scriptCodes.Add(file.ReadString());
                    else
                    {
                        string path = Context.Instance.Cache.GetResourceFileName(fileName);
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
#if NETFRAMEWORK
            var csc = new CSharpCodeProvider();

            var metadataReferences = new List<string>();
            metadataReferences.AddRange(AppDomain.CurrentDomain.GetAssemblies()
                .Where(assembly => assembly.Location.EndsWith(".dll"))
                .Select(assembly => assembly.Location));
            if (extraReferences != null)
                metadataReferences.AddRange(extraReferences);

            var compileParameters = new CompilerParameters(metadataReferences.ToArray())
            {
                GenerateExecutable = false,
                GenerateInMemory = true,
                TreatWarningsAsErrors = false,
            };

            CompilerResults results = null;
            if (scriptCodes?.Count() > 0)
                results = csc.CompileAssemblyFromSource(compileParameters, scriptCodes.ToArray());
            else if (sourceFiles?.Count() > 0)
                results = csc.CompileAssemblyFromFile(compileParameters, sourceFiles.ToArray());

            if (results != null && results.Errors.HasErrors)
            {
                foreach (CompilerError error in results.Errors)
                {
                    string resourceName = error.FileName;
                    // ressource paths use lower key c: and '/' as separator and but Windows uses upper key C: and '\'
                    // using Uri to mitigate that
                    var resourcePath = new Uri(resourceName);
                    foreach (string resourceDir in Context.Instance.Cache.ResourceDirs)
                    {
                        var resourceDirPath = new Uri(resourceDir);
                        if (resourceDirPath.IsBaseOf( resourcePath) )
                        {
                            resourceName = resourceDirPath.MakeRelativeUri(resourcePath).ToString();
                            break;
                        }
                    }

                    string message = $"{resourceName}:{error.Line}:{error.Column}: {error.ErrorText}";
                    if (error.IsWarning)
                    { 
                        Log.Warning(message);
                    }
                    else
                    {
                        Log.Error(message);
                        // no compiled assembly is generated when there's an error !
                        return null;
                    }
                }
            }

            return results?.CompiledAssembly;
#else
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
                        foreach (string resourceDir in Context.Instance.Cache.ResourceDirs)
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
#endif
        }
    }
}
