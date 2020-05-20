using System;
using System.Reflection;

namespace Urho3DNet
{
    public class RuntimeCompiledScriptPluginApplication : PluginApplication
    {
        /// Assembly that is being managed by this plugin.
        private Assembly _slaveAssembly;
        /// Construct.
        public RuntimeCompiledScriptPluginApplication(Context context) : base(context)
        {
        }
        /// Sets assembly that is being managed by this plugin.
        public void SetSlaveAssembly(Assembly assembly)
        {
            _slaveAssembly = assembly;
        }

        public override void Load()
        {
            if (_slaveAssembly != null)
                Context.RegisterFactories(_slaveAssembly);
        }

        public override void Unload()
        {
            if (_slaveAssembly != null)
                Context.RemoveFactories(_slaveAssembly);
        }
    }
}
