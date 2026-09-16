// Copyright (c) 2026-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using System.Reflection;

namespace Urho3DNet
{
    public partial class RuntimeCompiledScriptPlugin : Plugin
    {
        /// Assembly that is being managed by this plugin.
        private Assembly _slaveAssembly;
        /// Construct.
        public RuntimeCompiledScriptPlugin(Context context) : base(context)
        {
        }
        /// Sets assembly that is being managed by this plugin.
        public void SetSlaveAssembly(Assembly assembly)
        {
            _slaveAssembly = assembly;
        }

        protected override void Load()
        {
            if (_slaveAssembly != null)
                Context.RegisterFactories(_slaveAssembly);
        }

        protected override void Unload()
        {
            if (_slaveAssembly != null)
                Context.RemoveFactories(_slaveAssembly);
        }
    }
}
