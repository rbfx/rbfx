%constant const char* VectorStructElements = "VectorStructElements";
%ignore Urho3D::AttributeMetadata::VectorStructElements;
%constant const char* IsAction = "IsAction";
%ignore Urho3D::AttributeMetadata::IsAction;
%constant const char* AllowResize = "AllowResize";
%ignore Urho3D::AttributeMetadata::AllowResize;
%constant const char* DynamicMetadata = "DynamicMetadata";
%ignore Urho3D::AttributeMetadata::DynamicMetadata;
%constant unsigned int FirstReplicatedId = Urho3D::FIRST_REPLICATED_ID;
%ignore Urho3D::FIRST_REPLICATED_ID;
%ignore Urho3D::LAST_REPLICATED_ID;
%csconst(1) Urho3D::LAST_REPLICATED_ID;
%constant unsigned int LastReplicatedId = 16777215;
%csconstvalue("0") Urho3D::REMOVE_DISABLED;
%csconstvalue("0") Urho3D::PrefabArchiveFlag::None;
%typemap(csattributes) Urho3D::PrefabArchiveFlag "[global::System.Flags]";
using PrefabArchiveFlags = Urho3D::PrefabArchiveFlag;
%typemap(ctype) PrefabArchiveFlags "size_t";
%typemap(out) PrefabArchiveFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::PrefabSaveFlag::None;
%typemap(csattributes) Urho3D::PrefabSaveFlag "[global::System.Flags]";
using PrefabSaveFlags = Urho3D::PrefabSaveFlag;
%typemap(ctype) PrefabSaveFlags "size_t";
%typemap(out) PrefabSaveFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::PrefabLoadFlag::None;
%typemap(csattributes) Urho3D::PrefabLoadFlag "[global::System.Flags]";
using PrefabLoadFlags = Urho3D::PrefabLoadFlag;
%typemap(ctype) PrefabLoadFlags "size_t";
%typemap(out) PrefabLoadFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::None;
%typemap(csattributes) Urho3D::SceneLookupFlag "[global::System.Flags]";
using SceneLookupFlags = Urho3D::SceneLookupFlag;
%typemap(ctype) SceneLookupFlags "size_t";
%typemap(out) SceneLookupFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::ComponentSearchFlag::None;
%csconstvalue("1") Urho3D::ComponentSearchFlag::Self;
%csconstvalue("2") Urho3D::ComponentSearchFlag::Parent;
%csconstvalue("6") Urho3D::ComponentSearchFlag::ParentRecursive;
%csconstvalue("8") Urho3D::ComponentSearchFlag::Children;
%csconstvalue("24") Urho3D::ComponentSearchFlag::ChildrenRecursive;
%csconstvalue("256") Urho3D::ComponentSearchFlag::Derived;
%csconstvalue("512") Urho3D::ComponentSearchFlag::Disabled;
%csconstvalue("7") Urho3D::ComponentSearchFlag::SelfOrParentRecursive;
%csconstvalue("25") Urho3D::ComponentSearchFlag::SelfOrChildrenRecursive;
%typemap(csattributes) Urho3D::ComponentSearchFlag "[global::System.Flags]";
using ComponentSearchFlags = Urho3D::ComponentSearchFlag;
%typemap(ctype) ComponentSearchFlags "size_t";
%typemap(out) ComponentSearchFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::TS_LOCAL;
%typemap(csattributes) Urho3D::UpdateEvent "[global::System.Flags]";
using UpdateEventFlags = Urho3D::UpdateEvent;
%typemap(ctype) UpdateEventFlags "size_t";
%typemap(out) UpdateEventFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::PrefabInlineFlag::None;
%typemap(csattributes) Urho3D::PrefabInlineFlag "[global::System.Flags]";
using PrefabInlineFlags = Urho3D::PrefabInlineFlag;
%typemap(ctype) PrefabInlineFlags "size_t";
%typemap(out) PrefabInlineFlags "$result = (size_t)$1.AsInteger();"
%csconstvalue("0") Urho3D::PrefabInstanceFlag::None;
%csconstvalue("1") Urho3D::PrefabInstanceFlag::UpdateName;
%csconstvalue("2") Urho3D::PrefabInstanceFlag::UpdateTags;
%csconstvalue("4") Urho3D::PrefabInstanceFlag::UpdatePosition;
%csconstvalue("8") Urho3D::PrefabInstanceFlag::UpdateRotation;
%csconstvalue("16") Urho3D::PrefabInstanceFlag::UpdateScale;
%csconstvalue("32") Urho3D::PrefabInstanceFlag::UpdateVariables;
%csconstvalue("2147483647") Urho3D::PrefabInstanceFlag::UpdateAll;
%typemap(csattributes) Urho3D::PrefabInstanceFlag "[global::System.Flags]";
using PrefabInstanceFlags = Urho3D::PrefabInstanceFlag;
%typemap(ctype) PrefabInstanceFlags "size_t";
%typemap(out) PrefabInstanceFlags "$result = (size_t)$1.AsInteger();"
%csattribute(Urho3D::Serializable, %arg(unsigned int), NumAttributes, GetNumAttributes);
%csattribute(Urho3D::Serializable, %arg(bool), IsTemporary, IsTemporary, SetTemporary);
%csattribute(Urho3D::Component, %arg(Urho3D::AttributeScopeHint), EffectiveScopeHint, GetEffectiveScopeHint);
%csattribute(Urho3D::Component, %arg(unsigned int), Id, GetID);
%csattribute(Urho3D::Component, %arg(Urho3D::Node *), Node, GetNode);
%csattribute(Urho3D::Component, %arg(Urho3D::Scene *), Scene, GetScene);
%csattribute(Urho3D::Component, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::Component, %arg(bool), IsEnabledEffective, IsEnabledEffective);
%csattribute(Urho3D::Component, %arg(ea::string), FullNameDebug, GetFullNameDebug);
%csattribute(Urho3D::Component, %arg(unsigned int), IndexInParent, GetIndexInParent);
%csattribute(Urho3D::ValueAnimationInfo, %arg(Urho3D::Object *), Target, GetTarget);
%csattribute(Urho3D::ValueAnimationInfo, %arg(Urho3D::ValueAnimation *), Animation, GetAnimation);
%csattribute(Urho3D::ValueAnimationInfo, %arg(Urho3D::WrapMode), WrapMode, GetWrapMode, SetWrapMode);
%csattribute(Urho3D::ValueAnimationInfo, %arg(float), Time, GetTime, SetTime);
%csattribute(Urho3D::ValueAnimationInfo, %arg(float), Speed, GetSpeed, SetSpeed);
%csattribute(Urho3D::AttributeAnimationInfo, %arg(Urho3D::AttributeInfo), AttributeInfo, GetAttributeInfo);
%csattribute(Urho3D::Animatable, %arg(bool), AnimationEnabled, GetAnimationEnabled, SetAnimationEnabled);
%csattribute(Urho3D::Animatable, %arg(Urho3D::ObjectAnimation *), ObjectAnimation, GetObjectAnimation, SetObjectAnimation);
%csattribute(Urho3D::Animatable, %arg(Urho3D::ResourceRef), ObjectAnimationAttr, GetObjectAnimationAttr, SetObjectAnimationAttr);
%csattribute(Urho3D::Node, %arg(Urho3D::AttributeScopeHint), EffectiveScopeHint, GetEffectiveScopeHint);
%csattribute(Urho3D::Node, %arg(unsigned int), Id, GetID, SetID);
%csattribute(Urho3D::Node, %arg(ea::string), Name, GetName, SetName);
%csattribute(Urho3D::Node, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::Node, %arg(ea::string), FullNameDebug, GetFullNameDebug);
%csattribute(Urho3D::Node, %arg(Urho3D::StringVector), Tags, GetTags, SetTags);
%csattribute(Urho3D::Node, %arg(Urho3D::Node *), Parent, GetParent, SetParent);
%csattribute(Urho3D::Node, %arg(Urho3D::Scene *), Scene, GetScene, SetScene);
%csattribute(Urho3D::Node, %arg(bool), IsTemporaryEffective, IsTemporaryEffective);
%csattribute(Urho3D::Node, %arg(bool), IsTransformHierarchyRoot, IsTransformHierarchyRoot);
%csattribute(Urho3D::Node, %arg(bool), IsEnabled, IsEnabled, SetEnabled);
%csattribute(Urho3D::Node, %arg(bool), IsEnabledSelf, IsEnabledSelf);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), Position, GetPosition, SetPosition);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector2), Position2D, GetPosition2D, SetPosition2D);
%csattribute(Urho3D::Node, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::Node, %arg(float), Rotation2D, GetRotation2D, SetRotation2D);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), Direction, GetDirection, SetDirection);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), Up, GetUp);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), Right, GetRight);
%csattribute(Urho3D::Node, %arg(Urho3D::Matrix3x4), TransformMatrix, GetTransformMatrix, SetTransformMatrix);
%csattribute(Urho3D::Node, %arg(Urho3D::Transform), Transform, GetTransform, SetTransform);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), WorldPosition, GetWorldPosition, SetWorldPosition);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector2), WorldPosition2D, GetWorldPosition2D, SetWorldPosition2D);
%csattribute(Urho3D::Node, %arg(Urho3D::Quaternion), WorldRotation, GetWorldRotation, SetWorldRotation);
%csattribute(Urho3D::Node, %arg(float), WorldRotation2D, GetWorldRotation2D, SetWorldRotation2D);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), WorldDirection, GetWorldDirection, SetWorldDirection);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), WorldUp, GetWorldUp);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), WorldRight, GetWorldRight);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), WorldScale, GetWorldScale, SetWorldScale);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector3), SignedWorldScale, GetSignedWorldScale);
%csattribute(Urho3D::Node, %arg(Urho3D::Vector2), WorldScale2D, GetWorldScale2D, SetWorldScale2D);
%csattribute(Urho3D::Node, %arg(Urho3D::Matrix3x4), WorldTransform, GetWorldTransform, SetWorldTransform);
%csattribute(Urho3D::Node, %arg(bool), IsDirty, IsDirty);
%csattribute(Urho3D::Node, %arg(unsigned int), IndexInParent, GetIndexInParent);
%csattribute(Urho3D::Node, %arg(unsigned int), NumComponents, GetNumComponents);
%csattribute(Urho3D::Node, %arg(ea::vector<WeakPtr<Component>>), Listeners, GetListeners);
%csattribute(Urho3D::Node, %arg(Urho3D::StringVariantMap), Vars, GetVars);
%csattribute(Urho3D::Node, %arg(ea::vector<Node *>), DependencyNodes, GetDependencyNodes);
%csattribute(Urho3D::Node, %arg(unsigned int), NumPersistentChildren, GetNumPersistentChildren);
%csattribute(Urho3D::Node, %arg(unsigned int), NumPersistentComponents, GetNumPersistentComponents);
%csattribute(Urho3D::Scene, %arg(unsigned int), NumLightmaps, GetNumLightmaps);
%csattribute(Urho3D::Scene, %arg(bool), IsUpdateEnabled, IsUpdateEnabled, SetUpdateEnabled);
%csattribute(Urho3D::Scene, %arg(bool), IsAsyncLoading, IsAsyncLoading);
%csattribute(Urho3D::Scene, %arg(float), AsyncProgress, GetAsyncProgress);
%csattribute(Urho3D::Scene, %arg(Urho3D::LoadMode), AsyncLoadMode, GetAsyncLoadMode);
%csattribute(Urho3D::Scene, %arg(ea::string), FileName, GetFileName);
%csattribute(Urho3D::Scene, %arg(unsigned int), Checksum, GetChecksum);
%csattribute(Urho3D::Scene, %arg(float), TimeScale, GetTimeScale, SetTimeScale);
%csattribute(Urho3D::Scene, %arg(float), ElapsedTime, GetElapsedTime, SetElapsedTime);
%csattribute(Urho3D::Scene, %arg(int), AsyncLoadingMs, GetAsyncLoadingMs, SetAsyncLoadingMs);
%csattribute(Urho3D::Scene, %arg(ea::vector<SharedPtr<PackageFile>>), RequiredPackageFiles, GetRequiredPackageFiles);
%csattribute(Urho3D::Scene, %arg(bool), IsThreadedUpdate, IsThreadedUpdate);
%csattribute(Urho3D::Scene, %arg(unsigned int), FreeNodeID, GetFreeNodeID);
%csattribute(Urho3D::Scene, %arg(unsigned int), FreeComponentID, GetFreeComponentID);
%csattribute(Urho3D::TrackedComponentBase, %arg(unsigned int), IndexInArray, GetIndexInArray, SetIndexInArray);
%csattribute(Urho3D::TrackedComponentBase, %arg(bool), IsTrackedInRegistry, IsTrackedInRegistry);
%csattribute(Urho3D::TrackedComponentRegistryBase, %arg(unsigned int), NumTrackedComponents, GetNumTrackedComponents);
%csattribute(Urho3D::TrackedComponentRegistryBase, %arg(ea::span<TrackedComponentBase *const>), TrackedComponents, GetTrackedComponents);
%csattribute(Urho3D::ReferencedComponentBase, %arg(Urho3D::ComponentReference), Reference, GetReference, SetReference);
%csattribute(Urho3D::ReferencedComponentRegistryBase, %arg(unsigned int), ReferenceIndexUpperBound, GetReferenceIndexUpperBound);
%csattribute(Urho3D::LogicComponent, %arg(Urho3D::UpdateEventFlags), UpdateEventMask, GetUpdateEventMask, SetUpdateEventMask);
%csattribute(Urho3D::LogicComponent, %arg(bool), IsDelayedStartCalled, IsDelayedStartCalled);
%csattribute(Urho3D::AttributePrefab, %arg(Urho3D::AttributeId), Id, GetId);
%csattribute(Urho3D::AttributePrefab, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::AttributePrefab, %arg(Urho3D::StringHash), NameHash, GetNameHash);
%csattribute(Urho3D::AttributePrefab, %arg(Urho3D::Variant), Value, GetValue, SetValue);
%csattribute(Urho3D::SerializablePrefab, %arg(ea::string), TypeName, GetTypeName);
%csattribute(Urho3D::SerializablePrefab, %arg(Urho3D::StringHash), TypeNameHash, GetTypeNameHash);
%csattribute(Urho3D::SerializablePrefab, %arg(Urho3D::SerializableId), Id, GetId, SetId);
%csattribute(Urho3D::SerializablePrefab, %arg(ea::vector<AttributePrefab>), Attributes, GetAttributes);
%csattribute(Urho3D::SerializablePrefab, %arg(ea::vector<AttributePrefab>), MutableAttributes, GetMutableAttributes);
%csattribute(Urho3D::NodePrefab, %arg(Urho3D::SerializablePrefab), Node, GetNode);
%csattribute(Urho3D::NodePrefab, %arg(Urho3D::SerializablePrefab), MutableNode, GetMutableNode);
%csattribute(Urho3D::NodePrefab, %arg(ea::vector<SerializablePrefab>), Components, GetComponents);
%csattribute(Urho3D::NodePrefab, %arg(ea::vector<SerializablePrefab>), MutableComponents, GetMutableComponents);
%csattribute(Urho3D::NodePrefab, %arg(ea::vector<NodePrefab>), Children, GetChildren);
%csattribute(Urho3D::NodePrefab, %arg(ea::vector<NodePrefab>), MutableChildren, GetMutableChildren);
%csattribute(Urho3D::NodePrefab, %arg(ea::string), NodeName, GetNodeName);
%csattribute(Urho3D::ObjectAnimation, %arg(ea::unordered_map<ea::string, SharedPtr<ValueAnimationInfo>>), AttributeAnimationInfos, GetAttributeAnimationInfos);
%csattribute(Urho3D::PrefabReaderFromMemory, %arg(bool), IsEOF, IsEOF);
%csattribute(Urho3D::PrefabReaderFromArchive, %arg(bool), IsEOF, IsEOF);
%csattribute(Urho3D::PrefabResource, %arg(Urho3D::NodePrefab), ScenePrefab, GetScenePrefab);
%csattribute(Urho3D::PrefabResource, %arg(Urho3D::NodePrefab), MutableScenePrefab, GetMutableScenePrefab);
%csattribute(Urho3D::PrefabResource, %arg(Urho3D::NodePrefab), NodePrefab, GetNodePrefab);
%csattribute(Urho3D::PrefabResource, %arg(Urho3D::NodePrefab), MutableNodePrefab, GetMutableNodePrefab);
%csattribute(Urho3D::PrefabReference, %arg(ea::string), Path, GetPath);
%csattribute(Urho3D::PrefabReference, %arg(Urho3D::ResourceRef), PrefabAttr, GetPrefabAttr, SetPrefabAttr);
%csattribute(Urho3D::PrefabWriterToMemory, %arg(bool), IsEOF, IsEOF);
%csattribute(Urho3D::PrefabWriterToMemory, %arg(Urho3D::PrefabSaveFlags), Flags, GetFlags);
%csattribute(Urho3D::PrefabWriterToArchive, %arg(bool), IsEOF, IsEOF);
%csattribute(Urho3D::PrefabWriterToArchive, %arg(Urho3D::PrefabSaveFlags), Flags, GetFlags);
%csattribute(Urho3D::SceneResource, %arg(Urho3D::Scene *), Scene, GetScene);
%csattribute(Urho3D::ShakeComponent, %arg(float), TimeScale, GetTimeScale, SetTimeScale);
%csattribute(Urho3D::ShakeComponent, %arg(float), Trauma, GetTrauma, SetTrauma);
%csattribute(Urho3D::ShakeComponent, %arg(float), TraumaPower, GetTraumaPower, SetTraumaPower);
%csattribute(Urho3D::ShakeComponent, %arg(float), TraumaFalloff, GetTraumaFalloff, SetTraumaFalloff);
%csattribute(Urho3D::ShakeComponent, %arg(Urho3D::Vector3), ShiftRange, GetShiftRange, SetShiftRange);
%csattribute(Urho3D::ShakeComponent, %arg(Urho3D::Vector3), RotationRange, GetRotationRange, SetRotationRange);
%csattribute(Urho3D::SplinePath, %arg(Urho3D::InterpolationMode), InterpolationMode, GetInterpolationMode, SetInterpolationMode);
%csattribute(Urho3D::SplinePath, %arg(float), Speed, GetSpeed, SetSpeed);
%csattribute(Urho3D::SplinePath, %arg(float), Length, GetLength);
%csattribute(Urho3D::SplinePath, %arg(Urho3D::Vector3), Position, GetPosition);
%csattribute(Urho3D::SplinePath, %arg(Urho3D::Node *), ControlledNode, GetControlledNode, SetControlledNode);
%csattribute(Urho3D::SplinePath, %arg(bool), IsFinished, IsFinished);
%csattribute(Urho3D::SplinePath, %arg(Urho3D::VariantVector), ControlPointIdsAttr, GetControlPointIdsAttr, SetControlPointIdsAttr);
%csattribute(Urho3D::SplinePath, %arg(unsigned int), ControlledIdAttr, GetControlledIdAttr, SetControlledIdAttr);
%csattribute(Urho3D::ValueAnimation, %arg(bool), IsValid, IsValid);
%csattribute(Urho3D::ValueAnimation, %arg(void *), Owner, GetOwner, SetOwner);
%csattribute(Urho3D::ValueAnimation, %arg(Urho3D::InterpMethod), InterpolationMethod, GetInterpolationMethod, SetInterpolationMethod);
%csattribute(Urho3D::ValueAnimation, %arg(float), SplineTension, GetSplineTension, SetSplineTension);
%csattribute(Urho3D::ValueAnimation, %arg(Urho3D::VariantType), ValueType, GetValueType, SetValueType);
%csattribute(Urho3D::ValueAnimation, %arg(float), BeginTime, GetBeginTime);
%csattribute(Urho3D::ValueAnimation, %arg(float), EndTime, GetEndTime);
%csattribute(Urho3D::ValueAnimation, %arg(ea::vector<VAnimKeyFrame>), KeyFrames, GetKeyFrames);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class SceneUpdateEvent {
        private StringHash _event = new StringHash("SceneUpdate");
        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public SceneUpdateEvent() { }
        public static implicit operator StringHash(SceneUpdateEvent e) { return e._event; }
    }
    public static SceneUpdateEvent SceneUpdate = new SceneUpdateEvent();
    public class SceneNetworkUpdateEvent {
        private StringHash _event = new StringHash("SceneNetworkUpdate");
        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStepReplica = new StringHash("TimeStepReplica");
        public StringHash TimeStepInput = new StringHash("TimeStepInput");
        public SceneNetworkUpdateEvent() { }
        public static implicit operator StringHash(SceneNetworkUpdateEvent e) { return e._event; }
    }
    public static SceneNetworkUpdateEvent SceneNetworkUpdate = new SceneNetworkUpdateEvent();
    public class SceneSubsystemUpdateEvent {
        private StringHash _event = new StringHash("SceneSubsystemUpdate");
        public StringHash Scene = new StringHash("Scene");
        public StringHash TimeStep = new StringHash("TimeStep");
        public SceneSubsystemUpdateEvent() { }
        public static implicit operator StringHash(SceneSubsystemUpdateEvent e) { return e._event; }
    }
    public static SceneSubsystemUpdateEvent SceneSubsystemUpdate = new SceneSubsystemUpdateEvent();
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
} %}
