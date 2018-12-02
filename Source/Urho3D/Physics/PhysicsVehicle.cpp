#include "PhysicsVehicle.h"
#include "Core/Context.h"
#include "PhysicsWorld.h"
#include "Scene/Component.h"
#include "Scene/Scene.h"
#include "CollisionShapesDerived.h"
#include "../dVehicle/dVehicleManager.h"
#include "../dVehicle/dVehicleChassis.h"
#include "RigidBody.h"
#include "UrhoNewtonConversions.h"
#include "Container/Ptr.h"
#include "Graphics/Model.h"
#include "Graphics/StaticModel.h"
#include "VehicleTire.h"
#include "NewtonDebugDrawing.h"
#include "Graphics/VisualDebugger.h"
#include "Graphics/DebugRenderer.h"
#include "IO/Log.h"

namespace Urho3D {





    PhysicsVehicle::PhysicsVehicle(Context* context) : Component(context)
    {

    }

    PhysicsVehicle::~PhysicsVehicle()
    {

    }

    void PhysicsVehicle::RegisterObject(Context* context)
    {
        context->RegisterFactory<PhysicsVehicle>(DEF_PHYSICS_CATEGORY.CString());
    }

    void PhysicsVehicle::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
    {
        Component::DrawDebugGeometry(debug, depthTest);

        for (VehicleTire* tire : tires_)
        {

            dMatrix matrix = tire->tireInterface_->GetGlobalMatrix();

            Matrix3x4 mat = Matrix3x4(NewtonToUrhoMat4(matrix));




            debugRenderOptions options;
            options.debug = debug;
            options.depthTest = depthTest;



            matrix = UrhoToNewton(mat);
            NewtonCollisionForEachPolygonDo(tire->tireInterface_->GetCollisionShape(), &matrix[0][0], NewtonDebug_ShowGeometryCollisionCallback, (void*)&options);


            debug->AddFrame(tire->node_->GetWorldTransform());
        }
    }

    VehicleTire* PhysicsVehicle::AddTire(Matrix3x4 worldTransform)
    {
        Node* tireNode = node_->CreateChild("Tire");
        VehicleTire* tire = node_->CreateComponent<VehicleTire>();

        MarkDirty();

        return tire;
    }

    void PhysicsVehicle::OnNodeSet(Node* node)
    {
        if (node)
        {

            ///auto create physics world
            physicsWorld_ = WeakPtr<PhysicsWorld>(GetScene()->GetOrCreateComponent<PhysicsWorld>());

            physicsWorld_->addVehicle(this);
            

        }
        else
        {
            if (physicsWorld_) {
                physicsWorld_->removeVehicle(this);
            }

        }
    }

    void PhysicsVehicle::reBuild()
    {
        if (vehicleChassis_)
        {
            physicsWorld_->vehicleManager_->DestroyController(vehicleChassis_);
            vehicleChassis_ = nullptr;
        }


        rigidBody_ = node_->GetOrCreateComponent<RigidBody>();
        NewtonBody* body = rigidBody_->GetNewtonBody();


        if (!body)
            return;


        Matrix3x4 worldTransform = physicsWorld_->SceneToPhysics_Domain(node_->GetWorldTransform());

        NewtonApplyForceAndTorque callback = NewtonBodyGetForceAndTorqueCallback(rigidBody_->GetNewtonBody());//use existing callback.
        vehicleChassis_ = physicsWorld_->vehicleManager_->CreateSingleBodyVehicle(body, UrhoToNewton(Matrix3x4(worldTransform.Translation(), worldTransform.Rotation(), 1.0f)), callback, 1.0f);


        //parse any tire components that are in child nodes
        PODVector<Node*> tireNodes;
        node_->GetChildrenWithComponent<VehicleTire>(tireNodes, false);

        tires_.Clear();

        for (Node* tireNode : tireNodes)
        {
            tires_.Insert(0, tireNode->GetComponent<VehicleTire>());
        }

        //bind the tire to the vehicle
        int i = 0;
        for (VehicleTire* tire : tires_)
        {
            tire->tireInterface_ = vehicleChassis_->AddTire(UrhoToNewton(tire->GetNode()->GetWorldTransform()), *tire->tireInfo_);
            i++;
        }

        vehicleChassis_->Finalize();


        isDirty_ = false;
    }

    void PhysicsVehicle::applyTransforms()
    {
        if (isDirty_)
            return;

        for (VehicleTire* tire : tires_)
        {
            Matrix3x4 worldTransform = physicsWorld_->PhysicsToScene_Domain(Matrix3x4(NewtonToUrhoMat4(tire->tireInterface_->GetGlobalMatrix())));

            tire->node_->SetWorldTransform(worldTransform.Translation(), worldTransform.Rotation());
        }
    }
}
