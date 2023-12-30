%ignore Urho3D::PROFILER_COLOR_EVENTS;
%csconst(1) Urho3D::PROFILER_COLOR_EVENTS;
%constant unsigned int ProfilerColorEvents = 11693337;
%ignore Urho3D::PROFILER_COLOR_RESOURCES;
%csconst(1) Urho3D::PROFILER_COLOR_RESOURCES;
%constant unsigned int ProfilerColorResources = 27522;
%constant unsigned int VariantValueSize = Urho3D::VARIANT_VALUE_SIZE;
%ignore Urho3D::VARIANT_VALUE_SIZE;
%constant const char* CategoryAudio = "Component/Audio";
%ignore Urho3D::Category_Audio;
%constant const char* CategoryGeometry = "Component/Geometry";
%ignore Urho3D::Category_Geometry;
%constant const char* CategoryIk = "Component/Inverse Kinematics";
%ignore Urho3D::Category_IK;
%constant const char* CategoryLogic = "Component/Logic";
%ignore Urho3D::Category_Logic;
%constant const char* CategoryNavigation = "Component/Navigation";
%ignore Urho3D::Category_Navigation;
%constant const char* CategoryNetwork = "Component/Network";
%ignore Urho3D::Category_Network;
%constant const char* CategoryPhysics = "Component/Physics";
%ignore Urho3D::Category_Physics;
%constant const char* CategoryPhysics2d = "Component/Physics2D";
%ignore Urho3D::Category_Physics2D;
%constant const char* CategoryRmlui = "Component/RmlUI";
%ignore Urho3D::Category_RmlUI;
%constant const char* CategoryScene = "Component/Scene";
%ignore Urho3D::Category_Scene;
%constant const char* CategorySubsystem = "Component/Subsystem";
%ignore Urho3D::Category_Subsystem;
%constant const char* CategoryUrho2d = "Component/Urho2D";
%ignore Urho3D::Category_Urho2D;
%constant const char* CategoryUser = "Component/User";
%ignore Urho3D::Category_User;
%constant const char* CategoryTransformer = "Transformer";
%ignore Urho3D::Category_Transformer;
%constant const char* CategoryUi = "UI";
%ignore Urho3D::Category_UI;
%constant char * DynLibSuffix = Urho3D::DYN_LIB_SUFFIX;
%ignore Urho3D::DYN_LIB_SUFFIX;
%constant char * DefaultDateTimeFormat = Urho3D::DEFAULT_DATE_TIME_FORMAT;
%ignore Urho3D::DEFAULT_DATE_TIME_FORMAT;
%constant unsigned int TaskBufferSize = Urho3D::TaskBufferSize;
%ignore Urho3D::TaskBufferSize;
%csconstvalue("0") Urho3D::VAR_NONE;
%typemap(csattributes) AttributeMode "[global::System.Flags]";
using AttributeModeFlags = AttributeMode;
%typemap(ctype) AttributeModeFlags "size_t";
%typemap(out) AttributeModeFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("9") Urho3D::PlatformId::Unknown;
%csattribute(Urho3D::RuntimeException, %arg(ea::string), Message, GetMessage);
%csattribute(Urho3D::CustomVariantValue, %arg(std::type_info), TypeInfo, GetTypeInfo);
%csattribute(Urho3D::Variant, %arg(int), Int, GetInt);
%csattribute(Urho3D::Variant, %arg(long long), Int64, GetInt64);
%csattribute(Urho3D::Variant, %arg(unsigned long long), UInt64, GetUInt64);
%csattribute(Urho3D::Variant, %arg(unsigned int), UInt, GetUInt);
%csattribute(Urho3D::Variant, %arg(StringHash), StringHash, GetStringHash);
%csattribute(Urho3D::Variant, %arg(bool), Bool, GetBool);
%csattribute(Urho3D::Variant, %arg(float), Float, GetFloat);
%csattribute(Urho3D::Variant, %arg(double), Double, GetDouble);
%csattribute(Urho3D::Variant, %arg(Vector2), Vector2, GetVector2);
%csattribute(Urho3D::Variant, %arg(Vector3), Vector3, GetVector3);
%csattribute(Urho3D::Variant, %arg(Vector4), Vector4, GetVector4);
%csattribute(Urho3D::Variant, %arg(Quaternion), Quaternion, GetQuaternion);
%csattribute(Urho3D::Variant, %arg(Color), Color, GetColor);
%csattribute(Urho3D::Variant, %arg(ea::string), String, GetString);
%csattribute(Urho3D::Variant, %arg(VariantBuffer), Buffer, GetBuffer);
%csattribute(Urho3D::Variant, %arg(VectorBuffer), VectorBuffer, GetVectorBuffer);
%csattribute(Urho3D::Variant, %arg(void *), VoidPtr, GetVoidPtr);
%csattribute(Urho3D::Variant, %arg(ResourceRef), ResourceRef, GetResourceRef);
%csattribute(Urho3D::Variant, %arg(ResourceRefList), ResourceRefList, GetResourceRefList);
%csattribute(Urho3D::Variant, %arg(VariantVector), VariantVector, GetVariantVector);
%csattribute(Urho3D::Variant, %arg(StringVector), StringVector, GetStringVector);
%csattribute(Urho3D::Variant, %arg(VariantMap), VariantMap, GetVariantMap);
%csattribute(Urho3D::Variant, %arg(Rect), Rect, GetRect);
%csattribute(Urho3D::Variant, %arg(IntRect), IntRect, GetIntRect);
%csattribute(Urho3D::Variant, %arg(IntVector2), IntVector2, GetIntVector2);
%csattribute(Urho3D::Variant, %arg(IntVector3), IntVector3, GetIntVector3);
%csattribute(Urho3D::Variant, %arg(RefCounted *), Ptr, GetPtr);
%csattribute(Urho3D::Variant, %arg(Matrix3), Matrix3, GetMatrix3);
%csattribute(Urho3D::Variant, %arg(Matrix3x4), Matrix3x4, GetMatrix3x4);
%csattribute(Urho3D::Variant, %arg(Matrix4), Matrix4, GetMatrix4);
%csattribute(Urho3D::Variant, %arg(VariantCurve), VariantCurve, GetVariantCurve);
//%csattribute(Urho3D::Variant, %arg(StringVariantMap), StringVariantMap, GetStringVariantMap);
%csattribute(Urho3D::Variant, %arg(CustomVariantValue *), CustomVariantValue, GetCustomVariantValuePtr);
%csattribute(Urho3D::Variant, %arg(bool), IsZero, IsZero);
%csattribute(Urho3D::Variant, %arg(VariantBuffer *), Buffer, GetBufferPtr);
%csattribute(Urho3D::Variant, %arg(VariantVector *), VariantVector, GetVariantVectorPtr);
%csattribute(Urho3D::Variant, %arg(StringVector *), StringVector, GetStringVectorPtr);
%csattribute(Urho3D::Variant, %arg(VariantMap *), VariantMap, GetVariantMapPtr);
%csattribute(Urho3D::Variant, %arg(StringVariantMap *), StringVariantMap, GetStringVariantMapPtr);
%csattribute(Urho3D::StringHashRegister, %arg(StringMap), InternalMap, GetInternalMap);
%csattribute(Urho3D::SubsystemCache, %arg(Container), Container, GetContainer);
%csattribute(Urho3D::TypeInfo, %arg(ea::string), TypeName, GetTypeName);
%csattribute(Urho3D::TypeInfo, %arg(TypeInfo *), BaseTypeInfo, GetBaseTypeInfo);
%csattribute(Urho3D::Object, %arg(VariantMap), EventDataMap, GetEventDataMap);
%csattribute(Urho3D::Object, %arg(Context *), Context, GetContext);
%csattribute(Urho3D::Object, %arg(VariantMap), GlobalVars, GetGlobalVars);
%csattribute(Urho3D::Object, %arg(Object *), EventSender, GetEventSender);
%csattribute(Urho3D::Object, %arg(EventHandler *), EventHandler, GetEventHandler);
%csattribute(Urho3D::Object, %arg(ea::string), Category, GetCategory);
%csattribute(Urho3D::Object, %arg(bool), BlockEvents, GetBlockEvents, SetBlockEvents);
%csattribute(Urho3D::EventHandler, %arg(Object *), Receiver, GetReceiver);
%csattribute(Urho3D::EventHandler, %arg(Object *), Sender, GetSender);
%csattribute(Urho3D::EventHandler, %arg(StringHash), EventType, GetEventType);
%csattribute(Urho3D::ObjectReflection, %arg(ea::string), Category, GetCategory);
%csattribute(Urho3D::ObjectReflection, %arg(TypeInfo *), TypeInfo, GetTypeInfo);
%csattribute(Urho3D::ObjectReflection, %arg(ea::string), TypeName, GetTypeName);
%csattribute(Urho3D::ObjectReflection, %arg(StringHash), TypeNameHash, GetTypeNameHash);
%csattribute(Urho3D::ObjectReflection, %arg(ea::vector<AttributeInfo>), Attributes, GetAttributes);
%csattribute(Urho3D::ObjectReflection, %arg(unsigned int), NumAttributes, GetNumAttributes);
%csattribute(Urho3D::ObjectReflection, %arg(AttributeScopeHint), ScopeHint, GetScopeHint);
%csattribute(Urho3D::ObjectReflection, %arg(AttributeScopeHint), EffectiveScopeHint, GetEffectiveScopeHint);
%csattribute(Urho3D::Context, %arg(VariantMap), EventDataMap, GetEventDataMap);
%csattribute(Urho3D::Context, %arg(VariantMap), GlobalVars, GetGlobalVars);
%csattribute(Urho3D::Context, %arg(SubsystemCache), Subsystems, GetSubsystems);
%csattribute(Urho3D::Context, %arg(Object *), EventSender, GetEventSender);
%csattribute(Urho3D::Context, %arg(EventHandler *), EventHandler, GetEventHandler);
%csattribute(Urho3D::Spline, %arg(InterpolationMode), InterpolationMode, GetInterpolationMode);
%csattribute(Urho3D::Spline, %arg(VariantVector), Knots, GetKnots);
%csattribute(Urho3D::StopToken, %arg(bool), IsStopped, IsStopped);
%csattribute(Urho3D::Time, %arg(unsigned int), FrameNumber, GetFrameNumber);
%csattribute(Urho3D::Time, %arg(float), TimeStep, GetTimeStep);
%csattribute(Urho3D::Time, %arg(unsigned int), TimerPeriod, GetTimerPeriod, SetTimerPeriod);
%csattribute(Urho3D::Time, %arg(float), ElapsedTime, GetElapsedTime);
%csattribute(Urho3D::Time, %arg(float), FramesPerSecond, GetFramesPerSecond);
%csattribute(Urho3D::TimedCounter, %arg(int), Current, GetCurrent);
%csattribute(Urho3D::TimedCounter, %arg(ea::vector<float>), Data, GetData);
%csattribute(Urho3D::TimedCounter, %arg(float), Average, GetAverage);
%csattribute(Urho3D::TimedCounter, %arg(float), Last, GetLast);
%csattribute(Urho3D::WorkQueue, %arg(unsigned int), NumIncomplete, GetNumIncomplete);
%csattribute(Urho3D::WorkQueue, %arg(bool), IsCompleted, IsCompleted);
%csattribute(Urho3D::WorkQueue, %arg(int), NonThreadedWorkMs, GetNonThreadedWorkMs, SetNonThreadedWorkMs);
%csattribute(Urho3D::WorkQueue, %arg(unsigned int), NumProcessingThreads, GetNumProcessingThreads);
%csattribute(Urho3D::WorkQueue, %arg(bool), IsMultithreaded, IsMultithreaded);
%csattribute(Urho3D::WorkQueue, %arg(SharedPtr<WorkItem>), FreeItem, GetFreeItem);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class BeginFrameEvent {
        private StringHash _event = new StringHash("BeginFrame");
        public StringHash FrameNumber = new StringHash("FrameNumber");
        public StringHash TimeStep = new StringHash("TimeStep");
        public BeginFrameEvent() { }
        public static implicit operator StringHash(BeginFrameEvent e) { return e._event; }
    }
    public static BeginFrameEvent BeginFrame = new BeginFrameEvent();
    public class InputReadyEvent {
        private StringHash _event = new StringHash("InputReady");
        public StringHash TimeStep = new StringHash("TimeStep");
        public InputReadyEvent() { }
        public static implicit operator StringHash(InputReadyEvent e) { return e._event; }
    }
    public static InputReadyEvent InputReady = new InputReadyEvent();
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
    public class EndFramePrivateEvent {
        private StringHash _event = new StringHash("EndFramePrivate");
        public EndFramePrivateEvent() { }
        public static implicit operator StringHash(EndFramePrivateEvent e) { return e._event; }
    }
    public static EndFramePrivateEvent EndFramePrivate = new EndFramePrivateEvent();
} %}
