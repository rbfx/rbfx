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
using Urho3D;
using ImGui;


namespace Editor.Tabs
{
    public class ResourcesTab : Tab
    {
        public ResourcesTab(Context context, string title, TabLifetime lifetime, Vector2? initialSize = null,
            string placeNextToDock = null, DockSlot slot = DockSlot.SlotNone) : base(context, title, lifetime,
            initialSize, placeNextToDock, slot)
        {
            Uuid = "4fcf9895-e363-441d-a665-2a97f7da64ef";
        }

        protected override void Render()
        {
            string selectedPath = null;
            if (ResourceBrowser.ResourceBrowserWidget(ref selectedPath))
            {
                var type = ContentUtilities.GetContentType(selectedPath);
                switch (type)
                {
                    case ContentType.CtypeUnknown:
                        break;
                    case ContentType.CtypeScene:
                        GetSubsystem<Editor>().OpenTab<SceneTab>().LoadResource(selectedPath);
                        break;
                    case ContentType.CtypeSceneobject:
                        break;
                    case ContentType.CtypeUilayout:
                        break;
                    case ContentType.CtypeUistyle:
                        break;
                    case ContentType.CtypeModel:
                        break;
                    case ContentType.CtypeAnimation:
                        break;
                    case ContentType.CtypeMaterial:
                        break;
                    case ContentType.CtypeParticle:
                        break;
                    case ContentType.CtypeRenderpath:
                        break;
                    case ContentType.CtypeSound:
                        break;
                    case ContentType.CtypeTexture:
                        break;
                    case ContentType.CtypeTexturexml:
                        break;
                    default:
                        throw new ArgumentOutOfRangeException();
                }
            }
        }
    }
}
