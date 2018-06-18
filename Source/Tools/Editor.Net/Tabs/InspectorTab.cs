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
    public class InspectorTab : Tab
    {
        private IInspectable _inspectable;

        public InspectorTab(Context context, string title, TabLifetime lifetime, Vector2? initialSize = null,
            string placeNextToDock = null, DockSlot slot = DockSlot.SlotNone) : base(context, title, lifetime,
            initialSize, placeNextToDock, slot)
        {
            Uuid = "edd8df51-a31b-4585-bc0f-7665c99a12ac";
            SubscribeToEvent<InspectItem>(OnInspect);
            SubscribeToEvent<EditorTabClosed>(OnTabClosed);
        }

        private void OnTabClosed(Event args)
        {
            if (args.GetObject(EditorTabClosed.TabInstance) == _inspectable)
                _inspectable = null;
        }

        private void OnInspect(Event args)
        {
            _inspectable = (IInspectable) args.GetObject(InspectItem.Inspectable);
        }

        protected override void Render()
        {
            _inspectable?.RenderInspector();
        }
    }
}
