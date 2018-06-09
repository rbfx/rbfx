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
using IconFonts;
using Urho3D;
using ImGui;


namespace Editor.Tabs
{
    public class SceneTab : Tab
    {
        private readonly SceneView _view;
        private readonly Gizmo _gizmo;
        private bool _mouseHoversViewport;

        public SceneTab(Context context, string title, Vector2? initialSize = null, string placeNextToDock = null,
            DockSlot slot = DockSlot.SlotNone) : base(context, title, initialSize, placeNextToDock, slot)
        {
            _windowFlags = WindowFlags.NoScrollbar;
            _view = new SceneView(Context);
            _gizmo = new Gizmo(Context);
            _view.Scene.LoadXml(Cache.GetResource<XMLFile>("Scenes/SceneLoadExample.xml").GetRoot());
            CreateObjects();

            SubscribeToEvent(CoreEvents.E_UPDATE, OnUpdate);
        }

        private void RenderToolbar()
        {
            ui.PushStyleVar(StyleVar.FrameRounding, 0);

            if (Widgets.EditorToolbarButton(FontAwesome.Undo, "Undo"))
            {
            }

            if (Widgets.EditorToolbarButton(FontAwesome.Repeat, "Redo"))
            {
            }

            ui.SameLine(0, 3);

            if (Widgets.EditorToolbarButton(FontAwesome.Arrows, "Translate", _gizmo.Operation == GizmoOperation.Translate))
                _gizmo.Operation = GizmoOperation.Translate;
            if (Widgets.EditorToolbarButton(FontAwesome.Repeat, "Rotate", _gizmo.Operation == GizmoOperation.Rotate))
                _gizmo.Operation = GizmoOperation.Rotate;
            if (Widgets.EditorToolbarButton(FontAwesome.ArrowsAlt, "Scale", _gizmo.Operation == GizmoOperation.Scale))
                _gizmo.Operation = GizmoOperation.Scale;

            ui.SameLine(0, 3);

            if (Widgets.EditorToolbarButton(FontAwesome.ArrowsAlt, "World", _gizmo.TransformSpace == TransformSpace.World))
                _gizmo.TransformSpace = TransformSpace.World;
            if (Widgets.EditorToolbarButton(FontAwesome.ArrowsAlt, "Local", _gizmo.TransformSpace == TransformSpace.Local))
                _gizmo.TransformSpace = TransformSpace.Local;

            ui.SameLine(0, 3);

            var light = _view.Camera.Node.GetComponent<Light>();
            if (light != null)
            {
                if (Widgets.EditorToolbarButton(FontAwesome.LightbulbO, "Camera Headlight", light.IsEnabled))
                    light.IsEnabled = !light.IsEnabled;
            }

            ui.SameLine(0, 3);

            ui.PopStyleVar();
            ui.NewLine();
        }

        protected override void Render()
        {
            RenderToolbar();

            var padding = ui.GetStyle().FramePadding;
            var spacing = ui.GetStyle().ItemSpacing;
            var size = new System.Numerics.Vector2(ui.GetWindowWidth() - padding.X - spacing.X,
                ui.GetWindowHeight() - ui.GetCursorPos().Y - spacing.Y);
            if (ui.BeginChild(0xF00, size, false, WindowFlags.Default))
            {
                var viewSize = ui.GetWindowSize();
                _view.Size = viewSize;
                var viewPos = ui.GetCursorScreenPos();
                ui.Image(_view.Texture.NativeInstance, viewSize, Vector2.Zero, Vector2.One, Vector4.One, Vector4.Zero);

                _gizmo.SetScreenRect(viewPos, viewSize);

                if (Input.IsMouseVisible)
                    _mouseHoversViewport = ui.IsItemHovered(HoveredFlags.Default);

                var isClickedLeft = Input.GetMouseButtonClick((int) MouseButton.Left) && ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup);
                var isClickedRight = Input.GetMouseButtonClick((int) MouseButton.Right) && ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup);

                _gizmo.ManipulateSelection(_view.Camera);

                if (ui.IsWindowHovered())
                    _windowFlags |= WindowFlags.NoMove;
                else
                    _windowFlags &= ~WindowFlags.NoMove;

                if (!_gizmo.IsActive && (isClickedLeft || isClickedRight) && Input.IsMouseVisible)
                {
                    // Handle object selection.
                    var pos = Input.MousePosition - new IntVector2(viewPos);

                    var cameraRay = _view.Camera.GetScreenRay(pos.X / size.X, pos.Y / size.Y);
                    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit

                    var query = new RayOctreeQuery(cameraRay, RayQueryLevel.Triangle, float.PositiveInfinity, DrawableFlags.Geometry);
                    _view.Scene.GetComponent<Octree>().RaycastSingle(query);

                    if (query.Result == null)
                    {
                        // When object geometry was not hit by a ray - query for object bounding box.
                        query = new RayOctreeQuery(cameraRay, RayQueryLevel.Obb, float.PositiveInfinity, DrawableFlags.Geometry);
                        _view.Scene.GetComponent<Octree>().RaycastSingle(query);
                    }

                    if (query.Result != null)
                    {
                        var clickNode = query.Result[0].Drawable.Node;
                        // Temporary nodes can not be selected.
                        while (clickNode != null && clickNode.HasTag("__EDITOR_OBJECT__"))
                            clickNode = clickNode.Parent;

                        if (clickNode != null)
                        {
                            var appendSelection = Input.GetKeyDown(InputEvents.KeyCtrl);
                            if (!appendSelection)
                                _gizmo.UnselectAll();
                            _gizmo.ToggleSelection(clickNode);

                            if (isClickedRight)
                                ui.OpenPopup("Node context menu"/*, true*/);
                        }
                    }
                    else
                        _gizmo.UnselectAll();
                }

            }
            ui.EndChild();
        }

        private void CreateObjects()
        {
            _view.CreateObjects();
            _view.Camera.Node.GetOrCreateComponent<DebugCameraController>();
        }

        private void OnUpdate(VariantMap args)
        {
            if (_mouseHoversViewport)
                _view.Camera.GetComponent<DebugCameraController>().Update(args[Update.P_TIMESTEP].Float);
        }
    }
}
