%pragma(csharp) moduleimports=%{
public static class E
{
    public class SoundFinishedEvent {
        private StringHash _event = new StringHash("SoundFinished");

        public StringHash Node = new StringHash("Node");
        public StringHash SoundSource = new StringHash("SoundSource");
        public StringHash Sound = new StringHash("Sound");
        public SoundFinishedEvent() { }
        public static implicit operator StringHash(SoundFinishedEvent e) { return e._event; }
    }
    public static SoundFinishedEvent SoundFinished = new SoundFinishedEvent();

    public class BeginFrameEvent {
        private StringHash _event = new StringHash("BeginFrame");

        public StringHash FrameNumber = new StringHash("FrameNumber");
        public StringHash TimeStep = new StringHash("TimeStep");
        public BeginFrameEvent() { }
        public static implicit operator StringHash(BeginFrameEvent e) { return e._event; }
    }
    public static BeginFrameEvent BeginFrame = new BeginFrameEvent();

    public class UpdateEvent {
        private StringHash _event = new StringHash("Update");

        public StringHash TimeStep = new StringHash("TimeStep");
        public UpdateEvent() { }
        public static implicit operator StringHash(UpdateEvent e) { return e._event; }
    }
    public static UpdateEvent Update = new UpdateEvent();

    public class PostUpdateEvent {
        private StringHash _event = new StringHash("PostUpdate");

        public StringHash TimeStep = new StringHash("TimeStep");
        public PostUpdateEvent() { }
        public static implicit operator StringHash(PostUpdateEvent e) { return e._event; }
    }
    public static PostUpdateEvent PostUpdate = new PostUpdateEvent();

    public class RenderUpdateEvent {
        private StringHash _event = new StringHash("RenderUpdate");

        public StringHash TimeStep = new StringHash("TimeStep");
        public RenderUpdateEvent() { }
        public static implicit operator StringHash(RenderUpdateEvent e) { return e._event; }
    }
    public static RenderUpdateEvent RenderUpdate = new RenderUpdateEvent();

    public class PostRenderUpdateEvent {
        private StringHash _event = new StringHash("PostRenderUpdate");

        public StringHash TimeStep = new StringHash("TimeStep");
        public PostRenderUpdateEvent() { }
        public static implicit operator StringHash(PostRenderUpdateEvent e) { return e._event; }
    }
    public static PostRenderUpdateEvent PostRenderUpdate = new PostRenderUpdateEvent();

    public class EndFrameEvent {
        private StringHash _event = new StringHash("EndFrame");

        public EndFrameEvent() { }
        public static implicit operator StringHash(EndFrameEvent e) { return e._event; }
    }
    public static EndFrameEvent EndFrame = new EndFrameEvent();

    public class WorkItemCompletedEvent {
        private StringHash _event = new StringHash("WorkItemCompleted");

        public StringHash Item = new StringHash("Item");
        public WorkItemCompletedEvent() { }
        public static implicit operator StringHash(WorkItemCompletedEvent e) { return e._event; }
    }
    public static WorkItemCompletedEvent WorkItemCompleted = new WorkItemCompletedEvent();

    public class ConsoleCommandEvent {
        private StringHash _event = new StringHash("ConsoleCommand");

        public StringHash Command = new StringHash("Command");
        public StringHash Id = new StringHash("Id");
        public ConsoleCommandEvent() { }
        public static implicit operator StringHash(ConsoleCommandEvent e) { return e._event; }
    }
    public static ConsoleCommandEvent ConsoleCommand = new ConsoleCommandEvent();

    public class EngineInitializedEvent {
        private StringHash _event = new StringHash("EngineInitialized");

        public EngineInitializedEvent() { }
        public static implicit operator StringHash(EngineInitializedEvent e) { return e._event; }
    }
    public static EngineInitializedEvent EngineInitialized = new EngineInitializedEvent();

    public class ApplicationStartedEvent {
        private StringHash _event = new StringHash("ApplicationStarted");

        public ApplicationStartedEvent() { }
        public static implicit operator StringHash(ApplicationStartedEvent e) { return e._event; }
    }
    public static ApplicationStartedEvent ApplicationStarted = new ApplicationStartedEvent();

    public class BoneHierarchyCreatedEvent {
        private StringHash _event = new StringHash("BoneHierarchyCreated");

        public StringHash Node = new StringHash("Node");
        public BoneHierarchyCreatedEvent() { }
        public static implicit operator StringHash(BoneHierarchyCreatedEvent e) { return e._event; }
    }
    public static BoneHierarchyCreatedEvent BoneHierarchyCreated = new BoneHierarchyCreatedEvent();

    public class AnimationTriggerEvent {
        private StringHash _event = new StringHash("AnimationTrigger");

        public StringHash Node = new StringHash("Node");
        public StringHash Animation = new StringHash("Animation");
        public StringHash Name = new StringHash("Name");
        public StringHash Time = new StringHash("Time");
        public StringHash Data = new StringHash("Data");
        public AnimationTriggerEvent() { }
        public static implicit operator StringHash(AnimationTriggerEvent e) { return e._event; }
    }
    public static AnimationTriggerEvent AnimationTrigger = new AnimationTriggerEvent();

    public class AnimationFinishedEvent {
        private StringHash _event = new StringHash("AnimationFinished");

        public StringHash Node = new StringHash("Node");
        public StringHash Animation = new StringHash("Animation");
        public StringHash Name = new StringHash("Name");
        public StringHash Looped = new StringHash("Looped");
        public AnimationFinishedEvent() { }
        public static implicit operator StringHash(AnimationFinishedEvent e) { return e._event; }
    }
    public static AnimationFinishedEvent AnimationFinished = new AnimationFinishedEvent();

    public class ParticleEffectFinishedEvent {
        private StringHash _event = new StringHash("ParticleEffectFinished");

        public StringHash Node = new StringHash("Node");
        public StringHash Effect = new StringHash("Effect");
        public ParticleEffectFinishedEvent() { }
        public static implicit operator StringHash(ParticleEffectFinishedEvent e) { return e._event; }
    }
    public static ParticleEffectFinishedEvent ParticleEffectFinished = new ParticleEffectFinishedEvent();

    public class TerrainCreatedEvent {
        private StringHash _event = new StringHash("TerrainCreated");

        public StringHash Node = new StringHash("Node");
        public TerrainCreatedEvent() { }
        public static implicit operator StringHash(TerrainCreatedEvent e) { return e._event; }
    }
    public static TerrainCreatedEvent TerrainCreated = new TerrainCreatedEvent();

    public class ScreenModeEvent {
        private StringHash _event = new StringHash("ScreenMode");

        public StringHash Width = new StringHash("Width");
        public StringHash Height = new StringHash("Height");
        public StringHash Fullscreen = new StringHash("Fullscreen");
        public StringHash Borderless = new StringHash("Borderless");
        public StringHash Resizable = new StringHash("Resizable");
        public StringHash HighDPI = new StringHash("HighDPI");
        public StringHash Monitor = new StringHash("Monitor");
        public StringHash RefreshRate = new StringHash("RefreshRate");
        public ScreenModeEvent() { }
        public static implicit operator StringHash(ScreenModeEvent e) { return e._event; }
    }
    public static ScreenModeEvent ScreenMode = new ScreenModeEvent();

    public class WindowPosEvent {
        private StringHash _event = new StringHash("WindowPos");

        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public WindowPosEvent() { }
        public static implicit operator StringHash(WindowPosEvent e) { return e._event; }
    }
    public static WindowPosEvent WindowPos = new WindowPosEvent();

    public class RenderSurfaceUpdateEvent {
        private StringHash _event = new StringHash("RenderSurfaceUpdate");

        public RenderSurfaceUpdateEvent() { }
        public static implicit operator StringHash(RenderSurfaceUpdateEvent e) { return e._event; }
    }
    public static RenderSurfaceUpdateEvent RenderSurfaceUpdate = new RenderSurfaceUpdateEvent();

    public class BeginRenderingEvent {
        private StringHash _event = new StringHash("BeginRendering");

        public BeginRenderingEvent() { }
        public static implicit operator StringHash(BeginRenderingEvent e) { return e._event; }
    }
    public static BeginRenderingEvent BeginRendering = new BeginRenderingEvent();

    public class EndRenderingEvent {
        private StringHash _event = new StringHash("EndRendering");

        public EndRenderingEvent() { }
        public static implicit operator StringHash(EndRenderingEvent e) { return e._event; }
    }
    public static EndRenderingEvent EndRendering = new EndRenderingEvent();

    public class BeginViewUpdateEvent {
        private StringHash _event = new StringHash("BeginViewUpdate");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public BeginViewUpdateEvent() { }
        public static implicit operator StringHash(BeginViewUpdateEvent e) { return e._event; }
    }
    public static BeginViewUpdateEvent BeginViewUpdate = new BeginViewUpdateEvent();

    public class EndViewUpdateEvent {
        private StringHash _event = new StringHash("EndViewUpdate");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public EndViewUpdateEvent() { }
        public static implicit operator StringHash(EndViewUpdateEvent e) { return e._event; }
    }
    public static EndViewUpdateEvent EndViewUpdate = new EndViewUpdateEvent();

    public class BeginViewRenderEvent {
        private StringHash _event = new StringHash("BeginViewRender");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public BeginViewRenderEvent() { }
        public static implicit operator StringHash(BeginViewRenderEvent e) { return e._event; }
    }
    public static BeginViewRenderEvent BeginViewRender = new BeginViewRenderEvent();

    public class ViewBuffersReadyEvent {
        private StringHash _event = new StringHash("ViewBuffersReady");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public ViewBuffersReadyEvent() { }
        public static implicit operator StringHash(ViewBuffersReadyEvent e) { return e._event; }
    }
    public static ViewBuffersReadyEvent ViewBuffersReady = new ViewBuffersReadyEvent();

    public class ViewGlobalShaderParametersEvent {
        private StringHash _event = new StringHash("ViewGlobalShaderParameters");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public ViewGlobalShaderParametersEvent() { }
        public static implicit operator StringHash(ViewGlobalShaderParametersEvent e) { return e._event; }
    }
    public static ViewGlobalShaderParametersEvent ViewGlobalShaderParameters = new ViewGlobalShaderParametersEvent();

    public class EndViewRenderEvent {
        private StringHash _event = new StringHash("EndViewRender");

        public StringHash View = new StringHash("View");
        public StringHash Texture = new StringHash("Texture");
        public StringHash Surface = new StringHash("Surface");
        public StringHash Scene = new StringHash("Scene");
        public StringHash Camera = new StringHash("Camera");
        public EndViewRenderEvent() { }
        public static implicit operator StringHash(EndViewRenderEvent e) { return e._event; }
    }
    public static EndViewRenderEvent EndViewRender = new EndViewRenderEvent();

    public class EndAllViewsRenderEvent {
        private StringHash _event = new StringHash("EndAllViewsRender");

        public EndAllViewsRenderEvent() { }
        public static implicit operator StringHash(EndAllViewsRenderEvent e) { return e._event; }
    }
    public static EndAllViewsRenderEvent EndAllViewsRender = new EndAllViewsRenderEvent();

    public class RenderPathEventEvent {
        private StringHash _event = new StringHash("RenderPathEvent");

        public StringHash Name = new StringHash("Name");
        public RenderPathEventEvent() { }
        public static implicit operator StringHash(RenderPathEventEvent e) { return e._event; }
    }
    public static RenderPathEventEvent RenderPathEvent = new RenderPathEventEvent();

    public class DeviceLostEvent {
        private StringHash _event = new StringHash("DeviceLost");

        public DeviceLostEvent() { }
        public static implicit operator StringHash(DeviceLostEvent e) { return e._event; }
    }
    public static DeviceLostEvent DeviceLost = new DeviceLostEvent();

    public class DeviceResetEvent {
        private StringHash _event = new StringHash("DeviceReset");

        public DeviceResetEvent() { }
        public static implicit operator StringHash(DeviceResetEvent e) { return e._event; }
    }
    public static DeviceResetEvent DeviceReset = new DeviceResetEvent();

    public class IKEffectorTargetChangedEvent {
        private StringHash _event = new StringHash("IKEffectorTargetChanged");

        public StringHash EffectorNode = new StringHash("EffectorNode");
        public StringHash TargetNode = new StringHash("TargetNode");
        public IKEffectorTargetChangedEvent() { }
        public static implicit operator StringHash(IKEffectorTargetChangedEvent e) { return e._event; }
    }
    public static IKEffectorTargetChangedEvent IKEffectorTargetChanged = new IKEffectorTargetChangedEvent();

    public class LogMessageEvent {
        private StringHash _event = new StringHash("LogMessage");

        public StringHash Message = new StringHash("Message");
        public StringHash Logger = new StringHash("Logger");
        public StringHash Level = new StringHash("Level");
        public StringHash Time = new StringHash("Time");
        public LogMessageEvent() { }
        public static implicit operator StringHash(LogMessageEvent e) { return e._event; }
    }
    public static LogMessageEvent LogMessage = new LogMessageEvent();

    public class AsyncExecFinishedEvent {
        private StringHash _event = new StringHash("AsyncExecFinished");

        public StringHash RequestID = new StringHash("RequestID");
        public StringHash ExitCode = new StringHash("ExitCode");
        public AsyncExecFinishedEvent() { }
        public static implicit operator StringHash(AsyncExecFinishedEvent e) { return e._event; }
    }
    public static AsyncExecFinishedEvent AsyncExecFinished = new AsyncExecFinishedEvent();

    public class MouseButtonDownEvent {
        private StringHash _event = new StringHash("MouseButtonDown");

        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseButtonDownEvent() { }
        public static implicit operator StringHash(MouseButtonDownEvent e) { return e._event; }
    }
    public static MouseButtonDownEvent MouseButtonDown = new MouseButtonDownEvent();

    public class MouseButtonUpEvent {
        private StringHash _event = new StringHash("MouseButtonUp");

        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseButtonUpEvent() { }
        public static implicit operator StringHash(MouseButtonUpEvent e) { return e._event; }
    }
    public static MouseButtonUpEvent MouseButtonUp = new MouseButtonUpEvent();

    public class MouseMoveEvent {
        private StringHash _event = new StringHash("MouseMove");

        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Dx = new StringHash("DX");
        public StringHash Dy = new StringHash("DY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseMoveEvent() { }
        public static implicit operator StringHash(MouseMoveEvent e) { return e._event; }
    }
    public static MouseMoveEvent MouseMove = new MouseMoveEvent();

    public class MouseWheelEvent {
        private StringHash _event = new StringHash("MouseWheel");

        public StringHash Wheel = new StringHash("Wheel");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public MouseWheelEvent() { }
        public static implicit operator StringHash(MouseWheelEvent e) { return e._event; }
    }
    public static MouseWheelEvent MouseWheel = new MouseWheelEvent();

    public class KeyDownEvent {
        private StringHash _event = new StringHash("KeyDown");

        public StringHash Key = new StringHash("Key");
        public StringHash Scancode = new StringHash("Scancode");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public StringHash Repeat = new StringHash("Repeat");
        public KeyDownEvent() { }
        public static implicit operator StringHash(KeyDownEvent e) { return e._event; }
    }
    public static KeyDownEvent KeyDown = new KeyDownEvent();

    public class KeyUpEvent {
        private StringHash _event = new StringHash("KeyUp");

        public StringHash Key = new StringHash("Key");
        public StringHash Scancode = new StringHash("Scancode");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public KeyUpEvent() { }
        public static implicit operator StringHash(KeyUpEvent e) { return e._event; }
    }
    public static KeyUpEvent KeyUp = new KeyUpEvent();

    public class TextInputEvent {
        private StringHash _event = new StringHash("TextInput");

        public StringHash Text = new StringHash("Text");
        public TextInputEvent() { }
        public static implicit operator StringHash(TextInputEvent e) { return e._event; }
    }
    public static TextInputEvent TextInput = new TextInputEvent();

    public class TextEditingEvent {
        private StringHash _event = new StringHash("TextEditing");

        public StringHash Composition = new StringHash("Composition");
        public StringHash Cursor = new StringHash("Cursor");
        public StringHash SelectionLength = new StringHash("SelectionLength");
        public TextEditingEvent() { }
        public static implicit operator StringHash(TextEditingEvent e) { return e._event; }
    }
    public static TextEditingEvent TextEditing = new TextEditingEvent();

    public class JoystickConnectedEvent {
        private StringHash _event = new StringHash("JoystickConnected");

        public StringHash JoystickID = new StringHash("JoystickID");
        public JoystickConnectedEvent() { }
        public static implicit operator StringHash(JoystickConnectedEvent e) { return e._event; }
    }
    public static JoystickConnectedEvent JoystickConnected = new JoystickConnectedEvent();

    public class JoystickDisconnectedEvent {
        private StringHash _event = new StringHash("JoystickDisconnected");

        public StringHash JoystickID = new StringHash("JoystickID");
        public JoystickDisconnectedEvent() { }
        public static implicit operator StringHash(JoystickDisconnectedEvent e) { return e._event; }
    }
    public static JoystickDisconnectedEvent JoystickDisconnected = new JoystickDisconnectedEvent();

    public class JoystickButtonDownEvent {
        private StringHash _event = new StringHash("JoystickButtonDown");

        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public JoystickButtonDownEvent() { }
        public static implicit operator StringHash(JoystickButtonDownEvent e) { return e._event; }
    }
    public static JoystickButtonDownEvent JoystickButtonDown = new JoystickButtonDownEvent();

    public class JoystickButtonUpEvent {
        private StringHash _event = new StringHash("JoystickButtonUp");

        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public JoystickButtonUpEvent() { }
        public static implicit operator StringHash(JoystickButtonUpEvent e) { return e._event; }
    }
    public static JoystickButtonUpEvent JoystickButtonUp = new JoystickButtonUpEvent();

    public class JoystickAxisMoveEvent {
        private StringHash _event = new StringHash("JoystickAxisMove");

        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public StringHash Position = new StringHash("Position");
        public JoystickAxisMoveEvent() { }
        public static implicit operator StringHash(JoystickAxisMoveEvent e) { return e._event; }
    }
    public static JoystickAxisMoveEvent JoystickAxisMove = new JoystickAxisMoveEvent();

    public class JoystickHatMoveEvent {
        private StringHash _event = new StringHash("JoystickHatMove");

        public StringHash JoystickID = new StringHash("JoystickID");
        public StringHash Button = new StringHash("Button");
        public StringHash Position = new StringHash("Position");
        public JoystickHatMoveEvent() { }
        public static implicit operator StringHash(JoystickHatMoveEvent e) { return e._event; }
    }
    public static JoystickHatMoveEvent JoystickHatMove = new JoystickHatMoveEvent();

    public class TouchBeginEvent {
        private StringHash _event = new StringHash("TouchBegin");

        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Pressure = new StringHash("Pressure");
        public TouchBeginEvent() { }
        public static implicit operator StringHash(TouchBeginEvent e) { return e._event; }
    }
    public static TouchBeginEvent TouchBegin = new TouchBeginEvent();

    public class TouchEndEvent {
        private StringHash _event = new StringHash("TouchEnd");

        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public TouchEndEvent() { }
        public static implicit operator StringHash(TouchEndEvent e) { return e._event; }
    }
    public static TouchEndEvent TouchEnd = new TouchEndEvent();

    public class TouchMoveEvent {
        private StringHash _event = new StringHash("TouchMove");

        public StringHash TouchID = new StringHash("TouchID");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Dx = new StringHash("DX");
        public StringHash Dy = new StringHash("DY");
        public StringHash Pressure = new StringHash("Pressure");
        public TouchMoveEvent() { }
        public static implicit operator StringHash(TouchMoveEvent e) { return e._event; }
    }
    public static TouchMoveEvent TouchMove = new TouchMoveEvent();

    public class GestureRecordedEvent {
        private StringHash _event = new StringHash("GestureRecorded");

        public StringHash GestureID = new StringHash("GestureID");
        public GestureRecordedEvent() { }
        public static implicit operator StringHash(GestureRecordedEvent e) { return e._event; }
    }
    public static GestureRecordedEvent GestureRecorded = new GestureRecordedEvent();

    public class GestureInputEvent {
        private StringHash _event = new StringHash("GestureInput");

        public StringHash GestureID = new StringHash("GestureID");
        public StringHash CenterX = new StringHash("CenterX");
        public StringHash CenterY = new StringHash("CenterY");
        public StringHash NumFingers = new StringHash("NumFingers");
        public StringHash Error = new StringHash("Error");
        public GestureInputEvent() { }
        public static implicit operator StringHash(GestureInputEvent e) { return e._event; }
    }
    public static GestureInputEvent GestureInput = new GestureInputEvent();

    public class MultiGestureEvent {
        private StringHash _event = new StringHash("MultiGesture");

        public StringHash CenterX = new StringHash("CenterX");
        public StringHash CenterY = new StringHash("CenterY");
        public StringHash NumFingers = new StringHash("NumFingers");
        public StringHash DTheta = new StringHash("DTheta");
        public StringHash DDist = new StringHash("DDist");
        public MultiGestureEvent() { }
        public static implicit operator StringHash(MultiGestureEvent e) { return e._event; }
    }
    public static MultiGestureEvent MultiGesture = new MultiGestureEvent();

    public class DropFileEvent {
        private StringHash _event = new StringHash("DropFile");

        public StringHash FileName = new StringHash("FileName");
        public DropFileEvent() { }
        public static implicit operator StringHash(DropFileEvent e) { return e._event; }
    }
    public static DropFileEvent DropFile = new DropFileEvent();

    public class InputFocusEvent {
        private StringHash _event = new StringHash("InputFocus");

        public StringHash Focus = new StringHash("Focus");
        public StringHash Minimized = new StringHash("Minimized");
        public InputFocusEvent() { }
        public static implicit operator StringHash(InputFocusEvent e) { return e._event; }
    }
    public static InputFocusEvent InputFocus = new InputFocusEvent();

    public class MouseVisibleChangedEvent {
        private StringHash _event = new StringHash("MouseVisibleChanged");

        public StringHash Visible = new StringHash("Visible");
        public MouseVisibleChangedEvent() { }
        public static implicit operator StringHash(MouseVisibleChangedEvent e) { return e._event; }
    }
    public static MouseVisibleChangedEvent MouseVisibleChanged = new MouseVisibleChangedEvent();

    public class MouseModeChangedEvent {
        private StringHash _event = new StringHash("MouseModeChanged");

        public StringHash Mode = new StringHash("Mode");
        public StringHash MouseLocked = new StringHash("MouseLocked");
        public MouseModeChangedEvent() { }
        public static implicit operator StringHash(MouseModeChangedEvent e) { return e._event; }
    }
    public static MouseModeChangedEvent MouseModeChanged = new MouseModeChangedEvent();

    public class ExitRequestedEvent {
        private StringHash _event = new StringHash("ExitRequested");

        public ExitRequestedEvent() { }
        public static implicit operator StringHash(ExitRequestedEvent e) { return e._event; }
    }
    public static ExitRequestedEvent ExitRequested = new ExitRequestedEvent();

    public class SDLRawInputEvent {
        private StringHash _event = new StringHash("SDLRawInput");

        public StringHash SDLEvent = new StringHash("SDLEvent");
        public StringHash Consumed = new StringHash("Consumed");
        public SDLRawInputEvent() { }
        public static implicit operator StringHash(SDLRawInputEvent e) { return e._event; }
    }
    public static SDLRawInputEvent SDLRawInput = new SDLRawInputEvent();

    public class InputBeginEvent {
        private StringHash _event = new StringHash("InputBegin");

        public InputBeginEvent() { }
        public static implicit operator StringHash(InputBeginEvent e) { return e._event; }
    }
    public static InputBeginEvent InputBegin = new InputBeginEvent();

    public class InputEndEvent {
        private StringHash _event = new StringHash("InputEnd");

        public InputEndEvent() { }
        public static implicit operator StringHash(InputEndEvent e) { return e._event; }
    }
    public static InputEndEvent InputEnd = new InputEndEvent();

    public class NavigationMeshRebuiltEvent {
        private StringHash _event = new StringHash("NavigationMeshRebuilt");

        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public NavigationMeshRebuiltEvent() { }
        public static implicit operator StringHash(NavigationMeshRebuiltEvent e) { return e._event; }
    }
    public static NavigationMeshRebuiltEvent NavigationMeshRebuilt = new NavigationMeshRebuiltEvent();

    public class NavigationAreaRebuiltEvent {
        private StringHash _event = new StringHash("NavigationAreaRebuilt");

        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash BoundsMin = new StringHash("BoundsMin");
        public StringHash BoundsMax = new StringHash("BoundsMax");
        public NavigationAreaRebuiltEvent() { }
        public static implicit operator StringHash(NavigationAreaRebuiltEvent e) { return e._event; }
    }
    public static NavigationAreaRebuiltEvent NavigationAreaRebuilt = new NavigationAreaRebuiltEvent();

    public class NavigationTileAddedEvent {
        private StringHash _event = new StringHash("NavigationTileAdded");

        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash Tile = new StringHash("Tile");
        public NavigationTileAddedEvent() { }
        public static implicit operator StringHash(NavigationTileAddedEvent e) { return e._event; }
    }
    public static NavigationTileAddedEvent NavigationTileAdded = new NavigationTileAddedEvent();

    public class NavigationTileRemovedEvent {
        private StringHash _event = new StringHash("NavigationTileRemoved");

        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public StringHash Tile = new StringHash("Tile");
        public NavigationTileRemovedEvent() { }
        public static implicit operator StringHash(NavigationTileRemovedEvent e) { return e._event; }
    }
    public static NavigationTileRemovedEvent NavigationTileRemoved = new NavigationTileRemovedEvent();

    public class NavigationAllTilesRemovedEvent {
        private StringHash _event = new StringHash("NavigationAllTilesRemoved");

        public StringHash Node = new StringHash("Node");
        public StringHash Mesh = new StringHash("Mesh");
        public NavigationAllTilesRemovedEvent() { }
        public static implicit operator StringHash(NavigationAllTilesRemovedEvent e) { return e._event; }
    }
    public static NavigationAllTilesRemovedEvent NavigationAllTilesRemoved = new NavigationAllTilesRemovedEvent();

    public class CrowdAgentFormationEvent {
        private StringHash _event = new StringHash("CrowdAgentFormation");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Index = new StringHash("Index");
        public StringHash Size = new StringHash("Size");
        public StringHash Position = new StringHash("Position");
        public CrowdAgentFormationEvent() { }
        public static implicit operator StringHash(CrowdAgentFormationEvent e) { return e._event; }
    }
    public static CrowdAgentFormationEvent CrowdAgentFormation = new CrowdAgentFormationEvent();

    public class CrowdAgentNodeFormationEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeFormation");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Index = new StringHash("Index");
        public StringHash Size = new StringHash("Size");
        public StringHash Position = new StringHash("Position");
        public CrowdAgentNodeFormationEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeFormationEvent e) { return e._event; }
    }
    public static CrowdAgentNodeFormationEvent CrowdAgentNodeFormation = new CrowdAgentNodeFormationEvent();

    public class CrowdAgentRepositionEvent {
        private StringHash _event = new StringHash("CrowdAgentReposition");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash Arrived = new StringHash("Arrived");
        public StringHash TimeStep = new StringHash("TimeStep");
        public CrowdAgentRepositionEvent() { }
        public static implicit operator StringHash(CrowdAgentRepositionEvent e) { return e._event; }
    }
    public static CrowdAgentRepositionEvent CrowdAgentReposition = new CrowdAgentRepositionEvent();

    public class CrowdAgentNodeRepositionEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeReposition");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash Arrived = new StringHash("Arrived");
        public StringHash TimeStep = new StringHash("TimeStep");
        public CrowdAgentNodeRepositionEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeRepositionEvent e) { return e._event; }
    }
    public static CrowdAgentNodeRepositionEvent CrowdAgentNodeReposition = new CrowdAgentNodeRepositionEvent();

    public class CrowdAgentFailureEvent {
        private StringHash _event = new StringHash("CrowdAgentFailure");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentFailureEvent() { }
        public static implicit operator StringHash(CrowdAgentFailureEvent e) { return e._event; }
    }
    public static CrowdAgentFailureEvent CrowdAgentFailure = new CrowdAgentFailureEvent();

    public class CrowdAgentNodeFailureEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeFailure");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentNodeFailureEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeFailureEvent e) { return e._event; }
    }
    public static CrowdAgentNodeFailureEvent CrowdAgentNodeFailure = new CrowdAgentNodeFailureEvent();

    public class CrowdAgentStateChangedEvent {
        private StringHash _event = new StringHash("CrowdAgentStateChanged");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentStateChangedEvent() { }
        public static implicit operator StringHash(CrowdAgentStateChangedEvent e) { return e._event; }
    }
    public static CrowdAgentStateChangedEvent CrowdAgentStateChanged = new CrowdAgentStateChangedEvent();

    public class CrowdAgentNodeStateChangedEvent {
        private StringHash _event = new StringHash("CrowdAgentNodeStateChanged");

        public StringHash Node = new StringHash("Node");
        public StringHash CrowdAgent = new StringHash("CrowdAgent");
        public StringHash Position = new StringHash("Position");
        public StringHash Velocity = new StringHash("Velocity");
        public StringHash CrowdAgentState = new StringHash("CrowdAgentState");
        public StringHash CrowdTargetState = new StringHash("CrowdTargetState");
        public CrowdAgentNodeStateChangedEvent() { }
        public static implicit operator StringHash(CrowdAgentNodeStateChangedEvent e) { return e._event; }
    }
    public static CrowdAgentNodeStateChangedEvent CrowdAgentNodeStateChanged = new CrowdAgentNodeStateChangedEvent();

    public class NavigationObstacleAddedEvent {
        private StringHash _event = new StringHash("NavigationObstacleAdded");

        public StringHash Node = new StringHash("Node");
        public StringHash Obstacle = new StringHash("Obstacle");
        public StringHash Position = new StringHash("Position");
        public StringHash Radius = new StringHash("Radius");
        public StringHash Height = new StringHash("Height");
        public NavigationObstacleAddedEvent() { }
        public static implicit operator StringHash(NavigationObstacleAddedEvent e) { return e._event; }
    }
    public static NavigationObstacleAddedEvent NavigationObstacleAdded = new NavigationObstacleAddedEvent();

    public class NavigationObstacleRemovedEvent {
        private StringHash _event = new StringHash("NavigationObstacleRemoved");

        public StringHash Node = new StringHash("Node");
        public StringHash Obstacle = new StringHash("Obstacle");
        public StringHash Position = new StringHash("Position");
        public StringHash Radius = new StringHash("Radius");
        public StringHash Height = new StringHash("Height");
        public NavigationObstacleRemovedEvent() { }
        public static implicit operator StringHash(NavigationObstacleRemovedEvent e) { return e._event; }
    }
    public static NavigationObstacleRemovedEvent NavigationObstacleRemoved = new NavigationObstacleRemovedEvent();

    public class ServerConnectedEvent {
        private StringHash _event = new StringHash("ServerConnected");

        public ServerConnectedEvent() { }
        public static implicit operator StringHash(ServerConnectedEvent e) { return e._event; }
    }
    public static ServerConnectedEvent ServerConnected = new ServerConnectedEvent();

    public class ServerDisconnectedEvent {
        private StringHash _event = new StringHash("ServerDisconnected");

        public ServerDisconnectedEvent() { }
        public static implicit operator StringHash(ServerDisconnectedEvent e) { return e._event; }
    }
    public static ServerDisconnectedEvent ServerDisconnected = new ServerDisconnectedEvent();

    public class ConnectFailedEvent {
        private StringHash _event = new StringHash("ConnectFailed");

        public ConnectFailedEvent() { }
        public static implicit operator StringHash(ConnectFailedEvent e) { return e._event; }
    }
    public static ConnectFailedEvent ConnectFailed = new ConnectFailedEvent();

    public class ClientConnectedEvent {
        private StringHash _event = new StringHash("ClientConnected");

        public StringHash Connection = new StringHash("Connection");
        public ClientConnectedEvent() { }
        public static implicit operator StringHash(ClientConnectedEvent e) { return e._event; }
    }
    public static ClientConnectedEvent ClientConnected = new ClientConnectedEvent();

    public class ClientDisconnectedEvent {
        private StringHash _event = new StringHash("ClientDisconnected");

        public StringHash Connection = new StringHash("Connection");
        public ClientDisconnectedEvent() { }
        public static implicit operator StringHash(ClientDisconnectedEvent e) { return e._event; }
    }
    public static ClientDisconnectedEvent ClientDisconnected = new ClientDisconnectedEvent();

    public class ClientIdentityEvent {
        private StringHash _event = new StringHash("ClientIdentity");

        public StringHash Connection = new StringHash("Connection");
        public StringHash Allow = new StringHash("Allow");
        public ClientIdentityEvent() { }
        public static implicit operator StringHash(ClientIdentityEvent e) { return e._event; }
    }
    public static ClientIdentityEvent ClientIdentity = new ClientIdentityEvent();

    public class ClientSceneLoadedEvent {
        private StringHash _event = new StringHash("ClientSceneLoaded");

        public StringHash Connection = new StringHash("Connection");
        public ClientSceneLoadedEvent() { }
        public static implicit operator StringHash(ClientSceneLoadedEvent e) { return e._event; }
    }
    public static ClientSceneLoadedEvent ClientSceneLoaded = new ClientSceneLoadedEvent();

    public class NetworkMessageEvent {
        private StringHash _event = new StringHash("NetworkMessage");

        public StringHash Connection = new StringHash("Connection");
        public StringHash MessageID = new StringHash("MessageID");
        public StringHash Data = new StringHash("Data");
        public NetworkMessageEvent() { }
        public static implicit operator StringHash(NetworkMessageEvent e) { return e._event; }
    }
    public static NetworkMessageEvent NetworkMessage = new NetworkMessageEvent();

    public class NetworkUpdateEvent {
        private StringHash _event = new StringHash("NetworkUpdate");

        public NetworkUpdateEvent() { }
        public static implicit operator StringHash(NetworkUpdateEvent e) { return e._event; }
    }
    public static NetworkUpdateEvent NetworkUpdate = new NetworkUpdateEvent();

    public class NetworkUpdateSentEvent {
        private StringHash _event = new StringHash("NetworkUpdateSent");

        public NetworkUpdateSentEvent() { }
        public static implicit operator StringHash(NetworkUpdateSentEvent e) { return e._event; }
    }
    public static NetworkUpdateSentEvent NetworkUpdateSent = new NetworkUpdateSentEvent();

    public class NetworkSceneLoadFailedEvent {
        private StringHash _event = new StringHash("NetworkSceneLoadFailed");

        public StringHash Connection = new StringHash("Connection");
        public NetworkSceneLoadFailedEvent() { }
        public static implicit operator StringHash(NetworkSceneLoadFailedEvent e) { return e._event; }
    }
    public static NetworkSceneLoadFailedEvent NetworkSceneLoadFailed = new NetworkSceneLoadFailedEvent();

    public class RemoteEventDataEvent {
        private StringHash _event = new StringHash("RemoteEventData");

        public StringHash Connection = new StringHash("Connection");
        public RemoteEventDataEvent() { }
        public static implicit operator StringHash(RemoteEventDataEvent e) { return e._event; }
    }
    public static RemoteEventDataEvent RemoteEventData = new RemoteEventDataEvent();

    public class NetworkBannedEvent {
        private StringHash _event = new StringHash("NetworkBanned");

        public NetworkBannedEvent() { }
        public static implicit operator StringHash(NetworkBannedEvent e) { return e._event; }
    }
    public static NetworkBannedEvent NetworkBanned = new NetworkBannedEvent();

    public class NetworkInvalidPasswordEvent {
        private StringHash _event = new StringHash("NetworkInvalidPassword");

        public NetworkInvalidPasswordEvent() { }
        public static implicit operator StringHash(NetworkInvalidPasswordEvent e) { return e._event; }
    }
    public static NetworkInvalidPasswordEvent NetworkInvalidPassword = new NetworkInvalidPasswordEvent();

    public class NetworkHostDiscoveredEvent {
        private StringHash _event = new StringHash("NetworkHostDiscovered");

        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public StringHash Beacon = new StringHash("Beacon");
        public NetworkHostDiscoveredEvent() { }
        public static implicit operator StringHash(NetworkHostDiscoveredEvent e) { return e._event; }
    }
    public static NetworkHostDiscoveredEvent NetworkHostDiscovered = new NetworkHostDiscoveredEvent();

    public class NetworkNatPunchtroughSucceededEvent {
        private StringHash _event = new StringHash("NetworkNatPunchtroughSucceeded");

        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public NetworkNatPunchtroughSucceededEvent() { }
        public static implicit operator StringHash(NetworkNatPunchtroughSucceededEvent e) { return e._event; }
    }
    public static NetworkNatPunchtroughSucceededEvent NetworkNatPunchtroughSucceeded = new NetworkNatPunchtroughSucceededEvent();

    public class NetworkNatPunchtroughFailedEvent {
        private StringHash _event = new StringHash("NetworkNatPunchtroughFailed");

        public StringHash Address = new StringHash("Address");
        public StringHash Port = new StringHash("Port");
        public NetworkNatPunchtroughFailedEvent() { }
        public static implicit operator StringHash(NetworkNatPunchtroughFailedEvent e) { return e._event; }
    }
    public static NetworkNatPunchtroughFailedEvent NetworkNatPunchtroughFailed = new NetworkNatPunchtroughFailedEvent();

    public class PhysicsPreStepEvent {
        private StringHash _event = new StringHash("PhysicsPreStep");

        public StringHash World = new StringHash("World");
        public StringHash TimeStep = new StringHash("TimeStep");
        public PhysicsPreStepEvent() { }
        public static implicit operator StringHash(PhysicsPreStepEvent e) { return e._event; }
    }
    public static PhysicsPreStepEvent PhysicsPreStep = new PhysicsPreStepEvent();

    public class PhysicsPostStepEvent {
        private StringHash _event = new StringHash("PhysicsPostStep");

        public StringHash World = new StringHash("World");
        public StringHash TimeStep = new StringHash("TimeStep");
        public PhysicsPostStepEvent() { }
        public static implicit operator StringHash(PhysicsPostStepEvent e) { return e._event; }
    }
    public static PhysicsPostStepEvent PhysicsPostStep = new PhysicsPostStepEvent();

    public class PhysicsCollisionStartEvent {
        private StringHash _event = new StringHash("PhysicsCollisionStart");

        public StringHash World = new StringHash("World");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash Trigger = new StringHash("Trigger");
        public StringHash Contacts = new StringHash("Contacts");
        public PhysicsCollisionStartEvent() { }
        public static implicit operator StringHash(PhysicsCollisionStartEvent e) { return e._event; }
    }
    public static PhysicsCollisionStartEvent PhysicsCollisionStart = new PhysicsCollisionStartEvent();

    public class PhysicsCollisionEvent {
        private StringHash _event = new StringHash("PhysicsCollision");

        public StringHash World = new StringHash("World");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash Trigger = new StringHash("Trigger");
        public StringHash Contacts = new StringHash("Contacts");
        public PhysicsCollisionEvent() { }
        public static implicit operator StringHash(PhysicsCollisionEvent e) { return e._event; }
    }
    public static PhysicsCollisionEvent PhysicsCollision = new PhysicsCollisionEvent();

    public class PhysicsCollisionEndEvent {
        private StringHash _event = new StringHash("PhysicsCollisionEnd");

        public StringHash World = new StringHash("World");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash Trigger = new StringHash("Trigger");
        public PhysicsCollisionEndEvent() { }
        public static implicit operator StringHash(PhysicsCollisionEndEvent e) { return e._event; }
    }
    public static PhysicsCollisionEndEvent PhysicsCollisionEnd = new PhysicsCollisionEndEvent();

    public class NodeCollisionStartEvent {
        private StringHash _event = new StringHash("NodeCollisionStart");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Trigger = new StringHash("Trigger");
        public StringHash Contacts = new StringHash("Contacts");
        public NodeCollisionStartEvent() { }
        public static implicit operator StringHash(NodeCollisionStartEvent e) { return e._event; }
    }
    public static NodeCollisionStartEvent NodeCollisionStart = new NodeCollisionStartEvent();

    public class NodeCollisionEvent {
        private StringHash _event = new StringHash("NodeCollision");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Trigger = new StringHash("Trigger");
        public StringHash Contacts = new StringHash("Contacts");
        public NodeCollisionEvent() { }
        public static implicit operator StringHash(NodeCollisionEvent e) { return e._event; }
    }
    public static NodeCollisionEvent NodeCollision = new NodeCollisionEvent();

    public class NodeCollisionEndEvent {
        private StringHash _event = new StringHash("NodeCollisionEnd");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Trigger = new StringHash("Trigger");
        public NodeCollisionEndEvent() { }
        public static implicit operator StringHash(NodeCollisionEndEvent e) { return e._event; }
    }
    public static NodeCollisionEndEvent NodeCollisionEnd = new NodeCollisionEndEvent();

    public class ReloadStartedEvent {
        private StringHash _event = new StringHash("ReloadStarted");

        public ReloadStartedEvent() { }
        public static implicit operator StringHash(ReloadStartedEvent e) { return e._event; }
    }
    public static ReloadStartedEvent ReloadStarted = new ReloadStartedEvent();

    public class ReloadFinishedEvent {
        private StringHash _event = new StringHash("ReloadFinished");

        public ReloadFinishedEvent() { }
        public static implicit operator StringHash(ReloadFinishedEvent e) { return e._event; }
    }
    public static ReloadFinishedEvent ReloadFinished = new ReloadFinishedEvent();

    public class ReloadFailedEvent {
        private StringHash _event = new StringHash("ReloadFailed");

        public ReloadFailedEvent() { }
        public static implicit operator StringHash(ReloadFailedEvent e) { return e._event; }
    }
    public static ReloadFailedEvent ReloadFailed = new ReloadFailedEvent();

    public class FileChangedEvent {
        private StringHash _event = new StringHash("FileChanged");

        public StringHash FileName = new StringHash("FileName");
        public StringHash ResourceName = new StringHash("ResourceName");
        public FileChangedEvent() { }
        public static implicit operator StringHash(FileChangedEvent e) { return e._event; }
    }
    public static FileChangedEvent FileChanged = new FileChangedEvent();

    public class LoadFailedEvent {
        private StringHash _event = new StringHash("LoadFailed");

        public StringHash ResourceName = new StringHash("ResourceName");
        public LoadFailedEvent() { }
        public static implicit operator StringHash(LoadFailedEvent e) { return e._event; }
    }
    public static LoadFailedEvent LoadFailed = new LoadFailedEvent();

    public class ResourceNotFoundEvent {
        private StringHash _event = new StringHash("ResourceNotFound");

        public StringHash ResourceName = new StringHash("ResourceName");
        public ResourceNotFoundEvent() { }
        public static implicit operator StringHash(ResourceNotFoundEvent e) { return e._event; }
    }
    public static ResourceNotFoundEvent ResourceNotFound = new ResourceNotFoundEvent();

    public class UnknownResourceTypeEvent {
        private StringHash _event = new StringHash("UnknownResourceType");

        public StringHash ResourceType = new StringHash("ResourceType");
        public UnknownResourceTypeEvent() { }
        public static implicit operator StringHash(UnknownResourceTypeEvent e) { return e._event; }
    }
    public static UnknownResourceTypeEvent UnknownResourceType = new UnknownResourceTypeEvent();

    public class ResourceBackgroundLoadedEvent {
        private StringHash _event = new StringHash("ResourceBackgroundLoaded");

        public StringHash ResourceName = new StringHash("ResourceName");
        public StringHash Success = new StringHash("Success");
        public StringHash Resource = new StringHash("Resource");
        public ResourceBackgroundLoadedEvent() { }
        public static implicit operator StringHash(ResourceBackgroundLoadedEvent e) { return e._event; }
    }
    public static ResourceBackgroundLoadedEvent ResourceBackgroundLoaded = new ResourceBackgroundLoadedEvent();

    public class ChangeLanguageEvent {
        private StringHash _event = new StringHash("ChangeLanguage");

        public ChangeLanguageEvent() { }
        public static implicit operator StringHash(ChangeLanguageEvent e) { return e._event; }
    }
    public static ChangeLanguageEvent ChangeLanguage = new ChangeLanguageEvent();

    public class ResourceRenamedEvent {
        private StringHash _event = new StringHash("ResourceRenamed");

        public StringHash From = new StringHash("From");
        public StringHash To = new StringHash("To");
        public ResourceRenamedEvent() { }
        public static implicit operator StringHash(ResourceRenamedEvent e) { return e._event; }
    }
    public static ResourceRenamedEvent ResourceRenamed = new ResourceRenamedEvent();

    public class CameraViewportResizedEvent {
        private StringHash _event = new StringHash("CameraViewportResized");

        public StringHash Camera = new StringHash("Camera");
        public StringHash Viewport = new StringHash("Viewport");
        public StringHash SizeNorm = new StringHash("SizeNorm");
        public StringHash Size = new StringHash("Size");
        public CameraViewportResizedEvent() { }
        public static implicit operator StringHash(CameraViewportResizedEvent e) { return e._event; }
    }
    public static CameraViewportResizedEvent CameraViewportResized = new CameraViewportResizedEvent();

    public class SceneUpdateEvent {
        private StringHash _event = new StringHash("SceneUpdate");

        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public SceneUpdateEvent() { }
        public static implicit operator StringHash(SceneUpdateEvent e) { return e._event; }
    }
    public static SceneUpdateEvent SceneUpdate = new SceneUpdateEvent();

    public class SceneSubsystemUpdateEvent {
        private StringHash _event = new StringHash("SceneSubsystemUpdate");

        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public SceneSubsystemUpdateEvent() { }
        public static implicit operator StringHash(SceneSubsystemUpdateEvent e) { return e._event; }
    }
    public static SceneSubsystemUpdateEvent SceneSubsystemUpdate = new SceneSubsystemUpdateEvent();

    public class UpdateSmoothingEvent {
        private StringHash _event = new StringHash("UpdateSmoothing");

        public StringHash Constant = new StringHash("Constant");
        public StringHash SquaredSnapThreshold = new StringHash("SquaredSnapThreshold");
        public UpdateSmoothingEvent() { }
        public static implicit operator StringHash(UpdateSmoothingEvent e) { return e._event; }
    }
    public static UpdateSmoothingEvent UpdateSmoothing = new UpdateSmoothingEvent();

    public class SceneDrawableUpdateFinishedEvent {
        private StringHash _event = new StringHash("SceneDrawableUpdateFinished");

        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public SceneDrawableUpdateFinishedEvent() { }
        public static implicit operator StringHash(SceneDrawableUpdateFinishedEvent e) { return e._event; }
    }
    public static SceneDrawableUpdateFinishedEvent SceneDrawableUpdateFinished = new SceneDrawableUpdateFinishedEvent();

    public class AttributeAnimationUpdateEvent {
        private StringHash _event = new StringHash("AttributeAnimationUpdate");

        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public AttributeAnimationUpdateEvent() { }
        public static implicit operator StringHash(AttributeAnimationUpdateEvent e) { return e._event; }
    }
    public static AttributeAnimationUpdateEvent AttributeAnimationUpdate = new AttributeAnimationUpdateEvent();

    public class AttributeAnimationAddedEvent {
        private StringHash _event = new StringHash("AttributeAnimationAdded");

        public StringHash ObjectAnimation = new StringHash("ObjectAnimation");
        public StringHash AttributeAnimationName = new StringHash("AttributeAnimationName");
        public AttributeAnimationAddedEvent() { }
        public static implicit operator StringHash(AttributeAnimationAddedEvent e) { return e._event; }
    }
    public static AttributeAnimationAddedEvent AttributeAnimationAdded = new AttributeAnimationAddedEvent();

    public class AttributeAnimationRemovedEvent {
        private StringHash _event = new StringHash("AttributeAnimationRemoved");

        public StringHash ObjectAnimation = new StringHash("ObjectAnimation");
        public StringHash AttributeAnimationName = new StringHash("AttributeAnimationName");
        public AttributeAnimationRemovedEvent() { }
        public static implicit operator StringHash(AttributeAnimationRemovedEvent e) { return e._event; }
    }
    public static AttributeAnimationRemovedEvent AttributeAnimationRemoved = new AttributeAnimationRemovedEvent();

    public class ScenePostUpdateEvent {
        private StringHash _event = new StringHash("ScenePostUpdate");

        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public ScenePostUpdateEvent() { }
        public static implicit operator StringHash(ScenePostUpdateEvent e) { return e._event; }
    }
    public static ScenePostUpdateEvent ScenePostUpdate = new ScenePostUpdateEvent();

    public class AsyncLoadProgressEvent {
        private StringHash _event = new StringHash("AsyncLoadProgress");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Progress = new StringHash("Progress");
        public StringHash LoadedNodes = new StringHash("LoadedNodes");
        public StringHash TotalNodes = new StringHash("TotalNodes");
        public StringHash LoadedResources = new StringHash("LoadedResources");
        public StringHash TotalResources = new StringHash("TotalResources");
        public AsyncLoadProgressEvent() { }
        public static implicit operator StringHash(AsyncLoadProgressEvent e) { return e._event; }
    }
    public static AsyncLoadProgressEvent AsyncLoadProgress = new AsyncLoadProgressEvent();

    public class AsyncLoadFinishedEvent {
        private StringHash _event = new StringHash("AsyncLoadFinished");

        public StringHash Scene = new StringHash("Scene");
        public AsyncLoadFinishedEvent() { }
        public static implicit operator StringHash(AsyncLoadFinishedEvent e) { return e._event; }
    }
    public static AsyncLoadFinishedEvent AsyncLoadFinished = new AsyncLoadFinishedEvent();

    public class NodeAddedEvent {
        private StringHash _event = new StringHash("NodeAdded");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Node = new StringHash("Node");
        public NodeAddedEvent() { }
        public static implicit operator StringHash(NodeAddedEvent e) { return e._event; }
    }
    public static NodeAddedEvent NodeAdded = new NodeAddedEvent();

    public class NodeRemovedEvent {
        private StringHash _event = new StringHash("NodeRemoved");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Node = new StringHash("Node");
        public NodeRemovedEvent() { }
        public static implicit operator StringHash(NodeRemovedEvent e) { return e._event; }
    }
    public static NodeRemovedEvent NodeRemoved = new NodeRemovedEvent();

    public class ComponentAddedEvent {
        private StringHash _event = new StringHash("ComponentAdded");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash Component = new StringHash("Component");
        public ComponentAddedEvent() { }
        public static implicit operator StringHash(ComponentAddedEvent e) { return e._event; }
    }
    public static ComponentAddedEvent ComponentAdded = new ComponentAddedEvent();

    public class ComponentRemovedEvent {
        private StringHash _event = new StringHash("ComponentRemoved");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash Component = new StringHash("Component");
        public ComponentRemovedEvent() { }
        public static implicit operator StringHash(ComponentRemovedEvent e) { return e._event; }
    }
    public static ComponentRemovedEvent ComponentRemoved = new ComponentRemovedEvent();

    public class NodeNameChangedEvent {
        private StringHash _event = new StringHash("NodeNameChanged");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public NodeNameChangedEvent() { }
        public static implicit operator StringHash(NodeNameChangedEvent e) { return e._event; }
    }
    public static NodeNameChangedEvent NodeNameChanged = new NodeNameChangedEvent();

    public class NodeEnabledChangedEvent {
        private StringHash _event = new StringHash("NodeEnabledChanged");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public NodeEnabledChangedEvent() { }
        public static implicit operator StringHash(NodeEnabledChangedEvent e) { return e._event; }
    }
    public static NodeEnabledChangedEvent NodeEnabledChanged = new NodeEnabledChangedEvent();

    public class NodeTagAddedEvent {
        private StringHash _event = new StringHash("NodeTagAdded");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash Tag = new StringHash("Tag");
        public NodeTagAddedEvent() { }
        public static implicit operator StringHash(NodeTagAddedEvent e) { return e._event; }
    }
    public static NodeTagAddedEvent NodeTagAdded = new NodeTagAddedEvent();

    public class NodeTagRemovedEvent {
        private StringHash _event = new StringHash("NodeTagRemoved");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash Tag = new StringHash("Tag");
        public NodeTagRemovedEvent() { }
        public static implicit operator StringHash(NodeTagRemovedEvent e) { return e._event; }
    }
    public static NodeTagRemovedEvent NodeTagRemoved = new NodeTagRemovedEvent();

    public class ComponentEnabledChangedEvent {
        private StringHash _event = new StringHash("ComponentEnabledChanged");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash Component = new StringHash("Component");
        public ComponentEnabledChangedEvent() { }
        public static implicit operator StringHash(ComponentEnabledChangedEvent e) { return e._event; }
    }
    public static ComponentEnabledChangedEvent ComponentEnabledChanged = new ComponentEnabledChangedEvent();

    public class TemporaryChangedEvent {
        private StringHash _event = new StringHash("TemporaryChanged");

        public StringHash Serializable = new StringHash("Serializable");
        public TemporaryChangedEvent() { }
        public static implicit operator StringHash(TemporaryChangedEvent e) { return e._event; }
    }
    public static TemporaryChangedEvent TemporaryChanged = new TemporaryChangedEvent();

    public class NodeClonedEvent {
        private StringHash _event = new StringHash("NodeCloned");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Node = new StringHash("Node");
        public StringHash CloneNode = new StringHash("CloneNode");
        public NodeClonedEvent() { }
        public static implicit operator StringHash(NodeClonedEvent e) { return e._event; }
    }
    public static NodeClonedEvent NodeCloned = new NodeClonedEvent();

    public class ComponentClonedEvent {
        private StringHash _event = new StringHash("ComponentCloned");

        public StringHash Scene = new StringHash("Scene");
        public StringHash Component = new StringHash("Component");
        public StringHash CloneComponent = new StringHash("CloneComponent");
        public ComponentClonedEvent() { }
        public static implicit operator StringHash(ComponentClonedEvent e) { return e._event; }
    }
    public static ComponentClonedEvent ComponentCloned = new ComponentClonedEvent();

    public class InterceptNetworkUpdateEvent {
        private StringHash _event = new StringHash("InterceptNetworkUpdate");

        public StringHash Serializable = new StringHash("Serializable");
        public StringHash TimeStamp = new StringHash("TimeStamp");
        public StringHash Index = new StringHash("Index");
        public StringHash Name = new StringHash("Name");
        public StringHash Value = new StringHash("Value");
        public InterceptNetworkUpdateEvent() { }
        public static implicit operator StringHash(InterceptNetworkUpdateEvent e) { return e._event; }
    }
    public static InterceptNetworkUpdateEvent InterceptNetworkUpdate = new InterceptNetworkUpdateEvent();

    public class SceneActivatedEvent {
        private StringHash _event = new StringHash("SceneActivated");

        public StringHash OldScene = new StringHash("OldScene");
        public StringHash NewScene = new StringHash("NewScene");
        public SceneActivatedEvent() { }
        public static implicit operator StringHash(SceneActivatedEvent e) { return e._event; }
    }
    public static SceneActivatedEvent SceneActivated = new SceneActivatedEvent();

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

    public class UIMouseClickEvent {
        private StringHash _event = new StringHash("UIMouseClick");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseClickEvent() { }
        public static implicit operator StringHash(UIMouseClickEvent e) { return e._event; }
    }
    public static UIMouseClickEvent UIMouseClick = new UIMouseClickEvent();

    public class UIMouseClickEndEvent {
        private StringHash _event = new StringHash("UIMouseClickEnd");

        public StringHash Element = new StringHash("Element");
        public StringHash BeginElement = new StringHash("BeginElement");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseClickEndEvent() { }
        public static implicit operator StringHash(UIMouseClickEndEvent e) { return e._event; }
    }
    public static UIMouseClickEndEvent UIMouseClickEnd = new UIMouseClickEndEvent();

    public class UIMouseDoubleClickEvent {
        private StringHash _event = new StringHash("UIMouseDoubleClick");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash XBegin = new StringHash("XBegin");
        public StringHash YBegin = new StringHash("YBegin");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UIMouseDoubleClickEvent() { }
        public static implicit operator StringHash(UIMouseDoubleClickEvent e) { return e._event; }
    }
    public static UIMouseDoubleClickEvent UIMouseDoubleClick = new UIMouseDoubleClickEvent();

    public class ClickEvent {
        private StringHash _event = new StringHash("Click");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ClickEvent() { }
        public static implicit operator StringHash(ClickEvent e) { return e._event; }
    }
    public static ClickEvent Click = new ClickEvent();

    public class ClickEndEvent {
        private StringHash _event = new StringHash("ClickEnd");

        public StringHash Element = new StringHash("Element");
        public StringHash BeginElement = new StringHash("BeginElement");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ClickEndEvent() { }
        public static implicit operator StringHash(ClickEndEvent e) { return e._event; }
    }
    public static ClickEndEvent ClickEnd = new ClickEndEvent();

    public class DoubleClickEvent {
        private StringHash _event = new StringHash("DoubleClick");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash XBegin = new StringHash("XBegin");
        public StringHash YBegin = new StringHash("YBegin");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public DoubleClickEvent() { }
        public static implicit operator StringHash(DoubleClickEvent e) { return e._event; }
    }
    public static DoubleClickEvent DoubleClick = new DoubleClickEvent();

    public class DragDropTestEvent {
        private StringHash _event = new StringHash("DragDropTest");

        public StringHash Source = new StringHash("Source");
        public StringHash Target = new StringHash("Target");
        public StringHash Accept = new StringHash("Accept");
        public DragDropTestEvent() { }
        public static implicit operator StringHash(DragDropTestEvent e) { return e._event; }
    }
    public static DragDropTestEvent DragDropTest = new DragDropTestEvent();

    public class DragDropFinishEvent {
        private StringHash _event = new StringHash("DragDropFinish");

        public StringHash Source = new StringHash("Source");
        public StringHash Target = new StringHash("Target");
        public StringHash Accept = new StringHash("Accept");
        public DragDropFinishEvent() { }
        public static implicit operator StringHash(DragDropFinishEvent e) { return e._event; }
    }
    public static DragDropFinishEvent DragDropFinish = new DragDropFinishEvent();

    public class FocusChangedEvent {
        private StringHash _event = new StringHash("FocusChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash ClickedElement = new StringHash("ClickedElement");
        public FocusChangedEvent() { }
        public static implicit operator StringHash(FocusChangedEvent e) { return e._event; }
    }
    public static FocusChangedEvent FocusChanged = new FocusChangedEvent();

    public class NameChangedEvent {
        private StringHash _event = new StringHash("NameChanged");

        public StringHash Element = new StringHash("Element");
        public NameChangedEvent() { }
        public static implicit operator StringHash(NameChangedEvent e) { return e._event; }
    }
    public static NameChangedEvent NameChanged = new NameChangedEvent();

    public class ResizedEvent {
        private StringHash _event = new StringHash("Resized");

        public StringHash Element = new StringHash("Element");
        public StringHash Width = new StringHash("Width");
        public StringHash Height = new StringHash("Height");
        public StringHash Dx = new StringHash("DX");
        public StringHash Dy = new StringHash("DY");
        public ResizedEvent() { }
        public static implicit operator StringHash(ResizedEvent e) { return e._event; }
    }
    public static ResizedEvent Resized = new ResizedEvent();

    public class PositionedEvent {
        private StringHash _event = new StringHash("Positioned");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public PositionedEvent() { }
        public static implicit operator StringHash(PositionedEvent e) { return e._event; }
    }
    public static PositionedEvent Positioned = new PositionedEvent();

    public class VisibleChangedEvent {
        private StringHash _event = new StringHash("VisibleChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Visible = new StringHash("Visible");
        public VisibleChangedEvent() { }
        public static implicit operator StringHash(VisibleChangedEvent e) { return e._event; }
    }
    public static VisibleChangedEvent VisibleChanged = new VisibleChangedEvent();

    public class FocusedEvent {
        private StringHash _event = new StringHash("Focused");

        public StringHash Element = new StringHash("Element");
        public StringHash ByKey = new StringHash("ByKey");
        public FocusedEvent() { }
        public static implicit operator StringHash(FocusedEvent e) { return e._event; }
    }
    public static FocusedEvent Focused = new FocusedEvent();

    public class DefocusedEvent {
        private StringHash _event = new StringHash("Defocused");

        public StringHash Element = new StringHash("Element");
        public DefocusedEvent() { }
        public static implicit operator StringHash(DefocusedEvent e) { return e._event; }
    }
    public static DefocusedEvent Defocused = new DefocusedEvent();

    public class LayoutUpdatedEvent {
        private StringHash _event = new StringHash("LayoutUpdated");

        public StringHash Element = new StringHash("Element");
        public LayoutUpdatedEvent() { }
        public static implicit operator StringHash(LayoutUpdatedEvent e) { return e._event; }
    }
    public static LayoutUpdatedEvent LayoutUpdated = new LayoutUpdatedEvent();

    public class PressedEvent {
        private StringHash _event = new StringHash("Pressed");

        public StringHash Element = new StringHash("Element");
        public PressedEvent() { }
        public static implicit operator StringHash(PressedEvent e) { return e._event; }
    }
    public static PressedEvent Pressed = new PressedEvent();

    public class ReleasedEvent {
        private StringHash _event = new StringHash("Released");

        public StringHash Element = new StringHash("Element");
        public ReleasedEvent() { }
        public static implicit operator StringHash(ReleasedEvent e) { return e._event; }
    }
    public static ReleasedEvent Released = new ReleasedEvent();

    public class ToggledEvent {
        private StringHash _event = new StringHash("Toggled");

        public StringHash Element = new StringHash("Element");
        public StringHash State = new StringHash("State");
        public ToggledEvent() { }
        public static implicit operator StringHash(ToggledEvent e) { return e._event; }
    }
    public static ToggledEvent Toggled = new ToggledEvent();

    public class SliderChangedEvent {
        private StringHash _event = new StringHash("SliderChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public SliderChangedEvent() { }
        public static implicit operator StringHash(SliderChangedEvent e) { return e._event; }
    }
    public static SliderChangedEvent SliderChanged = new SliderChangedEvent();

    public class SliderPagedEvent {
        private StringHash _event = new StringHash("SliderPaged");

        public StringHash Element = new StringHash("Element");
        public StringHash Offset = new StringHash("Offset");
        public StringHash Pressed = new StringHash("Pressed");
        public SliderPagedEvent() { }
        public static implicit operator StringHash(SliderPagedEvent e) { return e._event; }
    }
    public static SliderPagedEvent SliderPaged = new SliderPagedEvent();

    public class ProgressBarChangedEvent {
        private StringHash _event = new StringHash("ProgressBarChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public ProgressBarChangedEvent() { }
        public static implicit operator StringHash(ProgressBarChangedEvent e) { return e._event; }
    }
    public static ProgressBarChangedEvent ProgressBarChanged = new ProgressBarChangedEvent();

    public class ScrollBarChangedEvent {
        private StringHash _event = new StringHash("ScrollBarChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Value = new StringHash("Value");
        public ScrollBarChangedEvent() { }
        public static implicit operator StringHash(ScrollBarChangedEvent e) { return e._event; }
    }
    public static ScrollBarChangedEvent ScrollBarChanged = new ScrollBarChangedEvent();

    public class ViewChangedEvent {
        private StringHash _event = new StringHash("ViewChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public ViewChangedEvent() { }
        public static implicit operator StringHash(ViewChangedEvent e) { return e._event; }
    }
    public static ViewChangedEvent ViewChanged = new ViewChangedEvent();

    public class ModalChangedEvent {
        private StringHash _event = new StringHash("ModalChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Modal = new StringHash("Modal");
        public ModalChangedEvent() { }
        public static implicit operator StringHash(ModalChangedEvent e) { return e._event; }
    }
    public static ModalChangedEvent ModalChanged = new ModalChangedEvent();

    public class TextEntryEvent {
        private StringHash _event = new StringHash("TextEntry");

        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public TextEntryEvent() { }
        public static implicit operator StringHash(TextEntryEvent e) { return e._event; }
    }
    public static TextEntryEvent TextEntry = new TextEntryEvent();

    public class TextChangedEvent {
        private StringHash _event = new StringHash("TextChanged");

        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public TextChangedEvent() { }
        public static implicit operator StringHash(TextChangedEvent e) { return e._event; }
    }
    public static TextChangedEvent TextChanged = new TextChangedEvent();

    public class TextFinishedEvent {
        private StringHash _event = new StringHash("TextFinished");

        public StringHash Element = new StringHash("Element");
        public StringHash Text = new StringHash("Text");
        public StringHash Value = new StringHash("Value");
        public TextFinishedEvent() { }
        public static implicit operator StringHash(TextFinishedEvent e) { return e._event; }
    }
    public static TextFinishedEvent TextFinished = new TextFinishedEvent();

    public class MenuSelectedEvent {
        private StringHash _event = new StringHash("MenuSelected");

        public StringHash Element = new StringHash("Element");
        public MenuSelectedEvent() { }
        public static implicit operator StringHash(MenuSelectedEvent e) { return e._event; }
    }
    public static MenuSelectedEvent MenuSelected = new MenuSelectedEvent();

    public class ItemSelectedEvent {
        private StringHash _event = new StringHash("ItemSelected");

        public StringHash Element = new StringHash("Element");
        public StringHash Selection = new StringHash("Selection");
        public ItemSelectedEvent() { }
        public static implicit operator StringHash(ItemSelectedEvent e) { return e._event; }
    }
    public static ItemSelectedEvent ItemSelected = new ItemSelectedEvent();

    public class ItemDeselectedEvent {
        private StringHash _event = new StringHash("ItemDeselected");

        public StringHash Element = new StringHash("Element");
        public StringHash Selection = new StringHash("Selection");
        public ItemDeselectedEvent() { }
        public static implicit operator StringHash(ItemDeselectedEvent e) { return e._event; }
    }
    public static ItemDeselectedEvent ItemDeselected = new ItemDeselectedEvent();

    public class SelectionChangedEvent {
        private StringHash _event = new StringHash("SelectionChanged");

        public StringHash Element = new StringHash("Element");
        public SelectionChangedEvent() { }
        public static implicit operator StringHash(SelectionChangedEvent e) { return e._event; }
    }
    public static SelectionChangedEvent SelectionChanged = new SelectionChangedEvent();

    public class ItemClickedEvent {
        private StringHash _event = new StringHash("ItemClicked");

        public StringHash Element = new StringHash("Element");
        public StringHash Item = new StringHash("Item");
        public StringHash Selection = new StringHash("Selection");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ItemClickedEvent() { }
        public static implicit operator StringHash(ItemClickedEvent e) { return e._event; }
    }
    public static ItemClickedEvent ItemClicked = new ItemClickedEvent();

    public class ItemDoubleClickedEvent {
        private StringHash _event = new StringHash("ItemDoubleClicked");

        public StringHash Element = new StringHash("Element");
        public StringHash Item = new StringHash("Item");
        public StringHash Selection = new StringHash("Selection");
        public StringHash Button = new StringHash("Button");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public ItemDoubleClickedEvent() { }
        public static implicit operator StringHash(ItemDoubleClickedEvent e) { return e._event; }
    }
    public static ItemDoubleClickedEvent ItemDoubleClicked = new ItemDoubleClickedEvent();

    public class UnhandledKeyEvent {
        private StringHash _event = new StringHash("UnhandledKey");

        public StringHash Element = new StringHash("Element");
        public StringHash Key = new StringHash("Key");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash Qualifiers = new StringHash("Qualifiers");
        public UnhandledKeyEvent() { }
        public static implicit operator StringHash(UnhandledKeyEvent e) { return e._event; }
    }
    public static UnhandledKeyEvent UnhandledKey = new UnhandledKeyEvent();

    public class FileSelectedEvent {
        private StringHash _event = new StringHash("FileSelected");

        public StringHash FileName = new StringHash("FileName");
        public StringHash Filter = new StringHash("Filter");
        public StringHash Ok = new StringHash("OK");
        public FileSelectedEvent() { }
        public static implicit operator StringHash(FileSelectedEvent e) { return e._event; }
    }
    public static FileSelectedEvent FileSelected = new FileSelectedEvent();

    public class MessageACKEvent {
        private StringHash _event = new StringHash("MessageACK");

        public StringHash Ok = new StringHash("OK");
        public MessageACKEvent() { }
        public static implicit operator StringHash(MessageACKEvent e) { return e._event; }
    }
    public static MessageACKEvent MessageACK = new MessageACKEvent();

    public class ElementAddedEvent {
        private StringHash _event = new StringHash("ElementAdded");

        public StringHash Root = new StringHash("Root");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Element = new StringHash("Element");
        public ElementAddedEvent() { }
        public static implicit operator StringHash(ElementAddedEvent e) { return e._event; }
    }
    public static ElementAddedEvent ElementAdded = new ElementAddedEvent();

    public class ElementRemovedEvent {
        private StringHash _event = new StringHash("ElementRemoved");

        public StringHash Root = new StringHash("Root");
        public StringHash Parent = new StringHash("Parent");
        public StringHash Element = new StringHash("Element");
        public ElementRemovedEvent() { }
        public static implicit operator StringHash(ElementRemovedEvent e) { return e._event; }
    }
    public static ElementRemovedEvent ElementRemoved = new ElementRemovedEvent();

    public class HoverBeginEvent {
        private StringHash _event = new StringHash("HoverBegin");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public HoverBeginEvent() { }
        public static implicit operator StringHash(HoverBeginEvent e) { return e._event; }
    }
    public static HoverBeginEvent HoverBegin = new HoverBeginEvent();

    public class HoverEndEvent {
        private StringHash _event = new StringHash("HoverEnd");

        public StringHash Element = new StringHash("Element");
        public HoverEndEvent() { }
        public static implicit operator StringHash(HoverEndEvent e) { return e._event; }
    }
    public static HoverEndEvent HoverEnd = new HoverEndEvent();

    public class DragBeginEvent {
        private StringHash _event = new StringHash("DragBegin");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragBeginEvent() { }
        public static implicit operator StringHash(DragBeginEvent e) { return e._event; }
    }
    public static DragBeginEvent DragBegin = new DragBeginEvent();

    public class DragMoveEvent {
        private StringHash _event = new StringHash("DragMove");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash Dx = new StringHash("DX");
        public StringHash Dy = new StringHash("DY");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragMoveEvent() { }
        public static implicit operator StringHash(DragMoveEvent e) { return e._event; }
    }
    public static DragMoveEvent DragMove = new DragMoveEvent();

    public class DragEndEvent {
        private StringHash _event = new StringHash("DragEnd");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragEndEvent() { }
        public static implicit operator StringHash(DragEndEvent e) { return e._event; }
    }
    public static DragEndEvent DragEnd = new DragEndEvent();

    public class DragCancelEvent {
        private StringHash _event = new StringHash("DragCancel");

        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public StringHash Buttons = new StringHash("Buttons");
        public StringHash NumButtons = new StringHash("NumButtons");
        public DragCancelEvent() { }
        public static implicit operator StringHash(DragCancelEvent e) { return e._event; }
    }
    public static DragCancelEvent DragCancel = new DragCancelEvent();

    public class UIDropFileEvent {
        private StringHash _event = new StringHash("UIDropFile");

        public StringHash FileName = new StringHash("FileName");
        public StringHash Element = new StringHash("Element");
        public StringHash X = new StringHash("X");
        public StringHash Y = new StringHash("Y");
        public StringHash ElementX = new StringHash("ElementX");
        public StringHash ElementY = new StringHash("ElementY");
        public UIDropFileEvent() { }
        public static implicit operator StringHash(UIDropFileEvent e) { return e._event; }
    }
    public static UIDropFileEvent UIDropFile = new UIDropFileEvent();

    public class PhysicsUpdateContact2DEvent {
        private StringHash _event = new StringHash("PhysicsUpdateContact2D");

        public StringHash World = new StringHash("World");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash ShapeA = new StringHash("ShapeA");
        public StringHash ShapeB = new StringHash("ShapeB");
        public StringHash Enabled = new StringHash("Enabled");
        public PhysicsUpdateContact2DEvent() { }
        public static implicit operator StringHash(PhysicsUpdateContact2DEvent e) { return e._event; }
    }
    public static PhysicsUpdateContact2DEvent PhysicsUpdateContact2D = new PhysicsUpdateContact2DEvent();

    public class PhysicsBeginContact2DEvent {
        private StringHash _event = new StringHash("PhysicsBeginContact2D");

        public StringHash World = new StringHash("World");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash ShapeA = new StringHash("ShapeA");
        public StringHash ShapeB = new StringHash("ShapeB");
        public PhysicsBeginContact2DEvent() { }
        public static implicit operator StringHash(PhysicsBeginContact2DEvent e) { return e._event; }
    }
    public static PhysicsBeginContact2DEvent PhysicsBeginContact2D = new PhysicsBeginContact2DEvent();

    public class PhysicsEndContact2DEvent {
        private StringHash _event = new StringHash("PhysicsEndContact2D");

        public StringHash World = new StringHash("World");
        public StringHash BodyA = new StringHash("BodyA");
        public StringHash BodyB = new StringHash("BodyB");
        public StringHash NodeA = new StringHash("NodeA");
        public StringHash NodeB = new StringHash("NodeB");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash ShapeA = new StringHash("ShapeA");
        public StringHash ShapeB = new StringHash("ShapeB");
        public PhysicsEndContact2DEvent() { }
        public static implicit operator StringHash(PhysicsEndContact2DEvent e) { return e._event; }
    }
    public static PhysicsEndContact2DEvent PhysicsEndContact2D = new PhysicsEndContact2DEvent();

    public class NodeUpdateContact2DEvent {
        private StringHash _event = new StringHash("NodeUpdateContact2D");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash Shape = new StringHash("Shape");
        public StringHash OtherShape = new StringHash("OtherShape");
        public StringHash Enabled = new StringHash("Enabled");
        public NodeUpdateContact2DEvent() { }
        public static implicit operator StringHash(NodeUpdateContact2DEvent e) { return e._event; }
    }
    public static NodeUpdateContact2DEvent NodeUpdateContact2D = new NodeUpdateContact2DEvent();

    public class NodeBeginContact2DEvent {
        private StringHash _event = new StringHash("NodeBeginContact2D");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash Shape = new StringHash("Shape");
        public StringHash OtherShape = new StringHash("OtherShape");
        public NodeBeginContact2DEvent() { }
        public static implicit operator StringHash(NodeBeginContact2DEvent e) { return e._event; }
    }
    public static NodeBeginContact2DEvent NodeBeginContact2D = new NodeBeginContact2DEvent();

    public class NodeEndContact2DEvent {
        private StringHash _event = new StringHash("NodeEndContact2D");

        public StringHash Body = new StringHash("Body");
        public StringHash OtherNode = new StringHash("OtherNode");
        public StringHash OtherBody = new StringHash("OtherBody");
        public StringHash Contacts = new StringHash("Contacts");
        public StringHash Shape = new StringHash("Shape");
        public StringHash OtherShape = new StringHash("OtherShape");
        public NodeEndContact2DEvent() { }
        public static implicit operator StringHash(NodeEndContact2DEvent e) { return e._event; }
    }
    public static NodeEndContact2DEvent NodeEndContact2D = new NodeEndContact2DEvent();

    public class ParticlesEndEvent {
        private StringHash _event = new StringHash("ParticlesEnd");

        public StringHash Node = new StringHash("Node");
        public StringHash Effect = new StringHash("Effect");
        public ParticlesEndEvent() { }
        public static implicit operator StringHash(ParticlesEndEvent e) { return e._event; }
    }
    public static ParticlesEndEvent ParticlesEnd = new ParticlesEndEvent();

    public class ParticlesDurationEvent {
        private StringHash _event = new StringHash("ParticlesDuration");

        public StringHash Node = new StringHash("Node");
        public StringHash Effect = new StringHash("Effect");
        public ParticlesDurationEvent() { }
        public static implicit operator StringHash(ParticlesDurationEvent e) { return e._event; }
    }
    public static ParticlesDurationEvent ParticlesDuration = new ParticlesDurationEvent();

}
%}
