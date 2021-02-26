%constant float PixelSize = Urho3D::PIXEL_SIZE;
%ignore Urho3D::PIXEL_SIZE;
%constant unsigned int FlipHorizontal = Urho3D::FLIP_HORIZONTAL;
%ignore Urho3D::FLIP_HORIZONTAL;
%ignore Urho3D::FLIP_VERTICAL;
%csconst(1) Urho3D::FLIP_VERTICAL;
%constant unsigned int FlipVertical = 1073741824;
%ignore Urho3D::FLIP_DIAGONAL;
%csconst(1) Urho3D::FLIP_DIAGONAL;
%constant unsigned int FlipDiagonal = 536870912;
%ignore Urho3D::FLIP_RESERVED;
%csconst(1) Urho3D::FLIP_RESERVED;
%constant unsigned int FlipReserved = 268435456;
%constant unsigned int FlipAll = Urho3D::FLIP_ALL;
%ignore Urho3D::FLIP_ALL;
%csconstvalue("0") Urho3D::EMITTER_TYPE_GRAVITY;
%csconstvalue("0") Urho3D::Spriter::BONE;
%csconstvalue("0") Urho3D::Spriter::INSTANT;
%csconstvalue("0") Urho3D::Spriter::Default;
%csattribute(Urho3D::Drawable2D, %arg(int), Layer, GetLayer, SetLayer);
%csattribute(Urho3D::Drawable2D, %arg(int), OrderInLayer, GetOrderInLayer, SetOrderInLayer);
%csattribute(Urho3D::Drawable2D, %arg(ea::vector<SourceBatch2D>), SourceBatches, GetSourceBatches);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite, SetSprite);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Rect), DrawRect, GetDrawRect, SetDrawRect);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Rect), TextureRect, GetTextureRect, SetTextureRect);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::BlendMode), BlendMode, GetBlendMode, SetBlendMode);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), FlipX, GetFlipX, SetFlipX);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), FlipY, GetFlipY, SetFlipY);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), SwapXY, GetSwapXY, SetSwapXY);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Color), Color, GetColor, SetColor);
%csattribute(Urho3D::StaticSprite2D, %arg(float), Alpha, GetAlpha, SetAlpha);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), UseHotSpot, GetUseHotSpot, SetUseHotSpot);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), UseDrawRect, GetUseDrawRect, SetUseDrawRect);
%csattribute(Urho3D::StaticSprite2D, %arg(bool), UseTextureRect, GetUseTextureRect, SetUseTextureRect);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Vector2), HotSpot, GetHotSpot, SetHotSpot);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::Material *), CustomMaterial, GetCustomMaterial, SetCustomMaterial);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::ResourceRef), SpriteAttr, GetSpriteAttr, SetSpriteAttr);
%csattribute(Urho3D::StaticSprite2D, %arg(Urho3D::ResourceRef), CustomMaterialAttr, GetCustomMaterialAttr, SetCustomMaterialAttr);
%csattribute(Urho3D::AnimatedSprite2D, %arg(Urho3D::AnimationSet2D *), AnimationSet, GetAnimationSet, SetAnimationSet);
%csattribute(Urho3D::AnimatedSprite2D, %arg(ea::string), Entity, GetEntity, SetEntity);
%csattribute(Urho3D::AnimatedSprite2D, %arg(Urho3D::LoopMode2D), LoopMode, GetLoopMode, SetLoopMode);
%csattribute(Urho3D::AnimatedSprite2D, %arg(float), Speed, GetSpeed, SetSpeed);
%csattribute(Urho3D::AnimatedSprite2D, %arg(Urho3D::ResourceRef), AnimationSetAttr, GetAnimationSetAttr, SetAnimationSetAttr);
%csattribute(Urho3D::AnimationSet2D, %arg(unsigned int), NumAnimations, GetNumAnimations);
%csattribute(Urho3D::AnimationSet2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite);
%csattribute(Urho3D::AnimationSet2D, %arg(Spriter::SpriterData *), SpriterData, GetSpriterData);
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
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite, SetSprite);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Vector2), SourcePositionVariance, GetSourcePositionVariance, SetSourcePositionVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), Speed, GetSpeed, SetSpeed);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), SpeedVariance, GetSpeedVariance, SetSpeedVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), ParticleLifeSpan, GetParticleLifeSpan, SetParticleLifeSpan);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), ParticleLifespanVariance, GetParticleLifespanVariance, SetParticleLifespanVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), Angle, GetAngle, SetAngle);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), AngleVariance, GetAngleVariance, SetAngleVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Vector2), Gravity, GetGravity, SetGravity);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RadialAcceleration, GetRadialAcceleration, SetRadialAcceleration);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), TangentialAcceleration, GetTangentialAcceleration, SetTangentialAcceleration);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RadialAccelVariance, GetRadialAccelVariance, SetRadialAccelVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), TangentialAccelVariance, GetTangentialAccelVariance, SetTangentialAccelVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Color), StartColor, GetStartColor, SetStartColor);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Color), StartColorVariance, GetStartColorVariance, SetStartColorVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Color), FinishColor, GetFinishColor, SetFinishColor);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::Color), FinishColorVariance, GetFinishColorVariance, SetFinishColorVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(int), MaxParticles, GetMaxParticles, SetMaxParticles);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), StartParticleSize, GetStartParticleSize, SetStartParticleSize);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), StartParticleSizeVariance, GetStartParticleSizeVariance, SetStartParticleSizeVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), FinishParticleSize, GetFinishParticleSize, SetFinishParticleSize);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), FinishParticleSizeVariance, GetFinishParticleSizeVariance, SetFinishParticleSizeVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), Duration, GetDuration, SetDuration);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::EmitterType2D), EmitterType, GetEmitterType, SetEmitterType);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), MaxRadius, GetMaxRadius, SetMaxRadius);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), MaxRadiusVariance, GetMaxRadiusVariance, SetMaxRadiusVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), MinRadius, GetMinRadius, SetMinRadius);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), MinRadiusVariance, GetMinRadiusVariance, SetMinRadiusVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotatePerSecond, GetRotatePerSecond, SetRotatePerSecond);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotatePerSecondVariance, GetRotatePerSecondVariance, SetRotatePerSecondVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(Urho3D::BlendMode), BlendMode, GetBlendMode, SetBlendMode);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotationStart, GetRotationStart, SetRotationStart);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotationStartVariance, GetRotationStartVariance, SetRotationStartVariance);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotationEnd, GetRotationEnd, SetRotationEnd);
%csattribute(Urho3D::ParticleEffect2D, %arg(float), RotationEndVariance, GetRotationEndVariance, SetRotationEndVariance);
%csattribute(Urho3D::ParticleEmitter2D, %arg(Urho3D::ParticleEffect2D *), Effect, GetEffect, SetEffect);
%csattribute(Urho3D::ParticleEmitter2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite, SetSprite);
%csattribute(Urho3D::ParticleEmitter2D, %arg(Urho3D::BlendMode), BlendMode, GetBlendMode, SetBlendMode);
%csattribute(Urho3D::ParticleEmitter2D, %arg(unsigned int), MaxParticles, GetMaxParticles, SetMaxParticles);
%csattribute(Urho3D::ParticleEmitter2D, %arg(Urho3D::ResourceRef), ParticleEffectAttr, GetParticleEffectAttr, SetParticleEffectAttr);
%csattribute(Urho3D::ParticleEmitter2D, %arg(Urho3D::ResourceRef), SpriteAttr, GetSpriteAttr, SetSpriteAttr);
%csattribute(Urho3D::ParticleEmitter2D, %arg(bool), IsEmitting, IsEmitting, SetEmitting);
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
%csattribute(Urho3D::Renderer2D, %arg(Urho3D::UpdateGeometryType), UpdateGeometryType, GetUpdateGeometryType);
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
%csattribute(Urho3D::Sprite2D, %arg(Urho3D::Texture2D *), Texture, GetTexture, SetTexture);
%csattribute(Urho3D::Sprite2D, %arg(Urho3D::IntRect), Rectangle, GetRectangle, SetRectangle);
%csattribute(Urho3D::Sprite2D, %arg(Urho3D::Vector2), HotSpot, GetHotSpot, SetHotSpot);
%csattribute(Urho3D::Sprite2D, %arg(Urho3D::IntVector2), Offset, GetOffset, SetOffset);
%csattribute(Urho3D::Sprite2D, %arg(float), TextureEdgeOffset, GetTextureEdgeOffset, SetTextureEdgeOffset);
%csattribute(Urho3D::Sprite2D, %arg(Urho3D::SpriteSheet2D *), SpriteSheet, GetSpriteSheet, SetSpriteSheet);
%csattribute(Urho3D::SpriteSheet2D, %arg(Urho3D::Texture2D *), Texture, GetTexture, SetTexture);
%csattribute(Urho3D::SpriteSheet2D, %arg(ea::unordered_map<ea::string, SharedPtr<Sprite2D>>), SpriteMapping, GetSpriteMapping);
%csattribute(Urho3D::Spriter::BoneTimelineKey, %arg(Urho3D::Spriter::ObjectType), ObjectType, GetObjectType);
%csattribute(Urho3D::Spriter::SpriteTimelineKey, %arg(Urho3D::Spriter::ObjectType), ObjectType, GetObjectType);
%csattribute(Urho3D::Spriter::SpriterInstance, %arg(Spriter::Entity *), Entity, GetEntity);
%csattribute(Urho3D::Spriter::SpriterInstance, %arg(Spriter::Animation *), Animation, GetAnimation);
%csattribute(Urho3D::Spriter::SpriterInstance, %arg(Spriter::SpatialInfo), SpatialInfo, GetSpatialInfo);
%csattribute(Urho3D::Spriter::SpriterInstance, %arg(ea::vector<Spriter::SpatialTimelineKey *>), TimelineKeys, GetTimelineKeys);
%csattribute(Urho3D::StretchableSprite2D, %arg(Urho3D::IntRect), Border, GetBorder, SetBorder);
%csattribute(Urho3D::TileMapInfo2D, %arg(float), MapWidth, GetMapWidth);
%csattribute(Urho3D::TileMapInfo2D, %arg(float), MapHeight, GetMapHeight);
%csattribute(Urho3D::Tile2D, %arg(unsigned int), Gid, GetGid);
%csattribute(Urho3D::Tile2D, %arg(bool), FlipX, GetFlipX);
%csattribute(Urho3D::Tile2D, %arg(bool), FlipY, GetFlipY);
%csattribute(Urho3D::Tile2D, %arg(bool), SwapXY, GetSwapXY);
%csattribute(Urho3D::Tile2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite);
%csattribute(Urho3D::TileMapObject2D, %arg(Urho3D::TileMapObjectType2D), ObjectType, GetObjectType);
%csattribute(Urho3D::TileMapObject2D, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::TileMapObject2D, %arg(Urho3D::Vector2), Position, GetPosition);
%csattribute(Urho3D::TileMapObject2D, %arg(Urho3D::Vector2), Size, GetSize);
%csattribute(Urho3D::TileMapObject2D, %arg(unsigned int), NumPoints, GetNumPoints);
%csattribute(Urho3D::TileMapObject2D, %arg(unsigned int), TileGid, GetTileGid);
%csattribute(Urho3D::TileMapObject2D, %arg(bool), TileFlipX, GetTileFlipX);
%csattribute(Urho3D::TileMapObject2D, %arg(bool), TileFlipY, GetTileFlipY);
%csattribute(Urho3D::TileMapObject2D, %arg(bool), TileSwapXY, GetTileSwapXY);
%csattribute(Urho3D::TileMapObject2D, %arg(Urho3D::Sprite2D *), TileSprite, GetTileSprite);
%csattribute(Urho3D::TileMap2D, %arg(Urho3D::TmxFile2D *), TmxFile, GetTmxFile, SetTmxFile);
%csattribute(Urho3D::TileMap2D, %arg(Urho3D::TileMapInfo2D), Info, GetInfo);
%csattribute(Urho3D::TileMap2D, %arg(unsigned int), NumLayers, GetNumLayers);
%csattribute(Urho3D::TileMap2D, %arg(Urho3D::ResourceRef), TmxFileAttr, GetTmxFileAttr, SetTmxFileAttr);
%csattribute(Urho3D::TileMapLayer2D, %arg(Urho3D::TileMap2D *), TileMap, GetTileMap);
%csattribute(Urho3D::TileMapLayer2D, %arg(Urho3D::TmxLayer2D *), TmxLayer, GetTmxLayer);
%csattribute(Urho3D::TileMapLayer2D, %arg(int), DrawOrder, GetDrawOrder, SetDrawOrder);
%csattribute(Urho3D::TileMapLayer2D, %arg(bool), IsVisible, IsVisible, SetVisible);
%csattribute(Urho3D::TileMapLayer2D, %arg(Urho3D::TileMapLayerType2D), LayerType, GetLayerType);
%csattribute(Urho3D::TileMapLayer2D, %arg(int), Width, GetWidth);
%csattribute(Urho3D::TileMapLayer2D, %arg(int), Height, GetHeight);
%csattribute(Urho3D::TileMapLayer2D, %arg(unsigned int), NumObjects, GetNumObjects);
%csattribute(Urho3D::TileMapLayer2D, %arg(Urho3D::Node *), ImageNode, GetImageNode);
%csattribute(Urho3D::TmxLayer2D, %arg(Urho3D::TmxFile2D *), TmxFile, GetTmxFile);
%csattribute(Urho3D::TmxLayer2D, %arg(ea::string), Name, GetName);
%csattribute(Urho3D::TmxLayer2D, %arg(int), Width, GetWidth);
%csattribute(Urho3D::TmxLayer2D, %arg(int), Height, GetHeight);
%csattribute(Urho3D::TmxLayer2D, %arg(bool), IsVisible, IsVisible);
%csattribute(Urho3D::TmxObjectGroup2D, %arg(unsigned int), NumObjects, GetNumObjects);
%csattribute(Urho3D::TmxImageLayer2D, %arg(Urho3D::Vector2), Position, GetPosition);
%csattribute(Urho3D::TmxImageLayer2D, %arg(ea::string), Source, GetSource);
%csattribute(Urho3D::TmxImageLayer2D, %arg(Urho3D::Sprite2D *), Sprite, GetSprite);
%csattribute(Urho3D::TmxFile2D, %arg(Urho3D::TileMapInfo2D), Info, GetInfo);
%csattribute(Urho3D::TmxFile2D, %arg(unsigned int), NumLayers, GetNumLayers);
%csattribute(Urho3D::TmxFile2D, %arg(float), SpriteTextureEdgeOffset, GetSpriteTextureEdgeOffset, SetSpriteTextureEdgeOffset);
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
} %}
