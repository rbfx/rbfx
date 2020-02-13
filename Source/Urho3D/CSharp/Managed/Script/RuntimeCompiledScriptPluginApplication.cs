using System;
using System.Reflection;

namespace Urho3DNet
{
    public class RuntimeCompiledScriptPluginApplication : PluginApplication
    {
        /// Assembly that is being managed by this plugin.
        private Assembly _hostAssembly;
        /// Construct.
        public RuntimeCompiledScriptPluginApplication(Context context) : base(context)
        {
        }
        /// Sets assembly that is being managed by this plugin.
        internal void SetHostAssembly(Assembly assembly)
        {
            _hostAssembly = assembly;
        }

        public override void Load()
        {
            Context.RegisterFactories(_hostAssembly);
        }

        public override void Unload()
        {
            Context.RemoveFactories(_hostAssembly);
        }
    }
}
