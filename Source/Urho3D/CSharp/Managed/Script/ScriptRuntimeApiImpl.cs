using System;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;

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
            instance?.DisposeInternal();
        }

        public override void FreeGCHandle(IntPtr handle)
        {
            GCHandle gcHandle = GCHandle.FromIntPtr(handle);
            if (gcHandle.IsAllocated)
                gcHandle.Free();
        }

        public override IntPtr CloneGCHandle(IntPtr handle)
        {
            GCHandle gcHandle = GCHandle.FromIntPtr(handle);
            if (gcHandle.IsAllocated)
                return GCHandle.ToIntPtr(GCHandle.Alloc(gcHandle.Target));
            return IntPtr.Zero;
        }

        public override IntPtr RecreateGCHandle(IntPtr handle, bool strong)
        {
            if (handle == IntPtr.Zero)
                return IntPtr.Zero;

            GCHandle gcHandle = GCHandle.FromIntPtr(handle);
            if (gcHandle.Target != null)
            {
                GCHandle newHandle = GCHandle.Alloc(gcHandle.Target, strong ? GCHandleType.Normal : GCHandleType.Weak);
                GC.KeepAlive(gcHandle.Target);
                return GCHandle.ToIntPtr(newHandle);
            }
            if (gcHandle.IsAllocated)
                gcHandle.Free();
            return IntPtr.Zero;
        }

        public override void FullGC()
        {
            GC.Collect();                    // Find garbage and queue finalizers.
            GC.WaitForPendingFinalizers();   // Run finalizers, release references to remaining unreferenced objects.
            GC.Collect();                    // Collect those finalized objects.
        }

        public override PluginApplication CompileResourceScriptPlugin()
        {
            // Empty plugin. Essentially a noop.
            return new RuntimeCompiledScriptPluginApplication(Context.Instance);
        }
    }
}
