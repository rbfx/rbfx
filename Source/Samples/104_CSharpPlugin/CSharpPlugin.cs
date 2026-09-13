// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System;
using Urho3DNet;
using ImGuiNet;

namespace CSharpPlugin
{
    // Class can have any name, but it must inherit from PluginApplication.
    [LoadablePlugin]
    public class SamplePlugin : PluginApplication
    {
        public SamplePlugin(Context context) : base(context)
        {
        }

        protected override void Load()
        {
            SubscribeToEvent("EditorApplicationMenu", RenderMenu);
        }

        void RenderMenu(VariantMap args)
        {
            if (ImGui.BeginMenu("SamplePlugin"))
            {
                ImGui.TextUnformatted("C# says hello");
                ImGui.EndMenu();
            }
        }

        protected override void Start(bool isMain)
        {
        }

        protected override void Stop()
        {
        }

        protected override void Unload()
        {
        }
    }
}
