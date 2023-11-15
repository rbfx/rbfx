%ignore Urho3D::DEFAULT_FPS;
%csconst(1) Urho3D::DEFAULT_FPS;
%constant int DefaultFps = 60;
%ignore Urho3D::DEFAULT_MAX_NETWORK_ANGULAR_VELOCITY;
%csconst(1) Urho3D::DEFAULT_MAX_NETWORK_ANGULAR_VELOCITY;
%constant float DefaultMaxNetworkAngularVelocity = (float)100;
%csconstvalue("0") Urho3D::SHAPE_BOX;
%csconstvalue("0") Urho3D::CONSTRAINT_POINT;
%csconstvalue("0") Urho3D::COLLISION_NEVER;
%csattribute(Urho3D::PhysicsWorld, %arg(Urho3D::Vector3), Gravity, GetGravity, SetGravity);
%csattribute(Urho3D::PhysicsWorld, %arg(int), MaxSubSteps, GetMaxSubSteps, SetMaxSubSteps);
%csattribute(Urho3D::PhysicsWorld, %arg(int), NumIterations, GetNumIterations, SetNumIterations);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), IsUpdateEnabled, IsUpdateEnabled, SetUpdateEnabled);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), Interpolation, GetInterpolation, SetInterpolation);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), InternalEdge, GetInternalEdge, SetInternalEdge);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), SplitImpulse, GetSplitImpulse, SetSplitImpulse);
%csattribute(Urho3D::PhysicsWorld, %arg(int), Fps, GetFps, SetFps);
%csattribute(Urho3D::PhysicsWorld, %arg(float), MaxNetworkAngularVelocity, GetMaxNetworkAngularVelocity, SetMaxNetworkAngularVelocity);
%csattribute(Urho3D::PhysicsWorld, %arg(btDiscreteDynamicsWorld *), World, GetWorld);
%csattribute(Urho3D::PhysicsWorld, %arg(Urho3D::CollisionGeometryDataCache), TriMeshCache, GetTriMeshCache);
%csattribute(Urho3D::PhysicsWorld, %arg(Urho3D::CollisionGeometryDataCache), ConvexCache, GetConvexCache);
%csattribute(Urho3D::PhysicsWorld, %arg(Urho3D::CollisionGeometryDataCache), GImpactTrimeshCache, GetGImpactTrimeshCache);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), IsApplyingTransforms, IsApplyingTransforms, SetApplyingTransforms);
%csattribute(Urho3D::PhysicsWorld, %arg(bool), IsSimulating, IsSimulating);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::CollisionGeometryData *), GeometryData, GetGeometryData);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::PhysicsWorld *), PhysicsWorld, GetPhysicsWorld);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::ShapeType), ShapeType, GetShapeType, SetShapeType);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::Vector3), Size, GetSize, SetSize);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::Vector3), Position, GetPosition, SetPosition);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::CollisionShape, %arg(float), Margin, GetMargin, SetMargin);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::Model *), Model, GetModel, SetModel);
%csattribute(Urho3D::CollisionShape, %arg(unsigned int), LodLevel, GetLodLevel, SetLodLevel);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::BoundingBox), WorldBoundingBox, GetWorldBoundingBox);
%csattribute(Urho3D::CollisionShape, %arg(Urho3D::ResourceRef), ModelAttr, GetModelAttr, SetModelAttr);
%csattribute(Urho3D::Constraint, %arg(Urho3D::PhysicsWorld *), PhysicsWorld, GetPhysicsWorld);
%csattribute(Urho3D::Constraint, %arg(Urho3D::ConstraintType), ConstraintType, GetConstraintType, SetConstraintType);
%csattribute(Urho3D::Constraint, %arg(Urho3D::RigidBody *), OwnBody, GetOwnBody);
%csattribute(Urho3D::Constraint, %arg(Urho3D::RigidBody *), OtherBody, GetOtherBody, SetOtherBody);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Vector3), Position, GetPosition, SetPosition);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Vector3), OtherPosition, GetOtherPosition, SetOtherPosition);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Quaternion), OtherRotation, GetOtherRotation, SetOtherRotation);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Vector3), WorldPosition, GetWorldPosition, SetWorldPosition);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Vector2), HighLimit, GetHighLimit, SetHighLimit);
%csattribute(Urho3D::Constraint, %arg(Urho3D::Vector2), LowLimit, GetLowLimit, SetLowLimit);
%csattribute(Urho3D::Constraint, %arg(float), Erp, GetERP, SetERP);
%csattribute(Urho3D::Constraint, %arg(float), Cfm, GetCFM, SetCFM);
%csattribute(Urho3D::Constraint, %arg(bool), DisableCollision, GetDisableCollision, SetDisableCollision);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::PhysicsWorld *), PhysicsWorld, GetPhysicsWorld);
%csattribute(Urho3D::RigidBody, %arg(btRigidBody *), Body, GetBody);
%csattribute(Urho3D::RigidBody, %arg(btCompoundShape *), CompoundShape, GetCompoundShape);
%csattribute(Urho3D::RigidBody, %arg(float), Mass, GetMass, SetMass);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), Position, GetPosition, SetPosition);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), LinearVelocity, GetLinearVelocity, SetLinearVelocity);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), LinearFactor, GetLinearFactor, SetLinearFactor);
%csattribute(Urho3D::RigidBody, %arg(float), LinearRestThreshold, GetLinearRestThreshold, SetLinearRestThreshold);
%csattribute(Urho3D::RigidBody, %arg(float), LinearDamping, GetLinearDamping, SetLinearDamping);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), AngularVelocity, GetAngularVelocity, SetAngularVelocity);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), AngularFactor, GetAngularFactor, SetAngularFactor);
%csattribute(Urho3D::RigidBody, %arg(float), AngularRestThreshold, GetAngularRestThreshold, SetAngularRestThreshold);
%csattribute(Urho3D::RigidBody, %arg(float), AngularDamping, GetAngularDamping, SetAngularDamping);
%csattribute(Urho3D::RigidBody, %arg(float), Friction, GetFriction, SetFriction);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), AnisotropicFriction, GetAnisotropicFriction, SetAnisotropicFriction);
%csattribute(Urho3D::RigidBody, %arg(float), RollingFriction, GetRollingFriction, SetRollingFriction);
%csattribute(Urho3D::RigidBody, %arg(float), Restitution, GetRestitution, SetRestitution);
%csattribute(Urho3D::RigidBody, %arg(float), ContactProcessingThreshold, GetContactProcessingThreshold, SetContactProcessingThreshold);
%csattribute(Urho3D::RigidBody, %arg(float), CcdRadius, GetCcdRadius, SetCcdRadius);
%csattribute(Urho3D::RigidBody, %arg(float), CcdMotionThreshold, GetCcdMotionThreshold, SetCcdMotionThreshold);
%csattribute(Urho3D::RigidBody, %arg(bool), UseGravity, GetUseGravity, SetUseGravity);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), GravityOverride, GetGravityOverride, SetGravityOverride);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::Vector3), CenterOfMass, GetCenterOfMass);
%csattribute(Urho3D::RigidBody, %arg(bool), IsKinematic, IsKinematic, SetKinematic);
%csattribute(Urho3D::RigidBody, %arg(bool), IsTrigger, IsTrigger, SetTrigger);
%csattribute(Urho3D::RigidBody, %arg(bool), IsActive, IsActive);
%csattribute(Urho3D::RigidBody, %arg(unsigned int), CollisionLayer, GetCollisionLayer, SetCollisionLayer);
%csattribute(Urho3D::RigidBody, %arg(unsigned int), CollisionMask, GetCollisionMask, SetCollisionMask);
%csattribute(Urho3D::RigidBody, %arg(Urho3D::CollisionEventMode), CollisionEventMode, GetCollisionEventMode, SetCollisionEventMode);
%csattribute(Urho3D::KinematicCharacterController, %arg(Urho3D::Vector3), RawPosition, GetRawPosition);
%csattribute(Urho3D::KinematicCharacterController, %arg(unsigned int), CollisionLayer, GetCollisionLayer, SetCollisionLayer);
%csattribute(Urho3D::KinematicCharacterController, %arg(unsigned int), CollisionMask, GetCollisionMask, SetCollisionMask);
%csattribute(Urho3D::KinematicCharacterController, %arg(bool), IsTrigger, IsTrigger, SetTrigger);
%csattribute(Urho3D::KinematicCharacterController, %arg(Urho3D::Vector3), Gravity, GetGravity, SetGravity);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), LinearDamping, GetLinearDamping, SetLinearDamping);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), AngularDamping, GetAngularDamping, SetAngularDamping);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), Height, GetHeight, SetHeight);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), Diameter, GetDiameter, SetDiameter);
%csattribute(Urho3D::KinematicCharacterController, %arg(bool), ActivateTriggers, GetActivateTriggers, SetActivateTriggers);
%csattribute(Urho3D::KinematicCharacterController, %arg(Urho3D::Vector3), Offset, GetOffset, SetOffset);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), StepHeight, GetStepHeight, SetStepHeight);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), MaxJumpHeight, GetMaxJumpHeight, SetMaxJumpHeight);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), FallSpeed, GetFallSpeed, SetFallSpeed);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), JumpSpeed, GetJumpSpeed, SetJumpSpeed);
%csattribute(Urho3D::KinematicCharacterController, %arg(float), MaxSlope, GetMaxSlope, SetMaxSlope);
%csattribute(Urho3D::KinematicCharacterController, %arg(Urho3D::Vector3), AngularVelocity, GetAngularVelocity);
%csattribute(Urho3D::KinematicCharacterController, %arg(Urho3D::Vector3), LinearVelocity, GetLinearVelocity);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(unsigned int), WheelIndex, GetWheelIndex, SetWheelIndex);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), ConnectionPoint, GetConnectionPoint, SetConnectionPoint);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), Offset, GetOffset, SetOffset);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Quaternion), Rotation, GetRotation, SetRotation);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), Direction, GetDirection, SetDirection);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), Axle, GetAxle, SetAxle);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SteeringValue, GetSteeringValue, SetSteeringValue);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), BrakeValue, GetBrakeValue, SetBrakeValue);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), EngineForce, GetEngineForce, SetEngineForce);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), RollInfluence, GetRollInfluence, SetRollInfluence);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), Radius, GetRadius, SetRadius);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SuspensionRestLength, GetSuspensionRestLength, SetSuspensionRestLength);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), MaxSuspensionTravel, GetMaxSuspensionTravel, SetMaxSuspensionTravel);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SuspensionStiffness, GetSuspensionStiffness, SetSuspensionStiffness);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), DampingCompression, GetDampingCompression, SetDampingCompression);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), DampingRelaxation, GetDampingRelaxation, SetDampingRelaxation);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), FrictionSlip, GetFrictionSlip, SetFrictionSlip);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), MaxSuspensionForce, GetMaxSuspensionForce, SetMaxSuspensionForce);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SteeringFactor, GetSteeringFactor, SetSteeringFactor);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), EngineFactor, GetEngineFactor, SetEngineFactor);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), BrakeFactor, GetBrakeFactor, SetBrakeFactor);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SlidingFactor, GetSlidingFactor, SetSlidingFactor);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SkidInfoCumulative, GetSkidInfoCumulative, SetSkidInfoCumulative);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(float), SideSlipSpeed, GetSideSlipSpeed, SetSideSlipSpeed);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), ContactPosition, GetContactPosition, SetContactPosition);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(Urho3D::Vector3), ContactNormal, GetContactNormal, SetContactNormal);
%csattribute(Urho3D::RaycastVehicleWheel, %arg(bool), IsInContact, IsInContact, SetInContact);
%csattribute(Urho3D::RaycastVehicle, %arg(float), EngineForce, GetEngineForce, SetEngineForce);
%csattribute(Urho3D::RaycastVehicle, %arg(float), BrakingForce, GetBrakingForce, SetBrakingForce);
%csattribute(Urho3D::RaycastVehicle, %arg(int), NumWheels, GetNumWheels);
%csattribute(Urho3D::RaycastVehicle, %arg(float), MaxSideSlipSpeed, GetMaxSideSlipSpeed, SetMaxSideSlipSpeed);
%csattribute(Urho3D::RaycastVehicle, %arg(float), InAirRPM, GetInAirRPM, SetInAirRPM);
%csattribute(Urho3D::RaycastVehicle, %arg(Urho3D::IntVector3), CoordinateSystem, GetCoordinateSystem, SetCoordinateSystem);
%csattribute(Urho3D::TriggerAnimator, %arg(Urho3D::ResourceRef), EnterAnimationAttr, GetEnterAnimationAttr, SetEnterAnimationAttr);
%csattribute(Urho3D::TriggerAnimator, %arg(Urho3D::Animation *), EnterAnimation, GetEnterAnimation, SetEnterAnimation);
%csattribute(Urho3D::TriggerAnimator, %arg(Urho3D::ResourceRef), ExitAnimationAttr, GetExitAnimationAttr, SetExitAnimationAttr);
%csattribute(Urho3D::TriggerAnimator, %arg(Urho3D::Animation *), ExitAnimation, GetExitAnimation, SetExitAnimation);
%pragma(csharp) moduleimports=%{
public static partial class E
{
    public class PhysicsPreUpdateEvent {
        private StringHash _event = new StringHash("PhysicsPreUpdate");
        public StringHash World = new StringHash("World");
        public StringHash TimeStep = new StringHash("TimeStep");
        public PhysicsPreUpdateEvent() { }
        public static implicit operator StringHash(PhysicsPreUpdateEvent e) { return e._event; }
    }
    public static PhysicsPreUpdateEvent PhysicsPreUpdate = new PhysicsPreUpdateEvent();
    public class PhysicsPostUpdateEvent {
        private StringHash _event = new StringHash("PhysicsPostUpdate");
        public StringHash World = new StringHash("World");
        public StringHash TimeStep = new StringHash("TimeStep");
        public StringHash Overtime = new StringHash("Overtime");
        public PhysicsPostUpdateEvent() { }
        public static implicit operator StringHash(PhysicsPostUpdateEvent e) { return e._event; }
    }
    public static PhysicsPostUpdateEvent PhysicsPostUpdate = new PhysicsPostUpdateEvent();
    public class PhysicsPreStepEvent {
        private StringHash _event = new StringHash("PhysicsPreStep");
        public StringHash World = new StringHash("World");
        public StringHash TimeStep = new StringHash("TimeStep");
        public StringHash NetworkFrame = new StringHash("NetworkFrame");
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
} %}
