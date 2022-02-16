%csattribute(Urho3D::CollisionShape2D, %arg(bool), IsTrigger, IsTrigger, SetTrigger);
%csattribute(Urho3D::CollisionShape2D, %arg(int), CategoryBits, GetCategoryBits, SetCategoryBits);
%csattribute(Urho3D::CollisionShape2D, %arg(int), MaskBits, GetMaskBits, SetMaskBits);
%csattribute(Urho3D::CollisionShape2D, %arg(int), GroupIndex, GetGroupIndex, SetGroupIndex);
%csattribute(Urho3D::CollisionShape2D, %arg(float), Density, GetDensity, SetDensity);
%csattribute(Urho3D::CollisionShape2D, %arg(float), Friction, GetFriction, SetFriction);
%csattribute(Urho3D::CollisionShape2D, %arg(float), Restitution, GetRestitution, SetRestitution);
%csattribute(Urho3D::CollisionShape2D, %arg(float), Mass, GetMass);
%csattribute(Urho3D::CollisionShape2D, %arg(float), Inertia, GetInertia);
%csattribute(Urho3D::CollisionShape2D, %arg(Urho3D::Vector2), MassCenter, GetMassCenter);
%csattribute(Urho3D::CollisionShape2D, %arg(b2Fixture *), Fixture, GetFixture);
%csattribute(Urho3D::CollisionBox2D, %arg(Urho3D::Vector2), Size, GetSize, SetSize);
%csattribute(Urho3D::CollisionBox2D, %arg(Urho3D::Vector2), Center, GetCenter, SetCenter);
%csattribute(Urho3D::CollisionBox2D, %arg(float), Angle, GetAngle, SetAngle);
%csattribute(Urho3D::CollisionChain2D, %arg(bool), Loop, GetLoop, SetLoop);
%csattribute(Urho3D::CollisionChain2D, %arg(unsigned int), VertexCount, GetVertexCount, SetVertexCount);
%csattribute(Urho3D::CollisionChain2D, %arg(ea::vector<Vector2>), Vertices, GetVertices, SetVertices);
%csattribute(Urho3D::CollisionChain2D, %arg(ea::vector<unsigned char>), VerticesAttr, GetVerticesAttr, SetVerticesAttr);
%csattribute(Urho3D::CollisionCircle2D, %arg(float), Radius, GetRadius, SetRadius);
%csattribute(Urho3D::CollisionCircle2D, %arg(Urho3D::Vector2), Center, GetCenter, SetCenter);
%csattribute(Urho3D::CollisionEdge2D, %arg(Urho3D::Vector2), Vertex1, GetVertex1, SetVertex1);
%csattribute(Urho3D::CollisionEdge2D, %arg(Urho3D::Vector2), Vertex2, GetVertex2, SetVertex2);
%csattribute(Urho3D::CollisionPolygon2D, %arg(unsigned int), VertexCount, GetVertexCount, SetVertexCount);
%csattribute(Urho3D::CollisionPolygon2D, %arg(ea::vector<Vector2>), Vertices, GetVertices, SetVertices);
%csattribute(Urho3D::CollisionPolygon2D, %arg(ea::vector<unsigned char>), VerticesAttr, GetVerticesAttr, SetVerticesAttr);
%csattribute(Urho3D::Constraint2D, %arg(Urho3D::RigidBody2D *), OwnerBody, GetOwnerBody);
%csattribute(Urho3D::Constraint2D, %arg(Urho3D::RigidBody2D *), OtherBody, GetOtherBody, SetOtherBody);
%csattribute(Urho3D::Constraint2D, %arg(bool), CollideConnected, GetCollideConnected, SetCollideConnected);
%csattribute(Urho3D::Constraint2D, %arg(Urho3D::Constraint2D *), AttachedConstraint, GetAttachedConstraint, SetAttachedConstraint);
%csattribute(Urho3D::Constraint2D, %arg(b2Joint *), Joint, GetJoint);
%csattribute(Urho3D::ConstraintDistance2D, %arg(Urho3D::Vector2), OwnerBodyAnchor, GetOwnerBodyAnchor, SetOwnerBodyAnchor);
%csattribute(Urho3D::ConstraintDistance2D, %arg(Urho3D::Vector2), OtherBodyAnchor, GetOtherBodyAnchor, SetOtherBodyAnchor);
%csattribute(Urho3D::ConstraintDistance2D, %arg(float), FrequencyHz, GetFrequencyHz, SetFrequencyHz);
%csattribute(Urho3D::ConstraintDistance2D, %arg(float), DampingRatio, GetDampingRatio, SetDampingRatio);
%csattribute(Urho3D::ConstraintDistance2D, %arg(float), Length, GetLength, SetLength);
%csattribute(Urho3D::ConstraintFriction2D, %arg(Urho3D::Vector2), Anchor, GetAnchor, SetAnchor);
%csattribute(Urho3D::ConstraintFriction2D, %arg(float), MaxForce, GetMaxForce, SetMaxForce);
%csattribute(Urho3D::ConstraintFriction2D, %arg(float), MaxTorque, GetMaxTorque, SetMaxTorque);
%csattribute(Urho3D::ConstraintGear2D, %arg(Urho3D::Constraint2D *), OwnerConstraint, GetOwnerConstraint, SetOwnerConstraint);
%csattribute(Urho3D::ConstraintGear2D, %arg(Urho3D::Constraint2D *), OtherConstraint, GetOtherConstraint, SetOtherConstraint);
%csattribute(Urho3D::ConstraintGear2D, %arg(float), Ratio, GetRatio, SetRatio);
%csattribute(Urho3D::ConstraintMotor2D, %arg(Urho3D::Vector2), LinearOffset, GetLinearOffset, SetLinearOffset);
%csattribute(Urho3D::ConstraintMotor2D, %arg(float), AngularOffset, GetAngularOffset, SetAngularOffset);
%csattribute(Urho3D::ConstraintMotor2D, %arg(float), MaxForce, GetMaxForce, SetMaxForce);
%csattribute(Urho3D::ConstraintMotor2D, %arg(float), MaxTorque, GetMaxTorque, SetMaxTorque);
%csattribute(Urho3D::ConstraintMotor2D, %arg(float), CorrectionFactor, GetCorrectionFactor, SetCorrectionFactor);
%csattribute(Urho3D::ConstraintMouse2D, %arg(Urho3D::Vector2), Target, GetTarget, SetTarget);
%csattribute(Urho3D::ConstraintMouse2D, %arg(float), MaxForce, GetMaxForce, SetMaxForce);
%csattribute(Urho3D::ConstraintMouse2D, %arg(float), FrequencyHz, GetFrequencyHz, SetFrequencyHz);
%csattribute(Urho3D::ConstraintMouse2D, %arg(float), DampingRatio, GetDampingRatio, SetDampingRatio);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(Urho3D::Vector2), Anchor, GetAnchor, SetAnchor);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(Urho3D::Vector2), Axis, GetAxis, SetAxis);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(bool), EnableLimit, GetEnableLimit, SetEnableLimit);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(float), LowerTranslation, GetLowerTranslation, SetLowerTranslation);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(float), UpperTranslation, GetUpperTranslation, SetUpperTranslation);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(bool), EnableMotor, GetEnableMotor, SetEnableMotor);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(float), MaxMotorForce, GetMaxMotorForce, SetMaxMotorForce);
%csattribute(Urho3D::ConstraintPrismatic2D, %arg(float), MotorSpeed, GetMotorSpeed, SetMotorSpeed);
%csattribute(Urho3D::ConstraintPulley2D, %arg(Urho3D::Vector2), OwnerBodyGroundAnchor, GetOwnerBodyGroundAnchor, SetOwnerBodyGroundAnchor);
%csattribute(Urho3D::ConstraintPulley2D, %arg(Urho3D::Vector2), OtherBodyGroundAnchor, GetOtherBodyGroundAnchor, SetOtherBodyGroundAnchor);
%csattribute(Urho3D::ConstraintPulley2D, %arg(Urho3D::Vector2), OwnerBodyAnchor, GetOwnerBodyAnchor, SetOwnerBodyAnchor);
%csattribute(Urho3D::ConstraintPulley2D, %arg(Urho3D::Vector2), OtherBodyAnchor, GetOtherBodyAnchor, SetOtherBodyAnchor);
%csattribute(Urho3D::ConstraintPulley2D, %arg(float), Ratio, GetRatio, SetRatio);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(Urho3D::Vector2), Anchor, GetAnchor, SetAnchor);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(bool), EnableLimit, GetEnableLimit, SetEnableLimit);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(float), LowerAngle, GetLowerAngle, SetLowerAngle);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(float), UpperAngle, GetUpperAngle, SetUpperAngle);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(bool), EnableMotor, GetEnableMotor, SetEnableMotor);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(float), MotorSpeed, GetMotorSpeed, SetMotorSpeed);
%csattribute(Urho3D::ConstraintRevolute2D, %arg(float), MaxMotorTorque, GetMaxMotorTorque, SetMaxMotorTorque);
%csattribute(Urho3D::ConstraintRope2D, %arg(Urho3D::Vector2), OwnerBodyAnchor, GetOwnerBodyAnchor, SetOwnerBodyAnchor);
%csattribute(Urho3D::ConstraintRope2D, %arg(Urho3D::Vector2), OtherBodyAnchor, GetOtherBodyAnchor, SetOtherBodyAnchor);
%csattribute(Urho3D::ConstraintRope2D, %arg(float), MaxLength, GetMaxLength, SetMaxLength);
%csattribute(Urho3D::ConstraintWeld2D, %arg(Urho3D::Vector2), Anchor, GetAnchor, SetAnchor);
%csattribute(Urho3D::ConstraintWeld2D, %arg(float), FrequencyHz, GetFrequencyHz, SetFrequencyHz);
%csattribute(Urho3D::ConstraintWeld2D, %arg(float), DampingRatio, GetDampingRatio, SetDampingRatio);
%csattribute(Urho3D::ConstraintWheel2D, %arg(Urho3D::Vector2), Anchor, GetAnchor, SetAnchor);
%csattribute(Urho3D::ConstraintWheel2D, %arg(Urho3D::Vector2), Axis, GetAxis, SetAxis);
%csattribute(Urho3D::ConstraintWheel2D, %arg(bool), EnableMotor, GetEnableMotor, SetEnableMotor);
%csattribute(Urho3D::ConstraintWheel2D, %arg(float), MaxMotorTorque, GetMaxMotorTorque, SetMaxMotorTorque);
%csattribute(Urho3D::ConstraintWheel2D, %arg(float), MotorSpeed, GetMotorSpeed, SetMotorSpeed);
%csattribute(Urho3D::ConstraintWheel2D, %arg(float), FrequencyHz, GetFrequencyHz, SetFrequencyHz);
%csattribute(Urho3D::ConstraintWheel2D, %arg(float), DampingRatio, GetDampingRatio, SetDampingRatio);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), IsUpdateEnabled, IsUpdateEnabled, SetUpdateEnabled);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), DrawShape, GetDrawShape, SetDrawShape);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), DrawJoint, GetDrawJoint, SetDrawJoint);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), DrawAabb, GetDrawAabb, SetDrawAabb);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), DrawPair, GetDrawPair, SetDrawPair);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), DrawCenterOfMass, GetDrawCenterOfMass, SetDrawCenterOfMass);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), AllowSleeping, GetAllowSleeping, SetAllowSleeping);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), WarmStarting, GetWarmStarting, SetWarmStarting);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), ContinuousPhysics, GetContinuousPhysics, SetContinuousPhysics);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), SubStepping, GetSubStepping, SetSubStepping);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), AutoClearForces, GetAutoClearForces, SetAutoClearForces);
%csattribute(Urho3D::PhysicsWorld2D, %arg(Urho3D::Vector2), Gravity, GetGravity, SetGravity);
%csattribute(Urho3D::PhysicsWorld2D, %arg(int), VelocityIterations, GetVelocityIterations, SetVelocityIterations);
%csattribute(Urho3D::PhysicsWorld2D, %arg(int), PositionIterations, GetPositionIterations, SetPositionIterations);
%csattribute(Urho3D::PhysicsWorld2D, %arg(b2World *), World, GetWorld);
%csattribute(Urho3D::PhysicsWorld2D, %arg(bool), IsApplyingTransforms, IsApplyingTransforms, SetApplyingTransforms);
%csattribute(Urho3D::RigidBody2D, %arg(Urho3D::BodyType2D), BodyType, GetBodyType, SetBodyType);
%csattribute(Urho3D::RigidBody2D, %arg(float), Mass, GetMass, SetMass);
%csattribute(Urho3D::RigidBody2D, %arg(float), Inertia, GetInertia, SetInertia);
%csattribute(Urho3D::RigidBody2D, %arg(Urho3D::Vector2), MassCenter, GetMassCenter, SetMassCenter);
%csattribute(Urho3D::RigidBody2D, %arg(bool), UseFixtureMass, GetUseFixtureMass, SetUseFixtureMass);
%csattribute(Urho3D::RigidBody2D, %arg(float), LinearDamping, GetLinearDamping, SetLinearDamping);
%csattribute(Urho3D::RigidBody2D, %arg(float), AngularDamping, GetAngularDamping, SetAngularDamping);
%csattribute(Urho3D::RigidBody2D, %arg(bool), IsAllowSleep, IsAllowSleep, SetAllowSleep);
%csattribute(Urho3D::RigidBody2D, %arg(bool), IsFixedRotation, IsFixedRotation, SetFixedRotation);
%csattribute(Urho3D::RigidBody2D, %arg(bool), IsBullet, IsBullet, SetBullet);
%csattribute(Urho3D::RigidBody2D, %arg(float), GravityScale, GetGravityScale, SetGravityScale);
%csattribute(Urho3D::RigidBody2D, %arg(bool), IsAwake, IsAwake, SetAwake);
%csattribute(Urho3D::RigidBody2D, %arg(Urho3D::Vector2), LinearVelocity, GetLinearVelocity, SetLinearVelocity);
%csattribute(Urho3D::RigidBody2D, %arg(float), AngularVelocity, GetAngularVelocity, SetAngularVelocity);
%csattribute(Urho3D::RigidBody2D, %arg(b2Body *), Body, GetBody);
%pragma(csharp) moduleimports=%{
public static partial class E
{
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
} %}
