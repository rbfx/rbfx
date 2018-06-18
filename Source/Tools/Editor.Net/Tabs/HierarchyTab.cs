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

using Editor.Events;
using Urho3D;
using ImGui;


namespace Editor.Tabs
{
    public class HierarchyTab : Tab
    {
        private IHierarchyProvider _hierarchyProvider;

        public HierarchyTab(Context context, string title, TabLifetime lifetime, Vector2? initialSize = null,
            string placeNextToDock = null, DockSlot slot = DockSlot.SlotNone) : base(context, title, lifetime,
            initialSize, placeNextToDock, slot)
        {
            Uuid = "af235811-2493-4adf-aa88-e40332b45150";
            SubscribeToEvent<InspectHierarchy>(OnInspect);
            SubscribeToEvent<EditorTabClosed>(OnTabClosed);
        }

        private void OnTabClosed(Event args)
        {
            if (args.GetObject(EditorTabClosed.TabInstance) == _hierarchyProvider)
                _hierarchyProvider = null;
        }

        private void OnInspect(Event args)
        {
            _hierarchyProvider = (IHierarchyProvider) args.GetObject(InspectHierarchy.HierarchyProvider);
        }

        protected override void Render()
        {
            _hierarchyProvider?.RenderHierarchy();
        }
    }
}
