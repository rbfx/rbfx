//
// Copyright (c) 2018 Rokas Kupstys
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
using System;
using System.Collections.Generic;
using System.IO;
using Editor.Tabs;
using IconFonts;
using Urho3D;
using ImGui;

namespace Editor
{
    internal class Editor : Application
    {
        private const int MenubarSize = 20;
        private readonly List<Tab> _tabs = new List<Tab>();

        public Editor(Context context) : base(context)
        {
        }

        public override void Setup()
        {
            var currentDir = Directory.GetCurrentDirectory();
            EngineParameters[EngineDefs.EpFullScreen] = false;
            EngineParameters[EngineDefs.EpWindowWidth] = 1920;
            EngineParameters[EngineDefs.EpWindowHeight] = 1080;
            EngineParameters[EngineDefs.EpWindowTitle] = GetType().Name;
            EngineParameters[EngineDefs.EpResourcePrefixPaths] = $"{currentDir};{currentDir}/..";
            EngineParameters[EngineDefs.EpLogLevel] = Log.Debug;
            EngineParameters[EngineDefs.EpWindowResizable] = true;
            EngineParameters[EngineDefs.EpAutoloadPaths] = "Autoload";
            EngineParameters[EngineDefs.EpResourcePaths] = "Data;CoreData;EditorData";
        }

        public override void Start()
        {
            ToolboxApi.RegisterToolboxTypes(Context);

            Input.SetMouseMode(MouseMode.Absolute);
            Input.SetMouseVisible(true);

            SystemUi.ApplyStyleDefault(true, 1.0f);
            ui.GetStyle().WindowRounding = 3;

            Cache.AutoReloadResources = true;

            SystemUi.AddFont("Fonts/DejaVuSansMono.ttf");
            SystemUi.AddFont("Fonts/fontawesome-webfont.ttf",
                new ushort[]{FontAwesome.IconMin, FontAwesome.IconMax, 0}, 0, true);

            // These dock sizes are bullshit. Actually visible sizes do not match these numbers. Instead of spending
            // time implementing this functionality in ImGuiDock proper values were written down and then tweaked until
            // they looked right. Insertion order is also important here when specifying dock placement location.
            var screenSize = Graphics.Size;
            _tabs.Add(new HierarchyTab(Context, "Hierarchy", new Vector2(screenSize.X * 0.2f, screenSize.Y * 0.5f),
                null, DockSlot.SlotLeft));
            _tabs.Add(new SceneTab(Context, "Scene", new Vector2(screenSize.X * 0.6f, screenSize.Y * 0.8f),
                null, DockSlot.SlotRight));
            _tabs.Add(new ResourcesTab(Context, "Resources", new Vector2(screenSize.X * 0.2f, screenSize.Y * 0.15f),
                "Hierarchy", DockSlot.SlotBottom));
            _tabs.Add(new InspectorTab(Context, "Inspector", new Vector2(screenSize.X * 0.07f, screenSize.Y),
                "Scene", DockSlot.SlotRight));
            _tabs.Add(new ConsoleTab(Context, "Console", new Vector2(screenSize.X * 0.6f, screenSize.Y * 0.15f),
                "Scene", DockSlot.SlotBottom));

            SubscribeToEvent(EUpdate.Event, OnRender);
        }

        private void OnRenderMenubar()
        {
            if (ui.BeginMainMenuBar())
            {
                if (ui.BeginMenu("File"))
                {
                    if (ui.MenuItem($"{FontAwesome.File} New Project"))
                    {
                        string projectPath = null;
                        if (FileDialog.PickFolder("", ref projectPath) == FileDialogResult.DialogOk)
                        {
                            var project = new Project(Context, projectPath);
                            Context.RegisterSubsystem(project);
                        }
                    }

                    if (ui.MenuItem($"{FontAwesome.File} New Scene"))
                    {

                    }

                    if (ui.MenuItem($"{FontAwesome.FloppyO} Save"))
                    {

                    }

                    ui.Separator();

                    if (ui.MenuItem($"{FontAwesome.SignOut} Exit"))
                        Engine.Exit();

                    ui.EndMenu();
                }

                if (ui.BeginMenu("View"))
                {
                    foreach (var tab in _tabs)
                    {
                        if (ui.MenuItem(tab.Title, null, tab.IsOpen, true))
                            tab.IsOpen ^= true;
                    }

                    ui.EndMenu();
                }
                ui.EndMainMenuBar();
            }
        }

        private void OnRender(VariantMap args)
        {
            OnRenderMenubar();

            var screenSize = ui.GetIO().DisplaySize;
            var dockPos  = new System.Numerics.Vector2(0, MenubarSize);
            var dockSize = new System.Numerics.Vector2(screenSize.X, screenSize.Y - MenubarSize);
            ImGuiDock.RootDock(dockPos, dockSize);

            foreach (var tab in _tabs)
                tab.OnRender();
        }
    }

    internal class Program
    {
        public static void Main(string[] args)
        {
            using (var context = new Context())
            {
                using (var application = new Editor(context))
                {
                    application.Run();
                }
            }
        }
    }
}
