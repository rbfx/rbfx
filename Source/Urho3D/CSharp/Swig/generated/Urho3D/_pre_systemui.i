%ignore Urho3D::SYSTEMUI_DEFAULT_FONT_SIZE;
%csconst(1) Urho3D::SYSTEMUI_DEFAULT_FONT_SIZE;
%constant float SystemuiDefaultFontSize = (float)14;
%constant const char* DragDropPayloadType = "DragDropPayload";
%ignore Urho3D::DragDropPayloadType;
%constant const char* DragDropPayloadVariable = "SystemUI_DragDropPayload";
%ignore Urho3D::DragDropPayloadVariable;
%csconstvalue("0") Urho3D::DEBUGHUD_SHOW_NONE;
%csconstvalue("1") Urho3D::DEBUGHUD_SHOW_STATS;
%csconstvalue("2") Urho3D::DEBUGHUD_SHOW_MODE;
%csconstvalue("7") Urho3D::DEBUGHUD_SHOW_ALL;
%typemap(csattributes) Urho3D::DebugHudMode "[global::System.Flags]";
using DebugHudModeFlags = Urho3D::DebugHudMode;
%typemap(ctype) DebugHudModeFlags "size_t";
%typemap(out) DebugHudModeFlags "$result = (size_t)$1.AsInteger();"
%csattribute(Urho3D::ResourceInspectorWidget, %arg(Urho3D::ResourceInspectorWidget::ResourceVector), Resources, GetResources);
%csattribute(Urho3D::Console, %arg(bool), IsVisible, IsVisible, SetVisible);
%csattribute(Urho3D::Console, %arg(bool), IsAutoVisibleOnError, IsAutoVisibleOnError, SetAutoVisibleOnError);
%csattribute(Urho3D::Console, %arg(ea::string), CommandInterpreter, GetCommandInterpreter, SetCommandInterpreter);
%csattribute(Urho3D::Console, %arg(unsigned int), NumHistoryRows, GetNumHistoryRows, SetNumHistoryRows);
%csattribute(Urho3D::Console, %arg(Urho3D::StringVector), Loggers, GetLoggers);
%csattribute(Urho3D::SystemUI, %arg(Urho3D::Vector2), RelativeMouseMove, GetRelativeMouseMove);
%csattribute(Urho3D::SystemUI, %arg(bool), PassThroughEvents, GetPassThroughEvents, SetPassThroughEvents);
%csattribute(Urho3D::DebugHud, %arg(Urho3D::DebugHudModeFlags), Mode, GetMode, SetMode);
%csattribute(Urho3D::DebugHud, %arg(bool), UseRendererStats, GetUseRendererStats, SetUseRendererStats);
%csattribute(Urho3D::ResourceDragDropPayload, %arg(ea::string), DisplayString, GetDisplayString);
%csattribute(Urho3D::MaterialInspectorWidget, %arg(Urho3D::MaterialInspectorWidget::MaterialVector), Materials, GetMaterials);
%csattribute(Urho3D::SerializableInspectorWidget, %arg(ea::string), Title, GetTitle);
%csattribute(Urho3D::SerializableInspectorWidget, %arg(Urho3D::SerializableInspectorWidget::SerializableVector), Objects, GetObjects);
%csattribute(Urho3D::NodeInspectorWidget, %arg(Urho3D::NodeInspectorWidget::NodeVector), Nodes, GetNodes);
%csattribute(Urho3D::SceneHierarchyWidget, %arg(Urho3D::SceneHierarchySettings), Settings, GetSettings, SetSettings);
%csattribute(Urho3D::SceneWidget, %arg(Urho3D::Scene *), Scene, GetScene);
%csattribute(Urho3D::SceneWidget, %arg(Urho3D::SceneRendererToTexture *), Renderer, GetRenderer);
%csattribute(Urho3D::SceneWidget, %arg(Urho3D::Camera *), Camera, GetCamera);
%csattribute(Urho3D::SystemMessageBox, %arg(ea::string), Title, GetTitle, SetTitle);
%csattribute(Urho3D::SystemMessageBox, %arg(ea::string), Message, GetMessage, SetMessage);
%csattribute(Urho3D::SystemMessageBox, %arg(bool), IsOpen, IsOpen);
%csattribute(Urho3D::Texture2DWidget, %arg(Urho3D::Texture2D *), Texture2D, GetTexture2D);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class EndRenderingSystemUIEvent {
        private StringHash _event = new StringHash("EndRenderingSystemUI");
        public EndRenderingSystemUIEvent() { }
        public static implicit operator StringHash(EndRenderingSystemUIEvent e) { return e._event; }
    }
    public static EndRenderingSystemUIEvent EndRenderingSystemUI = new EndRenderingSystemUIEvent();
    public class ConsoleClosedEvent {
        private StringHash _event = new StringHash("ConsoleClosed");
        public ConsoleClosedEvent() { }
        public static implicit operator StringHash(ConsoleClosedEvent e) { return e._event; }
    }
    public static ConsoleClosedEvent ConsoleClosed = new ConsoleClosedEvent();
    public class AttributeInspectorMenuEvent {
        private StringHash _event = new StringHash("AttributeInspectorMenu");
        public StringHash Serializable = new StringHash("Serializable");
        public StringHash AttributeInfo = new StringHash("AttributeInfo");
        public AttributeInspectorMenuEvent() { }
        public static implicit operator StringHash(AttributeInspectorMenuEvent e) { return e._event; }
    }
    public static AttributeInspectorMenuEvent AttributeInspectorMenu = new AttributeInspectorMenuEvent();
    public class AttributeInspectorValueModifiedEvent {
        private StringHash _event = new StringHash("AttributeInspectorValueModified");
        public StringHash Serializable = new StringHash("Serializable");
        public StringHash AttributeInfo = new StringHash("AttributeInfo");
        public StringHash OldValue = new StringHash("OldValue");
        public StringHash NewValue = new StringHash("NewValue");
        public StringHash Reason = new StringHash("Reason");
        public AttributeInspectorValueModifiedEvent() { }
        public static implicit operator StringHash(AttributeInspectorValueModifiedEvent e) { return e._event; }
    }
    public static AttributeInspectorValueModifiedEvent AttributeInspectorValueModified = new AttributeInspectorValueModifiedEvent();
    public class AttributeInspectorAttributeEvent {
        private StringHash _event = new StringHash("AttributeInspectorAttribute");
        public StringHash Serializable = new StringHash("Serializable");
        public StringHash AttributeInfo = new StringHash("AttributeInfo");
        public StringHash Color = new StringHash("Color");
        public StringHash Hidden = new StringHash("Hidden");
        public StringHash Tooltip = new StringHash("Tooltip");
        public StringHash ValueKind = new StringHash("ValueKind");
        public AttributeInspectorAttributeEvent() { }
        public static implicit operator StringHash(AttributeInspectorAttributeEvent e) { return e._event; }
    }
    public static AttributeInspectorAttributeEvent AttributeInspectorAttribute = new AttributeInspectorAttributeEvent();
    public class GizmoNodeModifiedEvent {
        private StringHash _event = new StringHash("GizmoNodeModified");
        public StringHash Node = new StringHash("Node");
        public StringHash OldTransform = new StringHash("OldTransform");
        public StringHash NewTransform = new StringHash("NewTransform");
        public GizmoNodeModifiedEvent() { }
        public static implicit operator StringHash(GizmoNodeModifiedEvent e) { return e._event; }
    }
    public static GizmoNodeModifiedEvent GizmoNodeModified = new GizmoNodeModifiedEvent();
    public class GizmoSelectionChangedEvent {
        private StringHash _event = new StringHash("GizmoSelectionChanged");
        public GizmoSelectionChangedEvent() { }
        public static implicit operator StringHash(GizmoSelectionChangedEvent e) { return e._event; }
    }
    public static GizmoSelectionChangedEvent GizmoSelectionChanged = new GizmoSelectionChangedEvent();
} %}
