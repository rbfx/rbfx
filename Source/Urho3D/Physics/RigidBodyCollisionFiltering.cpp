#include "RigidBody.h"
#include "IO/Log.h"
#include "PhysicsWorld.h"

namespace Urho3D
{

    void RigidBody::SetCollisionLayer(unsigned layer)
    {
        if (layer >= sizeof(unsigned) * 8)
            URHO3D_LOGWARNING("Collision Layer To Large");

        collisionLayer_ = layer;
    }

    void RigidBody::SetCollisionLayerMask(unsigned mask)
    {
        collisionLayerMask_ = mask;
    }

    void RigidBody::SetCollidableLayer(unsigned layer)
    {
        if (layer >= sizeof(unsigned) * 8)
            URHO3D_LOGWARNING("Collision Layer To Large");

        collisionLayerMask_ |= CollisionLayerAsBit(layer);
    }

    void RigidBody::UnSetCollidableLayer(unsigned layer)
    {
        if (layer >= sizeof(unsigned) * 8)
            URHO3D_LOGWARNING("Collision Layer To Large");
        collisionLayerMask_ &= ~CollisionLayerAsBit(layer);
    }

    void RigidBody::SetCollisionOverride(RigidBody* otherBody, bool enableCollisions)
    {
        SetCollisionOverride(otherBody->GetID(), enableCollisions);
    }

    void RigidBody::SetCollisionOverride(unsigned otherBodyId, bool enableCollisions)
    {
        if (collisionExceptions_.Contains(StringHash(otherBodyId)))
        {
            collisionExceptions_[StringHash(otherBodyId)] = enableCollisions;
        }
        else
        {
            collisionExceptions_.Insert(Pair<StringHash, Variant>(StringHash(otherBodyId), Variant( enableCollisions )));
        }
    }

    void RigidBody::RemoveCollisionOverride(RigidBody* otherBody)
    {
        RemoveCollisionOverride(otherBody->GetID());
    }

    void RigidBody::RemoveCollisionOverride(unsigned otherBodyId)
    {
        collisionExceptions_.Erase(StringHash(otherBodyId));
    }

    void RigidBody::GetCollisionExceptions(PODVector<RigidBodyCollisionExceptionEntry>& exceptions)
    {
        for (auto pair : collisionExceptions_)
        {
            RigidBodyCollisionExceptionEntry entry;
            entry.enableCollisions_ = pair.second_.GetBool();
            entry.rigidBodyComponentId_ = pair.first_.Value();
            exceptions.Insert(0, entry);
        }
    }
    void RigidBody::SetNoCollideOverride(bool noCollide)
    {
        noCollideOverride_ = noCollide;
    }

    bool RigidBody::CanCollideWith(RigidBody* otherBody)
    {
        bool canCollide = true;







        //first check excpections because they have highest priority.
        bool exceptionsSpecified = false;
        if (noCollideOverride_)
        {
            canCollide = false;
            exceptionsSpecified = true;
        }
        if (otherBody->noCollideOverride_)
        {
            canCollide = false;
            exceptionsSpecified = true;
        }
        if (collisionExceptions_.Contains(StringHash(otherBody->GetID())))
        {
            canCollide &= collisionExceptions_[StringHash(otherBody->GetID())].GetBool();
            exceptionsSpecified = true;
        }
        if (otherBody->collisionExceptions_.Contains(StringHash(GetID())))
        {
            canCollide &= otherBody->collisionExceptions_[StringHash(GetID())].GetBool();
            exceptionsSpecified = true;
        }



        if (exceptionsSpecified)
            return canCollide;


        //now check collision masks/layers
        canCollide &= bool(collisionLayerMask_ & CollisionLayerAsBit(otherBody->collisionLayer_));
        canCollide &= bool(otherBody->collisionLayerMask_ & CollisionLayerAsBit(collisionLayer_));

        return canCollide;

    }



}

