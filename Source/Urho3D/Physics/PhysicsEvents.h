// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"

namespace Urho3D
{

/// Physics world is about to be updated. There may be zero, one, or more physics steps coming.
URHO3D_EVENT(E_PHYSICSPREUPDATE, PhysicsPreUpdate)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
};

/// Physics world has been updated. There may have been zero, one, or more physics steps evaluated.
/// Overtime indicates the amount of non-simulated time after latest step.
URHO3D_EVENT(E_PHYSICSPOSTUPDATE, PhysicsPostUpdate)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
    URHO3D_PARAM(P_OVERTIME, Overtime);            // float
};

/// Physics world is about to be stepped.
URHO3D_EVENT(E_PHYSICSPRESTEP, PhysicsPreStep)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
    URHO3D_PARAM(P_NETWORKFRAME, NetworkFrame);    // unsigned
}

/// Physics world has been stepped.
URHO3D_EVENT(E_PHYSICSPOSTSTEP, PhysicsPostStep)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_TIMESTEP, TimeStep);            // float
}

/// Physics collision started. Global event sent by the PhysicsWorld.
URHO3D_EVENT(E_PHYSICSCOLLISIONSTART, PhysicsCollisionStart)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ongoing. Global event sent by the PhysicsWorld.
URHO3D_EVENT(E_PHYSICSCOLLISION, PhysicsCollision)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer. Maybe be nullptr for KinematicCharacterController
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Physics collision ended. Global event sent by the PhysicsWorld.
URHO3D_EVENT(E_PHYSICSCOLLISIONEND, PhysicsCollisionEnd)
{
    URHO3D_PARAM(P_WORLD, World);                  // PhysicsWorld pointer
    URHO3D_PARAM(P_NODEA, NodeA);                  // Node pointer
    URHO3D_PARAM(P_NODEB, NodeB);                  // Node pointer
    URHO3D_PARAM(P_BODYA, BodyA);                  // RigidBody pointer
    URHO3D_PARAM(P_BODYB, BodyB);                  // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
}

/// Node's physics collision started. Sent by scene nodes participating in a collision.
URHO3D_EVENT(E_NODECOLLISIONSTART, NodeCollisionStart)
{
    URHO3D_PARAM(P_BODY, Body);                    // RigidBody pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ongoing. Sent by scene nodes participating in a collision.
URHO3D_EVENT(E_NODECOLLISION, NodeCollision)
{
    URHO3D_PARAM(P_BODY, Body);                    // RigidBody pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
    URHO3D_PARAM(P_CONTACTS, Contacts);            // Buffer containing position (Vector3), normal (Vector3), distance (float), impulse (float) for each contact
}

/// Node's physics collision ended. Sent by scene nodes participating in a collision.
URHO3D_EVENT(E_NODECOLLISIONEND, NodeCollisionEnd)
{
    URHO3D_PARAM(P_BODY, Body);                    // RigidBody pointer
    URHO3D_PARAM(P_OTHERNODE, OtherNode);          // Node pointer
    URHO3D_PARAM(P_OTHERBODY, OtherBody);          // RigidBody pointer
    URHO3D_PARAM(P_TRIGGER, Trigger);              // bool
}

}
