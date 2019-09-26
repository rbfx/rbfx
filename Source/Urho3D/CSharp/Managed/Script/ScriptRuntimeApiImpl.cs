using System;
using System.CodeDom.Compiler;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.CSharp;
using Urho3DNet.CSharp;

namespace Urho3DNet
{
    public class ScriptRuntimeApiImpl : ScriptRuntimeApi
    {
        private static readonly string ProgramFile = new Uri(Assembly.GetExecutingAssembly().CodeBase).LocalPath;
        private static readonly string ProgramDirectory = Path.GetDirectoryName(ProgramFile);
        private GCHandle _selfReference;

        public ScriptRuntimeApiImpl()
        {
            InstallAssemblyLoader(AppDomain.CurrentDomain);
            _selfReference = GCHandle.Alloc(this);
        }

        protected override void Dispose(bool disposing)
        {
            if (_selfReference.IsAllocated)
                _selfReference.Free();
            base.Dispose(disposing);
        }

        private static void InstallAssemblyLoader(AppDomain domain)
        {
            domain.ReflectionOnlyAssemblyResolve += (sender, args) => Assembly.ReflectionOnlyLoadFrom(
                Path.Combine(ProgramDirectory, args.Name.Substring(0, args.Name.IndexOf(',')) + ".dll"));
        }

        public override bool VerifyAssembly(string path)
        {
            // This method is called by player. Little point in verifying if file is a plugin because it would only
            // happen if someone tampered with application files manually. If this is not a plugin LoadAssembly() will
            // fail gracefully later anyway. And we get faster loading times.
            return true;
        }

        public override bool SetAssemblyVersion(string path, uint version)
        {
            throw new Exception("Assembly versioning is not supported in this build.");
        }

        public override PluginApplication CreatePluginApplication(int assembly)
        {
            var instance = GCHandle.FromIntPtr(new IntPtr(assembly)).Target as Assembly;
            if (instance == null)
                return null;

            Type pluginType = instance.GetTypes().First(t => t.IsClass && t.BaseType == typeof(PluginApplication));
            if (pluginType == null)
                return null;
            return Activator.CreateInstance(pluginType, Context.Instance) as PluginApplication;
        }

        public override int LoadAssembly(string path)
        {
            Assembly assembly;
            try
            {
                assembly = Assembly.LoadFile(path);
            }
            catch (Exception)
            {
                return 0;
            }
            return GCHandle.ToIntPtr(GCHandle.Alloc(assembly)).ToInt32();
        }

        public override void Dispose(RefCounted instance)
        {
            instance?.Dispose();
        }

        public override void FreeGCHandle(int handle)
        {
            GCHandle gcHandle = GCHandle.FromIntPtr(new IntPtr(handle));
            if (gcHandle.IsAllocated)
                gcHandle.Free();
        }

        public override int RecreateGCHandle(int handle, bool strong)
        {
            if (handle == 0)
                return 0;

            GCHandle gcHandle = GCHandle.FromIntPtr(new IntPtr(handle));
            if (gcHandle.Target != null)
            {
                GCHandle newHandle = GCHandle.Alloc(gcHandle.Target, strong ? GCHandleType.Normal : GCHandleType.Weak);
                GC.KeepAlive(gcHandle.Target);
                return GCHandle.ToIntPtr(newHandle).ToInt32();
            }
            if (gcHandle.IsAllocated)
                gcHandle.Free();
            return 0;
        }

        public override void FullGC()
        {
            GC.Collect();                    // Find garbage and queue finalizers.
            GC.WaitForPendingFinalizers();   // Run finalizers, release references to remaining unreferenced objects.
            GC.Collect();                    // Collect those finalized objects.
        }

        public override void ApplicationStart()
        {
            // TODO: This wont work with hot-reload.
            var sourceCode = new List<string>();
            var scriptFiles = new StringList();
            Context.Instance.Cache.Scan(scriptFiles, "Scripts/", "*.cs", Urho3D.ScanFiles, true);

            foreach (string fileName in scriptFiles)
            {
                File file = Context.Instance.Cache.GetFile($"Scripts/{fileName}");
                if (file != null)
                    sourceCode.Add(file.ReadText());
            }

            var csc = new CSharpCodeProvider();
            var compileParameters = new CompilerParameters(new[]    // TODO: User may need to extend this list
            {
                "mscorlib.dll",
                "System.dll",
                "System.Core.dll",
                "System.Data.dll",
                "System.Drawing.dll",
                "System.Numerics.dll",
                "System.Runtime.Serialization.dll",
                "System.Xml.dll",
                "System.Xml.Linq.dll",
                "Urho3DNet.dll",
            })
            {
                GenerateExecutable = false,
                GenerateInMemory = true,
                TreatWarningsAsErrors = false,
            };

            CompilerResults results = csc.CompileAssemblyFromSource(compileParameters, sourceCode.ToArray());
            if (results.Errors.HasErrors)
            {
                foreach (CompilerError error in results.Errors)
                    System.Console.WriteLine(error.ErrorText);    // TODO: Use Logging subsystem
            }
            else
            {
                foreach ((Type type, ObjectFactoryAttribute attr) in results.CompiledAssembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                    Context.Instance.RegisterFactory(type, attr.Category);
            }
        }
    }
}
