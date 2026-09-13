// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/Context.h"
#include "Urho3D/IO/Log.h"
#include "Urho3D/Physics2D/CollisionShape2D.h"
#include "Urho3D/Physics2D/PhysicsUtils2D.h"
#include "Urho3D/Physics2D/RigidBody2D.h"
#include "Urho3D/Scene/Node.h"
#include "Urho3D/Scene/Scene.h"

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

CollisionShape2D::CollisionShape2D(Context* context) :
    Component(context),
    fixture_(nullptr),
    cachedWorldScale_(Vector3::ONE)
{

}

CollisionShape2D::~CollisionShape2D()
{
    if (rigidBody_)
        rigidBody_->RemoveCollisionShape2D(this);

    ReleaseFixture();
}

void CollisionShape2D::RegisterObject(Context* context)
{
    URHO3D_ACCESSOR_ATTRIBUTE("Trigger", IsTrigger, SetTrigger, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Category Bits", GetCategoryBits, SetCategoryBits, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Mask Bits", GetMaskBits, SetMaskBits, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Group Index", GetGroupIndex, SetGroupIndex, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Density", GetDensity, SetDensity, float, 0.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Friction", GetFriction, SetFriction, float, 0.2f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Restitution", GetRestitution, SetRestitution, float, 0.0f, AM_DEFAULT);
}

void CollisionShape2D::OnSetEnabled()
{
    if (IsEnabledEffective())
    {
        CreateFixture();
        if (rigidBody_)
            rigidBody_->AddCollisionShape2D(this);
    }
    else
    {
        if (rigidBody_)
            rigidBody_->RemoveCollisionShape2D(this);
        ReleaseFixture();
    }
}

void CollisionShape2D::SetTrigger(bool trigger)
{
    if (fixtureDef_.isSensor == trigger)
        return;

    fixtureDef_.isSensor = trigger;

    if (fixture_)
        fixture_->SetSensor(trigger);
}

void CollisionShape2D::SetCategoryBits(int categoryBits)
{
    if (fixtureDef_.filter.categoryBits == categoryBits)
        return;

    fixtureDef_.filter.categoryBits = (uint16)categoryBits;

    if (fixture_)
        fixture_->SetFilterData(fixtureDef_.filter);
}

void CollisionShape2D::SetMaskBits(int maskBits)
{
    if (fixtureDef_.filter.maskBits == maskBits)
        return;

    fixtureDef_.filter.maskBits = (uint16)maskBits;

    if (fixture_)
        fixture_->SetFilterData(fixtureDef_.filter);
}

void CollisionShape2D::SetGroupIndex(int groupIndex)
{
    if (fixtureDef_.filter.groupIndex == groupIndex)
        return;

    fixtureDef_.filter.groupIndex = (int16)groupIndex;

    if (fixture_)
        fixture_->SetFilterData(fixtureDef_.filter);
}

void CollisionShape2D::SetDensity(float density)
{
    if (fixtureDef_.density == density)
        return;

    fixtureDef_.density = density;

    if (fixture_)
    {
        // This will not automatically adjust the mass of the body
        fixture_->SetDensity(density);

        if (rigidBody_->GetUseFixtureMass())
            rigidBody_->GetBody()->ResetMassData();
    }
}

void CollisionShape2D::SetFriction(float friction)
{
    if (fixtureDef_.friction == friction)
        return;

    fixtureDef_.friction = friction;

    if (fixture_)
    {
        // This will not change the friction of existing contacts
        fixture_->SetFriction(friction);

        b2ContactEdge* contractEdge = rigidBody_->GetBody()->GetContactList();
        while (contractEdge)
        {
            b2Contact* contact = contractEdge->contact;
            if (contact->GetFixtureA() == fixture_ || contact->GetFixtureB() == fixture_)
                contractEdge->contact->ResetFriction();

            contractEdge = contractEdge->next;
        }
    }
}

void CollisionShape2D::SetRestitution(float restitution)
{
    if (fixtureDef_.restitution == restitution)
        return;

    fixtureDef_.restitution = restitution;

    if (fixture_)
    {
        // This will not change the restitution of existing contacts
        fixture_->SetRestitution(restitution);

        b2ContactEdge* contractEdge = rigidBody_->GetBody()->GetContactList();
        while (contractEdge)
        {
            b2Contact* contact = contractEdge->contact;
            if (contact->GetFixtureA() == fixture_ || contact->GetFixtureB() == fixture_)
                contractEdge->contact->ResetRestitution();

            contractEdge = contractEdge->next;
        }
    }
}

void CollisionShape2D::CreateFixture()
{
    if (fixture_)
        return;

    if (!fixtureDef_.shape)
        return;

    if (!rigidBody_)
    {
        rigidBody_ = node_->GetComponent<RigidBody2D>(); // RigidBody2D can be created after CollisionShape2D
        if (!rigidBody_)
            return;
    }

    b2Body* body = rigidBody_->GetBody();
    if (!body)
        return;

    // Chain shape must have atleast two vertices before creating fixture
    if (fixtureDef_.shape->m_type != b2Shape::e_chain || static_cast<const b2ChainShape*>(fixtureDef_.shape)->m_count >= 2)
    {
        b2MassData massData;
        body->GetMassData(&massData);
        fixture_ = body->CreateFixture(&fixtureDef_);
        if (!rigidBody_->GetUseFixtureMass()) // Workaround for resetting mass in CreateFixture().
            body->SetMassData(&massData);
        fixture_->SetUserData(this);
    }
}

void CollisionShape2D::ReleaseFixture()
{
    if (!fixture_)
        return;

    if (!rigidBody_)
        return;

    b2Body* body = rigidBody_->GetBody();
    if (!body)
        return;

    b2MassData massData;
    body->GetMassData(&massData);
    body->DestroyFixture(fixture_);
    if (!rigidBody_->GetUseFixtureMass()) // Workaround for resetting mass in DestroyFixture().
        body->SetMassData(&massData);
    fixture_ = nullptr;
}

float CollisionShape2D::GetMass() const
{
    if (!fixture_)
        return 0.0f;

    b2MassData massData;
    fixture_->GetMassData(&massData);

    return massData.mass;
}

float CollisionShape2D::GetInertia() const
{
    if (!fixture_)
        return 0.0f;

    b2MassData massData;
    fixture_->GetMassData(&massData);

    return massData.I;
}

Vector2 CollisionShape2D::GetMassCenter() const
{
    if (!fixture_)
        return Vector2::ZERO;

    b2MassData massData;
    fixture_->GetMassData(&massData);

    return ToVector2(massData.center);
}

void CollisionShape2D::OnNodeSet(Node* previousNode, Node* currentNode)
{
    if (node_)
    {
        node_->AddListener(this);
        rigidBody_ = node_->GetComponent<RigidBody2D>();
        if (rigidBody_)
        {
            CreateFixture();
            rigidBody_->AddCollisionShape2D(this);
        }
    }
}

void CollisionShape2D::OnMarkedDirty(Node* node)
{
    // Use signed world scale to allow flipping of sprites by negative scale to work properly in regard to the collision shape
    Vector3 newWorldScale = node_->GetSignedWorldScale();

    Vector3 delta = newWorldScale - cachedWorldScale_;
    if (delta.DotProduct(delta) < 0.01f)
        return;

    // Physics operations are not safe from worker threads
    Scene* scene = GetScene();
    if (scene && scene->IsThreadedUpdate())
    {
        scene->DelayedMarkedDirty(this);
        return;
    }

    cachedWorldScale_ = newWorldScale;

    ApplyNodeWorldScale();
}

}
