#include "PhysicsSamplesUtils.h"
#include "Urho3D/Physics/CollisionShapesDerived.h"

Node* SpawnSamplePhysicsSphere(Node* parentNode, const Vector3& worldPosition, float radius)
{
        Node* sphere1 = parentNode->CreateChild("SamplePhysicsSphere");
        Node* sphereVis = sphere1->CreateChild();
        sphereVis->SetScale(Vector3(radius, radius, radius)*2.0f);

        Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Sphere.mdl");
        Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");
        
        StaticModel* sphere1StMdl = sphereVis->CreateComponent<StaticModel>();
        sphere1StMdl->SetCastShadows(true);
        sphere1StMdl->SetModel(sphereMdl);
        sphere1StMdl->SetMaterial(sphereMat);

        RigidBody* s1RigBody = sphere1->CreateComponent<RigidBody>();

        CollisionShape_Sphere* s1ColShape = sphere1->CreateComponent<CollisionShape_Sphere>();
        s1ColShape->SetScaleFactor(Vector3(radius*2.0f, radius*2.0f, radius*2.0f));

        sphere1->SetWorldPosition(worldPosition);

        s1RigBody->SetMassScale(1.0f);

        

        return sphere1;
}

Node* SpawnSamplePhysicsCylinder(Node* parentNode, const Vector3& worldPosition, float radius, float height)
{
    Node* sphere1 = parentNode->CreateChild("SamplePhysicsCylinder");
    Node* sphereVis = sphere1->CreateChild();
    sphereVis->SetScale(Vector3(radius*2.0f, height, radius*2.0f));

    Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Cylinder.mdl");
    Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    StaticModel* sphere1StMdl = sphereVis->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);

    RigidBody* s1RigBody = sphere1->CreateComponent<RigidBody>();

    CollisionShape_Cylinder* s1ColShape = sphere1->CreateComponent<CollisionShape_Cylinder>();
    s1ColShape->SetRadius1(radius);
    s1ColShape->SetRadius2(radius);
    s1ColShape->SetLength(height);
    s1ColShape->SetRotationOffset(Quaternion(0, 0, 90));
    sphere1->SetWorldPosition(worldPosition);

    s1RigBody->SetMassScale(1.0f);


    return sphere1;
}

Node* SpawnSamplePhysicsChamferCylinder(Node* parentNode, const Vector3& worldPosition, float radius, float height)
{
    Node* sphere1 = parentNode->CreateChild("SamplePhysicsCylinder");
    Node* sphereVis = sphere1->CreateChild();
    sphereVis->SetScale(Vector3(radius*2.0f, height, radius*2.0f));

    Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Cylinder.mdl");
    Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    StaticModel* sphere1StMdl = sphereVis->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);

    RigidBody* s1RigBody = sphere1->CreateComponent<RigidBody>();

    CollisionShape_ChamferCylinder* s1ColShape = sphere1->CreateComponent<CollisionShape_ChamferCylinder>();
    s1ColShape->SetRadius(radius);
    s1ColShape->SetLength(height);
    s1ColShape->SetRotationOffset(Quaternion(0, 0, 90));
    sphere1->SetWorldPosition(worldPosition);

    s1RigBody->SetMassScale(1.0f);


    return sphere1;
}

Node* SpawnSamplePhysicsCapsule(Node* parentNode, const Vector3& worldPosition, float radius, float height)
{
    Node* sphere1 = parentNode->CreateChild("SamplePhysicsCylinder");
    Node* sphereVis = sphere1->CreateChild();
    sphereVis->SetScale(Vector3(radius*2.0f, height, radius*2.0f));

    Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Cylinder.mdl");
    Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    StaticModel* sphere1StMdl = sphereVis->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);

    RigidBody* s1RigBody = sphere1->CreateComponent<RigidBody>();

    CollisionShape_Capsule* s1ColShape = sphere1->CreateComponent<CollisionShape_Capsule>();
    s1ColShape->SetRadius1(radius);
    s1ColShape->SetRadius2(radius);
    s1ColShape->SetLength(height);
    s1ColShape->SetRotationOffset(Quaternion(0, 0, 90));
    sphere1->SetWorldPosition(worldPosition);

    s1RigBody->SetMassScale(1.0f);


    return sphere1;
}


Node* SpawnSamplePhysicsCone(Node* parentNode, const Vector3& worldPosition, float radius, float height)
{
    Node* sphere1 = parentNode->CreateChild("SamplePhysicsCone");
    Node* sphereVis = sphere1->CreateChild();
    sphereVis->SetScale(Vector3(radius*2.0f, height, radius*2.0f));

    Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Cone.mdl");
    Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    StaticModel* sphere1StMdl = sphereVis->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);

    RigidBody* s1RigBody = sphere1->CreateComponent<RigidBody>();

    CollisionShape_Cone* s1ColShape = sphere1->CreateComponent<CollisionShape_Cone>();
    s1ColShape->SetRadius(radius);
    s1ColShape->SetLength(height);
    s1ColShape->SetRotationOffset(Quaternion(0, 0, 90));
    sphere1->SetWorldPosition(worldPosition);

    s1RigBody->SetMassScale(1.0f);


    return sphere1;
}



Node* SpawnSamplePhysicsBox(Node* parentNode, const Vector3& worldPosition, const Vector3& size)
{
    Node* box = parentNode->CreateChild();
    Node* boxVis = box->CreateChild();
    boxVis->SetScale(size);

    Model* sphereMdl = parentNode->GSS<ResourceCache>()->GetResource<Model>("Models/Box.mdl");
    Material* sphereMat = parentNode->GSS<ResourceCache>()->GetResource<Material>("Materials/Stone.xml");

    StaticModel* sphere1StMdl = boxVis->CreateComponent<StaticModel>();
    sphere1StMdl->SetCastShadows(true);
    sphere1StMdl->SetModel(sphereMdl);
    sphere1StMdl->SetMaterial(sphereMat);


    RigidBody* s1RigBody = box->CreateComponent<RigidBody>();

    CollisionShape_Box* s1ColShape = box->CreateComponent<CollisionShape_Box>();
    s1ColShape->SetScaleFactor(size);

    box->SetWorldPosition(worldPosition);

    s1RigBody->SetMassScale(1.0f);


    return box;
}

