#pragma once
#include "CollisionShape.h"

namespace Urho3D {



    class Geometry;
    class StringHash;



    class URHO3D_API CollisionShape_Box : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Box, CollisionShape);

    public:
        CollisionShape_Box(Context* context);
        virtual ~CollisionShape_Box();

        static void RegisterObject(Context* context);

        /// Set the size of the box
        void SetSize(const Vector3& size) { size_ = size; MarkDirty(true); }

        /// Get the size of the box
        Vector3 GetSize() const { return size_; }

    protected:
        Vector3 size_ = Vector3::ONE;

        virtual bool buildNewtonCollision() override;

    };

    class URHO3D_API CollisionShape_Sphere : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Sphere, CollisionShape);

    public:
        CollisionShape_Sphere(Context* context);
        virtual ~CollisionShape_Sphere();

        static void RegisterObject(Context* context);


        void SetRadius(float radius) { radius_ = radius; MarkDirty(true); }

        float GetRadius() const { return radius_; }

    protected:
        float radius_ = 0.5f;

        virtual bool buildNewtonCollision() override;

    };

    class URHO3D_API CollisionShape_Capsule : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Capsule, CollisionShape);

    public:
        CollisionShape_Capsule(Context* context);
        virtual ~CollisionShape_Capsule();

        static void RegisterObject(Context* context);

        void SetLength(float length) { length_ = length; MarkDirty(true); }
        float GetLength() const { return length_; }

        void SetRadius1(float radius) { radius1_ = radius; MarkDirty(true); }
        float GetRadius1() const { return radius1_; }


        void SetRadius2(float radius) { radius2_ = radius; MarkDirty(true); }
        float GetRadius2() const { return radius2_; }

        /// Set both radi
        void SetRadius(float radius) { SetRadius1(radius); SetRadius2(radius); }

    protected:
        float length_ = 1.0f;
        float radius1_ = 0.5f;
        float radius2_ = 0.5f;

        virtual bool buildNewtonCollision() override;

    };

    class URHO3D_API CollisionShape_Cone : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Cone, CollisionShape);

    public:
        CollisionShape_Cone(Context* context);
        virtual ~CollisionShape_Cone();

        static void RegisterObject(Context* context);

        void SetRadius(float radius) { radius_ = radius; MarkDirty(true); }
        float GetRadius() const { return radius_; }

        void SetLength(float length) { length_ = length; MarkDirty(true); }
        float GetLength() const { return length_; }

    protected:
        float length_ = 1.0f;
        float radius_ = 0.5f;


        virtual bool buildNewtonCollision() override;

    };


    class URHO3D_API CollisionShape_Cylinder : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Cylinder, CollisionShape);

    public:
        CollisionShape_Cylinder(Context* context);
        virtual ~CollisionShape_Cylinder();

        static void RegisterObject(Context* context);

        /// Set Radius in 1st dimension
        void SetRadius1(float radius) { radius1_ = radius; MarkDirty(true); }

        float GetRadius1() const { return radius1_; }

        /// Set Radius in 2nd dimension
        void SetRadius2(float radius) { radius2_ = radius; MarkDirty(true); }

        float GetRadius2() const { return radius2_; }

        /// Set both radi
        void SetRadius(float radius) { SetRadius1(radius); SetRadius2(radius); }

        /// set the length
        void SetLength(float length) { length_ = length; MarkDirty(true); }

        float GetLength() const { return length_; }

    protected:
        float radius1_ = 0.5f;
        float radius2_ = 0.5f;
        float length_ = 1.0f;

        virtual bool buildNewtonCollision() override;

    };

    class URHO3D_API CollisionShape_ChamferCylinder : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_ChamferCylinder, CollisionShape);

    public:
        CollisionShape_ChamferCylinder(Context* context);
        virtual ~CollisionShape_ChamferCylinder();

        static void RegisterObject(Context* context);

        /// Set Radius in 1st dimension
        void SetRadius(float radius) { radius_ = radius; MarkDirty(true); }
        float GetRadius() const { return radius_; }

        /// set the length
        void SetLength(float length) { length_ = length; MarkDirty(true); }

        float GetLength() const { return length_; }

    protected:
        float radius_ = 0.5f;
        float length_ = 1.0f;

        virtual bool buildNewtonCollision() override;

    };



    class URHO3D_API CollisionShape_Geometry : public CollisionShape {

        URHO3D_OBJECT(CollisionShape_Geometry, CollisionShape);

    public:
        CollisionShape_Geometry(Context* context);
        virtual ~CollisionShape_Geometry();

        static void RegisterObject(Context* context);

        /// Set model to create geometry from
        void SetModel(Model* model) { model_ = model; MarkDirty(); }

        Model* GetModel() const { return model_; }

        ResourceRef GetModelResourceRef() const;
        void SetModelByResourceRef(const ResourceRef& ref);

        void SetModelLodLevel(int lod) { modelLodLevel_ = lod; MarkDirty(); }

        int GetModelLodLevel() const { return modelLodLevel_; }

        /// Set the tolerancing for hull creation.
        void SetHullTolerance(float tolerance = 0.0f) { hullTolerance_ = 0.0f; MarkDirty(); }

        float GetHullTolerance() const { return hullTolerance_; }

    protected:
        /// optional Model reference
        WeakPtr<Model> model_;
        /// lod level
        unsigned modelLodLevel_ = 0;
        /// model geometry index to use
        unsigned modelGeomIndx_ = 0;

        /// Hulling tolerance
        float hullTolerance_ = 0.0f;

        virtual bool buildNewtonCollision() override;

        void autoSetModel();

        ///forms newtonMesh_ from model geometry for later use.
        bool resolveOrCreateTriangleMeshFromModel();

        NewtonMeshObject* getCreateTriangleMesh(Geometry* geom, StringHash meshkey);


        virtual void OnNodeSet(Node* node) override;

    };

    class URHO3D_API CollisionShape_ConvexHullCompound : public CollisionShape_Geometry {

        URHO3D_OBJECT(CollisionShape_ConvexHullCompound, CollisionShape_Geometry);

    public:
        CollisionShape_ConvexHullCompound(Context* context);
        virtual ~CollisionShape_ConvexHullCompound();

        static void RegisterObject(Context* context);

    protected:

        virtual bool buildNewtonCollision() override;


    };

    
    class URHO3D_API CollisionShape_ConvexDecompositionCompound : public CollisionShape_Geometry {

        URHO3D_OBJECT(CollisionShape_ConvexDecompositionCompound, CollisionShape_Geometry);

    public:
        CollisionShape_ConvexDecompositionCompound(Context* context);
        virtual ~CollisionShape_ConvexDecompositionCompound();

        static void RegisterObject(Context* context);


    protected:
        NewtonMeshObject* meshDecomposition_ = nullptr;

        virtual bool buildNewtonCollision() override;

    };

    class URHO3D_API CollisionShape_ConvexHull : public CollisionShape_Geometry {

        URHO3D_OBJECT(CollisionShape_ConvexHull, CollisionShape_Geometry);

    public:
        CollisionShape_ConvexHull(Context* context);
        virtual ~CollisionShape_ConvexHull();

        static void RegisterObject(Context* context);


    protected:

        virtual bool buildNewtonCollision() override;

    };


    ///Collision that matches goemetry mesh data - rigid body using this shape must have zero mass.
    class URHO3D_API CollisionShape_TreeCollision : public CollisionShape_Geometry {

        URHO3D_OBJECT(CollisionShape_TreeCollision, CollisionShape_Geometry);

    public:
        CollisionShape_TreeCollision(Context* context);
        virtual ~CollisionShape_TreeCollision();

        static void RegisterObject(Context* context);


    protected:

        virtual bool buildNewtonCollision() override;


    };






    class URHO3D_API CollisionShape_HeightmapTerrain : public CollisionShape_Geometry {

        URHO3D_OBJECT(CollisionShape_HeightmapTerrain, CollisionShape_Geometry);

    public:
        CollisionShape_HeightmapTerrain(Context* context);
        virtual ~CollisionShape_HeightmapTerrain();

        static void RegisterObject(Context* context);



    protected:


        virtual bool buildNewtonCollision() override;

    };









}

