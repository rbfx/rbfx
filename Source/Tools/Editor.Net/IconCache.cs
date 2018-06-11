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
using System.Diagnostics;
using System.Numerics;
using ImGui;
using Urho3D;
using Object = Urho3D.Object;
using Vector2 = System.Numerics.Vector2;
using Vector4 = System.Numerics.Vector4;

namespace Editor
{
    public class IconCache : Object
    {
        public class IconData
        {
            /// A texture which contains the icon.
            public ResourceRef TextureRef;
            /// Icon location and size.
            public IntRect Rect;
        }

        /// Editor icon cache.
        private readonly Dictionary<string, IconData> _iconCache = new Dictionary<string, IconData>();

        /// Reads EditorIcons.xml and stores information for later use by imgui.
        public IconCache(Context context) : base(context)
        {
            var icons = Cache.GetResource<XMLFile>("UI/EditorIcons.xml");
            Debug.Assert(icons != null);

            var root = icons.GetRoot();
            for (var element = root.GetChild("element"); element.NotNull(); element = element.GetNext("element"))
            {
                var type = element.GetAttribute("type");
                if (string.IsNullOrEmpty(type))
                {
                    Log.Write(Log.Error, "EditorIconCache.xml contains icon entry without a \"type\" attribute.");
                    continue;
                }

                var texture = element.SelectPrepared(new XPathQuery("attribute[@name=\"Texture\"]"));
                if (texture.Empty())
                {
                    Log.Write(Log.Error, "EditorIconCache.xml contains icon entry without a \"Texture\".");
                    continue;
                }

                var rect = element.SelectPrepared(new XPathQuery("attribute[@name=\"Image Rect\"]"));
                if (rect.Empty())
                {
                    Log.Write(Log.Error, "EditorIconCache.xml contains icon entry without a \"Image Rect\".");
                    continue;
                }

                var data = new IconData
                {
                    TextureRef = texture.FirstResult().GetVariantValue(VariantType.Resourceref).ResourceRef,
                    Rect = rect.FirstResult().GetVariantValue(VariantType.Intrect).IntRect
                };
                _iconCache[type] = data;
            }
        }

        /// Return pointer to icon data.
        IconData GetIconData(string name)
        {
            if (_iconCache.TryGetValue(name, out var icon))
                return icon;
            return _iconCache["Unknown"];
        }

        public void RenderIcon(string name)
        {
            var iconData = GetIconData(name);

            var rect = iconData.Rect;
            var texture = Cache.GetResource<Texture2D>(iconData.TextureRef.Name);

            ui.Image(texture.NativeInstance,
                new Vector2(ImGui.SystemUi.Pdpx(rect.Width), ImGui.SystemUi.Pdpy(rect.Height)),
                new Vector2((float) rect.Left / texture.Width, (float) rect.Top / texture.Height),
                new Vector2((float) rect.Right / texture.Width, (float) rect.Bottom / texture.Height),
                Vector4.One, Vector4.Zero
            );
        }
    }
}
