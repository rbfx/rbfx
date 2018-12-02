#pragma once
#include "../Resource/Resource.h"
#include "../Core/Context.h"
#include "../Core/Object.h"

namespace Urho3D {

    ///resource providing physics model data 
    class URHO3D_API PhysicsModel : public ResourceWithMetadata
    {
        URHO3D_OBJECT(PhysicsModel, ResourceWithMetadata);

    public:
        PhysicsModel(Context* context) : ResourceWithMetadata(context)
        {

        }
        virtual ~PhysicsModel() {}

        static void RegisterObject(Context* context) {
            context->RegisterFactory< PhysicsModel>();
        }

    protected:

        //friction, restitution, etc..
    };

}
