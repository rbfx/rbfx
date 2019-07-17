using System;
using System.IO;
using System.Linq;
using System.Reflection;

namespace Urho3DNet
{
    public class ScriptRuntimeApiImpl : ScriptRuntimeApi
    {
        private static readonly string ProgramFile = new Uri(Assembly.GetExecutingAssembly().CodeBase).LocalPath;
        private static readonly string ProgramDirectory = Path.GetDirectoryName(ProgramFile);

        public ScriptRuntimeApiImpl(Context context) : base(context)
        {
            InstallAssemblyLoader(AppDomain.CurrentDomain);
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

        public override PluginApplication LoadAssembly(string path, uint version)
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

        public override void Dispose(RefCounted instance)
        {
            instance?.Dispose();
        }
    }
}
