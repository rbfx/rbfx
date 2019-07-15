using System;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Urho3DNet
{
    internal class TypePresenceVerifier : MarshalByRefObject
    {
        public bool ContainsType(string path, string typeFullName)
        {
            try
            {
                Assembly assembly = Assembly.ReflectionOnlyLoadFrom(path);
                Type[] types = assembly.GetTypes();
                return types.Any(type => type.BaseType?.FullName == typeFullName);
            }
            catch (Exception)
            {
                return false;
            }
        }
    }

    public class ScriptRuntimeApiImpl : ScriptRuntimeApi
    {
        private static readonly string ProgramFile = new Uri(Assembly.GetExecutingAssembly().CodeBase).LocalPath;
        private static readonly string ProgramDirectory = Path.GetDirectoryName(ProgramFile);
        private static int _pluginCheckCounter;

        public ScriptRuntimeApiImpl(Context context) : base(context)
        {
            InstallAssemblyLoader(AppDomain.CurrentDomain);
        }

        private static void InstallAssemblyLoader(AppDomain domain)
        {
            domain.ReflectionOnlyAssemblyResolve += (sender, args) => Assembly.ReflectionOnlyLoadFrom(
                Path.Combine(ProgramDirectory, args.Name.Substring(0, args.Name.IndexOf(',')) + ".dll"));
        }
        public override bool LoadRuntime()
        {
            // Noop. Runtime is already loaded. Plugins will be executing in main appdomain.
            return true;
        }

        public override bool UnloadRuntime()
        {
            // Noop. Main appdomain can not be unloaded.
            return true;
        }

        public override bool VerifyAssembly(string path)
        {
            AppDomain tmpDomain = null;
            try
            {
                Type checkerType = typeof(TypePresenceVerifier);
                tmpDomain = AppDomain.CreateDomain($"TypePresenceVerifier{_pluginCheckCounter++}");
                InstallAssemblyLoader(tmpDomain);
                if (tmpDomain.CreateInstanceAndUnwrap(checkerType.Assembly.FullName,
                    checkerType.FullName) is TypePresenceVerifier checker)
                    return checker.ContainsType(path, typeof(PluginApplication).FullName);
            }
            catch (Exception e)
            {
                System.Console.WriteLine(e.Message);
            }
            finally
            {
                if (tmpDomain != null)
                    AppDomain.Unload(tmpDomain);
            }
            return false;
        }

        public override PluginApplication LoadAssembly(string path)
        {
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

            return Activator.CreateInstance(pluginType, GetContext()) as PluginApplication;
        }
    }
}
