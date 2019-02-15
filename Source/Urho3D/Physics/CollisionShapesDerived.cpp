#include "CollisionShapesDerived.h"
#include "PhysicsWorld.h"
#include "RigidBody.h"
#include "NewtonMeshObject.h"

#include "../Core/Context.h"
#include "../Scene/Component.h"
#include "../Scene/Node.h"
#include "../Scene/Scene.h"
#include "../Graphics/Model.h"
#include "../Resource/Image.h"
#include "../IO/Log.h"

#include "Newton.h"
#include "dMatrix.h"

#include "UrhoNewtonConversions.h"
#include "Graphics/Geometry.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/GraphicsDefs.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/StaticModel.h"
#include "Scene/SceneEvents.h"
#include "CollisionShape.h"
#include "Graphics/Terrain.h"
#include "Container/ArrayPtr.h"
#include "Resource/ResourceCache.h"
#include "Graphics/TerrainPatch.h"
#include "Math/StringHash.h"


namespace Urho3D {

    CollisionShape_Box::CollisionShape_Box(Context* context) : CollisionShape(context)
    {

    }

    CollisionShape_Box::~CollisionShape_Box()
    {

    }

    void CollisionShape_Box::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Box>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, Vector3, Vector3::ONE, AM_DEFAULT);

    }

    bool CollisionShape_Box::buildNewtonCollision()
    {
        // get a newton collision object (note: the same NewtonCollision could be shared between multiple component so this is not nessecarily a unique pointer)
        newtonCollision_ = NewtonCreateBox(physicsWorld_->GetNewtonWorld(), size_.x_,
            size_.y_,
            size_.z_, 0, nullptr);

        return true;
    }







    CollisionShape_Sphere::CollisionShape_Sphere(Context* context) : CollisionShape(context)
    {

    }

    CollisionShape_Sphere::~CollisionShape_Sphere()
    {

    }

    void CollisionShape_Sphere::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Sphere>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.5f, AM_DEFAULT);
    }

    bool CollisionShape_Sphere::buildNewtonCollision()
{
        // get a newton collision object (note: the same NewtonCollision could be shared between multiple component so this is not nessecarily a unique pointer)
        newtonCollision_ = NewtonCreateSphere(physicsWorld_->GetNewtonWorld(), radius_, 0, nullptr);
        return true;
    }












    CollisionShape_Geometry::CollisionShape_Geometry(Context* context) : CollisionShape(context)
    {

    }

    CollisionShape_Geometry::~CollisionShape_Geometry()
    {

    }

    void CollisionShape_Geometry::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Geometry>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Model", GetModelResourceRef, SetModelByResourceRef, ResourceRef, ResourceRef(Model::GetTypeStatic()), AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Model Lod", GetModelLodLevel, SetModelLodLevel, int, 0, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Hull Tolerance", GetHullTolerance, SetHullTolerance, float, 0.0f, AM_DEFAULT);
        


    }

    void CollisionShape_Geometry::SetModelByResourceRef(const ResourceRef& ref)
    {
        auto* cache = GetSubsystem<ResourceCache>();
        SetModel(cache->GetResource<Model>(ref.name_));
    }

    bool CollisionShape_Geometry::resolveOrCreateTriangleMeshFromModel()
{
        if (!model_)
            return false;

        NewtonWorld* world = physicsWorld_->GetNewtonWorld();

        /// if the newton mesh is in cache already - return that.
        StringHash meshKey = PhysicsWorld::NewtonMeshKey(model_->GetName(), modelLodLevel_, "");
        NewtonMeshObject* cachedMesh = physicsWorld_->GetNewtonMesh(meshKey);
        if (cachedMesh)
        {
            newtonMesh_ = cachedMesh;
            return true;
        }

        Geometry* geo = model_->GetGeometry(modelGeomIndx_, modelLodLevel_);
        newtonMesh_ = getCreateTriangleMesh(geo, meshKey);
        if(!newtonMesh_)
        { 
            URHO3D_LOGWARNING("Unable To Create NewtonMesh For Model: " + model_->GetName());
            return false;
        }

        return true;

    }

    NewtonMeshObject* CollisionShape_Geometry::getCreateTriangleMesh(Geometry* geom, StringHash meshkey)
    {
        NewtonMeshObject* cachedMesh = physicsWorld_->GetNewtonMesh(meshkey);
        if (cachedMesh)
        {
            return cachedMesh;
        }


        const unsigned char* vertexData;
        const unsigned char* indexData;
        unsigned elementSize, indexSize;
        const PODVector<VertexElement>* elements;

        geom->GetRawData(vertexData, elementSize, indexData, indexSize, elements);

        bool hasPosition = VertexBuffer::HasElement(*elements, TYPE_VECTOR3, SEM_POSITION);

        if (vertexData && indexData && hasPosition)
        {
            unsigned vertexStart = geom->GetVertexStart();
            unsigned vertexCount = geom->GetVertexCount();
            unsigned indexStart = geom->GetIndexStart();
            unsigned indexCount = geom->GetIndexCount();

            unsigned positionOffset = VertexBuffer::GetElementOffset(*elements, TYPE_VECTOR3, SEM_POSITION);


            cachedMesh = physicsWorld_->GetCreateNewtonMesh(meshkey);
            NewtonMeshBeginBuild(cachedMesh->mesh);



            for (unsigned curIdx = indexStart; curIdx < indexStart + indexCount; curIdx += 3)
            {
                //get indexes
                unsigned i1, i2, i3;
                if (indexSize == 2) {
                    i1 = *reinterpret_cast<const unsigned short*>(indexData + curIdx * indexSize);
                    i2 = *reinterpret_cast<const unsigned short*>(indexData + (curIdx + 1) * indexSize);
                    i3 = *reinterpret_cast<const unsigned short*>(indexData + (curIdx + 2) * indexSize);
                }
                else if (indexSize == 4)
                {
                    i1 = *reinterpret_cast<const unsigned *>(indexData + curIdx * indexSize);
                    i2 = *reinterpret_cast<const unsigned *>(indexData + (curIdx + 1) * indexSize);
                    i3 = *reinterpret_cast<const unsigned *>(indexData + (curIdx + 2) * indexSize);
                }

                //lookup triangle using indexes.
                const Vector3& v1 = *reinterpret_cast<const Vector3*>(vertexData + i1 * elementSize + positionOffset);
                const Vector3& v2 = *reinterpret_cast<const Vector3*>(vertexData + i2 * elementSize + positionOffset);
                const Vector3& v3 = *reinterpret_cast<const Vector3*>(vertexData + i3 * elementSize + positionOffset);


                NewtonMeshBeginFace(cachedMesh->mesh);


                NewtonMeshAddPoint(cachedMesh->mesh, v1.x_, v1.y_, v1.z_);
                NewtonMeshAddPoint(cachedMesh->mesh, v2.x_, v2.y_, v2.z_);
                NewtonMeshAddPoint(cachedMesh->mesh, v3.x_, v3.y_, v3.z_);


                NewtonMeshEndFace(cachedMesh->mesh);
            }

            NewtonMeshEndBuild(cachedMesh->mesh);
        }

        return cachedMesh;
    }

    void CollisionShape_Geometry::OnNodeSet(Node* node)
    {
        CollisionShape::OnNodeSet(node);

        if (node)
            autoSetModel();
    }

    bool CollisionShape_Geometry::buildNewtonCollision()
{
        return resolveOrCreateTriangleMeshFromModel();
    }


    void CollisionShape_Geometry::autoSetModel()
    {
        StaticModel* stMdl = node_->GetComponent<StaticModel>();

        if (stMdl)
        {
            SetModel(stMdl->GetModel());
        }
    }

    Urho3D::ResourceRef CollisionShape_Geometry::GetModelResourceRef() const
    {
        return GetResourceRef(model_, Model::GetTypeStatic());
    }



    CollisionShape_ConvexHullCompound::CollisionShape_ConvexHullCompound(Context* context) : CollisionShape_Geometry(context)
    {
    }

    CollisionShape_ConvexHullCompound::~CollisionShape_ConvexHullCompound()
    {
    }

    void CollisionShape_ConvexHullCompound::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_ConvexHullCompound>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape_Geometry);
    }


    bool CollisionShape_ConvexHullCompound::buildNewtonCollision()
{
        NewtonWorld* world = physicsWorld_->GetNewtonWorld();

        if (!resolveOrCreateTriangleMeshFromModel())
            return false;

        newtonCollision_ = NewtonCreateCompoundCollisionFromMesh(world, newtonMesh_->mesh, hullTolerance_, 0, 0);

        return true;
    }




    CollisionShape_ConvexDecompositionCompound::CollisionShape_ConvexDecompositionCompound(Context* context) : CollisionShape_Geometry(context)
    {

    }



    CollisionShape_ConvexDecompositionCompound::~CollisionShape_ConvexDecompositionCompound()
    {

    }




    void CollisionShape_ConvexDecompositionCompound::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_ConvexDecompositionCompound>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape_Geometry);
    }

    bool CollisionShape_ConvexDecompositionCompound::buildNewtonCollision()
    {
        return CollisionShape_Geometry::buildNewtonCollision();

        //NewtonWorld* world = physicsWorld_->GetNewtonWorld();


        //String keyData = String(maxConcavity_) + String(backFaceDistanceFactor_) + String(maxCompounds_) + String(maxVertexPerHull_);

        ///// if the newton mesh is in cache already - return that.
        //StringHash meshKey = UrhoNewtonPhysicsWorld::NewtonMeshKey(model_->GetName(), modelLodLevel_, keyData);
        //NewtonMeshObject* cachedMesh = physicsWorld_->GetNewtonMesh(meshKey);
        //if (cachedMesh)
        //{
        //    meshDecomposition_ = cachedMesh;
        //}
        //else
        //{
        //    meshDecomposition_ = physicsWorld_->GetCreateNewtonMesh(meshKey);
        //    meshDecomposition_->mesh = NewtonMeshApproximateConvexDecomposition(newtonMesh_->mesh, maxConcavity_, backFaceDistanceFactor_, maxCompounds_, maxVertexPerHull_, nullptr, nullptr);

        //}

        //newtonCollision_ = NewtonCreateCompoundCollisionFromMesh(world, meshDecomposition_->mesh, hullTolerance_, 0, 0);
    }

    CollisionShape_ConvexHull::CollisionShape_ConvexHull(Context* context) : CollisionShape_Geometry(context)
    {

    }

    CollisionShape_ConvexHull::~CollisionShape_ConvexHull()
    {

    }

    void CollisionShape_ConvexHull::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_ConvexHull>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape_Geometry);
    }

    bool CollisionShape_ConvexHull::buildNewtonCollision()
    {
        if (!CollisionShape_Geometry::buildNewtonCollision())
            return false;

        NewtonWorld* world = physicsWorld_->GetNewtonWorld();

        newtonCollision_ = NewtonCreateConvexHullFromMesh(world, newtonMesh_->mesh, hullTolerance_, 0);

        dMatrix offsetMat;
        NewtonCollisionGetMatrix(newtonCollision_, &offsetMat[0][0]);

        NewtonCollisionSetMatrix(newtonCollision_, &dGetIdentityMatrix()[0][0]);


        //offset by model geometry center (#todo this may be a quirk of newton - CompoundHulling does not need this.)
        position_ += NewtonToUrhoVec3(offsetMat.m_posit);//model_->GetGeometryCenter(modelLodLevel_);
        return true;
    }






    CollisionShape_Cylinder::CollisionShape_Cylinder(Context* context) : CollisionShape(context)
    {
    }

    CollisionShape_Cylinder::~CollisionShape_Cylinder()
    {

    }

    void CollisionShape_Cylinder::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Cylinder>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius 1", GetRadius1, SetRadius1, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius 2", GetRadius2, SetRadius2, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, 1.0f, AM_DEFAULT);
    }

    bool CollisionShape_Cylinder::buildNewtonCollision()
    {
        // get a newton collision object (note: the same NewtonCollision could be shared between multiple component so this is not nessecarily a unique pointer)
        newtonCollision_ = NewtonCreateCylinder(physicsWorld_->GetNewtonWorld(), radius1_, radius2_, length_, 0, nullptr);

        return true;
    }


    CollisionShape_ChamferCylinder::CollisionShape_ChamferCylinder(Context* context) : CollisionShape(context)
    {

    }

    CollisionShape_ChamferCylinder::~CollisionShape_ChamferCylinder()
    {

    }

    void CollisionShape_ChamferCylinder::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_ChamferCylinder>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, 1.0f, AM_DEFAULT);
    }

    bool CollisionShape_ChamferCylinder::buildNewtonCollision()
    {
        // get a newton collision object (note: the same NewtonCollision could be shared between multiple component so this is not nessecarily a unique pointer)
        newtonCollision_ = NewtonCreateChamferCylinder(physicsWorld_->GetNewtonWorld(), radius_, length_, 0, nullptr);

        return true;
    }




    CollisionShape_Capsule::CollisionShape_Capsule(Context* context) : CollisionShape(context)
    {
    }

    CollisionShape_Capsule::~CollisionShape_Capsule()
    {
    }

    void CollisionShape_Capsule::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Capsule>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius 1", GetRadius1, SetRadius1, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius 2", GetRadius2, SetRadius2, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, 1.0f, AM_DEFAULT);
    }

    bool CollisionShape_Capsule::buildNewtonCollision()
{
        newtonCollision_ = NewtonCreateCapsule(physicsWorld_->GetNewtonWorld(), radius1_, radius2_, length_, 0, nullptr);
        return true;
    }

    CollisionShape_Cone::CollisionShape_Cone(Context* context) : CollisionShape(context)
    {
    }

    CollisionShape_Cone::~CollisionShape_Cone()
    {
    }

    void CollisionShape_Cone::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_Cone>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape);
        URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.5f, AM_DEFAULT);
        URHO3D_ACCESSOR_ATTRIBUTE("Length", GetLength, SetLength, float, 0.5f, AM_DEFAULT);

    }

    bool CollisionShape_Cone::buildNewtonCollision()
{
        newtonCollision_ = NewtonCreateCone(physicsWorld_->GetNewtonWorld(), radius_, length_, 0, nullptr);
        return true;
    }















    CollisionShape_HeightmapTerrain::CollisionShape_HeightmapTerrain(Context* context) : CollisionShape_Geometry(context)
    {
        drawPhysicsDebugCollisionGeometry_ = false;//default newton debug lines for geometry is too many.
    }

    CollisionShape_HeightmapTerrain::~CollisionShape_HeightmapTerrain()
    {

    }

    void CollisionShape_HeightmapTerrain::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_HeightmapTerrain>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape_Geometry);

    }

    bool CollisionShape_HeightmapTerrain::buildNewtonCollision()
    {



        //find a terrain component
        Terrain* terrainComponent = node_->GetComponent<Terrain>();
        if (terrainComponent)
        {

            int size = terrainComponent->GetHeightMap()->GetHeight();
#ifndef _NEWTON_USE_DOUBLE
            SharedArrayPtr<float> heightData = terrainComponent->GetHeightData();
#else
            SharedArrayPtr<float> heightDataFloat = terrainComponent->GetHeightData();

            dFloat* heightData = new dFloat[size*size];
            for (int i = 0; i < size*size; i++) {
                heightData[i] = heightDataFloat[i];
            }

#endif



            char* const attibutes = new char[size * size];
            memset(attibutes, 0, size * size * sizeof(char));

            Vector3 spacing = terrainComponent->GetSpacing();
            newtonCollision_ = NewtonCreateHeightFieldCollision(physicsWorld_->GetNewtonWorld(), size, size, 0, 0, heightData, attibutes, 1.0f, spacing.x_, spacing.z_, 0);

            delete[] attibutes;


            ////set the internal offset correction to match where HeightmapTerrain renders
            position_ = -Vector3(float(size*spacing.x_)*0.5f - spacing.x_*0.5f, 0, float(size*spacing.z_)*0.5f - spacing.z_*0.5f);
            return true;
        }
        else
            return false;
    }

    CollisionShape_TreeCollision::CollisionShape_TreeCollision(Context* context) : CollisionShape_Geometry(context)
    {

    }

    CollisionShape_TreeCollision::~CollisionShape_TreeCollision()
    {

    }

    void CollisionShape_TreeCollision::RegisterObject(Context* context)
    {
        context->RegisterFactory<CollisionShape_TreeCollision>(DEF_PHYSICS_CATEGORY.CString());
        URHO3D_COPY_BASE_ATTRIBUTES(CollisionShape_Geometry);
    }

    bool CollisionShape_TreeCollision::buildNewtonCollision()
    {
        if (!CollisionShape_Geometry::buildNewtonCollision())
            return false;

        NewtonWorld* world = physicsWorld_->GetNewtonWorld();

        //newtonCollision_ = NewtonCreateConvexHullFromMesh(world, newtonMesh_->mesh, hullTolerance_, 0);
        newtonCollision_ = NewtonCreateTreeCollisionFromMesh(world, newtonMesh_->mesh, 0);
        return true;
    }



}

