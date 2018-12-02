#pragma once
#include "../Scene/Component.h"


class dVehicleChassis;
class NewtonBody;

namespace Urho3D {

    class RigidBody;
    class CollisionShape;
    class PhysicsWorld;
    class VehicleTire;


    /// component for creations of specialized physics vehicle.
    class URHO3D_API PhysicsVehicle : public Component
    {
        URHO3D_OBJECT(PhysicsVehicle, Component);
    public:

        friend class PhysicsWorld;
        friend class VehicleTire;

        PhysicsVehicle(Context* context);
        virtual ~PhysicsVehicle();

        /// Register object factory.
        static void RegisterObject(Context* context);

        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

        void MarkDirty(bool dirty = true) { isDirty_ = dirty; }

        /// add a tire with suspension. 
        VehicleTire* AddTire(Matrix3x4 worldTransform);





    protected:

        virtual void OnNodeSet(Node* node) override;

        void reBuild();

        void applyTransforms();


        WeakPtr<RigidBody> rigidBody_;

        WeakPtr<PhysicsWorld> physicsWorld_;

        bool isDirty_ = true;

        dVehicleChassis* vehicleChassis_ = nullptr;

        Vector<VehicleTire*> tires_;

    };




}

