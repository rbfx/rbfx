using System.Collections.Generic;

namespace Urho3DNet
{
    public class CompiledScriptRuntimeApiImpl : ScriptRuntimeApiImpl
    {
        public override PluginApplication CompileResourceScriptPlugin()
        {
            // New projects may have no C# scripts. In this case a dummy plugin instance that does nothing will be
            // returned. Once new scripts appear - plugin will be reloaded properly.
            var plugin = new RuntimeCompiledScriptPluginApplication(Context.Instance);
            plugin.SetSlaveAssembly(CsCompiler.FindAndCompile("", null));
            return plugin;
        }
    }
}
