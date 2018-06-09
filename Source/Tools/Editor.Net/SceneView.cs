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
using Urho3D;

namespace Editor
{
    public class SceneView
    {
        /// Rectangle dimensions that are rendered by this view.
        public IntVector2 Size
        {
            get => Viewport.Rect.Size;
            set
            {
                if (Size == value)
                    return;

                SetSize(value);
            }
        }
        /// Scene which is rendered by this view.
        public Scene Scene { get; }
        /// Texture to which scene is rendered.
        public Texture2D Texture { get; }
        /// Viewport which defines rendering area.
        public Viewport Viewport { get; }
        /// Camera which renders to a texture.
        private Node _camera;
        /// Camera component.
        public Camera Camera => _camera?.GetComponent<Camera>();

        /// Construct.
        public SceneView(Context context, IntVector2? size=null)
        {
            if (size == null)
                size = new IntVector2(1920, 1080);

            Scene = new Scene(context);
            Scene.CreateComponent<Octree>();
            Viewport = new Viewport(context, Scene, null)
            {
                Rect = new IntRect(IntVector2.Zero, size.Value)
            };
            CreateObjects();
            Texture = new Texture2D(context);
            // Make sure viewport is not using default renderpath. That would cause issues when renderpath is shared with other
            // viewports (like in resource inspector).
            Viewport.RenderPath = Viewport.RenderPath.Clone();
            SetSize(size.Value);
        }

        /// Creates scene camera and other objects required by editor.
        public void CreateObjects()
        {
            _camera = Scene.GetChild("EditorCamera", true);
            if (_camera == null)
            {
                _camera = Scene.CreateChild("EditorCamera", CreateMode.Local, uint.MaxValue, true);
                _camera.CreateComponent<Camera>();
                _camera.AddTag("__EDITOR_OBJECT__");
                _camera.IsTemporary = true;
                Viewport.Camera = Camera;
            }
            var debug = Scene.GetComponent<DebugRenderer>();
            if (debug == null)
            {
                debug = Scene.CreateComponent<DebugRenderer>(CreateMode.Local, uint.MaxValue - 1);
                debug.IsTemporary = true;
            }
            debug.SetView(Camera);
        }

        private void SetSize(IntVector2 value)
        {
            Viewport.Rect = new IntRect(IntVector2.Zero, value);
            Texture.SetSize(value.X, value.Y, Graphics.GetRgbformat(), TextureUsage.Rendertarget);
            Texture.RenderSurface.SetViewport(0, Viewport);
            Texture.RenderSurface.UpdateMode = RenderSurfaceUpdateMode.SurfaceUpdatealways;
        }
    }

}
