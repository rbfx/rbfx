/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _DG_WORLD_H_
#define _DG_WORLD_H_

#include "dgBody.h"
#include "dgContact.h"
#include "dgCollision.h"
#include "dgBroadPhase.h"
#include "dgWorldPlugins.h"
#include "dgCollisionScene.h"
#include "dgBodyMasterList.h"
#include "dgWorldDynamicUpdate.h"
#include "dgBilateralConstraint.h"
//#include "dgDeformableBodiesUpdate.h"
#include "dgCollisionCompoundFractured.h"

#define DG_REDUCE_CONTACT_TOLERANCE			dgFloat32 (5.0e-2f)
#define DG_PRUNE_CONTACT_TOLERANCE			dgFloat32 (5.0e-2f)

#define DG_SLEEP_ENTRIES					8
#define DG_MAX_DESTROYED_BODIES_BY_FORCE	8

class dgBody;
class dgDynamicBody;
class dgKinematicBody;
class dgCollisionPoint;
class dgUserConstraint;
class dgBallConstraint;
class dgHingeConstraint;
class dgInverseDynamics;
class dgUserMeshCreation;
class dgSlidingConstraint;
class dgCollisionInstance;
class dgSkeletonContainer;
class dgUpVectorConstraint;
class dgUniversalConstraint;
class dgCorkscrewConstraint;
class dgCollisionDeformableMesh;


class dgBodyCollisionList: public dgTree<const dgCollision*, dgUnsigned32>
{
	public:
	dgBodyCollisionList (dgMemoryAllocator* const allocator)
		:dgTree<const dgCollision*, dgUnsigned32>(allocator)
	{
	}
};

class dgBodyMaterialList: public dgTree<dgContactMaterial, dgUnsigned32>
{
	public:
	dgBodyMaterialList (dgMemoryAllocator* const allocator)
		:dgTree<dgContactMaterial, dgUnsigned32>(allocator)
	{
	}
};

class dgSkeletonList: public dgList<dgSkeletonContainer*>
{
	public:
	dgSkeletonList(dgMemoryAllocator* const allocator)
		:dgList<dgSkeletonContainer*>(allocator)
		,m_lruMarker(1)
		,m_skelListIsDirty(true)
	{
	}

	dgInt32 m_lruMarker;
	bool m_skelListIsDirty;
};

class dgInverseDynamicsList: public dgList<dgInverseDynamics*>
{
	public:
	dgInverseDynamicsList(dgMemoryAllocator* const allocator)
		:dgList<dgInverseDynamics*>(allocator)
	{
	}
};


class dgWorld;
class dgCollisionInstance;
class dgCollisionParamProxy;

class dgSolverProgressiveSleepEntry
{
	public:
	dgFloat32 m_maxAccel;
	dgFloat32 m_maxAlpha;
	dgFloat32 m_maxVeloc;
	dgFloat32 m_maxOmega;
	dgInt32 m_steps;
};

class dgWorldThreadPool: public dgThreadHive
{
	public:
	dgWorldThreadPool(dgMemoryAllocator* const allocator)
		:dgThreadHive(allocator)
	{
	}

	virtual void OnBeginWorkerThread (dgInt32 threadId);
	virtual void OnEndWorkerThread (dgInt32 threadId);
};

class dgDeadJoints: public dgTree<dgConstraint*, void* >
{
	public: 
	dgDeadJoints(dgMemoryAllocator* const allocator)
		:dgTree<dgConstraint*, void* >(allocator)
		,m_lock(0)
	{
	}
	
	void DestroyJoints(dgWorld& world);
	void DestroyJoint(dgConstraint* const joint);
	private:
	dgInt32 m_lock;
};

class dgDeadBodies: public dgTree<dgBody*, void* >
{
	public: 
	dgDeadBodies(dgMemoryAllocator* const allocator)
		:dgTree<dgBody*, void*>(allocator)
		,m_lock(0)
	{
	}
	void DestroyBody(dgBody* const body);
	void DestroyBodies(dgWorld& world);

