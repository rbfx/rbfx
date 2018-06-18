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
using System.Linq;
using System.Reflection;
using Editor.Events;
using Editor.Tabs;
using IconFonts;
using Urho3D;
using ImGui;
using Urho3D.Events;

namespace Editor
{
    public class Editor : Application
    {
        private const int MenubarSize = 20;
        private readonly List<Tab> _tabs = new List<Tab>();
        private Project _project;
        private readonly string[] _commandLineArguments;

        public Editor(Context context, string[] args) : base(context)
        {
            _commandLineArguments = args;
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
            Context.RegisterSubsystem(new IconCache(Context));
            Context.RegisterSubsystem(this);

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
            var inspector = new InspectorTab(Context, "Inspector", TabLifetime.Persistent,
                new Vector2(screenSize.X * 0.6f, screenSize.Y), null, DockSlot.SlotRight);
            var hierarchy = new HierarchyTab(Context, "Hierarchy", TabLifetime.Persistent,
                new Vector2(screenSize.X * 0.05f, screenSize.Y * 0.5f), null, DockSlot.SlotLeft);
            _tabs.AddRange(new Tab[]{inspector, hierarchy,
                new ResourcesTab(Context, "Resources", TabLifetime.Persistent,
                    new Vector2(screenSize.X * 0.05f, screenSize.Y * 0.15f), hierarchy.UniqueTitle, DockSlot.SlotBottom),
                new ConsoleTab(Context, "Console", TabLifetime.Persistent,
                    new Vector2(screenSize.X * 0.6f, screenSize.Y * 0.4f), inspector.UniqueTitle, DockSlot.SlotLeft)
            });

            SubscribeToEvent<Update>(OnRender);
            SubscribeToEvent<EditorProjectSave>(OnSaveProject);
            SubscribeToEvent<EditorProjectLoad>(OnLoadProject);

            if (_commandLineArguments.Length == 1)
                OpenProject(_commandLineArguments[0]);
        }

        public void CloseProject()
        {
            if (_project == null)
                return;

            // Remove all temporary tabs
            for (var i = 0; i < _tabs.Count;)
            {
                var tab = _tabs[i];
                if (tab.Lifetime == TabLifetime.Temporary)
                    _tabs.RemoveAt(i);
                else
                    ++i;
            }

            Context.RemoveSubsystem(_project.GetType());
            _project.Dispose();
            _project = null;
        }

        public void OpenProject(string projectPath)
        {
            CloseProject();

            _project = new Project(Context, projectPath);
            Context.RegisterSubsystem(_project);
            _project.Load();
        }

        private void OnSaveProject(Event e)
        {
            var save = (JSONValue) e.GetObject(EditorProjectSave.SaveData);

            var uiSettings = new JSONValue();
            uiSettings.Set("X", Graphics.WindowPosition.X);
            uiSettings.Set("Y", Graphics.WindowPosition.Y);
            uiSettings.Set("W", Graphics.Size.X);
            uiSettings.Set("H", Graphics.Size.Y);

            var dockLayout = new JSONValue();
            ImGuiDock.SaveDock(dockLayout);
            uiSettings.Set("docks", dockLayout);

            save.Set("ui", uiSettings);
        }

        private void OnLoadProject(Event e)
        {
            var save = (JSONValue) e.GetObject(EditorProjectSave.SaveData);
            var uiSettings = save.Get("ui");

            Graphics.SetWindowPosition(uiSettings.Get("X").Int, uiSettings.Get("Y").Int);
            Graphics.SetMode(uiSettings.Get("W").Int, uiSettings.Get("H").Int);

            ImGuiDock.LoadDock(uiSettings.Get("docks"));
        }

        private void OnRenderMenubar()
        {
            if (ui.BeginMainMenuBar())
            {
                if (ui.BeginMenu("File"))
                {
                    if (ui.MenuItem($"{FontAwesome.File} Create/Open Project"))
                    {
                        string projectPath = null;
                        if (FileDialog.PickFolder("", ref projectPath) == FileDialogResult.DialogOk)
                        {
                            OpenProject(projectPath);
                        }
                    }

                    if (_project != null)
                    {
                        if (ui.MenuItem($"{FontAwesome.File} New Scene"))
                        {

                        }

                        if (ui.MenuItem($"{FontAwesome.FloppyO} Save"))
                        {
                            _project.Save();
                        }
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

        private void OnRender(Event args)
        {
            SendKeyboardShortcuts();
            OnRenderMenubar();

            if (_project == null)
                return;

            var screenSize = ui.GetIO().DisplaySize;
            var dockPos  = new System.Numerics.Vector2(0, MenubarSize);
            var dockSize = new System.Numerics.Vector2(screenSize.X, screenSize.Y - MenubarSize);
            ImGuiDock.RootDock(dockPos, dockSize);

            for (var i = 0; i < _tabs.Count;)
            {
                var tab = _tabs[i];
                tab.OnRender();
                if (tab.Lifetime == TabLifetime.Temporary && !tab.IsOpen)
                {
                    _tabs.RemoveAt(i);
                    SendEvent<EditorTabClosed>(new Dictionary<StringHash, dynamic> {[EditorTabClosed.TabInstance] = tab});
                    tab.Dispose();
                }
                else
                    ++i;
            }
        }

        private void SendKeyboardShortcuts()
        {
            var combo = EditorKeyCombo.Kind.None;
            if (Input.GetKeyDown(Key.Ctrl))
            {
                if (Input.GetKeyPress(Key.Z))
                {
                    if (Input.GetKeyDown(Key.Shift))
                        combo = EditorKeyCombo.Kind.Redo;
                    else
                        combo = EditorKeyCombo.Kind.Undo;
                }
                else if (Input.GetKeyPress(Key.Y))
                    combo = EditorKeyCombo.Kind.Redo;
            }

            if (combo != EditorKeyCombo.Kind.None)
            {
                SendEvent<EditorKeyCombo>(new Dictionary<StringHash, dynamic>
                {
                    [EditorKeyCombo.KeyCombo] = (int)combo
                });
            }
        }

        public Tab OpenTab(Type tabType)
        {
            string tabName = null;
            var slot = DockSlot.SlotFloat;
            for (var i = _tabs.Count - 1; i >= 0; i--)
            {
                var openTab = _tabs[i];
                if (!openTab.IsOpen)
                    continue;

                if (openTab is SceneTab)
                {
                    tabName = openTab.UniqueTitle;
                    slot = DockSlot.SlotTab;
                    break;
                }

                if (openTab is ConsoleTab)
                {
                    tabName = openTab.UniqueTitle;
                    slot = DockSlot.SlotTop;
                }
            }

            var screenSize = Graphics.Size;
            var tab = (Tab) Activator.CreateInstance(tabType, BindingFlags.Public | BindingFlags.Instance, null,
                new object[]{Context, null, TabLifetime.Temporary,
                    new Vector2(screenSize.X * 0.6f, screenSize.Y * 0.6f), tabName, slot
                }, null);

            _tabs.Add(tab);

            return tab;
        }

        public T OpenTab<T>() where T: Tab
        {
            return (T) OpenTab(typeof(T));
        }

        public static void Main(string[] args)
        {
            using (var context = new Context())
            {
                using (var application = new Editor(context, args))
                {
                    application.Run();
                }
            }
        }
    }
}
