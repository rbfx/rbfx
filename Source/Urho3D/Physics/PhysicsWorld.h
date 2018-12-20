#pragma once



#include "../Scene/Component.h"
#include "Newton.h"
#include "../Container/Vector.h"
#include "CollisionShape.h"

class NewtonWorld;
class dMatrix;
class dCustomJoint;
class dVehicleManager;

namespace Urho3D
{
    class Component;
    class CollisionShape;
    class RigidBody;
    class Constraint;
    class Sphere;
    class Ray;
    class BoundingBox;
    class NewtonMeshObject;
    class Context;

    static const Vector3 DEF_GRAVITY = Vector3(0, -9.81, 0);
    static const String DEF_PHYSICS_CATEGORY = "Physics";
    static const int DEF_PHYSICS_MAX_CONTACT_POINTS = 512;//maximum number of contacts per contact entry.


    class URHO3D_API  RigidBodyContactEntry : public Object
    {
        URHO3D_OBJECT(RigidBodyContactEntry, Object);
    public:

        friend class PhysicsWorld;

        RigidBodyContactEntry(Context* context);
        virtual ~RigidBodyContactEntry() override;

        /// Register object factory.
        static void RegisterObject(Context* context);

        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);


        bool expired_ = true;

        WeakPtr<RigidBody> body0 = nullptr;
        WeakPtr<RigidBody> body1 = nullptr;
        CollisionShape* shapes0[DEF_PHYSICS_MAX_CONTACT_POINTS];
        CollisionShape* shapes1[DEF_PHYSICS_MAX_CONTACT_POINTS];


        int numContacts = 0;

        Vector3 contactForces[DEF_PHYSICS_MAX_CONTACT_POINTS];    //net forces.
        Vector3 contactPositions[DEF_PHYSICS_MAX_CONTACT_POINTS]; //global space
        Vector3 contactNormals[DEF_PHYSICS_MAX_CONTACT_POINTS];   //normal relative to body0
        Vector3 contactTangent0[DEF_PHYSICS_MAX_CONTACT_POINTS];  //tangent force in the 1st dimention.
        Vector3 contactTangent1[DEF_PHYSICS_MAX_CONTACT_POINTS];  //tangent force in the 2nd dimention.

