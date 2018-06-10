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

using System.Collections.Generic;
using System.Linq;
using Editor.Events;
using IconFonts;
using Urho3D;
using ImGui;
using Urho3D.Events;


namespace Editor.Tabs
{
    public class SceneTab : Tab, IInspectable, IHierarchyProvider
    {
        private readonly SceneView _view;
        private readonly Gizmo _gizmo;
        private readonly VariantMap _eventArgs = new VariantMap();
        private bool _wasFocused;
        private bool _isMouseHoveringViewport;
        private Component _selectedComponent;

        public SceneTab(Context context, string title, Vector2? initialSize = null, string placeNextToDock = null,
            DockSlot slot = DockSlot.SlotNone) : base(context, title, initialSize, placeNextToDock, slot)
        {
            WindowFlags = WindowFlags.NoScrollbar;
            _view = new SceneView(Context);
            _gizmo = new Gizmo(Context);
            _view.Scene.LoadXml(Cache.GetResource<XMLFile>("Scenes/SceneLoadExample.xml").GetRoot());
            CreateObjects();

            SubscribeToEvent<Update>(OnUpdate);
            SubscribeToEvent<GizmoSelectionChanged>(args => { _selectedComponent = null; });
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
                if (Input.IsMouseVisible)
                    _isMouseHoveringViewport = ui.IsItemHovered(HoveredFlags.Default);

                _gizmo.SetScreenRect(viewPos, viewSize);

                var isClickedLeft = Input.GetMouseButtonClick((int) MouseButton.Left) && ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup);
                var isClickedRight = Input.GetMouseButtonClick((int) MouseButton.Right) && ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup);

                _gizmo.ManipulateSelection(_view.Camera);

                if (ui.IsWindowHovered())
                    WindowFlags |= WindowFlags.NoMove;
                else
                    WindowFlags &= ~WindowFlags.NoMove;

                if (!_gizmo.IsActive && (isClickedLeft || isClickedRight) && Input.IsMouseVisible)
                {
                    // Handle object selection.
                    var pos = Input.MousePosition - new IntVector2(viewPos);

                    var cameraRay = _view.Camera.GetScreenRay(pos.X / size.X, pos.Y / size.Y);
                    // Pick only geometry objects, not eg. zones or lights, only get the first (closest) hit

                    var query = new RayOctreeQuery(cameraRay, RayQueryLevel.Triangle, float.PositiveInfinity, DrawableFlags.Geometry);
                    _view.Scene.GetComponent<Octree>().RaycastSingle(query);

                    if (query.Result.Length == 0)
                    {
                        // When object geometry was not hit by a ray - query for object bounding box.
                        query = new RayOctreeQuery(cameraRay, RayQueryLevel.Obb, float.PositiveInfinity, DrawableFlags.Geometry);
                        _view.Scene.GetComponent<Octree>().RaycastSingle(query);
                    }

                    if (query.Result.Length > 0)
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

                            // Notify inspector
                            _eventArgs.Clear();
                            _eventArgs[InspectItem.Inspectable] = Variant.FromObject(this);
                            SendEvent<InspectItem>(_eventArgs);

                            if (isClickedRight)
                                ui.OpenPopup("Node context menu");
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
            if (IsDockActive && _isMouseHoveringViewport)
            {
                _view.Camera.GetComponent<DebugCameraController>().Update(args[Update.TimeStep].Float);
                if (!_wasFocused)
                {
                    _wasFocused = true;
                    _eventArgs.Clear();
                    _eventArgs[InspectHierarchy.HierarchyProvider] = Variant.FromObject(this);
                    SendEvent<InspectHierarchy>(_eventArgs);
                }
            }
            else
                _wasFocused = false;
        }

        public Serializable[] GetInspectableObjects()
        {
            if (_gizmo.Selection == null)
                return new Serializable[0];

            var inspectables = _gizmo.Selection.Cast<Serializable>().ToList();
            if (_selectedComponent != null)
                inspectables.Add(_selectedComponent);

            return inspectables.ToArray();
        }

        private void RenderNodeTree(Node node)
        {
            if (node.IsTemporary)
                return;

            var flags = TreeNodeFlags.OpenOnArrow;
            if (node.Parent == null)
                flags |= TreeNodeFlags.DefaultOpen;

            var name = (node.Name.Length == 0 ? node.GetType().Name : node.Name) + $" ({node.Id})";
            var isSelected = _gizmo.IsSelected(node) || _selectedComponent != null && _selectedComponent.Node == node;

            if (isSelected)
                flags |= TreeNodeFlags.Selected;

            //ui::Image("Node");
            //ui::SameLine();
            var opened = ui.TreeNodeEx(name, flags);
            if (!opened)
            {
                // If TreeNode above is opened, it pushes it's label as an ID to the stack. However if it is not open then no
                // ID is pushed. This creates a situation where context menu is not properly attached to said tree node due to
                // missing ID on the stack. To correct this we ensure that ID is always pushed. This allows us to show context
                // menus even for closed tree nodes.
                ui.PushID(name);
            }

            if (ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup))
            {
                if (ImGui.SystemUi.IsMouseClicked(MouseButton.Left))
                {
                    if (!Input.GetKeyDown(InputEvents.KeyCtrl))
                        _gizmo.UnselectAll();
                    _gizmo.ToggleSelection(node);
                }
                else if (ImGui.SystemUi.IsMouseClicked(MouseButton.Right))
                {
                    _gizmo.UnselectAll();
                    _gizmo.ToggleSelection(node);
                    ui.OpenPopup("Node context menu");
                }
            }

            RenderNodeContextMenu();

            if (opened)
            {
                foreach (var component in node.Components)
                {
                    if (component.IsTemporary)
                        continue;

                    ui.PushID(component.NativeInstance.ToInt32());

                    //ui::Image(component->GetTypeName());
                    //ui::SameLine();

                    var selected = _selectedComponent != null && _selectedComponent.Equals(component);
                    selected = ui.Selectable(component.GetType().Name, selected);

                    if (ImGui.SystemUi.IsMouseClicked(MouseButton.Right) && ui.IsItemHovered(HoveredFlags.AllowWhenBlockedByPopup))
                    {
                        selected = true;
                        ui.OpenPopup("Component context menu");
                    }

                    if (selected)
                    {
                        _gizmo.UnselectAll();
                        _gizmo.ToggleSelection(node);
                        _selectedComponent = component;
                    }

                    if (ui.BeginPopup("Component context menu"))
                    {
                        if (ui.MenuItem("Remove"))
                            component.Remove();
                        ui.EndPopup();
                    }

                    ui.PopID();
                }

                // Do not use element->GetChildren() because child may be deleted during this loop.
                foreach (var child in node.Children)
                    RenderNodeTree(child);

                ui.TreePop();
            }
            else
                ui.PopID();
        }

        private void RenderNodeContextMenu()
        {
        }

        public void RenderHierarchy()
        {
            ui.PushStyleVar(StyleVar.IndentSpacing, 10);
            try
            {
                RenderNodeTree(_view.Scene);
            }
            finally
            {
                ui.PopStyleVar();
            }
        }
    }
}
