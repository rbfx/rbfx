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
using System.Linq;
using Editor.Events;
using ImGui;
using Urho3D;
using Urho3D.Events;


namespace Editor.Tabs
{
    public enum TabLifetime
    {
        Temporary,
        Persistent
    }

    public class Tab : Urho3D.Object
    {
        public struct Location
        {
            public string NextDockName;
            public DockSlot Slot;
        }

        private bool _isOpen = true;
        public bool IsOpen
        {
            get => _isOpen;
            set => _isOpen = value;
        }

        public string Title { get; protected set; }
        public string UniqueTitle => $"{Title}###{Uuid}";
        public System.Numerics.Vector2 InitialSize;
        public Location InitialLocation;
        protected WindowFlags WindowFlags = WindowFlags.Default;
        private bool _activateTab;
        public bool MouseHoversViewport { get; protected set; }
        public bool IsDockFocused { get; protected set; }
        public bool IsDockActive { get; protected set; }
        public TabLifetime Lifetime { get; protected set; }
        public string ResourcePath { get; protected set; }
        public string ResourceType { get; protected set; }
        public string Uuid { get; protected set; }

        public Tab(Context context, string title, TabLifetime lifetime, Vector2? initialSize = null,
            string placeNextToDock = null, DockSlot slot = DockSlot.SlotNone) : base(context)
        {
            Uuid = Guid.NewGuid().ToString().ToLower();
            Title = title;
            Lifetime = lifetime;
            InitialSize = initialSize.GetValueOrDefault(new Vector2(-1, -1));
            InitialLocation.NextDockName = placeNextToDock;
            InitialLocation.Slot = slot;

            SubscribeToEvent<EditorProjectSave>(OnSaveProject);
            SubscribeToEvent<EditorDeleteResource>(OnResourceDeleted);
            SubscribeToEvent<ResourceRenamed>(OnResourceRenamed);
        }

        private void OnResourceRenamed(Event e)
        {
            if (ResourcePath == null)
                return;

            var from = e.GetString(ResourceRenamed.From);
            var to = e.GetString(ResourceRenamed.To);

            if (from == ResourcePath)
                ResourcePath = Title = to;
        }

        private void OnResourceDeleted(Event e)
        {
            if (Lifetime == TabLifetime.Temporary && e.GetString(EditorDeleteResource.ResourceName) == ResourcePath)
                _isOpen = false;
        }

        public virtual void LoadResource(string resourcePath)
        {
            ResourcePath = resourcePath;
            Title = resourcePath;
        }

        public virtual void LoadSave(JSONValue save)
        {
            throw new NotImplementedException();
        }

        public void OnRender()
        {
            ImGuiDock.SetNextDockPos(InitialLocation.NextDockName, InitialLocation.Slot, ImGui.Condition.FirstUseEver);
            var render = ImGuiDock.BeginDock(UniqueTitle, ref _isOpen, WindowFlags, InitialSize);
            if (_activateTab)
            {
                ImGuiDock.SetDockActive();
                _activateTab = false;
            }

            if (render)
            {
                if (Input.IsMouseVisible)
                    MouseHoversViewport = ui.IsItemHovered(HoveredFlags.Default);
                IsDockFocused = ui.IsWindowFocused(FocusedFlags.ChildWindows);
                IsDockActive = ImGuiDock.IsDockActive();

                Render();
            }
            ImGuiDock.EndDock();
        }

        protected virtual void Render()
        {

        }

        public void Activate()
        {
            _activateTab = true;
        }

        protected virtual void OnSaveProject(Event e)
        {
            var projectSave = (JSONValue) e.GetObject(EditorProjectSave.SaveData);
            var resources = projectSave.Get("resources");

            var save = new JSONValue();            // This is object passed to LoadSave()
            save.Set("type", ResourceType);
            save.Set("path", ResourcePath);
            save.Set("uuid", Uuid);

            resources.Push(save);
            projectSave.Set("resources", resources);
        }
    }
}