    protected:
        bool wakeFlag_ = false;
        bool wakeFlagPrev_ = false;

    };

    struct PhysicsRayCastIntersection {
        NewtonBody* body = nullptr;
        float rayIntersectParameter = -1.0f;

        RigidBody* rigBody = nullptr;
        Vector3 rayIntersectWorldPosition;
        Vector3 rayIntersectWorldNormal;
        float rayDistance = -1.0f;
        Vector3 rayOriginWorld;
    };
    inline bool PhysicsRayCastIntersectionCompare(PhysicsRayCastIntersection& intersect1, PhysicsRayCastIntersection& intersect2) {
        return (intersect1.rayIntersectParameter < intersect2.rayIntersectParameter);
    }
    struct PhysicsRayCastUserData {
        PODVector<PhysicsRayCastIntersection> intersections;
    };


    class URHO3D_API PhysicsWorld : public Component
    {
        URHO3D_OBJECT(PhysicsWorld, Component);
    public:

        friend class CollisionShape;
        friend class CollisionShape_Geometry;
        friend class CollisionShape_ConvexDecompositionCompound;
        friend class NewtonCollisionShape_SceneCollision;
        friend class RigidBody;
        friend class Constraint;

        /// Construct.
        PhysicsWorld(Context* context);
        /// Destruct. Free the rigid body and geometries.
        ~PhysicsWorld() override;
        /// Register object factory.
        static void RegisterObject(Context* context);

        /// Return the internal Newton world.
        NewtonWorld* GetNewtonWorld() { return newtonWorld_; }


        /// Saves the NewtonWorld to a serializable newton file.
        void SerializeNewtonWorld(String fileName);

        /// Return a name for the currently used speed plugin (SSE, AVX, AVX2)
        String GetSolverPluginName();



        bool RigidBodyContainsPoint(RigidBody* rigidBody, const Vector3&worldPoint);
        /// Return rigid bodies by a ray query. bodies are returned in order from closest to farthest along the ray.
        void RayCast(
            PODVector<PhysicsRayCastIntersection>& intersections,
            const Ray& ray, float maxDistance = M_LARGE_VALUE,
            unsigned maxIntersections = M_MAX_UNSIGNED,
            unsigned collisionMask = M_MAX_UNSIGNED);
        /// Return rigid bodies by a ray query.
        void RayCast(
            PODVector<PhysicsRayCastIntersection>& intersections,
            const Vector3& pointOrigin, const Vector3& pointDestination,
            unsigned maxIntersections = M_MAX_UNSIGNED,
            unsigned collisionMask = M_MAX_UNSIGNED);


        /// Return rigid bodies by a sphere query.
        void GetRigidBodies(PODVector<RigidBody*>& result, const Sphere& sphere, unsigned collisionMask = M_MAX_UNSIGNED);
        /// Return rigid bodies by a box query.
        void GetRigidBodies(PODVector<RigidBody*>& result, const BoundingBox& box, unsigned collisionMask = M_MAX_UNSIGNED);
        /// Return rigid bodies by contact test with the specified body.
        void GetRigidBodies(PODVector<RigidBody*>& result, const RigidBody* body);


        /// Force the physics world to rebuild - usefull if you want to update the build if the scene is not being updated.
        void ForceRebuild() { freePhysicsInternals(); rebuildDirtyPhysicsComponents(); }


        
        ///set the global force acting on all rigid bodies in the world this force is always the same regardless of physics world scale.
        void SetGravity(const Vector3& force);
        ///return global force acting on all rigid bodies
        Vector3 GetGravity() const;

        ///set the physics scale of the world
        void SetPhysicsScale(float scale) { physicsScale_ = scale; }

        Matrix3x4 GetPhysicsWorldFrame() const { return Matrix3x4(Vector3::ZERO, Quaternion::IDENTITY, physicsScale_); }

        float GetPhysicsScale() const {
            return physicsScale_;
        }

        ///Conversions From Scene space to physics space.
        float SceneToPhysics_Domain(float x);
        Vector3 SceneToPhysics_Domain(Vector3 v);
        Matrix3x4 SceneToPhysics_Domain(Matrix3x4 sceneFrame);
        Vector3 SceneToPhysics_Domain_Torque(Vector3 torque);
        float SceneToPhysics_Domain_Torque(float torque);

        ///Conversions From physics space to scene space.
        float PhysicsToScene_Domain(float x);
        Vector3 PhysicsToScene_Domain(Vector3 v);
        Matrix3x4 PhysicsToScene_Domain(Matrix3x4 newtonFrame);
        Vector3 PhysicsToScene_Domain_Torque(Vector3 torque);
        float PhysicsToScene_Domain_Torque(float torque);





        /// set how many iterations newton will run.
        void SetIterationCount(int numIterations);

        int GetIterationCount() const;
        /// set how many sub-updates to run vs the core update rate. must be 8, 4, 2, or 1
        void SetSubstepFactor(int numSubsteps);

        int GetSubstepFactor() const;
        /// set how many threads the newton can use.
        void SetThreadCount(int numThreads);

        int GetThreadCount() const;

        virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest) override;

    protected:


        ///Global force
        Vector3 gravity_ = DEF_GRAVITY;

        /// number of thread to allow newton to use
        int newtonThreadCount_ = 4;
        /// number of iterations newton will internally use per substep
        int iterationCount_ = 4;
        /// number of substeps per scene subsystem update. (1,2,4,8)
        int subStepFactor = 2;

        float timeStepTarget_;

        virtual void OnSceneSet(Scene* scene) override;

        void addCollisionShape(CollisionShape* collision);
        void removeCollisionShape(CollisionShape* collision);

        void addRigidBody(RigidBody* body);
        void removeRigidBody(RigidBody* body);

        void addConstraint(Constraint* constraint);
        void removeConstraint(Constraint* constraint);



        void markRigidBodiesNeedSorted() { rigidBodyListNeedsSorted = true; }
        bool rigidBodyListNeedsSorted = true;

        Vector<WeakPtr<CollisionShape>> collisionComponentList;
        Vector<WeakPtr<RigidBody>> rigidBodyComponentList;
        Vector<WeakPtr<Constraint>> constraintList;


        void freeWorld();

        void addToFreeQueue(NewtonBody* newtonBody);
        void addToFreeQueue(dCustomJoint* newtonConstraint);
        void addToFreeQueue(NewtonCollision* newtonCollision);


        PODVector<NewtonBody*> freeBodyQueue_;
        PODVector<dCustomJoint*> freeConstraintQueue_;
        PODVector<NewtonCollision*> freeCollisionQueue_;


        void applyNewtonWorldSettings();


        Vector<SharedPtr<RigidBodyContactEntry>> contactEntryPool_;
        int contactEntryPoolCurIdx_ = 0;
        const int contactEntryPoolSize_ = 100;




        void formContacts(bool rootrate);
        void ParseContacts();
        bool contactMapLocked_ = false;

        /// Step the simulation forward.
        void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
        void Update(float timestep, bool isRootUpdate);
        void rebuildDirtyPhysicsComponents();
        bool sceneUpdated_ = false;
        bool simulationStarted_ = false;
        /// Internal newton world
        NewtonWorld* newtonWorld_ = nullptr;

        ///vehicle manager for instantiating vehicles.
        dVehicleManager* vehicleManager_ = nullptr;

        RigidBody* sceneBody_ = nullptr;


        float physicsScale_ = 1.0f;

        ///convex casts
        static const int convexCastRetInfoSize_ = 1000;
        NewtonWorldConvexCastReturnInfo convexCastRetInfoArray[convexCastRetInfoSize_];
        int DoNewtonCollideTest(const float* const matrix, const NewtonCollision* shape);
        void GetBodiesInConvexCast(PODVector<RigidBody*>& result, int numContacts);

        ///newton mesh caching
        HashMap<StringHash, SharedPtr<NewtonMeshObject>> newtonMeshCache_;

        ///returns a unique key for looking up an exising NewtonMesh from the cache.
        static StringHash NewtonMeshKey(String modelResourceName, int modelLodLevel, String otherData);
        NewtonMeshObject* GetCreateNewtonMesh(StringHash urhoNewtonMeshKey);
        NewtonMeshObject* GetNewtonMesh(StringHash urhoNewtonMeshKey);
        

        void freePhysicsInternals();
};



    String NewtonThreadProfilerString(int threadIndex);

    /// netwon body callbacks
    void Newton_ApplyForceAndTorqueCallback(const NewtonBody* body, dFloat timestep, int threadIndex);
    void Newton_SetTransformCallback(const NewtonBody* body, const dFloat* matrix, int threadIndex);
    void Newton_DestroyBodyCallback(const NewtonBody* body);
    unsigned Newton_WorldRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);
    dFloat Newton_WorldRayCastFilterCallback(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam);





    /// newton material callbacks
    void Newton_ProcessContactsCallback(const NewtonJoint* contactJoint, dFloat timestep, int threadIndex);
    int Newton_AABBOverlapCallback(const NewtonJoint* const contactJoint, dFloat timestep, int threadIndex);
    int Newton_AABBCompoundOverlapCallback(const NewtonJoint* const contact, dFloat timestep, const NewtonBody* const body0, const void* const collisionNode0, const NewtonBody* const body1, const void* const collisionNode1, int threadIndex);


    URHO3D_API RigidBody* GetRigidBody(Node* node, bool includeScene);
    URHO3D_API void  GetRootRigidBodies(PODVector<RigidBody*>& rigidBodies, Node* node, bool includeScene);
    URHO3D_API void  GetNextChildRigidBodies(PODVector<RigidBody*>& rigidBodies, Node* node);
    URHO3D_API void  GetAloneCollisionShapes(PODVector<CollisionShape*>& colShapes, Node* startingNode, bool includeStartingNodeShapes);

    URHO3D_API void  RebuildPhysicsNodeTree(Node* node);

    URHO3D_API unsigned CollisionLayerAsBit(unsigned layer);
    /// Register Physics library objects.
    URHO3D_API void RegisterPhysicsLibrary(Context* context);
}