	private:
	dgInt32 m_lock;
};

typedef void (*dgPostUpdateCallback) (const dgWorld* const world, dgFloat32 timestep);

DG_MSC_VECTOR_ALIGMENT
class dgWorld
	:public dgBodyMasterList
	,public dgBodyMaterialList
	,public dgBodyCollisionList
	,public dgSkeletonList
	,public dgInverseDynamicsList
	,public dgContactList 
	,public dgBilateralConstraintList
	,public dgWorldDynamicUpdate
	,public dgMutexThread
	,public dgWorldThreadPool
	,public dgDeadBodies
	,public dgDeadJoints
	,public dgWorldPluginList
{
	public:
	typedef dgUnsigned32 (dgApi *OnClusterUpdate) (const dgWorld* const world, void* island, dgInt32 bodyCount);
	typedef void (dgApi *OnListenerBodyDestroyCallback) (const dgWorld* const world, void* const listener, dgBody* const body);
	typedef void (dgApi *OnListenerUpdateCallback) (const dgWorld* const world, void* const listener, dgFloat32 timestep);
	typedef void (dgApi *OnListenerDestroyCallback) (const dgWorld* const world, void* const listener);
	typedef void (dgApi *OnListenerDebugCallback) (const dgWorld* const world, void* const listener, void* const debugContext);
	typedef void (dgApi *OnBodySerialize) (dgBody& me, void* const userData, dgSerialize funt, void* const serilalizeObject);
	typedef void (dgApi *OnBodyDeserialize) (dgBody& me, void* const userData, dgDeserialize funt, void* const serilalizeObject);
	typedef void (dgApi *OnCollisionInstanceDestroy) (const dgWorld* const world, const dgCollisionInstance* const collision);
	typedef void (dgApi *OnCollisionInstanceDuplicate) (const dgWorld* const world, dgCollisionInstance* const collision, const dgCollisionInstance* const sourceCollision);

	typedef void (dgApi *OnJointSerializationCallback) (const dgUserConstraint* const joint, dgSerialize funt, void* const serilalizeObject);
	typedef void (dgApi *OnJointDeserializationCallback) (const dgBody* const body0, const dgBody* const body1, dgDeserialize funt, void* const serilalizeObject);

	enum dgBroadPhaseType
	{
		m_broadphaseMixed,
		m_broadphaseSegregated,
	};

	class dgListener
	{
		public: 
		dgListener()
			:m_world(NULL)
			,m_userData(NULL)
			,m_onPreUpdate(NULL)
			,m_onPostUpdate(NULL)
			,m_onDebugCallback(NULL)
			,m_onListenerDestroy(NULL)
			,m_onBodyDestroy(NULL)
		{
		}
		
		~dgListener()
		{
			if (m_onListenerDestroy) {
				m_onListenerDestroy(m_world, m_userData);
			}
		}

		char m_name[32];
		dgWorld* m_world;
		void* m_userData;
		OnListenerUpdateCallback m_onPreUpdate;
		OnListenerUpdateCallback m_onPostUpdate;
		OnListenerDebugCallback m_onDebugCallback;
		OnListenerDestroyCallback m_onListenerDestroy;
		OnListenerBodyDestroyCallback m_onBodyDestroy;
	};

	class dgListenerList: public dgList <dgListener>
	{
		public: 
		dgListenerList (dgMemoryAllocator* const allocator)
			:dgList <dgListener>(allocator)
		{
		}

		~dgListenerList()
		{
		}
	};

	DG_CLASS_ALLOCATOR(allocator)

	dgWorld(dgMemoryAllocator* const allocator);
	~dgWorld();

	dgFloat32 GetUpdateTime() const;
	dgBroadPhase* GetBroadPhase() const;

	dgInt32 GetSolverIterations() const;
	void SetSolverIterations (dgInt32 mode);

	void SetPostUpdateCallback (const dgWorld* const newtonWorld, dgPostUpdateCallback callback);

	void EnableParallelSolverOnLargeIsland(dgInt32 mode);
	dgInt32 GetParallelSolverOnLargeIsland() const;

	void FlushCache();

	virtual dgUnsigned64 GetTimeInMicrosenconds() const;
	
	void* GetUserData() const;
	void SetUserData (void* const userData);

	void Update (dgFloat32 timestep);
	void UpdateAsync (dgFloat32 timestep);
	void StepDynamics (dgFloat32 timestep);
	
	dgInt32 Collide (const dgCollisionInstance* const collisionA, const dgMatrix& matrixA, 
					 const dgCollisionInstance* const collisionB, const dgMatrix& matrixB, 
					 dgTriplex* const points, dgTriplex* const normals, dgFloat32* const penetration, 
					 dgInt64* const attibuteA, dgInt64* const attibuteB, dgInt32 maxContacts, dgInt32 threadIndex);

	dgInt32 CollideContinue (const dgCollisionInstance* const collisionA, const dgMatrix& matrixA, const dgVector& velocA, const dgVector& omegaA, 
							 const dgCollisionInstance* const collisionB, const dgMatrix& matrixB, const dgVector& velocB, const dgVector& omegaB, 
		                     dgFloat32& timeStep, dgTriplex* const points, dgTriplex* const normals, dgFloat32* const penetration, 
							 dgInt64* const attibuteA, dgInt64* const attibuteB, dgInt32 maxContacts, dgInt32 threadIndex);

	bool IntersectionTest (const dgCollisionInstance* const collisionA, const dgMatrix& matrixA, 
						   const dgCollisionInstance* const collisionB, const dgMatrix& matrixB, 
						   dgInt32 threadIndex);
	
	dgInt32 ClosestPoint (dgTriplex& point, const dgCollisionInstance* const collision, const dgMatrix& matrix, dgTriplex& contact, dgTriplex& normal, dgInt32 threadIndex);
	dgInt32 ClosestPoint (const dgCollisionInstance* const collisionA, const dgMatrix& matrixA, 
						  const dgCollisionInstance* const collisionB, const dgMatrix& matrixB, 
						  dgTriplex& contactA, dgTriplex& contactB, dgTriplex& normalAB, dgInt32 threadIndex);

	void SetFrictionThreshold (dgFloat32 acceletion);

	void ListenersDebug(void* const debugContext);
	void* GetListenerUserData (void* const listener) const;
	void* FindListener (const char* const nameid) const;

	void* AddListener (const char* const nameid, void* const userData);
	void ListenerSetDestroyCallback (void* const listener, OnListenerDestroyCallback destroyCallback);
	void ListenerSetPreUpdate (void* const listener, OnListenerUpdateCallback updateCallback);
	void ListenerSetPostUpdate (void* const listener, OnListenerUpdateCallback updateCallback);
	
	void SetListenerBodyDebugCallback (void* const listener, OnListenerDebugCallback callback);
	void SetListenerBodyDestroyCallback (void* const listener, OnListenerBodyDestroyCallback callback);
	OnListenerBodyDestroyCallback GetListenerBodyDestroyCallback (void* const listener) const;

	void SetIslandUpdateCallback (OnClusterUpdate callback); 

	void InitBody (dgBody* const body, dgCollisionInstance* const collision, const dgMatrix& matrix);
	dgDynamicBody* CreateDynamicBody (dgCollisionInstance* const collision, const dgMatrix& matrix);
	dgKinematicBody* CreateKinematicBody (dgCollisionInstance* const collision, const dgMatrix& matrix);
	dgDynamicBody* CreateDynamicBodyAsymetric(dgCollisionInstance* const collision, const dgMatrix& matrix);
	void DestroyBody(dgBody* const body);
	void DestroyAllBodies ();

//	void AddToBreakQueue (const dgContact* const contactJoint, dgBody* const body, dgFloat32 maxForce);

    // modify the velocity and angular velocity of a body in such a way 
	// that the velocity of pointPosit is increase by pointDeltaVeloc 
	// pointVeloc and pointPosit are in world space
//	void AddBodyImpulse (dgBody* body, const dgVector& pointDeltaVeloc, const dgVector& pointPosit);
//	void ApplyImpulseArray (dgBody* body, dgInt32 count, dgInt32 strideInBytes, const dgFloat32* const impulseArray, const dgFloat32* const pointArray);

	// apply the transform matrix to the body and recurse trough all bodies attached to this body with a 
	// bilateral joint contact joint are ignored.
	void BodySetMatrix (dgBody* const body, const dgMatrix& matrix);
	
	dgInt32 GetBodiesCount() const;
	dgInt32 GetConstraintsCount() const;

    dgCollisionInstance* CreateInstance (const dgCollision* const child, dgInt32 shapeID, const dgMatrix& offsetMatrix);

	dgCollisionInstance* CreateNull ();
	dgCollisionInstance* CreateSphere (dgFloat32 radiusdg, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateCone (dgFloat32 radius, dgFloat32 height, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateCapsule (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateCylinder (dgFloat32 radio0, dgFloat32 radio1, dgFloat32 height, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateBox (dgFloat32 dx, dgFloat32 dy, dgFloat32 dz, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateConvexHull (dgInt32 count, const dgFloat32* const points, dgInt32 strideInBytes, dgFloat32 thickness, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateChamferCylinder (dgFloat32 radius, dgFloat32 height, dgInt32 shapeID, const dgMatrix& offsetMatrix = dgGetIdentityMatrix());
	dgCollisionInstance* CreateCompound ();
	dgCollisionInstance* CreateFracturedCompound (dgMeshEffect* const solidMesh, int shapeID, int fracturePhysicsMaterialID, int pointcloudCount, const dgFloat32* const vertexCloud, int strideInBytes, int materialID, const dgMatrix& textureMatrix,
												  dgCollisionCompoundFractured::OnEmitFractureChunkCallBack emitFrafuredChunk, dgCollisionCompoundFractured::OnEmitNewCompundFractureCallBack emitFracturedCompound, dgCollisionCompoundFractured::OnReconstructFractureMainMeshCallBack reconstructMainMesh);

	dgCollisionInstance* CreateDeformableSolid (dgMeshEffect* const mesh, dgInt32 shapeID);
	dgCollisionInstance* CreateMassSpringDamperSystem (dgInt32 shapeID, dgInt32 pointCount, const dgFloat32* const points, dgInt32 srideInBytes, const dgFloat32* const pointsMass, dgInt32 linksCount, const dgInt32* const links, const dgFloat32* const linksSpring, const dgFloat32* const LinksDamper);

	dgCollisionInstance* CreateBVH ();	
	dgCollisionInstance* CreateStaticUserMesh (const dgVector& boxP0, const dgVector& boxP1, const dgUserMeshCreation& data);
	dgCollisionInstance* CreateHeightField (dgInt32 width, dgInt32 height, dgInt32 contructionMode, dgInt32 elevationDataType, const void* const elevationMap, const dgInt8* const atributeMap, dgFloat32 verticalScale, dgFloat32 horizontalScale_x, dgFloat32 horizontalScale_z);
	dgCollisionInstance* CreateScene ();	

	dgBroadPhaseAggregate* CreateAggreGate() const; 
	void DestroyAggregate(dgBroadPhaseAggregate* const aggregate) const; 

	dgInverseDynamics* CreateInverseDynamics();
	void DestroyInverseDynamics(dgInverseDynamics* const inverseDynamics);

	void SetCollisionInstanceConstructorDestructor (OnCollisionInstanceDuplicate constructor, OnCollisionInstanceDestroy destructor);

	static void OnDeserializeFromFile(void* const userData, void* const buffer, dgInt32 size);
	static void OnSerializeToFile(void* const userData, const void* const buffer, dgInt32 size);
	static dgInt32 SerializeToFileSort (const dgBody* const body0, const dgBody* const body1, void* const context);
	static void OnBodySerializeToFile (dgBody& body, void* const userData, dgSerialize serializeCallback, void* const serializeHandle);
	static void OnBodyDeserializeFromFile (dgBody& body, void* const userData, dgDeserialize deserializeCallback, void* const serializeHandle);

	dgBody* FindBodyFromSerializedID(dgInt32 serializedID) const;
	void SetJointSerializationCallbacks(OnJointSerializationCallback serializeJointCallback, OnJointDeserializationCallback deserializeJointCallback);
	void GetJointSerializationCallbacks(OnJointSerializationCallback* const serializeJointCallback, OnJointDeserializationCallback* const deserializeJointCallback) const;

	void SerializeScene(void* const userData, OnBodySerialize bodyCallback, dgSerialize serializeCallback, void* const serializeHandle) const;
	void DeserializeScene(void* const userData, OnBodyDeserialize bodyCallback, dgDeserialize deserializeCallback, void* const serializeHandle);

	void SerializeBodyArray (void* const userData, OnBodySerialize bodyCallback, dgBody** const array, dgInt32 count, dgSerialize serializeCallback, void* const serializeHandle) const;
	void DeserializeBodyArray (void* const userData, OnBodyDeserialize bodyCallback, dgTree<dgBody*, dgInt32>&bodyMap, dgDeserialize deserializeCallback, void* const serializeHandle);

	void SerializeJointArray (dgInt32 count, dgSerialize serializeCallback, void* const serializeHandle) const;
	void DeserializeJointArray (const dgTree<dgBody*, dgInt32>&bodyMap, dgDeserialize serializeCallback, void* const serializeHandle);

	void SerializeCollision (dgCollisionInstance* const shape, dgSerialize deserialization, void* const userData) const;
	dgCollisionInstance* CreateCollisionFromSerialization (dgDeserialize deserialization, void* const userData);
	void ReleaseCollision(const dgCollision* const collision);
	
	dgUpVectorConstraint* CreateUpVectorConstraint (const dgVector& pin, dgBody *body);
	
	dgBallConstraint* CreateBallConstraint (const dgVector& pivot, dgBody* const body0, dgBody *refBody = NULL);
	dgHingeConstraint* CreateHingeConstraint (const dgVector& pivot, const dgVector& pin, dgBody* const body0, dgBody *refBody = NULL);
	dgSlidingConstraint* CreateSlidingConstraint (const dgVector& pivot, const dgVector& pin, dgBody* const body0, dgBody *refBody = NULL);
	dgCorkscrewConstraint* CreateCorkscrewConstraint (const dgVector& pivot, const dgVector& pin, dgBody* const body0, dgBody *refBody = NULL);
	dgUniversalConstraint* CreateUniversalConstraint (const dgVector& pivot, const dgVector& pin0, const dgVector& pin1, dgBody* const body0, dgBody *body1 = NULL);

	void DestroyConstraint (dgConstraint* constraint);

	dgSkeletonContainer* CreateNewtonSkeletonContainer (dgBody* const rootBone);
	void DestroySkeletonContainer (dgSkeletonContainer* const container);

	dgUnsigned32 CreateBodyGroupID();
	void RemoveAllGroupID();

	dgUnsigned32 GetDefualtBodyGroupID() const;
	dgContactMaterial* GetMaterial (dgUnsigned32 bodyGroupId0, dgUnsigned32 bodyGroupId1) const;

	dgContactMaterial* GetFirstMaterial () const;
	dgContactMaterial* GetNextMaterial (dgContactMaterial* material) const;
	dgContact* FindContactJoint (const dgBody* body0, const dgBody* body1) const;

	void SetThreadsCount (dgInt32 count);
	
	//Parallel Job dispatcher for user related stuff
	void ExecuteUserJob (dgWorkerThreadTaskCallback userJobKernel, void* const userJobKernelContext, const char* const functionName);

	void BodyEnableSimulation (dgBody* const body);
	void BodyDisableSimulation (dgBody* const body);
	bool GetBodyEnableDisableSimulationState (dgBody* const body) const;

	dgDynamicBody* GetSentinelBody() const;
	dgMemoryAllocator* GetAllocator() const;

	dgInt32 GetBroadPhaseType() const;
	void SetBroadPhaseType (dgInt32 type);
	void ResetBroadPhase();
	
	dgFloat32 GetContactMergeTolerance() const;
	void SetContactMergeTolerance(dgFloat32 tolerenace);

	void Sync ();

	void SetSubsteps (dgInt32 subSteps);
	dgInt32 GetSubsteps () const;
	
	private:
	class dgAdressDistPair
	{
		public:
		dgInt32 m_adress;
		dgFloat32 m_dist;
	};

	void RunStep ();
	void CalculateContacts (dgBroadPhase::dgPair* const pair, dgInt32 threadIndex, bool ccdMode, bool intersectionTestOnly);
	dgInt32 PruneContacts (dgInt32 count, dgContactPoint* const contact, dgFloat32 distTolerenace, dgInt32 maxCount = (DG_CONSTRAINT_MAX_ROWS / 3)) const;
	dgInt32 ReduceContacts (dgInt32 count, dgContactPoint* const contact, dgInt32 maxCount, dgFloat32 tol, dgInt32 arrayIsSorted = 0) const;
	dgInt32 CalculateConvexPolygonToHullContactsDescrete (dgCollisionParamProxy& proxy) const;
	dgInt32 CalculatePolySoupToHullContactsDescrete (dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateConvexToNonConvexContactsContinue (dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateUserContacts (dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateConvexToNonConvexContacts (dgCollisionParamProxy& proxy) const;
	dgInt32 CalculateConvexToConvexContacts (dgCollisionParamProxy& proxy) const;
	dgInt32 PruneContactsByRank(dgInt32 count, dgCollisionParamProxy& proxy, dgInt32 maxCount) const;
	
	void PopulateContacts (dgBroadPhase::dgPair* const pair, dgInt32 threadIndex);	
	void ProcessContacts (dgBroadPhase::dgPair* const pair, dgInt32 threadIndex);
	void ProcessCachedContacts (dgContact* const contact, dgFloat32 timestep, dgInt32 threadIndex) const;

	void ConvexContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	void CompoundContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	
	void SceneContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	void SceneChildContacts (dgBroadPhase::dgPair* const pair, dgCollisionParamProxy& proxy) const;
	
	dgFloat32 CalculateTimeToImpact (dgContact* const contactJoint, dgFloat32 timestep, dgInt32 threadIndex, dgVector& p, dgVector& q, dgVector& normal, dgFloat32 dist) const;
	dgInt32 ClosestPoint (dgCollisionParamProxy& proxy) const;
	//dgInt32 ClosestCompoundPoint (dgBody* const compoundConvexA, dgBody* const collisionB, dgTriplex& contactA, dgTriplex& contactB, dgTriplex& normalAB, dgInt32 threadIndex) const;
	dgInt32 ClosestCompoundPoint (dgCollisionParamProxy& proxy) const;

	bool AreBodyConnectedByJoints (dgBody* const origin, dgBody* const target);
	
	void UpdateSkeletons();
	void UpdateBroadphase(dgFloat32 timestep);
	
	void AddSentinelBody();
	void InitConvexCollision ();

	DG_INLINE dgBody* FindRoot(dgBody* const body) const;
	DG_INLINE dgBody* FindRootAndSplit(dgBody* const body) const;
	DG_INLINE void UnionSet(const dgConstraint* const joint) const;
	
	virtual void Execute (dgInt32 threadID);
	virtual void TickCallback (dgInt32 threadID);
	void UpdateTransforms(dgBodyMasterList::dgListNode* node, dgInt32 threadID);

	static dgUnsigned32 dgApi GetPerformanceCount ();
	static void UpdateTransforms(void* const context, void* const node, dgInt32 threadID);
	static dgInt32 SortFaces (const dgAdressDistPair* const A, const dgAdressDistPair* const B, void* const context);
	static dgInt32 CompareJointByInvMass (const dgBilateralConstraint* const jointA, const dgBilateralConstraint* const jointB, void* notUsed);

	dgUnsigned32 m_numberOfSubsteps;
	dgUnsigned32 m_dynamicsLru;
	dgUnsigned32 m_inUpdate;
	dgUnsigned32 m_solverIterations;
	dgUnsigned32 m_bodyGroupID;
	dgUnsigned32 m_defualtBodyGroupID;
	dgUnsigned32 m_bodiesUniqueID;
	dgUnsigned32 m_useParallelSolver;
	dgUnsigned32 m_genericLRUMark;
	dgInt32 m_delayDelateLock;
	dgInt32 m_clusterLRU;

	dgFloat32 m_freezeAccel2;
	dgFloat32 m_freezeAlpha2;
	dgFloat32 m_freezeSpeed2;
	dgFloat32 m_freezeOmega2;
	dgFloat32 m_frictiomTheshold;
	dgFloat32 m_savetimestep;
	dgFloat32 m_contactTolerance;
	dgFloat32 m_lastExecutionTime;

	dgSolverProgressiveSleepEntry m_sleepTable[DG_SLEEP_ENTRIES];
	
	dgBroadPhase* m_broadPhase; 
	dgDynamicBody* m_sentinelBody;
	dgCollisionInstance* m_pointCollision;

	void* m_userData;
	dgMemoryAllocator* m_allocator;

	dgSemaphore m_mutex;
	OnClusterUpdate m_clusterUpdate;
	OnCollisionInstanceDestroy	m_onCollisionInstanceDestruction;
	OnCollisionInstanceDuplicate m_onCollisionInstanceCopyConstrutor;
	OnJointSerializationCallback m_serializedJointCallback;	
	OnJointDeserializationCallback m_deserializedJointCallback;	
	dgPostUpdateCallback m_postUpdateCallback;

	dgListenerList m_listeners;
	dgTree<void*, unsigned> m_perInstanceData;
	dgArray<dgBodyInfo> m_bodiesMemory; 
	dgArray<dgJointInfo> m_jointsMemory; 
	dgArray<dgBodyCluster> m_clusterMemory;
	dgArray<dgUnsigned8> m_solverJacobiansMemory;  
	dgArray<dgUnsigned8> m_solverRightHandSideMemory;
	dgArray<dgUnsigned8> m_solverForceAccumulatorMemory;
	
	
	bool m_concurrentUpdate;
	
	friend class dgBody;
	friend class dgSolver;
	friend class dgContact;
	friend class dgBroadPhase;
	friend class dgDeadBodies;
	friend class dgDeadJoints;
	friend class dgWorldPlugin;
	friend class dgContactList;
	friend class dgUserConstraint;
	friend class dgBodyMasterList;
	friend class dgJacobianMemory;
	friend class dgCollisionScene;
	friend class dgCollisionConvex;
	friend class dgBroadPhaseMixed;
	friend class dgCollisionInstance;
	friend class dgCollisionCompound;
	friend class dgParallelBodySolver;
	friend class dgWorldDynamicUpdate;
	friend class dgParallelSolverClear;	
	friend class dgParallelSolverSolve;
	friend class dgCollisionHeightField;
	friend class dgSolverWorlkerThreads;
	friend class dgBroadPhaseSegregated;
	friend class dgCollisionConvexPolygon;
	friend class dgCollidingPairCollector;
	friend class dgCollisionDeformableMesh;
	friend class dgParallelSolverUpdateForce;
	friend class dgParallelSolverUpdateVeloc;
	friend class dgParallelSolverBodyInertia;
	friend class dgCollisionDeformableSolidMesh;
	friend class dgBroadPhaseApplyExternalForce;
	friend class dgParallelSolverCalculateForces;
	friend class dgCollisionMassSpringDamperSystem;
	friend class dgParallelSolverJointAcceleration;
	friend class dgParallelSolverBuildJacobianRows;
	friend class dgParallelSolverInitFeedbackUpdate;
	friend class dgParallelSolverInitInternalForces;
	friend class dgParallelSolverBuildJacobianMatrix;
	
	friend class dgBroadPhaseMaterialCallbackWorkerThread;
	friend class dgBroadPhaseCalculateContactsWorkerThread;
} DG_GCC_VECTOR_ALIGMENT ;


inline dgMemoryAllocator* dgWorld::GetAllocator() const
{
	return m_allocator;
}

inline dgBroadPhase* dgWorld::GetBroadPhase() const
{
	return m_broadPhase;
}

inline void dgWorld::SetSubsteps (dgInt32 subSteps)
{
	m_numberOfSubsteps = dgClamp(subSteps, 1, 8);
}

inline dgInt32 dgWorld::GetSubsteps () const
{
	return m_numberOfSubsteps;
}

inline dgFloat32 dgWorld::GetUpdateTime() const
{
	return m_lastExecutionTime;
}

inline void dgWorld::SetPostUpdateCallback(const dgWorld* const newtonWorld, dgPostUpdateCallback callback)
{
	m_postUpdateCallback = callback;
}

inline dgUnsigned64 dgWorld::GetTimeInMicrosenconds() const
{
	return dgGetTimeInMicrosenconds();
}

inline void dgWorld::SetSolverIterations(dgInt32 mode)
{
	m_solverIterations = dgUnsigned32(dgMax(1, mode));
}

inline dgInt32 dgWorld::GetSolverIterations() const
{
	return m_solverIterations;
}

DG_INLINE dgBody* dgWorld::FindRoot(dgBody* const body) const
{
	dgBody* node = body;
	for (; node->m_disjointInfo.m_parent != node; node = node->m_disjointInfo.m_parent);
	return node;
}

DG_INLINE dgBody* dgWorld::FindRootAndSplit(dgBody* const body) const
{
	dgBody* node = body;
	while (node->m_disjointInfo.m_parent != node) {
		dgBody* const prev = node;
		node = node->m_disjointInfo.m_parent;
		prev->m_disjointInfo.m_parent = node->m_disjointInfo.m_parent;
	}
	return node;
}

DG_INLINE void dgWorld::UnionSet(const dgConstraint* const joint) const
{
	dgBody* const body0 = joint->GetBody0();
	dgBody* const body1 = joint->GetBody1();
	dgBody* root0 = FindRootAndSplit(body0);
	dgBody* root1 = FindRootAndSplit(body1);
	if (root0 != root1) {
		if (root0->m_disjointInfo.m_rank < root1->m_disjointInfo.m_rank) {
			dgSwap(root0, root1);
		}
		root1->m_disjointInfo.m_parent = root0;
		if (root0->m_disjointInfo.m_rank == root1->m_disjointInfo.m_rank) {
			root0->m_disjointInfo.m_rank += 1;
			dgAssert(root0->m_disjointInfo.m_rank <= 6);
		}
		root0->m_disjointInfo.m_rowCount += root1->m_disjointInfo.m_rowCount;
		root0->m_disjointInfo.m_bodyCount += root1->m_disjointInfo.m_bodyCount;
		root0->m_disjointInfo.m_jointCount += root1->m_disjointInfo.m_jointCount;
	}
	root0->m_disjointInfo.m_jointCount++;
	root0->m_disjointInfo.m_rowCount += joint->m_maxDOF;
}


#endif
