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

#include "NewtonStdAfx.h"
#include "Newton.h"
#include "NewtonClass.h"


#ifdef _NEWTON_BUILD_DLL
	#if (defined (_MINGW_32_VER) || defined (_MINGW_64_VER))
		int main(int argc, char* argv[])
		{
			return 0;
		}
	#endif

	#ifdef _MSC_VER
		BOOL APIENTRY DllMain( HMODULE hModule,	DWORD  ul_reason_for_call, LPVOID lpReserved)
		{
			switch (ul_reason_for_call)
			{
				case DLL_THREAD_ATTACH:
				case DLL_PROCESS_ATTACH:
					// check for memory leaks
					#ifdef _DEBUG
						// Track all memory leaks at the operating system level.
						// make sure no Newton tool or utility leaves leaks behind.
						_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF));
					#endif

				case DLL_THREAD_DETACH:
				case DLL_PROCESS_DETACH:
				break;
			}
			return TRUE;
		}
	#endif
#endif



/*! @defgroup Misc Misc
Misc
@{
*/

//#define SAVE_COLLISION

#ifdef SAVE_COLLISION
void SerializeFile (void* serializeHandle, const void* buffer, size_t size)
{
	fwrite (buffer, size, 1, (FILE*) serializeHandle);
}

void DeSerializeFile (void* serializeHandle, void* buffer, size_t size)
{
	fread (buffer, size, 1, (FILE*) serializeHandle);
}


void SaveCollision (const NewtonCollision* const collisionPtr)
{
	FILE* file;
	// save the collision file
	file = fopen ("collisiontest.bin", "wb");
	//SerializeFile (file, MAGIC_NUMBER, strlen (MAGIC_NUMBER) + 1);
	NewtonCollisionSerialize (collisionPtr, SerializeFile, file);
	fclose (file);
}
#endif


/*!
  Return the exact amount of memory (in Bytes) use by the engine at any given time.

  @return total memory use by the engine.

  Applications can use this function to ascertain that the memory use by the
  engine is balanced at all times.

  See also: ::NewtonCreate
*/
int NewtonGetMemoryUsed()
{
	TRACE_FUNCTION(__FUNCTION__);
	return dgMemoryAllocator::GetGlobalMemoryUsed();
}

// fixme: needs docu
// @param mallocFnt is a pointer to the memory allocator callback function. If this parameter is NULL the standard *malloc* function is used.
// @param mfreeFnt is a pointer to the memory release callback function. If this parameter is NULL the standard *free* function is used.
//
void NewtonSetMemorySystem (NewtonAllocMemory mallocFnt, NewtonFreeMemory mfreeFnt)
{
	dgMemFree _free;
	dgMemAlloc _malloc;

	TRACE_FUNCTION(__FUNCTION__);

	if (mallocFnt && mfreeFnt) {
		_malloc = (dgMemAlloc) mallocFnt;
		_free = (dgMemFree) mfreeFnt;
	} else {
		_malloc = (dgMemAlloc) Newton::DefaultAllocMemory;
		_free = (dgMemFree) Newton::DefaultFreeMemory;
	}

	dgMemoryAllocator::SetGlobalAllocators (_malloc, _free);
}


void* NewtonAlloc (int sizeInBytes)
{
	return dgMallocStack(sizeInBytes);
}

void NewtonFree (void* const ptr)
{
	dgFreeStack(ptr); 
}

/*! @} */ // end of group Misc


/*! @defgroup World World
World interface
@{
*/

/*!
  Create an instance of the Newton world.

  @return Pointer to new Newton world.

  This function must be called before any of the other API functions.

  See also: ::NewtonDestroy, ::NewtonDestroyAllBodies
*/
NewtonWorld* NewtonCreate()
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMemoryAllocator* const allocator = new dgMemoryAllocator();

	NewtonWorld* const world = (NewtonWorld*) new (allocator) Newton (allocator);
	return world;
}


/*!
  Destroy an instance of the Newton world.

  @param *newtonWorld Pointer to the Newton world.
  @return Nothing.

  This function will destroy the entire Newton world.

  See also: ::NewtonCreate, ::NewtonDestroyAllBodies
*/
void NewtonDestroy(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *) newtonWorld;
	dgMemoryAllocator* const allocator = world->dgWorld::GetAllocator();

	delete world;
	delete allocator;
}

void NewtonSetPostUpdateCallback(const NewtonWorld* const newtonWorld, NewtonPostUpdateCallback callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetPostUpdateCallback(world, (dgPostUpdateCallback) callback);
}


int NewtonGetBroadphaseAlgorithm (const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetBroadPhaseType();
}

void NewtonSelectBroadphaseAlgorithm (const NewtonWorld* const newtonWorld, int algorithmType)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetBroadPhaseType(algorithmType);
}

void NewtonResetBroadphase(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->ResetBroadPhase();
}


dFloat NewtonGetContactMergeTolerance (const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetContactMergeTolerance();
}

void NewtonSetContactMergeTolerance (const NewtonWorld* const newtonWorld, dFloat tolerance)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetContactMergeTolerance(tolerance);
}


/*!
  Reset all internal engine states.

  @param *newtonWorld Pointer to the Newton world.

  Call this function whenever you want to create a reproducible simulation from
  a pre-defined initial condition.

  It does *not* suffice to merely reset the position and velocity of
  objects. This is because Newton takes advantage of frame-to-frame coherence for
  performance reasons.

  This function must be called outside of a Newton Update.

  Note: this kind of synchronization incurs a heavy performance penalty if
  called during each update.

  See also: ::NewtonUpdate
*/
void NewtonInvalidateCache(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->FlushCache();
}

void NewtonSetJointSerializationCallbacks (const NewtonWorld* const newtonWorld, NewtonOnJointSerializationCallback serializeJoint, NewtonOnJointDeserializationCallback deserializeJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetJointSerializationCallbacks (dgWorld::OnJointSerializationCallback(serializeJoint), dgWorld::OnJointDeserializationCallback(deserializeJoint));
}

void NewtonGetJointSerializationCallbacks (const NewtonWorld* const newtonWorld, NewtonOnJointSerializationCallback* const serializeJoint, NewtonOnJointDeserializationCallback* const deserializeJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->GetJointSerializationCallbacks ((dgWorld::OnJointSerializationCallback*)serializeJoint, (dgWorld::OnJointDeserializationCallback*)deserializeJoint);
}


void NewtonSerializeScene(const NewtonWorld* const newtonWorld, NewtonOnBodySerializationCallback bodyCallback, void* const bodyUserData,
	NewtonSerializeCallback serializeCallback, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SerializeScene(bodyUserData, dgWorld::OnBodySerialize(bodyCallback), (dgSerialize) serializeCallback, serializeHandle);
}

void NewtonDeserializeScene(const NewtonWorld* const newtonWorld, NewtonOnBodyDeserializationCallback bodyCallback, void* const bodyUserData,
							NewtonDeserializeCallback deserializeCallback, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->DeserializeScene(bodyUserData, (dgWorld::OnBodyDeserialize)bodyCallback, (dgDeserialize) deserializeCallback, serializeHandle);
}


void NewtonSerializeToFile (const NewtonWorld* const newtonWorld, const char* const filename, NewtonOnBodySerializationCallback bodyCallback, void* const bodyUserData)
{
	TRACE_FUNCTION(__FUNCTION__);
	FILE* const file = fopen(filename, "wb");
	if (file) {
		NewtonSerializeScene(newtonWorld, bodyCallback, bodyUserData, dgWorld::OnSerializeToFile, file);
		fclose (file);
	}
}

void NewtonDeserializeFromFile (const NewtonWorld* const newtonWorld, const char* const filename, NewtonOnBodyDeserializationCallback bodyCallback, void* const bodyUserData)
{
	TRACE_FUNCTION(__FUNCTION__);
	FILE* const file = fopen(filename, "rb");
	if (file) {
		NewtonDeserializeScene(newtonWorld, bodyCallback, bodyUserData, dgWorld::OnDeserializeFromFile, file);
		fclose (file);
	}
}

NewtonBody* NewtonFindSerializedBody(const NewtonWorld* const newtonWorld, int bodySerializedID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	dgAssert (0);
	return (NewtonBody*) world->FindBodyFromSerializedID(bodySerializedID);
}

void NewtonLoadPlugins(const NewtonWorld* const newtonWorld, const char* const plugInPath)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->LoadPlugins(plugInPath);
}

void NewtonUnloadPlugins(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->UnloadPlugins();
}

void* NewtonCurrentPlugin(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetCurrentPlugin();
}

void* NewtonGetFirstPlugin(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetFirstPlugin();
}

void* NewtonGetNextPlugin(const NewtonWorld* const newtonWorld, const void* const plugin)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgWorldPluginList::dgListNode* const node = (dgWorldPluginList::dgListNode*) plugin;
	return world->GetNextPlugin(node);
}

const char* NewtonGetPluginString(const NewtonWorld* const newtonWorld, const void* const plugin)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgWorldPluginList::dgListNode* const node = (dgWorldPluginList::dgListNode*) plugin;
	return world->GetPluginId (node);
}

void NewtonSelectPlugin(const NewtonWorld* const newtonWorld, const void* const plugin)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgWorldPluginList::dgListNode* const node = (dgWorldPluginList::dgListNode*) plugin;
	return world->SelectPlugin(node);
}

void* NewtonGetPreferedPlugin(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetpreferedPlugin();
}


/*!
  this function block all other threads from executing the same subsequent code simultaneously.

  @param *newtonWorld Pointer to the Newton world.
  @param threadIndex thread index from whe thsi function is called, zero if call form outsize a newton update

  this function should use to present racing conditions when when a call back ins executed form a mutithreaded loop.
  In general most call back are thread safe when they do not write to object outside the scope of the call back.
  this means for example that the application can modify values of object pointed by the arguments and or call that function
  that are allowed to be call for such callback.
  There are cases, however, when the application need to collect data for the client logic, example of such case are collecting
  information to display debug information, of collecting data for feedback.
  In these situations it is possible the the same critical code could be execute at the same time but several thread causing unpredictable side effect.
  so it is necessary to block all of the thread from executing any pieces of critical code.

  Not calling function *NewtonWorldCriticalSectionUnlock* will result on the engine going into an infinite loop.

  it is important that the critical section wrapped by functions *NewtonWorldCriticalSectionLock* and
  *NewtonWorldCriticalSectionUnlock* be keep small if the application is using the multi threaded functionality of the engine
  no doing so will lead to serialization of the parallel treads since only one thread can run the a critical section at a time.

  @return Nothing.

  See also: ::NewtonWorldCriticalSectionUnlock
*/
void NewtonWorldCriticalSectionLock (const NewtonWorld* const newtonWorld, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	world->GlobalLock();
}


int NewtonAtomicSwap (int* const ptr, int value)
{
	TRACE_FUNCTION(__FUNCTION__);
	return dgInterlockedExchange(ptr, value);
}

int NewtonAtomicAdd (int* const ptr, int value)
{
	TRACE_FUNCTION(__FUNCTION__);
	return dgAtomicExchangeAndAdd (ptr, value);
}

void NewtonYield ()
{
	TRACE_FUNCTION(__FUNCTION__);
	dgThreadYield();
}


/*!
  this function block all other threads from executing the same subsequent code simultaneously.

  @param *newtonWorld Pointer to the Newton world.


  this function should use to present racing conditions when when a call back ins executed form a multi threaded loop.
  In general most call back are thread safe when they do not write to object outside the scope of the call back.
  this means for example that the application can modify values of object pointed by the arguments and or call that function
  that are allowed to be call for such callback.
  There are cases, however, when the application need to collect data for the client logic, example of such case are collecting
  information to display debug information, of collecting data for feedback.
  In these situations it is possible the the same critical code could be execute at the same time but several thread causing unpredictable side effect.
  so it is necessary to block all of the thread from executing any pieces of critical code.

  it is important that the critical section wrapped by functions *NewtonWorldCriticalSectionLock* and
  *NewtonWorldCriticalSectionUnlock* be keep small if the application is using the multi threaded functionality of the engine
  no doing so will lead to serialization of the parallel treads since only one thread can run the a critical section at a time.

  @return Nothing.

  See also: ::NewtonWorldCriticalSectionLock
*/
void NewtonWorldCriticalSectionUnlock(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	world->GlobalUnlock();
}



/*!
  Set the maximum number of threads the engine can use.

  @param *newtonWorld Pointer to the Newton world.
  @param threads Maximum number of allowed threads.

  @return Nothing

  The maximum number of threaded is set on initialization to the maximum number
  of CPU in the system.
  fixme: this appears to be wrong. It is set to 1.

  See also: ::NewtonGetThreadsCount
*/
void NewtonSetThreadsCount(const NewtonWorld* const newtonWorld, int threads)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	world->SetThreadsCount(threads);
}


/*!
  Return the number of threads currently used by the engine.

  @param *newtonWorld Pointer to the Newton world.

  @return Number threads

  See also: ::NewtonSetThreadsCount, ::NewtonSetParallelSolverOnLargeIsland
*/
int NewtonGetThreadsCount(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	return world->GetThreadCount();
}


/*!
  Return the maximum number of threads supported on this platform.

  @param *newtonWorld Pointer to the Newton world.

  @return Number threads.

  This function will return 1 on single core version of the library.
  // fixme; what is a single core version?

  See also: ::NewtonSetThreadsCount, ::NewtonSetParallelSolverOnLargeIsland
*/
int NewtonGetMaxThreadsCount(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	return world->GetMaxThreadCount();
}


/*!
  Enable/disable multi-threaded constraint resolution for large islands
  (disabled by default).

  @param *newtonWorld Pointer to the Newton world.
  @param mode 1: enabled  0: disabled (default)

  @return Nothing

  Multi threaded mode is not always faster. Among the reasons are

  1 - Significant software cost to set up threads, as well as instruction overhead.
  2 - Different systems have different cost for running separate threads in a shared memory environment.
  3 - Parallel algorithms often have decreased converge rate. This can be as
      high as half of the of the sequential version. Consequently, the parallel
      solver requires a higher number of interactions to achieve similar convergence.

  It is recommended this option is enabled on system with more than two cores,
  since the performance gain in a dual core system are marginally better. Your
  mileage may vary.

  At the very least the application must test the option to verify the performance gains.

  This option has no impact on other subsystems of the engine.

  See also: ::NewtonGetThreadsCount, ::NewtonSetThreadsCount
*/
void NewtonSetParallelSolverOnLargeIsland(const NewtonWorld* const newtonWorld, int mode)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->EnableParallelSolverOnLargeIsland (mode);
}


void NewtonDispachThreadJob(const NewtonWorld* const newtonWorld, NewtonJobTask task, void* const usedData, const char* const functionName)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->ExecuteUserJob (dgWorkerThreadTaskCallback (task), usedData, functionName);
}

void NewtonSyncThreadJobs(const NewtonWorld* const newtonWorld)
{
	Newton* const world = (Newton *)newtonWorld;
	world->SynchronizationBarrier();
}

int NewtonGetParallelSolverOnLargeIsland(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetParallelSolverOnLargeIsland();
}

/*!
  Set the solver precision mode.

  @param *newtonWorld is the pointer to the Newton world
  @param model model of operation n = number of iteration default value is 4.

  @return Nothing

  n: the solve will execute a maximum of n iteration per cluster of connected joints and will terminate regardless of the 
  of the joint residual acceleration. 
  If it happen that the joints residual acceleration fall below the minimum tolerance 1.0e-5
  then the solve will terminate before the number of iteration reach N.
*/
void NewtonSetSolverIterations(const NewtonWorld* const newtonWorld, int model)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	world->SetSolverIterations (model);
}

/*!
Get the solver precision mode.
*/
int NewtonGetSolverIterations(const NewtonWorld* const newtonWorld)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	return world->GetSolverIterations();
}


/*!
  Advance the simulation by a user defined amount of time.

  @param *newtonWorld is the pointer to the Newton world
  @param timestep time step in seconds.

  @return Nothing

  This function will advance the simulation by the specified amount of time.

  The Newton Engine does not perform sub-steps, nor  does it need
  tuning parameters. As a consequence, the application is responsible for
  requesting sane time steps.

  See also: ::NewtonInvalidateCache
*/
void NewtonUpdate(const NewtonWorld* const newtonWorld, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;

//NewtonSerializeToFile (newtonWorld, "xxx.bin", NULL, NULL);
	world->UpdatePhysics (timestep);
}


void NewtonUpdateAsync (const NewtonWorld* const newtonWorld, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;

	world->UpdatePhysicsAsync(timestep);
}


void NewtonWaitForUpdateToFinish (const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->Sync ();
}

dFloat NewtonGetLastUpdateTime (const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetUpdateTime();
}


void NewtonSetNumberOfSubsteps (const NewtonWorld* const newtonWorld, int subSteps)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetSubsteps (subSteps);
}

int NewtonGetNumberOfSubsteps (const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetSubsteps ();
}



/*!
  Remove all bodies and joints from the Newton world.

  @param *newtonWorld Pointer to the Newton world.

  @return Nothing

  This function will destroy all bodies and all joints in the Newton world, but
  will retain group IDs.

  Use this function for when you want to clear the world but preserve all the
  group IDs and material pairs.

  See also: ::NewtonMaterialDestroyAllGroupID
*/
void NewtonDestroyAllBodies(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *) newtonWorld;
	world->DestroyAllBodies ();
}

/*!
  Set a function callback to be call on each island update.

  @param *newtonWorld Pointer to the Newton world.
  @param islandUpdate callback function.

  @return Nothing.

  The application can set a function callback to be called just after the array
  of all bodies making an island of connected bodies are collected. This
  function will be called just before the array is accepted for contact
  resolution and integration.

  The callback function must return an integer 0 or 1 to either skip or process
  the bodies in that particular island.

  Applications can leverage this function to implement an game physics LOD. For
  example the application can determine the AABB of the island and check it
  against the view frustum. If the entire island AABB is invisible, then the
  application can suspend its simulation, even if it is not in equilibrium.

  Other possible applications are to implement of a visual debugger, or freeze
  entire islands for application specific reasons.

  The application must not create, modify, or destroy bodies inside the callback
  or risk putting the engine into an undefined state (ie it will crash, if you
  are lucky).

  See also: ::NewtonIslandGetBody
*/
void NewtonSetIslandUpdateEvent(const NewtonWorld* const newtonWorld, NewtonIslandUpdate islandUpdate) 
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetIslandUpdateCallback((dgWorld::OnClusterUpdate) islandUpdate); 
}



/*!
  Get the first body in the body in the world body list.

  @param *newtonWorld Pointer to the Newton world.

  @return nothing

  The application can call this function to iterate thought every body in the world.

  The application call this function for debugging purpose
  See also: ::NewtonWorldGetNextBody, ::NewtonWorldForEachBodyInAABBDo, ::NewtonWorldForEachJointDo
*/
NewtonBody* NewtonWorldGetFirstBody(const NewtonWorld* const newtonWorld)
{
	Newton* const world = (Newton *) newtonWorld;
	dgBodyMasterList& masterList = *world;

	TRACE_FUNCTION(__FUNCTION__);
	dgAssert (masterList.GetFirst()->GetInfo().GetBody() == world->GetSentinelBody());
	dgBodyMasterList::dgListNode* const node = masterList.GetFirst()->GetNext();
//		body = node->GetInfo().GetBody();
//		node = node->GetNext();
//		callback ((const NewtonBody*) body);
//	}
	if (node) {
		return (NewtonBody*)node->GetInfo().GetBody();
	} else {
		return NULL;
	}
}


/*!
  Get the first body in the general body.

  @param *newtonWorld Pointer to the Newton world.
  @param curBody fixme

  @return nothing

  The application can call this function to iterate through every body in the world.

  The application call this function for debugging purpose

  See also: ::NewtonWorldGetFirstBody, ::NewtonWorldForEachBodyInAABBDo, ::NewtonWorldForEachJointDo
*/
NewtonBody* NewtonWorldGetNextBody(const NewtonWorld* const newtonWorld, const NewtonBody* const curBody)
{
	dgBody* const body = (dgBody*) curBody;

	TRACE_FUNCTION(__FUNCTION__);

	dgBodyMasterList::dgListNode* const node = body->GetMasterList()->GetNext();
	if (node) {
		return (NewtonBody*)node->GetInfo().GetBody();
	} else {
		return NULL;
	}

}


/*!
  Trigger callback function for each joint in the world.

  @param *newtonWorld Pointer to the Newton world.
  @param callback The callback function to invoke for each joint.
  @param *userData User data to pass into the callback.

  @return nothing

  The application should provide the function *NewtonJointIterator callback* to
  be called by Newton for every joint in the world.

  Note that this function is primarily for debugging. The performance penalty
  for calling it is high.

  See also: ::NewtonWorldForEachBodyInAABBDo, ::NewtonWorldGetFirstBody
*/
void NewtonWorldForEachJointDo(const NewtonWorld* const newtonWorld, NewtonJointIterator callback, void* const userData)
{
	Newton* const world = (Newton *) newtonWorld;
	dgBodyMasterList& masterList = *world;

	TRACE_FUNCTION(__FUNCTION__);
	dgTree<dgConstraint*, dgConstraint*> jointMap(world->dgWorld::GetAllocator());
	for (dgBodyMasterList::dgListNode* node = masterList.GetFirst()->GetNext(); node; node = node->GetNext()) {
		dgBodyMasterListRow& row = node->GetInfo();
		for (dgBodyMasterListRow::dgListNode* jointNode = row.GetFirst(); jointNode; jointNode = jointNode->GetNext()) {
			const dgBodyMasterListCell& cell = jointNode->GetInfo();
			if (cell.m_joint->GetId() != dgConstraint::m_contactConstraint) {
				if (!jointMap.Find(cell.m_joint)) {
					jointMap.Insert(cell.m_joint, cell.m_joint);
					callback ((const NewtonJoint*) cell.m_joint, userData);
				}
			}
		}
	}
}


/*!
  Trigger a callback for every body that intersects the specified AABB.

  @param *newtonWorld Pointer to the Newton world.
  @param *p0 - pointer to an array of at least three floats to hold minimum value for the AABB.
  @param *p1 - pointer to an array of at least three floats to hold maximum value for the AABB.
  @param callback application defined callback
  @param *userData pointer to the user defined user data value.

  @return nothing

  The application should provide the function *NewtonBodyIterator callback* to
  be called by Newton for every body in the world.

  For small AABB volumes this function is much more inefficients (fixme: more or
  less efficient?) than NewtonWorldGetFirstBody. However, if the AABB contains
  the majority of objects in the scene, the overhead of scanning the internal
  Broadphase collision plus the AABB test make this function more expensive.

  See also: ::NewtonWorldGetFirstBody
*/
void NewtonWorldForEachBodyInAABBDo(const NewtonWorld* const newtonWorld, const dFloat* const p0, const dFloat* const p1, NewtonBodyIterator callback, void* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *) newtonWorld;
	dgVector q0 (dgMin (p0[0], p1[0]), dgMin (p0[1], p1[1]), dgMin (p0[2], p1[2]), dgFloat32 (0.0f));
	dgVector q1 (dgMax (p0[0], p1[0]), dgMax (p0[1], p1[1]), dgMax (p0[2], p1[2]), dgFloat32 (0.0f));

	world->GetBroadPhase()->ForEachBodyInAABB (q0, q1, (OnBodiesInAABB) callback, userData);
}


/*!
  Return the current library version number.

  @return version number as an integer, eg 314.

  The version number is a three-digit integer.

  First digit:  major version (interface changes among other things)
  Second digit: major patch number (new features, and bug fixes)
  Third Digit:  minor bug fixed patch.
*/
int NewtonWorldGetVersion()
{
	TRACE_FUNCTION(__FUNCTION__);
	return NEWTON_MAJOR_VERSION * 100 + NEWTON_MINOR_VERSION;
}


/*!
  Return the size of a Newton dFloat in bytes.

  @return sizeof(dFloat)
*/
int NewtonWorldFloatSize ()
{
	TRACE_FUNCTION(__FUNCTION__);
	return sizeof (dFloat);
}


/*!
  Store a user defined data value with the world.

  @param *newtonWorld is the pointer to the newton world.
  @param *userData pointer to the user defined user data value.

  @return Nothing.

  The application can attach custom data to the Newton world. Newton will never
  look at this data.

  The user data is useful for application developing object oriented classes
  based on the Newton API.

  See also: ::NewtonBodyGetUserData, ::NewtonWorldSetUserData, ::NewtonWorldGetUserData
*/
void NewtonWorldSetUserData(const NewtonWorld* const newtonWorld, void* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetUserData (userData);
}

/*!
  Retrieve the user data attached to the world.

  @param *newtonWorld Pointer to the Newton world.

  @return Pointer to user data.

  See also: ::NewtonBodySetUserData, ::NewtonWorldSetUserData, ::NewtonWorldGetUserData
  */
void* NewtonWorldGetUserData(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetUserData();
}

/*!
  Specify a custom destructor callback for destroying the world.

  @param *newtonWorld Pointer to the Newton world.
  @param destructor function poiter callback

  The application may specify its own world destructor.

  See also: ::NewtonWorldSetDestructorCallback, ::NewtonWorldGetUserData
*/
void NewtonWorldSetDestructorCallback(const NewtonWorld* const newtonWorld, NewtonWorldDestructorCallback destructor)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->m_destructor =  destructor;
}


/*!
  Return pointer to destructor call back function.

  @param *newtonWorld Pointer to the Newton world.

  See also: ::NewtonWorldGetUserData, ::NewtonWorldSetDestructorCallback
*/
NewtonWorldDestructorCallback NewtonWorldGetDestructorCallback(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->m_destructor;
}


void NewtonWorldSetCollisionConstructorDestructorCallback (const NewtonWorld* const newtonWorld, NewtonCollisionCopyConstructionCallback constructor, NewtonCollisionDestructorCallback destructor)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetCollisionInstanceConstructorDestructor((dgWorld::OnCollisionInstanceDuplicate) constructor, (dgWorld::OnCollisionInstanceDestroy)destructor);
}


void* NewtonWorldGetListenerUserData (const NewtonWorld* const newtonWorld, void* const listener)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetListenerUserData (listener);
}

NewtonWorldListenerBodyDestroyCallback NewtonWorldListenerGetBodyDestroyCallback (const NewtonWorld* const newtonWorld, void* const listener)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return (NewtonWorldListenerBodyDestroyCallback) world->GetListenerBodyDestroyCallback (listener);
}

void NewtonWorldListenerSetBodyDestroyCallback (const NewtonWorld* const newtonWorld, void* const listener, NewtonWorldListenerBodyDestroyCallback bodyDestroyCallback)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->SetListenerBodyDestroyCallback (listener, (dgWorld::OnListenerBodyDestroyCallback) bodyDestroyCallback);
}


void* NewtonWorldAddListener (const NewtonWorld* const newtonWorld, const char* const nameId, void* const listenerUserData)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->AddListener (nameId, listenerUserData);
}

void NewtonWorldListenerSetDestructorCallback(const NewtonWorld* const newtonWorld, void* const listener, NewtonWorldDestroyListenerCallback destroy)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ListenerSetDestroyCallback(listener, (dgWorld::OnListenerDestroyCallback) destroy);
}

void NewtonWorldListenerSetPreUpdateCallback(const NewtonWorld* const newtonWorld, void* const listener, NewtonWorldUpdateListenerCallback update)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ListenerSetPreUpdate(listener, (dgWorld::OnListenerUpdateCallback) update);
}

void NewtonWorldListenerSetPostUpdateCallback(const NewtonWorld* const newtonWorld, void* const listener, NewtonWorldUpdateListenerCallback update)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ListenerSetPostUpdate(listener, (dgWorld::OnListenerUpdateCallback) update);
}


void* NewtonWorldGetListener (const NewtonWorld* const newtonWorld, const char* const nameId)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->FindListener (nameId);
}


void NewtonWorldListenerSetDebugCallback (const NewtonWorld* const newtonWorld, void* const listener, NewtonWorldListenerDebugCallback debugCallback)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->SetListenerBodyDebugCallback (listener, (dgWorld::OnListenerDebugCallback) debugCallback);
}

void NewtonWorldListenerDebug(const NewtonWorld* const newtonWorld, void* const context)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ListenersDebug(context);
}

/*!
  Return the total number of rigid bodies in the world.

  @param *newtonWorld Pointer to the Newton world.

  @return Number of rigid bodies in the world.

*/
int NewtonWorldGetBodyCount(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetBodiesCount();
}

/*!
  Return the total number of constraints in the world.

  @param *newtonWorld pointer to the Newton world.

  @return number of constraints.

*/
int NewtonWorldGetConstraintCount(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return world->GetConstraintsCount();
}


/*!
  Shoot ray from point p0 to p1 and trigger callback for each body on that line.

  @param *newtonWorld Pointer to the Newton world.
  @param *p0 - pointer to an array of at least three floats containing the beginning of the ray in global space.
  @param *p1 - pointer to an array of at least three floats containing the end of the ray in global space.
  @param filter Callback function for each hit during the ray scan.
  @param *userData user data to pass along to the filter callback.
  @param prefilter user defined function to be called for each body before intersection.
  @param threadIndex Index of thread that called this function (zero if called form outsize a newton update).

  @return nothing

  The ray cast function will trigger the callback for every intersection between
  the line segment (from p0 to p1) and a body in the world.

  By writing the callback filter function in different ways the application can
  implement different flavors of ray casting. For example an all body ray cast
  can be easily implemented by having the filter function always returning 1.0,
  and copying each rigid body into an array of pointers; a closest hit ray cast
  can be implemented by saving the body with the smaller intersection parameter
  and returning the parameter t; and a report the first body hit can be
  implemented by having the filter function returning zero after the first call
  and saving the pointer to the rigid body.

  The most common use for the ray cast function is the closest body hit, In this
  case it is important, for performance reasons, that the filter function
  returns the intersection parameter. If the filter function returns a value of
  zero the ray cast will terminate immediately.

  if prefilter is not NULL, Newton will call the application right before
  executing the intersections between the ray and the primitive. if the function
  returns zero the Newton will not ray cast the primitive. passing a NULL
  pointer will ray cast the. The application can use this implement faster or
  smarter filters when implementing complex logic, otherwise for normal all ray
  cast this parameter could be NULL.

  The ray cast function is provided as an utility function, this means that even
  thought the function is very high performance by function standards, it can
  not by batched and therefore it can not be an incremental function. For
  example the cost of calling 1000 ray cast is 1000 times the cost of calling
  one ray cast. This is much different than the collision system where the cost
  of calculating collision for 1000 pairs in much, much less that the 1000 times
  the cost of one pair. Therefore this function must be used with care, as
  excessive use of it can degrade performance.

  See also: ::NewtonWorldConvexCast
*/
void NewtonWorldRayCast(const NewtonWorld* const newtonWorld, const dFloat* const p0, const dFloat* const p1, NewtonWorldRayFilterCallback filter, void* const userData, NewtonWorldRayPrefilterCallback prefilter, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	if (filter) {
		dgVector pp0 (p0[0], p0[1], p0[2], dgFloat32 (0.0f));
		dgVector pp1 (p1[0], p1[1], p1[2], dgFloat32 (0.0f));
		Newton* const world = (Newton *) newtonWorld;
		world->GetBroadPhase()->RayCast (pp0, pp1, (OnRayCastAction) filter, (OnRayPrecastAction) prefilter, userData);
	}
}


/*!
  cast a simple convex shape along the ray that goes for the matrix position to the destination and get the firsts contacts of collision.

  @param *newtonWorld Pointer to the Newton world.
  @param *matrix pointer to an array of at least three floats containing the beginning and orienetaion of the shape in global space.
  @param *target pointer to an array of at least three floats containing the end of the ray in global space.
  @param shape collision shap[e use to cat the ray.
  @param param pointe to a variable the will contart the time to closet aproah to the collision.
  @param *userData user data to be passed to the prefilter callback.
  @param prefilter user define function to be called for each body before intersection.
  @param *info pointer to an array of contacts at the point of intesections.
  @param maxContactsCount maximun number of contacts to be conclaculated, the variable sould be initialized to the capaciaty of *info*
  @param threadIndex thread index from whe thsi function is called, zero if call form outsize a newton update

  @return the number of contact at the intesection point (a value equal o lower than maxContactsCount.
  variable *hitParam* will be set the uintesation parameter an the momen of impact.

  passing and value of NULL in *info* an dzero in maxContactsCount will turn thos function into a spcial Ray cast
  where the function will only calculate the *hitParam* at the momenet of contacts. tshi si one of the most effiecnet way to use thsio function.

  these function is similar to *NewtonWorldRayCast* but instead of casting a point it cast a simple convex shape along a ray for maoprix.m_poit
  to target position. the shape is global orientation and position is set to matrix and then is swept along the segment to target and it will stop at the very first intersession contact.

  for case where the application need to cast solid short to medium rays, it is better to use this function instead of casting and array of parallel rays segments.
  examples of these are: implementation of ray cast cars with cylindrical tires, foot placement of character controllers, kinematic motion of objects, user controlled continuous collision, etc.
  this function may not be as efficient as sampling ray for long segment, for these cases try using parallel ray cast.

  The most common use for the ray cast function is the closest body hit, In this case it is important, for performance reasons,
  that the filter function returns the intersection parameter. If the filter function returns a value of zero the ray cast will terminate
  immediately.

  if prefilter is not NULL, Newton will call the application right before executing the intersections between the ray and the primitive.
  if the function returns zero the Newton will not ray cast the primitive.
  The application can use this callback to implement faster or smarter filters when implementing complex logic, otherwise for normal all ray cast
  this parameter could be NULL.

  See also: ::NewtonWorldRayCast
*/
int NewtonWorldConvexCast(const NewtonWorld* const newtonWorld, const dFloat* const matrix, const dFloat* const target, const NewtonCollision* const shape, 
						  dFloat* const param, void* const userData, NewtonWorldRayPrefilterCallback prefilter, NewtonWorldConvexCastReturnInfo* const info, 
						  int maxContactsCount, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgVector destination (target[0], target[1], target[2], dgFloat32 (0.0f));
	Newton* const world = (Newton *) newtonWorld;
	return world->GetBroadPhase()->ConvexCast ((dgCollisionInstance*) shape, dgMatrix (matrix), destination, param, (OnRayPrecastAction) prefilter, userData, (dgConvexCastReturnInfo*)info, maxContactsCount, threadIndex);
}

int NewtonWorldCollide (const NewtonWorld* const newtonWorld, const dFloat* const matrix, const NewtonCollision* const shape, void* const userData,  
					   NewtonWorldRayPrefilterCallback prefilter, NewtonWorldConvexCastReturnInfo* const info, int maxContactsCount, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->GetBroadPhase()->Collide((dgCollisionInstance*)shape, dgMatrix(matrix), (OnRayPrecastAction)prefilter, userData, (dgConvexCastReturnInfo*)info, maxContactsCount, threadIndex);
}


/*!
  Retrieve body by index from island.

  @param island Pointer to simulation island.
  @param bodyIndex Index of body on current island.

  @return requested body. fixme: does it return NULL on error?

  This function can only be called from an island update callback.

  See also: ::NewtonSetIslandUpdateEvent
*/
NewtonBody* NewtonIslandGetBody(const void* const island, int bodyIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgWorld* const world = *(dgWorld**)island;
	return (NewtonBody*)world->GetClusterBody (island, bodyIndex);
}


/*!
  Return the AABB of the body on this island

  @param island Pointer to simulation island.
  @param bodyIndex index to the body in current island.
  @param p0 - fixme
  @param p1 - fixme

  This function can only be called from an island update callback.

  See also: ::NewtonSetIslandUpdateEvent
*/
void NewtonIslandGetBodyAABB(const void* const island, int bodyIndex, dFloat* const p0, dFloat* const p1)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody*) NewtonIslandGetBody(island, bodyIndex);
	if (body) {
		body->GetAABB ((dgVector&) *p0, (dgVector&) *p1);
	}
}

/*! @} */ // end of group World

/*! @defgroup GroupID GroupID
GroupID interface
@{
*/


/*!
  Get the value of the default MaterialGroupID.

  @param *newtonWorld pointer to the Newton world.

  @return The ID number for the default Group ID.

  Group IDs can be interpreted as the nodes of a dense graph. The edges of the graph are the physics materials.
  When the Newton world is created, the default Group ID is created by the engine.
  When bodies are created the application assigns a group ID to the body.
*/
int NewtonMaterialGetDefaultGroupID(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return int (world->GetDefualtBodyGroupID());
}


/*!
  Create a new MaterialGroupID.

  @param *newtonWorld pointer to the Newton world.

  @return The ID of a new GroupID.

  Group IDs can be interpreted as the nodes of a dense graph. The edges of the graph are the physics materials.
  When the Newton world is created, the default Group ID is created by the engine.
  When bodies are created the application assigns a group ID to the body.

  Note: The only way to destroy a Group ID after its creation is by destroying all the bodies and calling the function  *NewtonMaterialDestroyAllGroupID*.

  See also: ::NewtonMaterialDestroyAllGroupID
*/
int NewtonMaterialCreateGroupID(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return int (world->CreateBodyGroupID());
}

/*!
  Remove all groups ID from the Newton world.

  @param *newtonWorld pointer to the Newton world.

  @return Nothing.

  This function removes all groups ID from the Newton world.
  This function must be called after there are no more rigid bodies in the word.

  See also: ::NewtonDestroyAllBodies
*/
void NewtonMaterialDestroyAllGroupID(const NewtonWorld* const newtonWorld)
{
//	dgAssert (0);
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->	RemoveAllGroupID();
}

/*! @} */ // end of GroupID

/*! @defgroup MaterialSetup MaterialSetup
Material setup interface
@{
*/


/*!
  Set the material interaction between two physics materials  to be collidable or non-collidable by default.

  @param *newtonWorld pointer to the Newton world.
  @param  id0 - group id0
  @param  id1 - group id1
  @param state state for this material: 1 = collidable; 0 = non collidable

  @return Nothing.
*/
void NewtonMaterialSetDefaultCollidable(const NewtonWorld* const newtonWorld, int id0, int id1, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));
//	material->m_collisionEnable = state ? true : false;
	if (state) {
		material->m_flags |= dgContactMaterial::m_collisionEnable;
	} else {
		material->m_flags &= ~dgContactMaterial::m_collisionEnable;
	}
}


/*!
  Set an imaginary thickness between the collision geometry of two colliding bodies whose physics
  properties are defined by this material pair

  @param *newtonWorld pointer to the Newton world.
  @param  id0 - group id0
  @param  id1 - group id1
  @param thickness material thickness a value form 0.0 to 0.125; the default surface value is 0.0

  @return Nothing.

  when two bodies collide the engine resolve contact inter penetration by applying a small restoring
  velocity at each contact point. By default this restoring velocity will stop when the two contacts are
  at zero inter penetration distance. However by setting a non zero thickness the restoring velocity will
  continue separating the contacts until the distance between the two point of the collision geometry is equal
  to the surface thickness.

  Surfaces thickness can improve the behaviors of rolling objects on flat surfaces.

  Surface thickness does not alter the performance of contact calculation.
*/
void NewtonMaterialSetSurfaceThickness(const NewtonWorld* const newtonWorld, int id0, int id1, dFloat thickness)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	//material->m_skinThickness = dgMin (dgMax (thickness, dgFloat32 (0.0)), DG_MAX_COLLISION_AABB_PADDING * dgFloat32 (0.5f));
	material->m_skinThickness = dgClamp (thickness, dgFloat32 (0.0f), DG_MAX_COLLISION_AABB_PADDING * dgFloat32 (0.5f));
}


/*!
  Set the default coefficients of friction for the material interaction between two physics materials .

  @param *newtonWorld pointer to the Newton world.
  @param  id0 - group id0
  @param  id1 - group id1
  @param staticFriction static friction coefficients
  @param kineticFriction dynamic coefficient of friction

  @return Nothing.

  *staticFriction* and *kineticFriction* must be positive values. *kineticFriction* must be lower than *staticFriction*.
  It is recommended that *staticFriction* and *kineticFriction* be set to a value lower or equal to 1.0, however because some synthetic materials
  can have higher than one coefficient of friction Newton allows for the coefficient of friction to be as high as 2.0.
*/
void NewtonMaterialSetDefaultFriction(const NewtonWorld* const newtonWorld, int id0, int id1, dFloat staticFriction, dFloat kineticFriction)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	staticFriction = dgAbs (staticFriction);
	kineticFriction = dgAbs (kineticFriction);

	if (material) {
		if (staticFriction >= dgFloat32 (1.0e-2f)) {
			dFloat stat = dgClamp (staticFriction, dFloat(0.01f), dFloat(2.0f));
			dFloat kine = dgClamp (kineticFriction, dFloat(0.01f), dFloat(2.0f));
			stat = dgMax (stat, kine);
			material->m_staticFriction0 = stat;
			material->m_staticFriction1 = stat;
			material->m_dynamicFriction0 = kine;
			material->m_dynamicFriction1 = kine;
		} else {

			//material->m_friction0Enable = false;
			//material->m_friction1Enable = false;
			material->m_flags &= ~(dgContactMaterial::m_friction0Enable | dgContactMaterial::m_friction1Enable);
		}
	}
}


/*!
  Set the default coefficients of restitution (elasticity) for the material interaction between two physics materials .

  @param *newtonWorld pointer to the Newton world.
  @param  id0 - group id0
  @param  id1 - group id1
  @param elasticCoef static friction coefficients

  @return Nothing.

  *elasticCoef* must be a positive value.
  It is recommended that *elasticCoef* be set to a value lower or equal to 1.0
*/
void NewtonMaterialSetDefaultElasticity(const NewtonWorld* const newtonWorld, int id0, int id1, dFloat elasticCoef)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	material->m_restitution = dgClamp (elasticCoef, dFloat(0.01f), dFloat(2.0f));
}



/*!
  Set the default softness coefficients for the material interaction between two physics materials .

  @param *newtonWorld pointer to the Newton world.
  @param  id0 - group id0
  @param  id1 - group id1
  @param softnessCoef softness coefficient

  @return Nothing.

  *softnessCoef* must be a positive value.
  It is recommended that *softnessCoef* be set to value lower or equal to 1.0
  A low value for *softnessCoef* will make the material soft. A typical value for *softnessCoef* is 0.15
*/
void NewtonMaterialSetDefaultSoftness(const NewtonWorld* const newtonWorld, int id0, int id1, dFloat softnessCoef)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	material->m_softness = dgClamp (softnessCoef, dFloat(0.01f), dFloat(dgFloat32(1.0f)));
}

void NewtonMaterialSetCallbackUserData (const NewtonWorld* const newtonWorld, int id0, int id1, void* const userData)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = world->GetMaterial(dgUnsigned32(id0), dgUnsigned32(id1));
	material->SetUserData(userData);
}

/*!
  Set userData and the functions event handlers for the material interaction between two physics materials .

  @param *newtonWorld Pointer to the Newton world.
  @param  id0 - group id0.
  @param  id1 - group id1.
  @param *aabbOverlap address of the event function called when the AABB of tow bodyes overlap. This parameter can be NULL.
  @param *processCallback address of the event function called for every contact resulting from contact calculation. This parameter can be NULL.

  @return Nothing.

  When the AABB extend of the collision geometry of two bodies overlap, Newton collision system retrieves the material
  interaction that defines the behavior between the pair of bodies. The material interaction is collected from a database of materials,
  indexed by the material gruopID assigned to the bodies. If the material is tagged as non collidable,
  then no action is taken and the simulation continues.
  If the material is tagged as collidable, and a *aabbOverlap* was set for this material, then the *aabbOverlap* function is called.
  If the function  *aabbOverlap* returns 0, no further action is taken for this material (this can be use to ignore the interaction under
  certain conditions). If the function  *aabbOverlap* returns 1, Newton proceeds to calculate the array of contacts for the pair of
  colliding bodies. If the function *processCallback* was set, the application receives a callback for every contact found between the
  two colliding bodies. Here the application can perform fine grain control over the behavior of the collision system. For example,
  rejecting the contact, making the contact frictionless, applying special effects to the surface etc.
  After all contacts are processed and if the function *endCallback* was set, Newton calls *endCallback*.
  Here the application can collect information gathered during the contact-processing phase and provide some feedback to the player.
  A typical use for the material callback is to play sound effects. The application passes the address of structure in the *userData* along with
  three event function callbacks. When the function *aabbOverlap* is called by Newton, the application resets a variable say *maximumImpactSpeed*.
  Then for every call to the function *processCallback*, the application compares the impact speed for this contact with the value of
  *maximumImpactSpeed*, if the value is larger, then the application stores the new value along with the position, and any other quantity desired.
  When the application receives the call to *endCallback* the application plays a 3d sound based in the position and strength of the contact.
*/
void NewtonMaterialSetCollisionCallback(const NewtonWorld* const newtonWorld, int id0, int id1, NewtonOnAABBOverlap aabbOverlap, NewtonContactsProcess processCallback)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));
	material->SetCollisionCallback ((dgContactMaterial::OnAABBOverlap) aabbOverlap, (dgContactMaterial::OnContactCallback) processCallback);
}

void NewtonMaterialSetContactGenerationCallback (const NewtonWorld* const newtonWorld, int id0, int id1, NewtonOnContactGeneration contactGeneration)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = world->GetMaterial(dgUnsigned32(id0), dgUnsigned32(id1));
	material->SetCollisionGenerationCallback ((dgContactMaterial::OnContactGeneration) contactGeneration);
}

/*!
  Set userData and the functions event handlers for the material interaction between two physics materials .

  @param *newtonWorld Pointer to the Newton world.
  @param  id0 - group id0.
  @param  id1 - group id1.
  @param  *compoundAabbOverlap: fixme (can this be NULL?)

  @return Nothing.

  When the AABB extents of the collision geometry of two bodies overlap, the Newton collision system retrieves the material
  interaction that defines the behavior between the pair of bodies. The material interaction is collected from a database of materials,
  indexed by the material gruopID assigned to the bodies. If the material is tagged as non collidable,
  then no action is taken and the simulation continues.
  If the material is tagged as collidable, and a *aabbOverlap* was set for this material, then the *aabbOverlap* function is called.
  If the function  *aabbOverlap* returns 0, no further action is taken for this material (this can be use to ignore the interaction under
  certain conditions). If the function  *aabbOverlap* returns 1, Newton proceeds to calculate the array of contacts for the pair of
  colliding bodies. If the function *processCallback* was set, the application receives a callback for every contact found between the
  two colliding bodies. Here the application can perform fine grain control over the behavior of the collision system. For example,
  rejecting the contact, making the contact frictionless, applying special effects to the surface etc.
  After all contacts are processed and if the function *endCallback* was set, Newton calls *endCallback*.
  Here the application can collect information gathered during the contact-processing phase and provide some feedback to the player.
  A typical use for the material callback is to play sound effects. The application passes the address of structure in the *userData* along with
  three event function callbacks. When the function *aabbOverlap* is called by Newton, the application resets a variable say *maximumImpactSpeed*.
  Then for every call to the function *processCallback*, the application compares the impact speed for this contact with the value of
  *maximumImpactSpeed*, if the value is larger, then the application stores the new value along with the position, and any other quantity desired.
  When the application receives the call to *endCallback* the application plays a 3d sound based in the position and strength of the contact.
*/
void NewtonMaterialSetCompoundCollisionCallback(const NewtonWorld* const newtonWorld, int id0, int id1, NewtonOnCompoundSubCollisionAABBOverlap compoundAabbOverlap)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	material->SetCompoundCollisionCallback ((dgContactMaterial::OnCompoundCollisionPrefilter) compoundAabbOverlap);
}


/*!
  Get userData associated with this material.

  @param *newtonWorld Pointer to the Newton world.
  @param  id0 - group id0.
  @param  id1 - group id1.

  @return Nothing.
*/
void* NewtonMaterialGetUserData (const NewtonWorld* const newtonWorld, int id0, int id1)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgContactMaterial* const material = world->GetMaterial (dgUnsigned32 (id0), dgUnsigned32 (id1));

	return material->GetUserData();
}


/*!
  Get the first Material pair from the material array.

  @param *newtonWorld Pointer to the Newton world.

  @return the first material.

  See also: ::NewtonWorldGetNextMaterial
*/
NewtonMaterial* NewtonWorldGetFirstMaterial(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return (NewtonMaterial*) world->GetFirstMaterial ();
}

/*!
  Get the next Material pair from the material array.

  @param *newtonWorld Pointer to the Newton world.
  @param *material corrent material

  @return next material in material array or NULL if material is the last material in the list.

  See also: ::NewtonWorldGetFirstMaterial
*/
NewtonMaterial* NewtonWorldGetNextMaterial(const NewtonWorld* const newtonWorld, const NewtonMaterial* const material)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;

	return (NewtonMaterial*)world->GetNextMaterial ((dgContactMaterial*) material);
}

/*! @} */ // end of MaterialSetup


/*! @defgroup ContactBehaviour ContactBehaviour
Contact behavior control interface
@{
*/

/*!
  Get the userData set by the application when it created this material pair.

  @param materialHandle pointer to a material pair

  @return Application user data.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void* NewtonMaterialGetMaterialPairUserData(const NewtonMaterial* const materialHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	return material->GetUserData();
}

/*!
  Return the face attribute assigned to this face when for a user defined collision or a Newton collision tree.

  @param materialHandle pointer to a material pair

  @return face attribute for collision trees. Zero if the contact was generated by two convex collisions.

  This function can only be called from a material callback event handler.

  this function can be used by the application to retrieve the face id of a polygon for a collision tree.

  See also: ::NewtonMaterialSetCollisionCallback
*/
unsigned NewtonMaterialGetContactFaceAttribute(const NewtonMaterial* const materialHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
//	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
dgAssert (0);
//	return (unsigned) ((dgContactMaterial*) materialHandle->m_userId);
return 0;
}


/*!
  Calculate the speed of this contact along the normal vector of the contact.

  @param materialHandle pointer to a material pair

  @return Contact speed. A positive value means the contact is repulsive.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
dFloat NewtonMaterialGetContactNormalSpeed(const NewtonMaterial* const materialHandle)
{
//	dgAssert (0);

	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
//	contact = (dgContact*) contactlHandle;

	const dgBody* const body0 = material->m_body0;
	const dgBody* const body1 = material->m_body1;

	dgVector p0 (material->m_point - body0->GetPosition());
	dgVector p1 (material->m_point - body1->GetPosition());

	dgVector v0 (body0->GetVelocity() + body0->GetOmega().CrossProduct(p0));
	dgVector v1 (body1->GetVelocity() + body1->GetOmega().CrossProduct(p1));

	dgVector dv (v1 - v0);

	dFloat speed = dv.DotProduct3(material->m_normal);
	return speed;
}

/*!
  Calculate the speed of this contact along the tangent vector of the contact.

  @param materialHandle pointer to a material pair.
  @param index index to the tangent vector. This value can be 0 for primary tangent direction or 1 for the secondary tangent direction.

  @return Contact tangent speed.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
dFloat NewtonMaterialGetContactTangentSpeed(const NewtonMaterial* const materialHandle, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	const dgBody* const body0 = material->m_body0;
	const dgBody* const body1 = material->m_body1;

	dgVector p0 (material->m_point - body0->GetPosition());
	dgVector p1 (material->m_point - body1->GetPosition());

	dgVector v0 (body0->GetVelocity() + body0->GetOmega().CrossProduct(p0));
	dgVector v1 (body1->GetVelocity() + body1->GetOmega().CrossProduct(p1));

	dgVector dv (v1 - v0);
	dgVector dir (index ? material->m_dir1 : material->m_dir0);
	dFloat speed = dv.DotProduct3(dir);
	return - speed;
}


dFloat NewtonMaterialGetContactMaxNormalImpact(const NewtonMaterial* const materialHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	return material->m_normal_Force.m_impact;
}

dFloat NewtonMaterialGetContactMaxTangentImpact(const NewtonMaterial* const materialHandle, int index)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	return index ? material->m_dir1_Force.m_impact: material->m_dir0_Force.m_impact;
}


/*!
  Get the contact position and normal in global space.

  @param materialHandle pointer to a material pair.
  @param *body pointer to body
  @param *positPtr pointer to an array of at least three floats to hold the contact position.
  @param *normalPtr pointer to an array of at least three floats to hold the contact normal.

  @return Nothing.

  This function can only be called from a material callback event handle.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void NewtonMaterialGetContactPositionAndNormal(const NewtonMaterial* const materialHandle, const NewtonBody* const body, dFloat* const positPtr, dFloat* const normalPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	positPtr[0] = material->m_point.m_x;
	positPtr[1] = material->m_point.m_y;
	positPtr[2] = material->m_point.m_z;

	normalPtr[0] = material->m_normal.m_x;
	normalPtr[1] = material->m_normal.m_y;
	normalPtr[2] = material->m_normal.m_z;

	if ((dgBody*)body != material->m_body0) {
		normalPtr[0] *= dgFloat32 (-1.0f);
		normalPtr[1] *= dgFloat32 (-1.0f);
		normalPtr[2] *= dgFloat32 (-1.0f);
	}
}



/*!
  Get the contact force vector in global space.

  @param materialHandle pointer to a material pair.
  @param *body pointer to body
  @param *forcePtr pointer to an array of at least three floats to hold the force vector in global space.

  @return Nothing.

  The contact force value is only valid when calculating resting contacts. This means if two bodies collide with
  non zero relative velocity, the reaction force will be an impulse, which is not a reaction force, this will return zero vector.
  this function will only return meaningful values when the colliding bodies are at rest.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void NewtonMaterialGetContactForce(const NewtonMaterial* const materialHandle, const NewtonBody* const body, dFloat* const forcePtr)
{

	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	dgVector force (material->m_normal.Scale(material->m_normal_Force.m_force) + material->m_dir0.Scale (material->m_dir0_Force.m_force) + material->m_dir1.Scale (material->m_dir1_Force.m_force));

	forcePtr[0] = force.m_x;
	forcePtr[1] = force.m_y;
	forcePtr[2] = force.m_z;

	if ((dgBody*)body != material->m_body0) {
		forcePtr[0] *= dgFloat32 (-1.0f);
		forcePtr[1] *= dgFloat32 (-1.0f);
		forcePtr[2] *= dgFloat32 (-1.0f);
	}
}



/*!
  Get the contact tangent vector to the contact point.

  @param materialHandle pointer to a material pair.
  @param *body pointer to body
  @param  *dir0 - pointer to an array of at least three floats to hold the contact primary tangent vector.
  @param  *dir1 - pointer to an array of at least three floats to hold the contact secondary tangent vector.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void NewtonMaterialGetContactTangentDirections(const NewtonMaterial* const materialHandle, const NewtonBody* const body, dFloat* const dir0, dFloat* const dir1)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	dir0[0] = material->m_dir0.m_x;
	dir0[1] = material->m_dir0.m_y;
	dir0[2] = material->m_dir0.m_z;

	dir1[0] = material->m_dir1.m_x;
	dir1[1] = material->m_dir1.m_y;
	dir1[2] = material->m_dir1.m_z;

	if ((dgBody*)body != material->m_body0) {
		dir0[0] *= dgFloat32 (-1.0f);
		dir0[1] *= dgFloat32 (-1.0f);
		dir0[2] *= dgFloat32 (-1.0f);

		dir1[0] *= dgFloat32 (-1.0f);
		dir1[1] *= dgFloat32 (-1.0f);
		dir1[2] *= dgFloat32 (-1.0f);
	}
}

dFloat NewtonMaterialGetContactPenetration (const NewtonMaterial* const materialHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	return material->m_penetration;
}

NewtonCollision* NewtonMaterialGetBodyCollidingShape(const NewtonMaterial* const materialHandle, const NewtonBody* const body)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const bodyPtr = (dgBody*) body;
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	const dgCollisionInstance* collision = material->m_collision0; 
	if (bodyPtr == material->m_body1) {
		collision = material->m_collision1; 
	}
	return (NewtonCollision*) collision;
}


//dFloat NewtonMaterialGetContactPruningTolerance(const NewtonBody* const body0, const NewtonBody* const body1)
dFloat NewtonMaterialGetContactPruningTolerance(const NewtonJoint* const contactJointPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const contact = (dgContact*) contactJointPtr;
	return contact->GetPruningTolerance ();
}

void NewtonMaterialSetContactPruningTolerance(const NewtonJoint* const contactJointPtr, dFloat tolerance)
{
	TRACE_FUNCTION(__FUNCTION__);
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const contact = (dgContact*) contactJointPtr;
	dgAssert(contact);
	contact->SetPruningTolerance (dgMax (tolerance, dFloat (1.0e-3f)));
}


/*!
  Override the default softness value for the contact.

  @param materialHandle pointer to a material pair.
  @param softness softness value, must be positive.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialSetDefaultSoftness
*/
void NewtonMaterialSetContactSoftness(const NewtonMaterial* const materialHandle, dFloat softness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	material->m_softness = dgClamp (softness, dFloat(0.01f), dFloat(0.7f));
}

/*!
  Override the default contact skin thickness value for the contact.

  @param materialHandle pointer to a material pair.
  @param thickness skin thickness value, must be positive.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialSetDefaultSoftness
*/
void NewtonMaterialSetContactThickness (const NewtonMaterial* const materialHandle, dFloat thickness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgAssert (thickness >= dgFloat32 (0.0f));
	dgContactMaterial* const material = (dgContactMaterial*)materialHandle;
	material->m_skinThickness = thickness;
}

/*!
  Override the default elasticity (coefficient of restitution) value for the contact.

  @param materialHandle pointer to a material pair.
  @param restitution elasticity value, must be positive.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialSetDefaultElasticity
*/
void NewtonMaterialSetContactElasticity(const NewtonMaterial* const materialHandle, dFloat restitution)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	material->m_restitution = dgClamp (restitution, dFloat(0.01f), dFloat(2.0f));
}


/*!
  Enable or disable friction calculation for this contact.

  @param materialHandle pointer to a material pair.
  @param state* new state. 0 makes the contact frictionless along the index tangent vector.
  @param index index to the tangent vector. 0 for primary tangent vector or 1 for the secondary tangent vector.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void NewtonMaterialSetContactFrictionState(const NewtonMaterial* const materialHandle, int state, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	if (index) {
		if (state) {
			material->m_flags |= dgContactMaterial::m_friction1Enable;
		} else {
			material->m_flags &= ~dgContactMaterial::m_friction1Enable;
		}
	} else {
		if (state) {
			material->m_flags |= dgContactMaterial::m_friction0Enable;
		} else {
			material->m_flags &= ~dgContactMaterial::m_friction0Enable;
		}
	}
}



/*!
  Override the default value of the kinetic and static coefficient of friction for this contact.

  @param materialHandle pointer to a material pair.
  @param staticFrictionCoef static friction coefficient. Must be positive.
  @param kineticFrictionCoef static friction coefficient. Must be positive.
  @param index index to the tangent vector. 0 for primary tangent vector or 1 for the secondary tangent vector.

  @return Nothing.

  This function can only be called from a material callback event handler.

  It is recommended that *coef* be set to a value lower or equal to 1.0, however because some synthetic materials
  can have hight than one coefficient of friction Newton allows for the coefficient of friction to be as high as 2.0.

  the value *staticFrictionCoef* and *kineticFrictionCoef* will be clamped between 0.01f and 2.0.
  If the application wants to set a kinetic friction higher than the current static friction it must increase the static friction first.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialSetDefaultFriction
*/
void NewtonMaterialSetContactFrictionCoef(const NewtonMaterial* const materialHandle, dFloat staticFrictionCoef, dFloat kineticFrictionCoef, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	if (staticFrictionCoef < kineticFrictionCoef) {
		staticFrictionCoef = kineticFrictionCoef;
	}

	if (index) {
		material->m_staticFriction1 = dgClamp (staticFrictionCoef, dFloat(0.01f), dFloat(2.0f));
		material->m_dynamicFriction1 = dgClamp (kineticFrictionCoef, dFloat(0.01f), dFloat(2.0f));
	} else {
		material->m_staticFriction0 = dgClamp (staticFrictionCoef, dFloat(0.01f), dFloat(2.0f));
		material->m_dynamicFriction0 = dgClamp (kineticFrictionCoef, dFloat(0.01f), dFloat(2.0f));
	}
}

/*!
  Force the contact point to have a non-zero acceleration aligned this the contact normal.

  @param materialHandle pointer to a material pair.
  @param accel desired contact acceleration, Must be a positive value

  @return Nothing.

  This function can only be called from a material callback event handler.

  This function can be used for spacial effects like implementing jump, of explosive contact in a call back.

  See also: ::NewtonMaterialSetCollisionCallback
*/
void NewtonMaterialSetContactNormalAcceleration(const NewtonMaterial* const materialHandle, dFloat accel)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	dgAssert(0);
	material->m_normal_Force.m_force = accel;
	material->m_flags |= dgContactMaterial::m_overrideNormalAccel;
}

/*!
  Force the contact point to have a non-zero acceleration along the surface plane.

  @param materialHandle pointer to a material pair.
  @param accel desired contact acceleration.
  @param index index to the tangent vector. 0 for primary tangent vector or 1 for the secondary tangent vector.

  @return Nothing.

  This function can only be called from a material callback event handler.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialContactRotateTangentDirections
*/
void NewtonMaterialSetContactTangentAcceleration(const NewtonMaterial* const materialHandle, dFloat accel, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	if (index) {
		material->m_dir1_Force.m_force = accel;
		material->m_flags |= dgContactMaterial::m_override1Accel;
	} else {
		material->m_dir0_Force.m_force = accel;
		material->m_flags |= dgContactMaterial::m_override0Accel;
	}
}

void NewtonMaterialSetContactTangentFriction (const NewtonMaterial* const materialHandle, dFloat friction, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	friction = dgMax (dFloat(0.01f), dgAbs (friction));
	if (index) {
		material->m_flags |= dgContactMaterial::m_override1Friction;
		dgAssert (index == 1);
		material->m_staticFriction1 = friction;
		material->m_dynamicFriction1 = friction;
	} else {
		material->m_flags |= dgContactMaterial::m_override0Friction;
		material->m_staticFriction0 = friction;
		material->m_dynamicFriction0 = friction;
	}
}

/*!
  Set the new direction of the for this contact point.

  @param materialHandle pointer to a material pair.
  @param *direction pointer to an array of at least three floats holding the direction vector.

  @return Nothing.

  This function can only be called from a material callback event handler.
  This function changes the basis of the contact point to one where the contact normal is aligned to the new direction vector
  and the tangent direction are recalculated to be perpendicular to the new contact normal.

  In 99.9% of the cases the collision system can calculates a very good contact normal.
  however this algorithm that calculate the contact normal use as criteria the normal direction
  that will resolve the inter penetration with the least amount on motion.
  There are situations however when this solution is not the best. Take for example a rolling
  ball over a tessellated floor, when the ball is over a flat polygon, the contact normal is always
  perpendicular to the floor and pass by the origin of the sphere, however when the sphere is going
  across two adjacent polygons, the contact normal is now perpendicular to the polygons edge and this does
  not guarantee they it will pass bay the origin of the sphere, but we know that the best normal is always
  the one passing by the origin of the sphere.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialContactRotateTangentDirections
*/
void NewtonMaterialSetContactNormalDirection(const NewtonMaterial* const materialHandle, const dFloat* const direction)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;
	dgVector normal (direction[0], direction[1], direction[2], dgFloat32 (0.0f));

	dgAssert (normal.DotProduct3(material->m_normal) > dgFloat32 (0.01f));
	if (normal.DotProduct3(material->m_normal) < dgFloat32 (0.0f)) {
		normal = normal * dgVector::m_negOne;
	}
	material->m_normal = normal;

	dgMatrix matrix (normal);
	material->m_dir1 = matrix.m_up;
	material->m_dir0 = matrix.m_right;
}

void NewtonMaterialSetContactPosition(const NewtonMaterial* const materialHandle, const dFloat* const position)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*)materialHandle;
	dgVector point(position[0], position[1], position[2], dgFloat32(1.0f));
	material->m_point = point;
}


/*!
  Rotate the tangent direction of the contacts until the primary direction is aligned with the alignVector.

  @param *materialHandle pointer to a material pair.
  @param *alignVector pointer to an array of at least three floats holding the aligning vector.

  @return Nothing.

  This function can only be called from a material callback event handler.
  This function rotates the tangent vectors of the contact point until the primary tangent vector and the align vector
  are perpendicular (ex. when the dot product between the primary tangent vector and the alignVector is 1.0). This
  function can be used in conjunction with NewtonMaterialSetContactTangentAcceleration in order to
  create special effects. For example, conveyor belts, cheap low LOD vehicles, slippery surfaces, etc.

  See also: ::NewtonMaterialSetCollisionCallback, ::NewtonMaterialSetContactNormalDirection
*/
void NewtonMaterialContactRotateTangentDirections(const NewtonMaterial* const materialHandle, const dFloat* const alignVector)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContactMaterial* const material = (dgContactMaterial*) materialHandle;

	const dgVector dir0 (alignVector[0], alignVector[1], alignVector[2], dgFloat32 (0.0f));
    	
	dgVector dir1 (material->m_normal.CrossProduct(dir0));
	dgAssert(dir1.m_w == dgFloat32(0.0f));
	dFloat mag2 = dir1.DotProduct(dir1).GetScalar();
	if (mag2 > dgFloat32(1.0e-6f)) {
		material->m_dir1 = dir1.Normalize();
		material->m_dir0 = material->m_dir1.CrossProduct(material->m_normal);
	}
}

/*! @} */ // end of ContactBehaviour

/*! @defgroup CshapesConvexSimple CshapesConvexSimple
Convex collision primitives interface
@{
*/


/*!
  Create a transparent collision primitive.

  @param *newtonWorld Pointer to the Newton world.

  @return Pointer to the collision object.

  Some times the application needs to create helper rigid bodies that will never collide with other bodies,
  for example the neck of a rag doll, or an internal part of an articulated structure. This can be done by using the material system
  but it too much work and it will increase unnecessarily the material count, and therefore the project complexity. The Null collision
  is a collision object that satisfy all this conditions without having to change the engine philosophy.

*/
NewtonCollision* NewtonCreateNull(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return (NewtonCollision*) world->CreateNull();
}


/*!
  Create a box primitive for collision.

  @param *newtonWorld Pointer to the Newton world.
  @param dx box side one x dimension.
  @param dy box side one y dimension.
  @param dz box side one z dimension.
  @param shapeID fixme
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the box

*/
NewtonCollision* NewtonCreateBox(const NewtonWorld* const newtonWorld, dFloat dx, dFloat dy, dFloat dz, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix (dgGetIdentityMatrix());
	if (offsetMatrix) {
		 matrix = dgMatrix (offsetMatrix);
	}
	return (NewtonCollision*) world->CreateBox (dx, dy, dz, shapeID, matrix);
}

/*!
  Create a generalized ellipsoid primitive..

  @param *newtonWorld Pointer to the Newton world.
  @param radius sphere radius
  @param shapeID user specified collision index that can be use for multi material collision.  
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the sphere relative to the body. If this parameter is NULL then the sphere is centered at the origin of the body.

  @return Pointer to the generalized sphere.

  Sphere collision are generalized ellipsoids, the application can create many different kind of objects by just playing with dimensions of the radius.
  for example to make a sphere set all tree radius to the same value, to make a ellipse of revolution just set two of the tree radius to the same value.

  General ellipsoids are very good hull geometries to represent the outer shell of avatars in a game.

*/
NewtonCollision* NewtonCreateSphere(const NewtonWorld* const newtonWorld, dFloat radius, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix (dgGetIdentityMatrix());
	if (offsetMatrix) {
		 matrix = dgMatrix (offsetMatrix);
	}

	return (NewtonCollision*) world->CreateSphere(dgAbs(radius), shapeID, matrix);
}


/*!
  Create a cone primitive for collision.

  @param *newtonWorld Pointer to the Newton world.
  @param radius cone radius at the base.
  @param height cone height along the x local axis from base to tip.
  @param shapeID user specified collision index that can be use for multi material collision.
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the box

*/
NewtonCollision* NewtonCreateCone(const NewtonWorld* const newtonWorld, dFloat radius, dFloat height, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix (dgGetIdentityMatrix());
	if (offsetMatrix) {
		 matrix = dgMatrix (offsetMatrix);
	}
	return (NewtonCollision*) world->CreateCone (radius, height, shapeID, matrix);
}


/*!
  Create a capsule primitive for collision.

  @param *newtonWorld Pointer to the Newton world.
  @param  radio0 - fixme
  @param  radio1 - fixme
  @param height capsule height along the x local axis from tip to tip.
  @param shapeID fixme
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the box

  the capsule height must equal of larger than the sum of the cap radius. If this is not the case the height will be clamped the 2 * radius.

*/
NewtonCollision* NewtonCreateCapsule(const NewtonWorld* const newtonWorld, dFloat radio0, dFloat radio1, dFloat height, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix(dgGetIdentityMatrix());
	if (offsetMatrix) {
		matrix = dgMatrix(offsetMatrix);
	}
	return (NewtonCollision*)world->CreateCapsule(radio0, radio1, height, shapeID, matrix);
}


/*!
  Create a cylinder primitive for collision.

  @param *newtonWorld Pointer to the Newton world.
  @param  radio0 - fixme
  @param  radio1 - fixme
  @param height cylinder height along the x local axis.
  @param shapeID fixme
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the box

*/
NewtonCollision* NewtonCreateCylinder(const NewtonWorld* const newtonWorld, dFloat radio0, dFloat radio1, dFloat height, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix(dgGetIdentityMatrix());
	if (offsetMatrix) {
		matrix = dgMatrix(offsetMatrix);
	}
	return (NewtonCollision*)world->CreateCylinder(radio0, radio1, height, shapeID, matrix);
}


/*!
  Create a ChamferCylinder primitive for collision.

  @param *newtonWorld Pointer to the Newton world.
  @param radius ChamferCylinder radius at the base.
  @param height ChamferCylinder height along the x local axis.
  @param shapeID fixme
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the box

*/
NewtonCollision* NewtonCreateChamferCylinder(const NewtonWorld* const newtonWorld, dFloat radius, dFloat height, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix (dgGetIdentityMatrix());
	if (offsetMatrix) {
		 matrix = dgMatrix (offsetMatrix);
	}
	return (NewtonCollision*) world->CreateChamferCylinder (radius, height, shapeID, matrix);
}


/*!
  Create a ConvexHull primitive from collision from a cloud of points.

  @param *newtonWorld Pointer to the Newton world.
  @param count number of consecutive point to follow must be at least 4.
  @param *vertexCloud pointer to and array of point.
  @param strideInBytes vertex size in bytes, must be at least 12.
  @param tolerance tolerance value for the hull generation.
  @param shapeID fixme
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix of the box relative to the body. If this parameter is NULL, then the primitive is centered at the origin of the body.

  @return Pointer to the collision mesh, NULL if the function fail to generate convex shape

  Convex hulls are the solution to collision primitive that can not be easily represented by an implicit solid.
  The implicit solid primitives (spheres, cubes, cylinders, capsules, cones, etc.), have constant time complexity for contact calculation
  and are also extremely efficient on memory usage, therefore the application get perfect smooth behavior.
  However for cases where the shape is too difficult or a polygonal representation is desired, convex hulls come closest to the to the model shape.
  For example it is a mistake to model a 10000 point sphere as a convex hull when the perfect sphere is available, but it is better to represent a
  pyramid by a convex hull than with a sphere or a box.

  There is not upper limit as to how many vertex the application can pass to make a hull shape,
  however for performance and memory usage concern it is the application responsibility to keep the max vertex at the possible minimum.
  The minimum number of vertex should be equal or larger than 4 and it is the application responsibility that the points are part of a solid geometry.
  Unpredictable results will occur if all points happen to be collinear or coplanar.

  The performance of collision with convex hull proxies is sensitive to the vertex count of the hull. Since a the convex hull
  of a visual geometry is already an approximation of the mesh, for visual purpose there is not significant difference between the
  appeal of a exact hull and one close to the exact hull but with but with a smaller vertex count.
  It just happens that sometime complex meshes lead to generation of convex hulls with lots of small detail that play not
  roll of the quality of the simulation but that have a significant impact on the performance because of a large vertex count.
  For this reason the application have the option to set a *tolerance* parameter.
  *tolerance* is use to post process the final geometry in the following faction, a point on the surface of the hull can
  be remove if the distance of all of the surrounding vertex immediately adjacent to the average plane equation formed the
  faces adjacent to that point, is smaller than the tolerance. A value of zero in *tolerance* will generate an exact hull and a value langer that zero
  will generate a loosely fitting hull and it willbe faster to generate.

*/
NewtonCollision* NewtonCreateConvexHull(const NewtonWorld* const newtonWorld, int count, const dFloat* const vertexCloud, int strideInBytes, dgFloat32 tolerance, int shapeID, const dFloat* const offsetMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMatrix matrix (dgGetIdentityMatrix());
	if (offsetMatrix) {
		 matrix = dgMatrix (offsetMatrix);
	}
	tolerance = dgClamp (tolerance, dgFloat32 (0.0f), dgFloat32 (0.125f));
	return (NewtonCollision*) world->CreateConvexHull (count, vertexCloud, strideInBytes, tolerance, shapeID, matrix);
}


/*!
  Create a ConvexHull primitive from a special effect mesh.

  @param *newtonWorld Pointer to the Newton world.
  @param *mesh special effect mesh
  @param tolerance tolerance value for the hull generation.
  @param shapeID fixme

  @return Pointer to the collision mesh, NULL if the function fail to generate convex shape

  Because the in general this function is used for runtime special effect like debris and or solid particles
  it is recommended that the source mesh complexity is kept small.

  See also: ::NewtonCreateConvexHull, ::NewtonMeshCreate
*/
NewtonCollision* NewtonCreateConvexHullFromMesh(const NewtonWorld* const newtonWorld, const NewtonMesh* const mesh, dFloat tolerance, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	return (NewtonCollision*) meshEffect->CreateConvexCollision(world, tolerance, shapeID);
}


/*!
  Create a container to hold an array of convex collision primitives.

  @param *newtonWorld Pointer to the Newton world.
  @param  shapeID: fixme

  @return Pointer to the compound collision.

  Compound collision primitives can only be made of convex collision primitives and they can not contain compound collision. Therefore they are treated as convex primitives.

  Compound collision primitives are treated as instance collision objects that can not shared by multiples rigid bodies.

*/
NewtonCollision* NewtonCreateCompoundCollision(const NewtonWorld* const newtonWorld, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgCollisionInstance* const collision = world->CreateCompound ();
	collision->SetUserDataID(dgUnsigned32 (shapeID));
	return (NewtonCollision*) collision;
}

void* NewtonCompoundCollisionAddSubCollision (NewtonCollision* const compoundCollision, const NewtonCollision* const convexCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	
	dgCollisionInstance* const compoundInstance = (dgCollisionInstance*) compoundCollision;
	dgCollisionInstance* const compoundInstanceChild = (dgCollisionInstance*) convexCollision;
	if (compoundInstance->IsType (dgCollision::dgCollisionCompound_RTTI) && compoundInstanceChild->IsType(dgCollision::dgCollisionConvexShape_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) compoundInstance->GetChildShape();
		return collision->AddCollision (compoundInstanceChild);
	}
	return NULL;
}


void NewtonCompoundCollisionRemoveSubCollision (NewtonCollision* const compoundCollision, const void* const collisionNode)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		dgCollisionInstance* const childCollision = collision->GetCollisionFromNode((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode);
		if (childCollision && childCollision->IsType(dgCollision::dgCollisionConvexShape_RTTI)) {
			collision->RemoveCollision ((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode);
		}
	}
}

void NewtonCompoundCollisionRemoveSubCollisionByIndex (NewtonCollision* const compoundCollision, int nodeIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		NewtonCompoundCollisionRemoveSubCollision (compoundCollision, collision->FindNodeByIndex(nodeIndex));
	}
}


void NewtonCompoundCollisionSetSubCollisionMatrix (NewtonCollision* const compoundCollision, const void* const collisionNode, const dFloat* const matrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const compoundInstance = (dgCollisionInstance*) compoundCollision;
	if (compoundInstance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) compoundInstance->GetChildShape();
		collision->SetCollisionMatrix((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode, dgMatrix(matrix));
	}
}


void NewtonCompoundCollisionBeginAddRemove (NewtonCollision* const compoundCollision)	
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		collision->BeginAddRemove();
	}
}

void NewtonCompoundCollisionEndAddRemove (NewtonCollision* const compoundCollision)	
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		collision->EndAddRemove();
	}
}


void* NewtonCompoundCollisionGetFirstNode (NewtonCollision* const compoundCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		return collision->GetFirstNode();
	}
	return NULL;
}

void* NewtonCompoundCollisionGetNextNode (NewtonCollision* const compoundCollision, const void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		return collision->GetNextNode((dgCollisionCompound::dgTreeArray::dgTreeNode*)node);
	}
	return NULL;
}

void* NewtonCompoundCollisionGetNodeByIndex (NewtonCollision* const compoundCollision, int index)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		return collision->FindNodeByIndex(index);
	}
	return NULL;
}

int NewtonCompoundCollisionGetNodeIndex (NewtonCollision* const compoundCollision, const void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) compoundCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		return collision->GetNodeIndex((dgCollisionCompound::dgTreeArray::dgTreeNode*)node);
	}
	return -1;
}


NewtonCollision* NewtonCompoundCollisionGetCollisionFromNode (NewtonCollision* const compoundCollision, const void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const compoundInstance = (dgCollisionInstance*) compoundCollision;
	if (compoundInstance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) compoundInstance->GetChildShape();
		return (NewtonCollision*) collision->GetCollisionFromNode((dgCollisionCompound::dgTreeArray::dgTreeNode*)node);
	}
	return NULL;
}


/*!
  Create a compound collision from a concave mesh by an approximate convex partition

  @param *newtonWorld Pointer to the Newton world.
  @param *convexAproximation fixme
  @param hullTolerance fixme
  @param shapeID fixme
  @param subShapeID fixme


  @return Pointer to the compound collision.

  The algorithm will separated the the original mesh into a series of sub meshes until either
  the worse concave point is smaller than the specified min concavity or the max number convex shapes is reached.

  is is recommended that convex approximation are made by person with a graphics toll by physically overlaying collision primitives over the concave mesh.
  but for quit test of maybe for simple meshes and algorithm approximations can be used.

  is is recommended that for best performance this function is used in an off line toll and serialize the output.

  Compound collision primitives are treated as instanced collision objects that cannot be shared by multiples rigid bodies.

*/
NewtonCollision* NewtonCreateCompoundCollisionFromMesh (const NewtonWorld* const newtonWorld, const NewtonMesh* const convexAproximation, dFloat hullTolerance, int shapeID, int subShapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonCollision* const compound = NewtonCreateCompoundCollision(newtonWorld, shapeID);
	NewtonCompoundCollisionBeginAddRemove(compound);

	NewtonMesh* nextSegment = NULL;
	for (NewtonMesh* segment = NewtonMeshCreateFirstSingleSegment (convexAproximation); segment; segment = nextSegment) {
		nextSegment = NewtonMeshCreateNextSingleSegment (convexAproximation, segment);

		NewtonCollision* const convexHull = NewtonCreateConvexHullFromMesh (newtonWorld, segment, hullTolerance, subShapeID);
		if (convexHull) {
			NewtonCompoundCollisionAddSubCollision (compound, convexHull);
			NewtonDestroyCollision(convexHull);
		}
		NewtonMeshDestroy(segment);
	}

	NewtonCompoundCollisionEndAddRemove(compound);

	return compound;
}


NewtonCollision* NewtonCreateFracturedCompoundCollision (const NewtonWorld* const newtonWorld, const NewtonMesh* const solidMesh, int shapeID, int fracturePhysicsMaterialID, int pointcloudCount, const dFloat* const vertexCloud, int strideInBytes, int materialID, const dFloat* const textureMatrix,
														 NewtonFractureCompoundCollisionReconstructMainMeshCallBack regenerateMainMeshCallback, 
														 NewtonFractureCompoundCollisionOnEmitCompoundFractured emitFracturedCompound, NewtonFractureCompoundCollisionOnEmitChunk emitFracfuredChunk)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *)newtonWorld;
	dgMeshEffect* const mesh = (dgMeshEffect*) solidMesh;

	dgMatrix textMatrix (textureMatrix);
	dgCollisionInstance* const collision = world->CreateFracturedCompound (mesh, shapeID, fracturePhysicsMaterialID, pointcloudCount, vertexCloud, strideInBytes, materialID, textMatrix, 
																		  (dgCollisionCompoundFractured::OnEmitFractureChunkCallBack) emitFracfuredChunk,
																		  (dgCollisionCompoundFractured::OnEmitNewCompundFractureCallBack) emitFracturedCompound,
																		  (dgCollisionCompoundFractured::OnReconstructFractureMainMeshCallBack) regenerateMainMeshCallback);
	return (NewtonCollision*) collision;
}

NewtonCollision* NewtonFracturedCompoundPlaneClip (const NewtonCollision* const fracturedCompound, const dFloat* const plane)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		dgWorld* const world = (dgWorld*)collision->GetWorld();
		dgCollisionCompoundFractured* const newCompound = compound->PlaneClip(dgVector (plane[0], plane[1], plane[2], plane[3]));
		if (newCompound) {
			dgCollisionInstance* const newCollision = world->CreateInstance (newCompound, collision->GetUserDataID(), dgGetIdentityMatrix());
			newCompound->Release();
			return (NewtonCollision*)newCollision;
		}
	}
	return NULL;
}

void NewtonFracturedCompoundSetCallbacks (const NewtonCollision* const fracturedCompound, 
										  NewtonFractureCompoundCollisionReconstructMainMeshCallBack regenerateMainMeshCallback, 
										  NewtonFractureCompoundCollisionOnEmitCompoundFractured emitFracturedCompound, NewtonFractureCompoundCollisionOnEmitChunk emitFracfuredChunk)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		compound->SetCallbacks ((dgCollisionCompoundFractured::OnEmitFractureChunkCallBack) emitFracfuredChunk, (dgCollisionCompoundFractured::OnEmitNewCompundFractureCallBack) emitFracturedCompound, (dgCollisionCompoundFractured::OnReconstructFractureMainMeshCallBack) regenerateMainMeshCallback);
	}
}


int NewtonFracturedCompoundNeighborNodeList (const NewtonCollision* const fracturedCompound, void* const collisionNode, void** const nodesArray, int maxCount)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		return  compound->GetFirstNiegborghArray ((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode, (dgCollisionCompound::dgTreeArray::dgTreeNode**) nodesArray, maxCount);
	}
	return 0;
}



int NewtonFracturedCompoundIsNodeFreeToDetach (const NewtonCollision* const fracturedCompound, void* const collisionNode)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		return compound->IsNodeSaseToDetach((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode) ? 1 : 0;
	}
	return 0;
}

NewtonFracturedCompoundMeshPart* NewtonFracturedCompoundGetFirstSubMesh(const NewtonCollision* const fracturedCompound)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	NewtonFracturedCompoundMeshPart* mesh = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		mesh = (NewtonFracturedCompoundMeshPart*) compound->GetFirstMesh();
	}
	return mesh;
}

NewtonFracturedCompoundMeshPart* NewtonFracturedCompoundGetNextSubMesh(const NewtonCollision* const fracturedCompound, NewtonFracturedCompoundMeshPart* const subMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	NewtonFracturedCompoundMeshPart* mesh = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		mesh = (NewtonFracturedCompoundMeshPart*) compound->GetNextMesh((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) subMesh);
	}
	return mesh;
}

NewtonFracturedCompoundMeshPart* NewtonFracturedCompoundGetMainMesh (const NewtonCollision* const fracturedCompound)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	NewtonFracturedCompoundMeshPart* mesh = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		mesh = (NewtonFracturedCompoundMeshPart*) compound->GetMainMesh();
	}
	return mesh;
}


int NewtonFracturedCompoundCollisionGetVertexCount (const NewtonCollision* const fracturedCompound, const NewtonFracturedCompoundMeshPart* const meshOwner)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	dgInt32 count = 0;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		count = compound->GetVertecCount((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) meshOwner);
	}
	return count;
}


const dFloat* NewtonFracturedCompoundCollisionGetVertexPositions (const NewtonCollision* const fracturedCompound, const NewtonFracturedCompoundMeshPart* const meshOwner)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	const dgFloat32* points = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		points = compound->GetVertexPositions((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) meshOwner);
	}
	return points;
}


const dFloat* NewtonFracturedCompoundCollisionGetVertexNormals (const NewtonCollision* const fracturedCompound, const NewtonFracturedCompoundMeshPart* const meshOwner)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	const dgFloat32* points = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		points = compound->GetVertexNormal((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) meshOwner);
	}
	return points;
}

const dFloat* NewtonFracturedCompoundCollisionGetVertexUVs (const NewtonCollision* const fracturedCompound, const NewtonFracturedCompoundMeshPart* const meshOwner)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;

	const dgFloat32* points = NULL;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision->GetChildShape();
		points = compound->GetVertexUVs((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) meshOwner);
	}
	return points;
}

int NewtonFracturedCompoundMeshPartGetIndexStream (const NewtonCollision* const fracturedCompound, const NewtonFracturedCompoundMeshPart* const meshOwner, const void* const segment, int* const index) 
{
	TRACE_FUNCTION(__FUNCTION__);

	dgInt32 count = 0;
	dgCollisionInstance* const collision = (dgCollisionInstance*) fracturedCompound;
	if (collision->IsType (dgCollision::dgCollisionCompoundBreakable_RTTI)) {
		dgCollisionCompoundFractured* const compound = (dgCollisionCompoundFractured*) collision;
		count = compound->GetSegmentIndexStream ((dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) meshOwner, (dgCollisionCompoundFractured::dgMesh::dgListNode*) segment, index);
	}
	return count;
}


void* NewtonFracturedCompoundMeshPartGetFirstSegment (const NewtonFracturedCompoundMeshPart* const breakableComponentMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionCompoundFractured::dgConectivityGraph::dgListNode* const node = (dgCollisionCompoundFractured::dgConectivityGraph::dgListNode*) breakableComponentMesh;
	return node->GetInfo().m_nodeData.m_mesh->GetFirst();
}

void* NewtonFracturedCompoundMeshPartGetNextSegment (const void* const breakableComponentSegment)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionCompoundFractured::dgMesh::dgListNode* const node = (dgCollisionCompoundFractured::dgMesh::dgListNode*) breakableComponentSegment;
	return node->GetNext();
}

int NewtonFracturedCompoundMeshPartGetMaterial (const void* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionCompoundFractured::dgMesh::dgListNode* const node = (dgCollisionCompoundFractured::dgMesh::dgListNode*) segment;
	return node->GetInfo().m_material;
}


int NewtonFracturedCompoundMeshPartGetIndexCount (const void* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionCompoundFractured::dgMesh::dgListNode* const node = (dgCollisionCompoundFractured::dgMesh::dgListNode*) segment;
	return node->GetInfo().m_faceCount * 3;
}





// Return the trigger volume flag of this shape.
//
// @param convexCollision is the pointer to a convex collision primitive.
// 
// @return 0 if collision shape is solid, non zero is collision shape is a trigger volume.
//
// this function can be used to place collision triggers in the scene. 
// Setting this flag is not really a necessary to place a collision trigger however this option hint the engine that 
// this particular shape is a trigger volume and no contact calculation is desired.
int NewtonCollisionGetMode(const NewtonCollision* const convexCollision)
{
	dgCollisionInstance* const collision = (dgCollisionInstance*) convexCollision;
	TRACE_FUNCTION(__FUNCTION__);
	return collision->GetCollisionMode() ? 1 : 0;
}


// Set a flag on a convex collision shape to indicate that no contacts should calculated for this shape.
//
// @param convexCollision is the pointer to a convex collision primitive.
// @param triggerMode 1 disable contact calculation, 0 enable contact calculation.
// 
// @return nothing
//
// this function can be used to place collision triggers in the scene. 
// Setting this flag is not really a necessary to place a collision trigger however this option hint the engine that 
// this particular shape is a trigger volume and no contact calculation is desired.
//
void NewtonCollisionSetMode (const NewtonCollision* const convexCollision, int mode)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) convexCollision;
	collision->SetCollisionMode(mode ? true : false);
}

/*
void NewtonCollisionSetMaxBreakImpactImpulse(const NewtonCollision* const convexHullCollision, dFloat maxImpactImpulse)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) convexHullCollision;
	collision->SetBreakImpulse(dgFloat32 (maxImpactImpulse));
}

dFloat NewtonCollisionGetMaxBreakImpactImpulse(const NewtonCollision* const convexHullCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) convexHullCollision;
	return dgFloat32 (collision->GetBreakImpulse());
}
*/

int NewtonConvexHullGetVertexData (const NewtonCollision* const convexHullCollision, dFloat** const vertexData, int* strideInBytes)
{
	dgAssert (0);
	return 0;
}


/*!
  Return the number of vertices of face and copy each index into array faceIndices.

  @param convexHullCollision is the pointer to a convex collision hull primitive.
  @param face fixme
  @param faceIndices fixme

  @return user face count of face.

  this function will return zero on all shapes other than a convex full collision shape.

  To get the number of faces of a convex hull shape see function *NewtonCollisionGetInfo*

  See also: ::NewtonCollisionGetInfo, ::NewtonCreateConvexHull
*/
int NewtonConvexHullGetFaceIndices(const NewtonCollision* const convexHullCollision, int face, int* const faceIndices)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const coll = (dgCollisionInstance*) convexHullCollision;
	
	if (coll->IsType (dgCollision::dgCollisionConvexHull_RTTI)) {
		//return ((dgCollisionConvexHull*)coll)->GetFaceIndices (face, faceIndices);
		return ((dgCollisionConvexHull*)coll->GetChildShape())->GetFaceIndices (face, faceIndices);
	} else {
		return 0;
	}
}

/*!
  calculate the total volume defined by a convex collision geometry.

  @param *convexCollision pointer to the collision.

  @return collision geometry volume. This function will return zero if the body collision geometry is no convex.

  The total volume calculated by the function is only an approximation of the ideal volume. This is not an error, it is a fact resulting from the polygonal representation of convex solids.

  This function can be used to assist the application in calibrating features like fluid density weigh factor when calibrating buoyancy forces for more realistic result.
*/
dFloat NewtonConvexCollisionCalculateVolume(const NewtonCollision* const convexCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)convexCollision;
	return collision->GetVolume();
}


/*!
  Calculate the three principal axis and the the values of the inertia matrix of a convex collision objects.

  @param convexCollision is the pointer to a convex collision primitive.
  @param *inertia pointer to and array of a least 3 floats to hold the values of the principal inertia.
  @param *origin pointer to and array of a least 3 floats to hold the values of the center of mass for the principal inertia.

  This function calculate a general inertial matrix for arbitrary convex collision including compound collisions.

  See also: ::NewtonBodySetMassMatrix, ::NewtonBodyGetMass, ::NewtonBodySetCentreOfMass, ::NewtonBodyGetCentreOfMass
*/
void NewtonConvexCollisionCalculateInertialMatrix(const NewtonCollision* convexCollision, dFloat* const inertia, dFloat* const origin)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)convexCollision;

//	dgVector tmpInertia;
//	dgVector tmpOringin;
//	collision->CalculateInertia(tmpInertia, tmpOringin);
	dgMatrix tmpInertia (collision->CalculateInertia());

	inertia[0] = tmpInertia[0][0];
	inertia[1] = tmpInertia[1][1];
	inertia[2] = tmpInertia[2][2];
	origin[0] = tmpInertia[3][0];
	origin[1] = tmpInertia[3][1];
	origin[2] = tmpInertia[3][2];
}


/*!
  Add buoyancy force and torque for bodies immersed in a fluid.

  @param convexCollision fixme
  @param matrix fixme
  @param shapeOrigin fixme
  @param *gravityVector pointer to an array of floats containing the gravity vector.
  @param fluidPlane fixme
  @param fluidDensity fluid density.
  @param fluidViscosity fluid linear viscosity (resistance to linear translation).
  @param accel fixme
  @param alpha fixme

  @return Nothing.

  This function is only effective when called from *NewtonApplyForceAndTorque callback*

  This function adds buoyancy force and torque to a body when it is immersed in a fluid.
  The force is calculated according to Archimedes Buoyancy Principle. When the parameter *buoyancyPlane* is set to NULL, the body is considered
  to completely immersed in the fluid. This can be used to simulate boats and lighter than air vehicles etc..

  If *buoyancyPlane* return 0 buoyancy calculation for this collision primitive is ignored, this could be used to filter buoyancy calculation
  of compound collision geometry with different IDs.

  See also: ::NewtonConvexCollisionCalculateVolume
*/
void NewtonConvexCollisionCalculateBuoyancyAcceleration (const NewtonCollision* const convexCollision, const dFloat* const matrix, const dFloat* const shapeOrigin, const dFloat* const gravityVector, const dFloat* const fluidPlane, dFloat fluidDensity, dFloat fluidViscosity, dFloat* const accel, dFloat* const alpha)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionInstance* const instance = (dgCollisionInstance*)convexCollision;
	
	dgVector origin (shapeOrigin);
	dgVector gravity (gravityVector);
	dgVector plane (fluidPlane[0], fluidPlane[1], fluidPlane[2], fluidPlane[3]);

	dgVector force;
	dgVector torque;
	instance->CalculateBuoyancyAcceleration (dgMatrix (matrix), origin, gravity, plane, fluidDensity, fluidViscosity, force, torque);

	accel[0] = force.m_x;
	accel[1] = force.m_y;
	accel[2] = force.m_z;

	alpha[0] = torque.m_x;
	alpha[1] = torque.m_y;
	alpha[2] = torque.m_z;
}



const void* NewtonCollisionDataPointer (const NewtonCollision* const convexCollision)
{
	dgCollisionInstance* const coll = (dgCollisionInstance*) convexCollision;
	return coll->GetChildShape();
}

/*! @} */ // end of CshapesConvexSimple

/*! @defgroup CshapesConvexComplex CshapesConvexComplex
Complex collision primitives interface
@{
*/


/*!
  Create a complex collision geometry to be controlled by the application.

  @param *newtonWorld Pointer to the Newton world.
  @param *minBox pointer to an array of at least three floats to hold minimum value for the box relative to the collision.
  @param *maxBox pointer to an array of at least three floats to hold maximum value for the box relative to the collision.
  @param *userData pointer to user data to be used as context for event callback.
  @param collideCallback pointer to an event function for providing Newton with the polygon inside a given box region.
  @param rayHitCallback pointer to an event function for providing Newton with ray intersection information.
  @param destroyCallback pointer to an event function for destroying any data allocated for use by the application.
  @param getInfoCallback fixme
  @param getAABBOverlapTestCallback fixme
  @param facesInAABBCallback fixme
  @param serializeCallback fixme
  @param shapeID fixme

  @return Pointer to the user collision.

  *UserMeshCollision* provides the application with a method of overloading the built-in collision system for background objects.
  UserMeshCollision can be used for implementing collisions with height maps, collisions with BSP, and any other collision structure the application
  supports and wishes to preserve.
  However, *UserMeshCollision* can not take advantage of the efficient and sophisticated algorithms and data structures of the
  built-in *TreeCollision*. We suggest you experiment with both methods and use the method best suited to your situation.

  When a *UserMeshCollision* is assigned to a body, the mass of the body is ignored in all dynamics calculations.
  This make the body behave as a static body.

*/
NewtonCollision* NewtonCreateUserMeshCollision(
	const NewtonWorld* const newtonWorld, 
	const dFloat* const minBox, 
	const dFloat* const maxBox, 
	void* const userData,
	NewtonUserMeshCollisionCollideCallback collideCallback, 
	NewtonUserMeshCollisionRayHitCallback rayHitCallback,
	NewtonUserMeshCollisionDestroyCallback destroyCallback,
	NewtonUserMeshCollisionGetCollisionInfo getInfoCallback, 
	NewtonUserMeshCollisionAABBTest getAABBOverlapTestCallback,
	NewtonUserMeshCollisionGetFacesInAABB facesInAABBCallback,
	NewtonOnUserCollisionSerializationCallback serializeCallback,
	int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgVector p0 (minBox[0], minBox[1], minBox[2], dgFloat32(1.0f)); 
	dgVector p1 (maxBox[0], maxBox[1], maxBox[2], dgFloat32(1.0f)); 

	Newton* const world = (Newton *)newtonWorld;

	dgUserMeshCreation data;
	data.m_userData = userData; 
	data.m_collideCallback = (dgCollisionUserMesh::OnUserMeshCollideCallback) collideCallback; 
	data.m_rayHitCallback = (dgCollisionUserMesh::OnUserMeshRayHitCallback) rayHitCallback; 
	data.m_destroyCallback = (dgCollisionUserMesh::OnUserMeshDestroyCallback) destroyCallback;
	data.m_getInfoCallback = (dgCollisionUserMesh::OnUserMeshCollisionInfo)getInfoCallback;
	data.m_getAABBOvelapTestCallback = (dgCollisionUserMesh::OnUserMeshAABBOverlapTest) getAABBOverlapTestCallback;
	data.m_faceInAABBCallback = (dgCollisionUserMesh::OnUserMeshFacesInAABB) facesInAABBCallback;
	data.m_serializeCallback = (dgCollisionUserMesh::OnUserMeshSerialize) serializeCallback;
	

	dgCollisionInstance* const collision = world->CreateStaticUserMesh (p0, p1, data);
	collision->SetUserDataID(dgUnsigned32 (shapeID));
	return (NewtonCollision*)collision; 
}



int NewtonUserMeshCollisionContinuousOverlapTest (const NewtonUserMeshCollisionCollideDesc* const collideDescData, const void* const rayHandle, const dFloat* const minAabb, const dFloat* const maxAabb)
{
	const dgFastRayTest* const ray = (dgFastRayTest*) rayHandle;

	dgVector p0 (minAabb);
	dgVector p1 (maxAabb);

	dgVector q0 (collideDescData->m_boxP0);
	dgVector q1 (collideDescData->m_boxP1);

	dgVector box0 (p0 - q1);
	dgVector box1 (p1 - q0);

	dgFloat32 dist = ray->BoxIntersect(box0, box1);
	return (dist < dgFloat32 (1.0f)) ? 1 : 0;
}



/*!
  Create an empty complex collision geometry tree.

  @param *newtonWorld Pointer to the Newton world.
  @param shapeID fixme

  @return Pointer to the collision tree.

  *TreeCollision* is the preferred method within Newton for collision with polygonal meshes of arbitrary complexity.
  The mesh must be made of flat non-intersecting polygons, but they do not explicitly need to be triangles.
  *TreeCollision* can be serialized by the application to/from an arbitrary storage device.

  When a *TreeCollision* is assigned to a body the mass of the body is ignored in all dynamics calculations.
  This makes the body behave as a static body.

  See also: ::NewtonTreeCollisionBeginBuild, ::NewtonTreeCollisionAddFace, ::NewtonTreeCollisionEndBuild, ::NewtonStaticCollisionSetDebugCallback, ::NewtonTreeCollisionGetFaceAttribute, ::NewtonTreeCollisionSetFaceAttribute
*/
NewtonCollision* NewtonCreateTreeCollision(const NewtonWorld* const newtonWorld, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgCollisionInstance* const collision =  world->CreateBVH ();
	collision->SetUserDataID(dgUnsigned32 (shapeID));
	return (NewtonCollision*) collision;
}


NewtonCollision* NewtonCreateTreeCollisionFromMesh (const NewtonWorld* const newtonWorld, const NewtonMesh* const mesh, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	dgCollisionInstance* const collision =  meshEffect->CreateCollisionTree(world, shapeID);
	return (NewtonCollision*) collision;
}


/*!
  set a function call back to be call during the face query of a collision tree.

  @param *staticCollision is the pointer to the static collision (a CollisionTree of a HeightFieldCollision)
  @param *userCallback pointer to an event function to call before Newton evaluates the polygons colliding with a body. This parameter can be NULL.

  because debug display display report all the faces of a collision primitive, it could get slow on very large static collision.
  this function can be used for debugging purpose to just report only faces intersection the collision AABB of the collision shape colliding with the polyginal mesh collision.

  this function is not recommended to use for production code only for debug purpose.

  See also: ::NewtonTreeCollisionGetFaceAttribute, ::NewtonTreeCollisionSetFaceAttribute
*/
void NewtonStaticCollisionSetDebugCallback(const NewtonCollision* const staticCollision, NewtonTreeCollisionCallback userCallback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)staticCollision;
	if (collision->IsType (dgCollision::dgCollisionMesh_RTTI)) {
		dgCollisionMesh* const mesh = (dgCollisionMesh*) collision->GetChildShape();
		mesh->SetDebugCollisionCallback ((dgCollisionMeshCollisionCallback) userCallback);
	}

}

/*!
  set a function call back to be called during the face query of a collision tree.

  @param *treeCollision is the pointer to the collision tree.
  @param rayHitCallback pointer to an event function for providing Newton with ray intersection information.

  In general a ray cast on a collision tree will stops at the first intersections with the closest face in the tree
  that was hit by the ray. In some cases the application may be interested in the intesation with faces other than the fiorst hit.
  In this cases the application can set this alternate callback and the ray scanner will notify the application of each face hit by the ray scan.

  since this function faces the ray scanner to visit all of the potential faces intersected by the ray,
  setting the function call back make the ray casting on collision tree less efficient than the default behavior.
  So it is this functionality is only recommended for cases were the application is using especial effects like transparencies, or other effects

  calling this function with *rayHitCallback* = NULL will rest the collision tree to it default raycast mode, which is return with the closest hit.

  when *rayHitCallback* is not null then the callback is dalled with the follwing arguments
  *const NewtonCollisio* collision - pointer to the collision tree
  interseption - inetstion parameters of the ray
  *normal - unnormalized face mormal in the space fo eth parent of the collision.
  faceId -  id of this face in the collision tree.

  See also: ::NewtonTreeCollisionGetFaceAttribute, ::NewtonTreeCollisionSetFaceAttribute
*/
void NewtonTreeCollisionSetUserRayCastCallback(const NewtonCollision* const treeCollision, NewtonCollisionTreeRayCastCallback rayHitCallback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)treeCollision;
//	dgCollisionBVH* const collision = (dgCollisionBVH*) treeCollision;
	if (collision->IsType (dgCollision::dgCollisionBVH_RTTI)) {
		dgCollisionBVH* const shape = (dgCollisionBVH*) collision->GetChildShape();
		shape->SetCollisionRayCastCallback ((dgCollisionBVHUserRayCastCallback) rayHitCallback);
	}
}


void NewtonHeightFieldSetUserRayCastCallback (const NewtonCollision* const heightField, NewtonHeightFieldRayCastCallback rayHitCallback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)heightField;
	if (collision->IsType (dgCollision::dgCollisionHeightField_RTTI)) {
		dgCollisionHeightField* const shape = (dgCollisionHeightField*) collision->GetChildShape();
		shape->SetCollisionRayCastCallback ((dgCollisionHeightFieldRayCastCallback) rayHitCallback);
	}
}

void NewtonHeightFieldSetHorizontalDisplacement (const NewtonCollision* const heightField, const unsigned short* const horizontalMap, dFloat scale)
{
	dgCollisionInstance* const collision = (dgCollisionInstance*)heightField;
	if (collision->IsType(dgCollision::dgCollisionHeightField_RTTI)) {
		dgCollisionHeightField* const shape = (dgCollisionHeightField*)collision->GetChildShape();
		shape->SetHorizontalDisplacement (horizontalMap, dgFloat32 (scale));
	}
}

/*!
  Prepare a *TreeCollision* to begin to accept the polygons that comprise the collision mesh.

  @param *treeCollision is the pointer to the collision tree.

  @return Nothing.

  See also: ::NewtonTreeCollisionAddFace, ::NewtonTreeCollisionEndBuild
*/
void NewtonTreeCollisionBeginBuild(const NewtonCollision* const treeCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));

	collision->BeginBuild();
}

/*!
  Add an individual polygon to a *TreeCollision*.

  @param *treeCollision is the pointer to the collision tree.
  @param vertexCount number of vertex in *vertexPtr*
  @param *vertexPtr pointer to an array of vertex. The vertex should consist of at least 3 floats each.
  @param strideInBytes size of each vertex in bytes. This value should be 12 or larger.
  @param faceAttribute id that identifies the polygon. The application can use this value to customize the behavior of the collision geometry.

  @return Nothing.

  After the call to *NewtonTreeCollisionBeginBuild* the *TreeCollision* is ready to accept polygons. The application should iterate
  through the application's mesh, adding the mesh polygons to the *TreeCollision* one at a time.
  The polygons must be flat and non-self intersecting.

  See also: ::NewtonTreeCollisionAddFace, ::NewtonTreeCollisionEndBuild
*/
void NewtonTreeCollisionAddFace(const NewtonCollision* const treeCollision, int vertexCount, const dFloat* const vertexPtr, int strideInBytes, int faceAttribute)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));
	collision->AddFace(vertexCount, vertexPtr, strideInBytes, faceAttribute);
}

/*!
  Finalize the construction of the polygonal mesh.

  @param *treeCollision is the pointer to the collision tree.
  @param optimize flag that indicates to Newton whether it should optimize this mesh. Set to 1 to optimize the mesh, otherwise 0.

  @return Nothing.


  After the application has finished adding polygons to the *TreeCollision*, it must call this function to finalize the construction of the collision mesh.
  If concave polygons are added to the *TreeCollision*, the application must call this function with the parameter *optimize* set to 1.
  With the *optimize* parameter set to 1, Newton will optimize the collision mesh by removing non essential edges from adjacent flat polygons.
  Newton will not change the topology of the mesh but significantly reduces the number of polygons in the mesh. The reduction factor of the number of polygons in the mesh depends upon the irregularity of the mesh topology.
  A reduction factor of 1.5 to 2.0 is common.
  Calling this function with the parameter *optimize* set to zero, will leave the mesh geometry unaltered.

  See also: ::NewtonTreeCollisionAddFace, ::NewtonTreeCollisionEndBuild
*/
void NewtonTreeCollisionEndBuild(const NewtonCollision* const treeCollision, int optimize)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));
	collision->EndBuild(optimize);
}


/*!
  Get the user defined collision attributes stored with each face of the collision mesh.

  @param treeCollision fixme
  @param *faceIndexArray pointer to the face index list passed to the function *NewtonTreeCollisionCallback userCallback
  @param indexCount fixme

  @return User id of the face.

  This function is used to obtain the user data stored in faces of the collision geometry.
  The application can use this user data to achieve per polygon material behavior in large static collision meshes.

  See also: ::NewtonTreeCollisionSetFaceAttribute, ::NewtonCreateTreeCollision
*/
int NewtonTreeCollisionGetFaceAttribute(const NewtonCollision* const treeCollision, const int* const faceIndexArray, int indexCount)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));

	return int (collision->GetTagId (faceIndexArray, indexCount));
}

/*!
  Change the user defined collision attribute stored with faces of the collision mesh.

  @param *treeCollision fixme
  @param *faceIndexArray pointer to the face index list passed to the NewtonTreeCollisionCallback function
  @param indexCount fixme
  @param attribute value of the user defined attribute to be stored with the face.

  @return User id of the face.

  This function is used to obtain the user data stored in faces of the collision geometry.
  The application can use this user data to achieve per polygon material behavior in large static collision meshes.
  By changing the value of this user data the application can achieve modifiable surface behavior with the collision geometry.
  For example, in a driving game, the surface of a polygon that represents the street can changed from pavement to oily or wet after
  some collision event occurs.

  See also: ::NewtonTreeCollisionGetFaceAttribute, ::NewtonCreateTreeCollision
*/
void NewtonTreeCollisionSetFaceAttribute(const NewtonCollision* const treeCollision, const int* const faceIndexArray, int indexCount, int attribute)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));

	collision->SetTagId (faceIndexArray, indexCount, dgUnsigned32 (attribute));
}

void NewtonTreeCollisionForEachFace (const NewtonCollision* const treeCollision, NewtonTreeCollisionFaceCallback forEachFaceCallback, void* const context) 
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionBVH* const collision = (dgCollisionBVH*) ((dgCollisionInstance*)treeCollision)->GetChildShape();
	dgAssert (collision->IsType (dgCollision::dgCollisionBVH_RTTI));

	collision->ForEachFace ((dgAABBIntersectCallback) forEachFaceCallback, context);
}



/*!
  collect the vertex list index list mesh intersecting the AABB in collision mesh.

  @param *treeCollision fixme
  @param  *p0 - pointer to an array of at least three floats representing the ray origin in the local space of the geometry.
  @param  *p1 - pointer to an array of at least three floats representing the ray end in the local space of the geometry.
  @param **vertexArray pointer to a the vertex array of vertex.
  @param *vertexCount pointer int to return the number of vertex in vertexArray.
  @param *vertexStrideInBytes pointer to int to return the size of each vertex in vertexArray.
  @param *indexList pointer to array on integers containing the triangles intersection the aabb.
  @param maxIndexCount maximum number of indices the function will copy to indexList.
  @param *faceAttribute pointer to array on integers top contain the face containing the .

  @return the number of triangles in indexList.

  indexList should be a list 3 * maxIndexCount the number of elements.

  faceAttributet should be a list maxIndexCount the number of elements.

  this function could be used by the application for many purposes.
  for example it can be used to draw the collision geometry intersecting a collision primitive instead
  of drawing the entire collision tree in debug mode.
  Another use for this function is to to efficient draw projective texture shadows.
*/
int NewtonTreeCollisionGetVertexListTriangleListInAABB(const NewtonCollision* const treeCollision, const dFloat* const p0, const dFloat* const p1,
													const dFloat** const vertexArray, int* const vertexCount, int* const vertexStrideInBytes, 
													const int* const indexList, int maxIndexCount, const int* const faceAttribute) 
{
	TRACE_FUNCTION(__FUNCTION__);

	dgInt32 count = 0;
	dgCollisionInstance* meshColl = (dgCollisionInstance*) treeCollision;
	if (meshColl->IsType (dgCollision::dgCollisionMesh_RTTI)) {
		dgCollisionMesh* const collision = (dgCollisionMesh*) ((dgCollisionInstance*)treeCollision)->GetChildShape();

		dgVector pmin (p0[0], p0[1], p0[2], dgFloat32 (0.0f));
		dgVector pmax (p1[0], p1[1], p1[2], dgFloat32 (0.0f));

		dgCollisionMesh::dgMeshVertexListIndexList data;
		data.m_indexList = (dgInt32 *)indexList;
		data.m_userDataList = (dgInt32 *)faceAttribute;
		data.m_maxIndexCount = maxIndexCount;
		data.m_triangleCount = 0; 
		collision->GetVertexListIndexList (pmin, pmax, data);

		count = data.m_triangleCount;
		*vertexArray = data.m_veterxArray; 
		*vertexCount = data.m_vertexCount;
		*vertexStrideInBytes = data.m_vertexStrideInBytes; 
	}
	return count;
}


/*!
  Create a height field collision geometry.

  @param *newtonWorld Pointer to the Newton world.
  @param width the number of sample points in the x direction (fixme)
  @param height the number of sample points in the y direction (fixme)
  @param gridsDiagonals fixme
  @param elevationdatType fixme
  @param elevationMap array holding elevation data of size = width*height (fixme)
  @param attributeMap array holding attribute data of size = width*height (fixme)
  @param verticalScale scale of the elevation (fixme)
  @param horizontalScale scale in the xy direction. (fixme)
  @param shapeID fixme

  @return Pointer to the collision.

  NewtonCollision* NewtonCreateHeightFieldCollision(const NewtonWorld* const newtonWorld, int width, int height, int cellsDiagonals,
  const dFloat* const elevationMap, const char* const atributeMap,
  dFloat horizontalScale, int shapeID)
*/
  NewtonCollision* NewtonCreateHeightFieldCollision (const NewtonWorld* const newtonWorld, int width, int height, int gridsDiagonals, dgInt32 elevationdatType,
													 const void* const elevationMap, const char* const attributeMap, dFloat verticalScale, dFloat horizontalScale_x, dFloat horizontalScale_z, int shapeID)
{
	Newton* const world = (Newton *)newtonWorld;

	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = world->CreateHeightField(width, height, gridsDiagonals, elevationdatType, elevationMap, (const dgInt8* const) attributeMap, verticalScale, horizontalScale_x, horizontalScale_z);
	collision->SetUserDataID(dgUnsigned32 (shapeID));
	return (NewtonCollision*) collision;
}



/*!
  Create a height field collision geometry.

  @param *newtonWorld Pointer to the Newton world.
  @param shapeID fixme

  @return Pointer to the collision.

*/
NewtonCollision* NewtonCreateSceneCollision (const NewtonWorld* const newtonWorld, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;

	dgCollisionInstance* const collision = world->CreateScene ();

	collision->SetUserDataID(dgUnsigned32 (shapeID));
	return (NewtonCollision*) collision; 
}


void NewtonSceneCollisionBeginAddRemove (NewtonCollision* const sceneCollision)	
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonCompoundCollisionBeginAddRemove (sceneCollision);
}

void NewtonSceneCollisionEndAddRemove (NewtonCollision* const sceneCollision)	
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonCompoundCollisionEndAddRemove (sceneCollision);
}


void NewtonSceneCollisionSetSubCollisionMatrix (NewtonCollision* const sceneCollision, const void* const collisionNode, const dFloat* const matrix)	
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonCompoundCollisionSetSubCollisionMatrix (sceneCollision, collisionNode, matrix);
}


void* NewtonSceneCollisionAddSubCollision (NewtonCollision* const sceneCollision, const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionInstance* const sceneInstance = (dgCollisionInstance*) sceneCollision;
	dgCollisionInstance* const sceneInstanceChild = (dgCollisionInstance*) collision;
	if (sceneInstance->IsType (dgCollision::dgCollisionScene_RTTI) && !sceneInstanceChild->IsType(dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionScene* const collision1 = (dgCollisionScene*) sceneInstance->GetChildShape();
		return collision1->AddCollision (sceneInstanceChild);
	}
	return NULL;
}

void NewtonSceneCollisionRemoveSubCollision (NewtonCollision* const sceneCollision, const void* const collisionNode)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const sceneInstance = (dgCollisionInstance*) sceneCollision;
	if (sceneInstance->IsType (dgCollision::dgCollisionScene_RTTI)) {
		dgCollisionScene* const collision = (dgCollisionScene*) sceneInstance->GetChildShape();
		dgCollisionInstance* const childCollision = collision->GetCollisionFromNode((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode);
		if (childCollision) {
			collision->RemoveCollision ((dgCollisionCompound::dgTreeArray::dgTreeNode*)collisionNode);
		}
	}
}

void NewtonSceneCollisionRemoveSubCollisionByIndex (NewtonCollision* const sceneCollision, int nodeIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) sceneCollision;
	if (instance->IsType (dgCollision::dgCollisionCompound_RTTI)) {
		dgCollisionCompound* const collision = (dgCollisionCompound*) instance->GetChildShape();
		NewtonSceneCollisionRemoveSubCollision (sceneCollision, collision->FindNodeByIndex(nodeIndex));
	}
}


void* NewtonSceneCollisionGetNodeByIndex (NewtonCollision* const sceneCollision, int index)
{
	TRACE_FUNCTION(__FUNCTION__);
	return NewtonCompoundCollisionGetNodeByIndex (sceneCollision, index);
}

int NewtonSceneCollisionGetNodeIndex (NewtonCollision* const sceneCollision, const void* const collisionNode)
{
	TRACE_FUNCTION(__FUNCTION__);
	return NewtonCompoundCollisionGetNodeIndex (sceneCollision, collisionNode);
}


NewtonCollision* NewtonSceneCollisionGetCollisionFromNode (NewtonCollision* const sceneCollision, const void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	return NewtonCompoundCollisionGetCollisionFromNode (sceneCollision, node);
}

void* NewtonSceneCollisionGetFirstNode (NewtonCollision* const sceneCollision)
{
	TRACE_FUNCTION(__FUNCTION__);
	return NewtonCompoundCollisionGetFirstNode (sceneCollision);
}

void* NewtonSceneCollisionGetNextNode (NewtonCollision* const sceneCollision, const void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	return NewtonCompoundCollisionGetNextNode (sceneCollision, node);
}


/*! @} */ // end of CshapesConvexComples


/*! @defgroup CollisionLibraryGeneric CollisionLibraryGeneric
Generic collision library functions
@{
*/

/*!
  Calculate the closest point between a point and convex collision primitive.

  @param *newtonWorld Pointer to the Newton world.
  @param *point pointer to and array of a least 3 floats representing the origin.
  @param *collision pointer to collision primitive.
  @param *matrix pointer to an array of 16 floats containing the offset matrix of collision primitiveA.
  @param *contact pointer to and array of a least 3 floats to contain the closest point to collisioA.
  @param *normal pointer to and array of a least 3 floats to contain the separating vector normal.
  @param  threadIndex -Thread index form where the call is made from, zeor otherwize

  @return one if the two bodies are disjoint and the closest point could be found,
  zero if the point is inside the convex primitive.

  This function can be used as a low-level building block for a stand-alone collision system.
  Applications that have already there own physics system, and only want and quick and fast collision solution,
  can use Newton advanced collision engine as the low level collision detection part.
  To do this the application only needs to initialize Newton, create the collision primitives at application discretion,
  and just call this function when the objects are in close proximity. Applications using Newton as a collision system
  only, are responsible for implementing their own broad phase collision determination, based on any high level tree structure.
  Also the application should implement their own trivial aabb test, before calling this function .

  the current implementation of this function do work on collision trees, or user define collision.

  See also: ::NewtonCollisionCollideContinue, ::NewtonCollisionClosestPoint, ::NewtonCollisionCollide, ::NewtonCollisionRayCast, ::NewtonCollisionCalculateAABB
*/
int NewtonCollisionPointDistance(const NewtonWorld* const newtonWorld, const dFloat* const point,
								 const NewtonCollision* const collision, const dFloat* const matrix,
								 dFloat* const contact, dFloat* const normal, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ClosestPoint (*((dgTriplex*) point), (dgCollisionInstance*)collision, dgMatrix (matrix), *((dgTriplex*) contact), *((dgTriplex*) normal), threadIndex);
}


/*!
  Calculate the closest points between two disjoint convex collision primitive.

  @param *newtonWorld Pointer to the Newton world.
  @param *collisionA pointer to collision primitive A.
  @param *matrixA pointer to an array of 16 floats containing the offset matrix of collision primitiveA.
  @param *collisionB pointer to collision primitive B.
  @param *matrixB pointer to an array of 16 floats containing the offset matrix of collision primitiveB.
  @param *contactA pointer to and array of a least 3 floats to contain the closest point to collisionA.
  @param *contactB pointer to and array of a least 3 floats to contain the closest point to collisionB.
  @param *normalAB pointer to and array of a least 3 floats to contain the separating vector normal.
  @param  threadIndex -Thread index form where the call is made from, zeor otherwize

  @return one if the tow bodies are disjoint and he closest point could be found,
  zero if the two collision primitives are intersecting.

  This function can be used as a low-level building block for a stand-alone collision system.
  Applications that have already there own physics system, and only want and quick and fast collision solution,
  can use Newton advanced collision engine as the low level collision detection part.
  To do this the application only needs to initialize Newton, create the collision primitives at application discretion,
  and just call this function when the objects are in close proximity. Applications using Newton as a collision system
  only, are responsible for implementing their own broad phase collision determination, based on any high level tree structure.
  Also the application should implement their own trivial aabb test, before calling this function .

  the current implementation of this function does not work on collision trees, or user define collision.

  See also: ::NewtonCollisionCollideContinue, ::NewtonCollisionPointDistance, ::NewtonCollisionCollide, ::NewtonCollisionRayCast, ::NewtonCollisionCalculateAABB
*/
int NewtonCollisionClosestPoint(const NewtonWorld* const newtonWorld, 
								const NewtonCollision* const collisionA, const dFloat* const matrixA,
								const NewtonCollision* const collisionB, const dFloat* const matrixB,
								dFloat* const contactA, dFloat* const contactB, dFloat* const normalAB, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->ClosestPoint ((dgCollisionInstance*)collisionA, dgMatrix (matrixA), 
								(dgCollisionInstance*)collisionB, dgMatrix (matrixB), 
								*((dgTriplex*) contactA), *((dgTriplex*) contactB), *((dgTriplex*) normalAB), threadIndex);
}


int NewtonCollisionIntersectionTest (const NewtonWorld* const newtonWorld, const NewtonCollision* const collisionA, const dFloat* const matrixA, const NewtonCollision* const collisionB, const dFloat* const matrixB, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->IntersectionTest ((dgCollisionInstance*)collisionA, dgMatrix (matrixA), 
									(dgCollisionInstance*)collisionB, dgMatrix (matrixB), 
									threadIndex) ? 1 : 0;
}


/*!
  Calculate contact points between two collision primitive.

  @param *newtonWorld Pointer to the Newton world.
  @param maxSize size of maximum number of elements in contacts, normals, and penetration.
  @param *collisionA pointer to collision primitive A.
  @param *matrixA pointer to an array of 16 floats containing the offset matrix of collision primitiveA.
  @param *collisionB pointer to collision primitive B.
  @param *matrixB pointer to an array of 16 floats containing the offset matrix of collision primitiveB.
  @param *contacts pointer to and array of a least 3 times maxSize floats to contain the collision contact points.
  @param *normals pointer to and array of a least 3 times maxSize floats to contain the collision contact normals.
  @param *penetration pointer to and array of a least maxSize floats to contain the collision penetration at each contact.
  @param attributeA fixme
  @param attributeB fixme
  @param threadIndex Thread index form where the call is made from, zeor otherwize

  @return the number of contact points.

  This function can be used as a low-level building block for a stand-alone collision system.
  Applications that have already there own physics system, and only want and quick and fast collision solution,
  can use Newton advanced collision engine as the low level collision detection part.
  To do this the application only needs to initialize Newton, create the collision primitives at application discretion,
  and just call this function when the objects are in close proximity. Applications using Newton as a collision system
  only, are responsible for implementing their own broad phase collision determination, based on any high level tree structure.
  Also the application should implement their own trivial aabb test, before calling this function .

  See also: ::NewtonCollisionCollideContinue, ::NewtonCollisionClosestPoint, ::NewtonCollisionPointDistance, ::NewtonCollisionRayCast, ::NewtonCollisionCalculateAABB
*/
int NewtonCollisionCollide (const NewtonWorld* const newtonWorld, int maxSize,
						   const NewtonCollision* const collisionA, const dFloat* const matrixA, 
						   const NewtonCollision* const collisionB, const dFloat* const matrixB,
						   dFloat* const contacts, dFloat* const normals, dFloat* const penetration, 
						   dLong* const attributeA, dLong* const attributeB, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return world->Collide ((dgCollisionInstance*)collisionA, dgMatrix (matrixA), 
						   (dgCollisionInstance*)collisionB, dgMatrix (matrixB), 
						   (dgTriplex*) contacts, (dgTriplex*) normals, penetration, attributeA, attributeB, maxSize, threadIndex);
}



/*!
  Calculate time of impact of impact and contact points between two collision primitive.

  @param *newtonWorld Pointer to the Newton world.
  @param maxSize size of maximum number of elements in contacts, normals, and penetration.
  @param timestep maximum time interval considered for the continuous collision calculation.
  @param *collisionA pointer to collision primitive A.
  @param *matrixA pointer to an array of 16 floats containing the offset matrix of collision primitiveA.
  @param *velocA pointer to and array of a least 3 times maxSize floats containing the linear velocity of collision primitiveA.
  @param *omegaA pointer to and array of a least 3 times maxSize floats containing the angular velocity of collision primitiveA.
  @param *collisionB pointer to collision primitive B.
  @param *matrixB pointer to an array of 16 floats containing the offset matrix of collision primitiveB.
  @param *velocB pointer to and array of a least 3 times maxSize floats containing the linear velocity of collision primitiveB.
  @param *omegaB pointer to and array of a least 3 times maxSize floats containing the angular velocity of collision primitiveB.
  @param *timeOfImpact pointer to least 1 float variable to contain the time of the intersection.
  @param *contacts pointer to and array of a least 3 times maxSize floats to contain the collision contact points.
  @param *normals pointer to and array of a least 3 times maxSize floats to contain the collision contact normals.
  @param *penetration pointer to and array of a least maxSize floats to contain the collision penetration at each contact.
  @param attributeA fixme
  @param attributeB fixme
  @param  threadIndex -Thread index form where the call is made from, zeor otherwize

  @return the number of contact points.

  by passing zero as *maxSize* not contact will be calculated and the function will just determine the time of impact is any.

  if the body are inter penetrating the time of impact will be zero.

  if the bodies do not collide time of impact will be set to *timestep*

  This function can be used as a low-level building block for a stand-alone collision system.
  Applications that have already there own physics system, and only want and quick and fast collision solution,
  can use Newton advanced collision engine as the low level collision detection part.
  To do this the application only needs to initialize Newton, create the collision primitives at application discretion,
  and just call this function when the objects are in close proximity. Applications using Newton as a collision system
  only, are responsible for implementing their own broad phase collision determination, based on any high level tree structure.
  Also the application should implement their own trivial aabb test, before calling this function .

  See also: ::NewtonCollisionCollide, ::NewtonCollisionClosestPoint, ::NewtonCollisionPointDistance, ::NewtonCollisionRayCast, ::NewtonCollisionCalculateAABB
*/
int NewtonCollisionCollideContinue(const NewtonWorld* const newtonWorld, int maxSize, dFloat timestep, 
		const NewtonCollision* const collisionA, const dFloat* const matrixA, const dFloat* const velocA, const dFloat* const omegaA, 
		const NewtonCollision* const collisionB, const dFloat* const matrixB, const dFloat* const velocB, const dFloat* const omegaB, 
		dFloat* const timeOfImpact, dFloat* const contacts, dFloat* const normals, dFloat* const penetration, 
		dLong* const attributeA, dLong* const attributeB, int threadIndex)
{
	Newton* const world = (Newton *)newtonWorld;

	*timeOfImpact = timestep;

	TRACE_FUNCTION(__FUNCTION__);
	return world->CollideContinue ((dgCollisionInstance*)collisionA, dgMatrix (matrixA), *((dgVector*) velocA), *((dgVector*) omegaA), 
								   (dgCollisionInstance*)collisionB, dgMatrix (matrixB), *((dgVector*) velocB), *((dgVector*) omegaB), 
								   *timeOfImpact, (dgTriplex*) contacts, (dgTriplex*) normals, penetration, attributeA, attributeB, maxSize, threadIndex);
}


/*!
  Calculate the most extreme point of a convex collision shape along the given direction.

  @param *collisionPtr pointer to the collision object.
  @param *dir pointer to an array of at least three floats representing the search direction.
  @param *vertex pointer to an array of at least three floats to hold the collision most extreme vertex along the search direction.

  @return nothing.

  the search direction must be in the space of the collision shape.

  See also: ::NewtonCollisionRayCast, ::NewtonCollisionClosestPoint, ::NewtonCollisionPointDistance
*/
void NewtonCollisionSupportVertex(const NewtonCollision* const collisionPtr, const dFloat* const dir, dFloat* const vertex)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;

	const dgMatrix& matrix = collision->GetLocalMatrix ();
	dgVector searchDir (matrix.UnrotateVector(dgVector (dir[0], dir[1], dir[2], dgFloat32 (0.0f)))); 
	searchDir = searchDir.Normalize();
	dgVector vertexOut (matrix.TransformVector(collision->SupportVertex(searchDir)));

	vertex[0] = vertexOut[0];
	vertex[1] = vertexOut[1];
	vertex[2] = vertexOut[2];
}


/*!
  Ray cast specific collision object.

  @param *collisionPtr pointer to the collision object.
  @param  *p0 - pointer to an array of at least three floats representing the ray origin in the local space of the geometry.
  @param  *p1 - pointer to an array of at least three floats representing the ray end in the local space of the geometry.
  @param *normal pointer to an array of at least three floats to hold the normal at the intersection point.
  @param *attribute pointer to an array of at least one floats to hold the ID of the face hit by the ray.

  @return the parametric value of the intersection, between 0.0 and 1.0, an value larger than 1.0 if the ray miss.

  This function is intended for applications using newton collision system separate from the dynamics system, also for applications
  implementing any king of special purpose logic like sensing distance to another object.

  the ray most be local to the collisions geometry, for example and application ray casting the collision geometry of
  of a rigid body, must first take the points p0, and p1 to the local space of the rigid body by multiplying the points by the
  inverse of he rigid body transformation matrix.

  See also: ::NewtonCollisionClosestPoint, ::NewtonCollisionSupportVertex, ::NewtonCollisionPointDistance, ::NewtonCollisionCollide, ::NewtonCollisionCalculateAABB
*/
dFloat NewtonCollisionRayCast(const NewtonCollision* const collisionPtr, const dFloat* const p0, const dFloat* const p1, dFloat* const normal, dLong* const attribute)
{
	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;

	TRACE_FUNCTION(__FUNCTION__);
	const dgMatrix& matrix = collision->GetLocalMatrix ();

	dgVector q0 (matrix.UntransformVector (dgVector (p0[0], p0[1], p0[2], dgFloat32 (0.0f)))); 
	dgVector q1 (matrix.UntransformVector (dgVector (p1[0], p1[1], p1[2], dgFloat32 (0.0f)))); 
	dgContactPoint contact;

	dFloat t = collision->RayCast (q0, q1, dgFloat32 (1.0f), contact, NULL, NULL, NULL);
	if (t >= dFloat (0.0f) && t <= dFloat (dgFloat32(1.0f))) {
		attribute[0] = (dLong) contact.m_shapeId0;

		dgVector n (matrix.RotateVector (contact.m_normal));
		normal[0] = n[0];
		normal[1] = n[1];
		normal[2] = n[2];
	}
	return t;
}

/*!
  Calculate an axis-aligned bounding box for this collision, the box is calculated relative to *offsetMatrix*.

  @param *collisionPtr pointer to the collision object.
  @param *offsetMatrix pointer to an array of 16 floats containing the offset matrix used as the coordinate system and center of the AABB.
  @param  *p0 - pointer to an array of at least three floats to hold minimum value for the AABB.
  @param  *p1 - pointer to an array of at least three floats to hold maximum value for the AABB.

  @return Nothing.

  See also: ::NewtonCollisionClosestPoint, ::NewtonCollisionPointDistance, ::NewtonCollisionCollide, ::NewtonCollisionRayCast
*/
void NewtonCollisionCalculateAABB(const NewtonCollision* const collisionPtr, const dFloat* const offsetMatrix, dFloat* const p0, dFloat* const p1)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;
	dgMatrix matrix (collision->GetLocalMatrix () * dgMatrix (offsetMatrix));

	dgVector q0;
	dgVector q1;

	collision->CalcAABB (matrix, q0, q1);
	p0[0] = q0.m_x;
	p0[1] = q0.m_y;
	p0[2] = q0.m_z;

	p1[0] = q1.m_x;
	p1[1] = q1.m_y;
	p1[2] = q1.m_z;
}

/*!
  Iterate thought polygon of the collision geometry of a body calling the function callback.

  @param *collisionPtr is the pointer to the collision objects.
  @param *matrixPtr is the pointer to the collision objects.
  @param callback application define callback
  @param *userDataPtr pointer to the user defined user data value.

  @return nothing

  This function used to be a member of the rigid body, but to making it a member of the collision object provides better
  low lever display capabilities. The application can still call this function to show the collision of a rigid body by
  getting the collision and the transformation matrix from the rigid, and then calling this functions.

  This function can be called by the application in order to show the collision geometry. The application should provide a pointer to the function *NewtonCollisionIterator*,
  Newton will convert the collision geometry into a polygonal mesh, and will call *callback* for every polygon of the mesh

  this function affect severely the performance of Newton. The application should call this function only for debugging purpose

  This function will ignore user define collision mesh
  See also: ::NewtonWorldGetFirstBody, ::NewtonWorldForEachBodyInAABBDo
*/
void NewtonCollisionForEachPolygonDo(const NewtonCollision* const collisionPtr, const dFloat* const matrixPtr, NewtonCollisionIterator callback, void* const userDataPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) (collisionPtr);
	collision->DebugCollision (dgMatrix (matrixPtr), (dgCollision::OnDebugCollisionMeshCallback) callback, userDataPtr);
}


int NewtonCollisionGetType(const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return instance->GetCollisionPrimityType();
}

int NewtonCollisionIsConvexShape(const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*)collision;
	return instance->IsType (dgCollision::dgCollisionConvexShape_RTTI) ? 1 : 0;
}

int NewtonCollisionIsStaticShape (const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*)collision;
	return (instance->IsType(dgCollision::dgCollisionMesh_RTTI) || instance->IsType(dgCollision::dgCollisionScene_RTTI)) ? 1 : 0;
}

/*!
  Store a user defined value with a convex collision primitive.

  @param collision is the pointer to a collision primitive.
  @param id value to store with the collision primitive.

  @return nothing

  the application can store an id with any collision primitive. This id can be used to identify what type of collision primitive generated a contact.

  See also: ::NewtonCollisionGetUserID, ::NewtonCreateBox, ::NewtonCreateSphere
*/
void NewtonCollisionSetUserID(const NewtonCollision* const collision, unsigned id)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	instance->SetUserDataID (id);
}

/*!
  Return a user define value with a convex collision primitive.

  @param collision is the pointer to a convex collision primitive.

  @return user id

  the application can store an id with any collision primitive. This id can be used to identify what type of collision primitive generated a contact.

  See also: ::NewtonCreateBox, ::NewtonCreateSphere
*/
unsigned NewtonCollisionGetUserID(const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return instance->GetUserDataID();
}

void NewtonCollisionSetUserData (const NewtonCollision* const collision, void* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	instance->SetUserData(userData);
}

void* NewtonCollisionGetUserData (const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return instance->GetUserData();
}

void NewtonCollisionSetMaterial (const NewtonCollision* const collision, const NewtonCollisionMaterial* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	dgCollisionInfo::dgInstanceMaterial& data = instance->m_material;
	data.m_userData = userData->m_userData;
	data.m_userId = userData->m_userId;
	data.m_userFlags = userData->m_userFlags;
	for (dgInt32 i = 0; i < 4; i++) {
		data.m_userParam[i] = userData->m_userParam[i];
	}
	instance->SetMaterial (data);
}

void NewtonCollisionGetMaterial (const NewtonCollision* const collision, NewtonCollisionMaterial* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	const dgCollisionInfo::dgInstanceMaterial data(instance->GetMaterial());
	userData->m_userData = data.m_userData;
	userData->m_userId = data.m_userId;
	userData->m_userFlags = data.m_userFlags;
	for (dgInt32 i = 0; i < 4; i ++) {
		userData->m_userParam[i] = data.m_userParam[i];
	}
}

void* NewtonCollisionGetSubCollisionHandle (const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return (void*)instance->GetCollisionHandle ();
}

NewtonCollision* NewtonCollisionGetParentInstance (const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return (NewtonCollision*)instance->GetParent();
}


void NewtonCollisionSetMatrix (const NewtonCollision* collision, const dFloat* const matrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	instance->SetLocalMatrix(dgMatrix (matrix));
}

void NewtonCollisionGetMatrix (const NewtonCollision* const collision, dFloat* const matrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	const dgMatrix& instanceMatrix = instance->GetLocalMatrix();
	memcpy (matrix, &instanceMatrix[0][0], sizeof (dgMatrix));
}


void NewtonCollisionSetScale (const NewtonCollision* const collision, dFloat scaleX, dFloat scaleY, dFloat scaleZ)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	instance->SetScale(dgVector (scaleX, scaleY, scaleZ, dgFloat32 (0.0f)));
}


void NewtonCollisionGetScale (const NewtonCollision* const collision, dFloat* const scaleX, dFloat* const scaleY, dFloat* const scaleZ)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;

	dgVector scale (instance->GetScale());
	*scaleX = scale.m_x;
	*scaleY = scale.m_y;
	*scaleZ = scale.m_z;
}




/*!
  Release a reference from this collision object returning control to Newton.

  @param *collisionPtr pointer to the collision object

  @return Nothing.

  to get the correct reference count of a collision primitive the application can call function *NewtonCollisionGetInfo*

*/
void NewtonDestroyCollision(const NewtonCollision* const collisionPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;
	collision->Release();
}

dFloat NewtonCollisionGetSkinThickness(const NewtonCollision* const collisionPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)collisionPtr;
	return collision->GetSkinThickness();
}

void NewtonCollisionSetSkinThickness(const NewtonCollision* const collisionPtr, dFloat thickness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)collisionPtr;
	collision->SetSkinThickness(thickness);
}

NewtonCollision* NewtonCollisionCreateInstance (const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const instance = (dgCollisionInstance*) collision;
	return (NewtonCollision*) new (instance->GetAllocator()) dgCollisionInstance (*instance);
}



/*!
  Serialize a general collision shape.

  @param *newtonWorld Pointer to the Newton world.
  @param *collision is the pointer to the collision tree shape.
  @param serializeFunction pointer to the event function that will do the serialization.
  @param  *serializeHandle	- user data that will be passed to the _NewtonSerialize_ callback.

  @return Nothing.

  Small and medium collision shapes like *TreeCollision* (under 50000 polygons) small convex hulls or compude collision can be constructed at application
  startup without significant processing overhead.


  See also: ::NewtonCollisionGetInfo
*/
void NewtonCollisionSerialize(const NewtonWorld* const newtonWorld, const NewtonCollision* const collision, NewtonSerializeCallback serializeFunction, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	world->SerializeCollision((dgCollisionInstance*) collision, (dgSerialize) serializeFunction, serializeHandle);
}


/*!
  Create a collision shape via a serialization function.

  @param *newtonWorld Pointer to the Newton world.
  @param deserializeFunction pointer to the event function that will do the deserialization.
  @param *serializeHandle user data that will be passed to the _NewtonSerialize_ callback.

  @return Nothing.

  this function is useful to to load collision primitive for and archive file. In the case of complex shapes like convex hull and compound collision the
  it save a significant amount of construction time.

  if this function is called to load a serialized tree collision, the tree collision will be loaded, but the function pointer callback will be set to NULL.
  for this operation see function *NewtonCreateTreeCollisionFromSerialization*

  See also: ::NewtonCollisionSerialize, ::NewtonCollisionGetInfo
*/
NewtonCollision* NewtonCreateCollisionFromSerialization(const NewtonWorld* const newtonWorld, NewtonDeserializeCallback deserializeFunction, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return  (NewtonCollision*) world->CreateCollisionFromSerialization ((dgDeserialize) deserializeFunction, serializeHandle);
}


/*!
  Get creation parameters for this collision objects.

  @param collision is the pointer to a convex collision primitive.
  @param *collisionInfo pointer to a collision information record.

  This function can be used by the application for writing file format and for serialization.

  See also: ::NewtonCollisionGetInfo, ::NewtonCollisionSerialize
*/
void NewtonCollisionGetInfo(const NewtonCollision* const collision, NewtonCollisionInfoRecord* const collisionInfo)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const coll = (dgCollisionInstance*)collision;

	dgAssert ( dgInt32 (sizeof (dgCollisionInfo)) <= dgInt32 (sizeof (NewtonCollisionInfoRecord)));
	dgCollisionInfo info;
	coll->GetCollisionInfo (&info);
	memcpy (collisionInfo, &info, sizeof (dgCollisionInfo));
}


/*! @} */ // end of CollisionLibraryGeneric


/*! @defgroup TransUtil TransUtil
Transform utility functions
@{
*/


/*!
  Get the three Euler angles from a 4x4 rotation matrix arranged in row-major order.

  @param matrix pointer to the 4x4 rotation matrix.
  @param  angles0 - fixme
  @param  angles1 - pointer to an array of at least three floats to hold the Euler angles.

  @return Nothing.

  The motivation for this function is that many graphics engines still use Euler angles to represent the orientation
  of graphics entities.
  The angles are expressed in radians and represent:
  *angle[0]* - rotation about first matrix row
  *angle[1]* - rotation about second matrix row
  *angle[2]* - rotation about third matrix row

  See also: ::NewtonSetEulerAngle
*/
void NewtonGetEulerAngle(const dFloat* const matrix, dFloat* const angles0, dFloat* const angles1)
{
	dgMatrix mat (matrix);

	TRACE_FUNCTION(__FUNCTION__);
	dgVector euler0;
	dgVector euler1;
	mat.CalcPitchYawRoll (euler0, euler1);

	angles0[0] = euler0.m_x;
	angles0[1] = euler0.m_y;
	angles0[2] = euler0.m_z;

	angles1[0] = euler1.m_x;
	angles1[1] = euler1.m_y;
	angles1[2] = euler1.m_z;

}


/*!
  Build a rotation matrix from the Euler angles in radians.

  @param matrix pointer to the 4x4 rotation matrix.
  @param angles pointer to an array of at least three floats to hold the Euler angles.

  @return Nothing.

  The motivation for this function is that many graphics engines still use Euler angles to represent the orientation
  of graphics entities.
  The angles are expressed in radians and represent:
  *angle[0]* - rotation about first matrix row
  *angle[1]* - rotation about second matrix row
  *angle[2]* - rotation about third matrix row

  See also: ::NewtonGetEulerAngle
*/
void NewtonSetEulerAngle(const dFloat* const angles, dFloat* const matrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMatrix mat (dgPitchMatrix (angles[0]) * dgYawMatrix(angles[1]) * dgRollMatrix(angles[2]));
	//dgMatrix retMatrix (matrix);
	dgMatrix& retMatrix = *((dgMatrix*) matrix);
	
	for (dgInt32 i = 0; i < 3; i ++) {
		retMatrix[3][i] = 0.0f;
		for (dgInt32 j = 0; j < 4; j ++) {
			retMatrix[i][j] = mat[i][j]; 
		}
	}
	retMatrix[3][3] = dgFloat32(1.0f);
}


/*!
  Calculates the acceleration to satisfy the specified the spring damper system.

  @param dt integration time step.
  @param ks spring stiffness, it must be a positive value.
  @param x spring position.
  @param kd desired spring damper, it must be a positive value.
  @param s spring velocity.

  return: the spring acceleration.

  the acceleration calculated by this function represent the mass, spring system of the form
  a = -ks * x - kd * v.
*/
dFloat NewtonCalculateSpringDamperAcceleration(dFloat dt, dFloat ks, dFloat x, dFloat kd, dFloat s)
{
//	accel = - (ks * x + kd * s);

	TRACE_FUNCTION(__FUNCTION__);
	//at =  [- ks (x2 - x1) - kd * (v2 - v1) - dt * ks * (v2 - v1)] / [1 + dt * kd + dt * dt * ks] 
	dgFloat32 ksd = dt * ks;
	dgFloat32 num = ks * x + kd * s + ksd * s;
	dgFloat32 den = dgFloat32 (1.0f) + dt * kd + dt * ksd;
	dgAssert (den > 0.0f);
	dFloat accel = - num / den;
//	dgCheckFloat (accel);
	return accel;
}

/*! @} */ // end of TransUtil

/*! @defgroup RigidBodyInterface RigidBodyInterface
Rigid Body Interface
@{
*/


/*!
  Create a rigid body.

  @param *newtonWorld Pointer to the Newton world.
  @param *collisionPtr pointer to the collision object.
  @param *matrixPtr fixme

  @return Pointer to the rigid body.

  This function creates a Newton rigid body and assigns a *collisionPtr* as the collision geometry representing the rigid body.
  This function increments the reference count of the collision geometry.
  All event functions are set to NULL and the material gruopID of the body is set to the default GroupID.

  See also: ::NewtonDestroyBody
*/
NewtonBody* NewtonCreateDynamicBody(const NewtonWorld* const newtonWorld, const NewtonCollision* const collisionPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgCollisionInstance* collision = (dgCollisionInstance*)collisionPtr;
	if (!collisionPtr) {
		collision = (dgCollisionInstance*) NewtonCreateNull(newtonWorld);
	}

	#ifdef SAVE_COLLISION
	SaveCollision (collisionPtr);
	#endif

	dgMatrix matrix (matrixPtr);
	matrix.m_front.m_w = dgFloat32 (0.0f);
	matrix.m_up.m_w    = dgFloat32 (0.0f);
	matrix.m_right.m_w = dgFloat32 (0.0f);
	matrix.m_posit.m_w = dgFloat32 (1.0f);

	NewtonBody* const body = (NewtonBody*)world->CreateDynamicBody (collision, matrix);
	if (!collisionPtr) {
		NewtonDestroyCollision((NewtonCollision*)collision);
	}
	return body;
}

NewtonBody* NewtonCreateAsymetricDynamicBody(const NewtonWorld* const newtonWorld, const NewtonCollision* const collisionPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgCollisionInstance* collision = (dgCollisionInstance*)collisionPtr;
	if (!collisionPtr) {
		collision = (dgCollisionInstance*)NewtonCreateNull(newtonWorld);
	}

#ifdef SAVE_COLLISION
	SaveCollision(collisionPtr);
#endif

	dgMatrix matrix(matrixPtr);
	matrix.m_front.m_w = dgFloat32(0.0f);
	matrix.m_up.m_w = dgFloat32(0.0f);
	matrix.m_right.m_w = dgFloat32(0.0f);
	matrix.m_posit.m_w = dgFloat32(1.0f);

	NewtonBody* const body = (NewtonBody*)world->CreateDynamicBodyAsymetric(collision, matrix);
	if (!collisionPtr) {
		NewtonDestroyCollision((NewtonCollision*)collision);
	}
	return body;
}

NewtonBody* NewtonCreateKinematicBody(const NewtonWorld* const newtonWorld, const NewtonCollision* const collisionPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgCollisionInstance* collision = (dgCollisionInstance*)collisionPtr;
	if (!collisionPtr) {
		collision = (dgCollisionInstance*)NewtonCreateNull(newtonWorld);
	}

#ifdef SAVE_COLLISION
	SaveCollision (collisionPtr);
#endif

	dgMatrix matrix (matrixPtr);
	matrix.m_front.m_w = dgFloat32 (0.0f);
	matrix.m_up.m_w    = dgFloat32 (0.0f);
	matrix.m_right.m_w = dgFloat32 (0.0f);
	matrix.m_posit.m_w = dgFloat32 (1.0f);

	NewtonBody* const body = (NewtonBody*) world->CreateKinematicBody(collision, matrix);
	if (!collisionPtr) {
		NewtonDestroyCollision((NewtonCollision*)collision);
	}
	return body;
}


/*!
  Destroy a rigid body.

  @param *bodyPtr pointer to the body to be destroyed.

  @return Nothing.

  If this function is called from inside a simulation step the destruction of the body will be delayed until end of the time step.
  This function will decrease the reference count of the collision geometry by one. If the reference count reaches zero, then the collision
  geometry will be destroyed. This function will destroy all joints associated with this body.

  See also: ::NewtonCreateDynamicBody
*/
void NewtonDestroyBody (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();
	world->DestroyBody(body);
}

/*
void NewtonBodyEnableSimulation(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();
	world->BodyEnableSimulation (body);
}

void NewtonBodyDisableSimulation(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();
	world->BodyDisableSimulation (body);
}
*/

/*!
  Gets the current simulation state of the specified body.

  @param *bodyPtr pointer to the body to be inspected.

  @return the current simulation state 0: disabled 1: active.

  See also: ::NewtonBodySetSimulationState
*/
int NewtonBodyGetSimulationState(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();
	return world->GetBodyEnableDisableSimulationState(body) ? 1 : 0;
}

/*!
  Sets the current simulation state of the specified body.

  @param *bodyPtr pointer to the body to be changed.
  @param state the new simulation state 0: disabled 1: active

  @return Nothing.

  See also: ::NewtonBodyGetSimulationState
*/
void NewtonBodySetSimulationState(const NewtonBody* const bodyPtr, const int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();

	if (state) {
		world->BodyEnableSimulation(body);
	} else {
		world->BodyDisableSimulation(body);
	}
}

int NewtonBodyGetCollidable (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->IsCollidable() ? 1 : 0;
}

void NewtonBodySetCollidable (const NewtonBody* const bodyPtr, int collidable)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetCollidable(collidable ? true : false);
}

int NewtonBodyGetType (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	if (body->IsRTTIType(dgBody::m_dynamicBodyRTTI)) {
		return NEWTON_DYNAMIC_BODY;
	} else if (body->IsRTTIType(dgBody::m_kinematicBodyRTTI)) {
		return NEWTON_KINEMATIC_BODY;
	} else if (body->IsRTTIType(dgBody::m_dynamicBodyAsymentricRTTI)) {
		return NEWTON_DYNAMIC_ASYMETRIC_BODY;
//	} else if (body->IsRTTIType(dgBody::m_deformableBodyRTTI)) {
//		return NEWTON_DEFORMABLE_BODY;
	}
	dgAssert (0);
	return 0;
}

int NewtonBodyGetID (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetUniqueID();
}

/*!
  Store a user defined data value with the body.

  @param *bodyPtr pointer to the body.
  @param *userDataPtr pointer to the user defined user data value.

  @return Nothing.

  The application can store a user defined value with the Body. This value can be the pointer to a structure containing some application data for special effect.
  if the application allocate some resource to store the user data, the application can register a joint destructor to get rid of the allocated resource when the body is destroyed

  See also: ::NewtonBodyGetUserData
*/
void  NewtonBodySetUserData(const NewtonBody* const bodyPtr, void* const userDataPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetUserData (userDataPtr);
}

/*!
  Retrieve a user defined data value stored with the body.

  @param *bodyPtr pointer to the body.

  @return The user defined data.

  The application can store a user defined value with a rigid body. This value can be the pointer
  to a structure which is the graphical representation of the rigid body.

  See also: ::NewtonBodySetUserData, 
*/
void* NewtonBodyGetUserData(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetUserData ();
}


/*!
  Return pointer to the Newton world of the specified body.

  @param *bodyPtr Pointer to the body.

  @return World that owns this body.

  The application can also determine the world from a joint, if it queries one
  of the bodies attached to that joint.
*/
NewtonWorld* NewtonBodyGetWorld(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonWorld*) body->GetWorld();
}


/*!
  Assign a transformation event function to the body.

  @param *bodyPtr pointer to the body.
  @param callback pointer to a function callback in used to update the transformation matrix of the visual object that represents the rigid body.

  @return Nothing.

  The function *NewtonSetTransform callback* is called by the Newton engine every time a visual object that represents the rigid body has changed.
  The application can obtain the pointer user data value that points to the visual object.
  The Newton engine does not call the *NewtonSetTransform callback* function for bodies that are inactive or have reached a state of stable equilibrium.

  The matrix should be organized in row-major order (this is the way directX and OpenGL stores matrices).

  See also: NewtonBodyGetTransformCallback
*/
void  NewtonBodySetTransformCallback(const NewtonBody* const bodyPtr, NewtonSetTransform callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetMatrixUpdateCallback ((dgBody::OnMatrixUpdateCallback) callback);
}


/*!
  Assign a transformation event function to the body.

  @param *bodyPtr pointer to the body.

  @return Nothing.

  The function *NewtonSetTransform callback* is called by the Newton engine every time a visual object that represents the rigid body has changed.
  The application can obtain the pointer user data value that points to the visual object.
  The Newton engine does not call the *NewtonSetTransform callback* function for bodies that are inactive or have reached a state of stable equilibrium.

  The matrix should be organized in row-major order (this is the way directX and OpenGL stores matrices).

  See also: ::NewtonBodySetTransformCallback
*/
NewtonSetTransform NewtonBodyGetTransformCallback (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonSetTransform) body->GetMatrixUpdateCallback();
}


/*!
  Assign an event function for applying external force and torque to a rigid body.

  @param *bodyPtr pointer to the body.
  @param callback pointer to a function callback used to apply force and torque to a rigid body.

  @return Nothing.

  Before the *NewtonApplyForceAndTorque callback* is called for a body, Newton first clears the net force and net torque for the body.

  The function *NewtonApplyForceAndTorque callback* is called by the Newton Engine every time an active body is going to be simulated.
  The Newton Engine does not call the *NewtonApplyForceAndTorque callback* function for bodies that are inactive or have reached a state of stable equilibrium.

  See also: ::NewtonBodyGetUserData, ::NewtonBodyGetUserData, ::NewtonBodyGetForceAndTorqueCallback
*/
void  NewtonBodySetForceAndTorqueCallback(const NewtonBody* const bodyPtr, NewtonApplyForceAndTorque callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetExtForceAndTorqueCallback ((dgBody::OnApplyExtForceAndTorque) callback);
}


/*!
  Return the pointer to the current force and torque call back function.

  @param *bodyPtr pointer to the body.

  @return pointer to the force call back.

  This function can be used to concatenate different force calculation components making more modular the
  design of function components dedicated to apply special effect. For example a body may have a basic force a force that
  only apply the effect of gravity, but that application can place a region in where there can be a fluid volume, or another gravity field.
  we this function the application can read the correct function and save into a local variable, and set a new one.
  this new function will firs call the save function pointer and upon return apply the correct effect.
  this similar to the concept of virtual methods on objected oriented languages.

  The function *NewtonApplyForceAndTorque callback* is called by the Newton Engine every time an active body is going to be simulated.
  The Newton Engine does not call the *NewtonApplyForceAndTorque callback* function for bodies that are inactive or have reached a state of stable equilibrium.

  See also: ::NewtonBodyGetUserData, ::NewtonBodyGetUserData, ::NewtonBodySetForceAndTorqueCallback
*/
NewtonApplyForceAndTorque NewtonBodyGetForceAndTorqueCallback(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonApplyForceAndTorque) body->GetExtForceAndTorqueCallback ();
}


/*!
  Assign an event function to be called when this body is about to be destroyed.

  @param *bodyPtr pointer to the body to be destroyed.
  @param callback pointer to a function callback.

  @return Nothing.


  This function *NewtonBodyDestructor callback* acts like a destruction function in CPP. This function
  is called when the body and all data joints associated with the body are about to be destroyed.
  The application could use this function to destroy or release any resource associated with this body.
  The application should not make reference to this body after this function returns.


  The destruction of a body will destroy all joints associated with the body.

  See also: ::NewtonBodyGetUserData, ::NewtonBodyGetUserData
*/
void NewtonBodySetDestructorCallback(const NewtonBody* const bodyPtr, NewtonBodyDestructor callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetDestructorCallback (dgBody::OnBodyDestroy (callback));
}

NewtonBodyDestructor NewtonBodyGetDestructorCallback (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return NewtonBodyDestructor (body->GetDestructorCallback ());
}

/*!
  Set the mass matrix of a rigid body.

  @param *bodyPtr pointer to the body.
  @param mass mass value.
  @param inertiaMatrix fixme

  @return Nothing.

  Newton algorithms have no restriction on the values for the mass, but due to floating point dynamic
  range (24 bit precision) it is best if the ratio between the heaviest and the lightest body in the scene is limited to 200.
  There are no special utility functions in Newton to calculate the moment of inertia of common primitives.
  The application should specify the inertial values, keeping in mind that realistic inertia values are necessary for
  realistic physics behavior.

  See also: ::NewtonConvexCollisionCalculateInertialMatrix, ::NewtonBodyGetMass, ::NewtonBodyGetInvMass
*/
void NewtonBodySetFullMassMatrix(const NewtonBody* const bodyPtr, dFloat mass, const dFloat* const inertiaMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgMatrix inertia(inertiaMatrix);
	body->SetMassMatrix (mass, inertia);
}


void NewtonBodySetMassMatrix(const NewtonBody* const bodyPtr, dFloat mass, dFloat Ixx, dFloat Iyy, dFloat Izz)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMatrix inertia (dgGetIdentityMatrix());
	inertia[0][0] = Ixx;
	inertia[1][1] = Iyy;
	inertia[2][2] = Izz;
	NewtonBodySetFullMassMatrix(bodyPtr, mass, &inertia[0][0]);
}


void  NewtonBodySetMassProperties (const NewtonBody* const bodyPtr, dFloat mass, const NewtonCollision* const collisionPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;
	body->SetMassProperties (mass, collision);
}



/*!
  Get the mass matrix of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *mass pointer to a variable that will hold the mass value of the body.
  @param *Ixx pointer to a variable that will hold the moment of inertia of the first principal axis of inertia of the body.
  @param *Iyy pointer to a variable that will hold the moment of inertia of the first principal axis of inertia of the body.
  @param *Izz pointer to a variable that will hold the moment of inertia of the first principal axis of inertia of the body.

  @return Nothing.

  See also: ::NewtonBodySetMassMatrix, ::NewtonBodyGetInvMass
*/
void  NewtonBodyGetMass(const NewtonBody* const bodyPtr, dFloat* const mass, dFloat* const Ixx, dFloat* const Iyy, dFloat* const Izz)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

//	dgVector vector (body->GetApparentMass());
	dgVector vector (body->GetMass());
	Ixx[0] = vector.m_x;
	Iyy[0] = vector.m_y; 
	Izz[0] = vector.m_z;
	mass[0] = vector.m_w;
	if (vector.m_w > DG_INFINITE_MASS * 0.5f) {
		Ixx[0] = 0.0f;
		Iyy[0] = 0.0f; 
		Izz[0] = 0.0f;
		mass[0] = 0.0f;
	}
}

/*!
  Get the inverse mass matrix of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *invMass pointer to a variable that will hold the mass inverse value of the body.
  @param *invIxx pointer to a variable that will hold the moment of inertia inverse of the first principal axis of inertia of the body.
  @param *invIyy pointer to a variable that will hold the moment of inertia inverse of the first principal axis of inertia of the body.
  @param *invIzz pointer to a variable that will hold the moment of inertia inverse of the first principal axis of inertia of the body.

  @return Nothing.

  See also: ::NewtonBodySetMassMatrix, ::NewtonBodyGetMass
*/
void NewtonBodyGetInvMass(const NewtonBody* const bodyPtr, dFloat* const invMass, dFloat* const invIxx, dFloat* const invIyy, dFloat* const invIzz)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

//	dgVector vector1 (body->GetMass());
//	invIxx[0] = dgFloat32 (1.0f) / (vector1.m_x + dgFloat32 (1.0e-8f));
//	invIyy[0] = dgFloat32 (1.0f) / (vector1.m_y + dgFloat32 (1.0e-8f)); 
//	invIzz[0] = dgFloat32 (1.0f) / (vector1.m_z + dgFloat32 (1.0e-8f));
//	invMass[0] = dgFloat32 (1.0f) / (vector1.m_w + dgFloat32 (1.0e-8f));
	dgVector inverseMass (body->GetInvMass());
	invIxx[0] = inverseMass.m_x;
	invIyy[0] = inverseMass.m_y; 
	invIzz[0] = inverseMass.m_z;
	invMass[0] = inverseMass.m_w;
}


void NewtonBodyGetInertiaMatrix(const NewtonBody* const bodyPtr, dFloat* const inertiaMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgMatrix matrix (body->CalculateInertiaMatrix());
	memcpy (inertiaMatrix, &matrix[0][0], sizeof (dgMatrix));
}

void NewtonBodyGetInvInertiaMatrix(const NewtonBody* const bodyPtr, dFloat* const invInertiaMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgMatrix matrix (body->CalculateInvInertiaMatrix ());
	memcpy (invInertiaMatrix, &matrix[0][0], sizeof (dgMatrix));
}



/*!
  Set the transformation matrix of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *matrixPtr pointer to an array of 16 floats containing the global matrix of the rigid body.

  @return Nothing.

  The matrix should be arranged in row-major order.
  If you are using OpenGL matrices (column-major) you will need to transpose you matrices into a local array, before
  passing them to Newton.

  That application should make sure the transformation matrix has not scale, otherwise unpredictable result will occur.

  See also: ::NewtonBodyGetMatrix
*/
void NewtonBodySetMatrix(const NewtonBody* const bodyPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgMatrix matrix (matrixPtr);

	matrix.m_front.m_w = dgFloat32 (0.0f);
	matrix.m_up.m_w    = dgFloat32 (0.0f);
	matrix.m_right.m_w = dgFloat32 (0.0f);
	matrix.m_posit.m_w = dgFloat32 (1.0f);
	body->SetMatrixResetSleep (matrix); 
}

void NewtonBodySetMatrixNoSleep (const NewtonBody* const bodyPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgMatrix matrix(matrixPtr);

	matrix.m_front.m_w = dgFloat32(0.0f);
	matrix.m_up.m_w = dgFloat32(0.0f);
	matrix.m_right.m_w = dgFloat32(0.0f);
	matrix.m_posit.m_w = dgFloat32(1.0f);
	body->SetMatrixNoSleep(matrix);
}

/*!
  Apply hierarchical transformation to a body.

  @param *bodyPtr pointer to the body.
  @param *matrixPtr pointer to an array of 16 floats containing the global matrix of the rigid body.

  @return Nothing.

  This function applies the transformation matrix to the *body* and also applies the appropriate transformation matrix to
  set of articulated bodies. If the body is in contact with another body the other body is not transformed.

  this function should not be used to transform set of articulated bodies that are connected to a static body.
  doing so will result in unpredictables results. Think for example moving a chain attached to a ceiling from one place to another,
  to do that in real life a person first need to disconnect the chain (destroy the joint), move the chain (apply the transformation to the
  entire chain), the reconnect it in the new position (recreate the joint again).

  this function will set to zero the linear and angular velocity of all bodies that are part of the set of articulated body array.

  The matrix should be arranged in row-major order (this is the way direct x stores matrices).
  If you are using OpenGL matrices (column-major) you will need to transpose you matrices into a local array, before
  passing them to Newton.

  See also: ::NewtonBodySetMatrix
*/
void NewtonBodySetMatrixRecursive(const NewtonBody* const bodyPtr, const dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	Newton* const world = (Newton *)body->GetWorld();

	world->BodySetMatrix (body, dgMatrix (matrixPtr));
}


/*!
  Get the transformation matrix of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *matrixPtr pointer to an array of 16 floats that will hold the global matrix of the rigid body.

  @return Nothing.

  The matrix should be arranged in row-major order (this is the way direct x stores matrices).
  If you are using OpenGL matrices (column-major) you will need to transpose you matrices into a local array, before
  passing them to Newton.

  See also: ::NewtonBodySetMatrix, ::NewtonBodyGetRotation
*/
void NewtonBodyGetMatrix(const NewtonBody* const bodyPtr, dFloat* const matrixPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	const dgMatrix& matrix = body->GetMatrix();
	memcpy (matrixPtr, &matrix[0][0], sizeof (dgMatrix));
}

void NewtonBodyGetPosition(const NewtonBody* const bodyPtr, dFloat* const posPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	const dgVector & rot = body->GetPosition();
	posPtr[0] = rot.m_x;
	posPtr[1] = rot.m_y;
	posPtr[2] = rot.m_z;
}

/*!
  Get the rotation part of the transformation matrix of a body, in form of a unit quaternion.

  @param *bodyPtr pointer to the body.
  @param *rotPtr pointer to an array of 4 floats that will hold the global rotation of the rigid body.

  @return Nothing.

  The rotation matrix is written set in the form of a unit quaternion in the format Rot (q0, q1, q1, q3)

  The rotation quaternion is the same as what the application would get by using at function to extract a quaternion form a matrix.
  however since the rigid body already contained the rotation in it, it is more efficient to just call this function avoiding expensive conversion.

  this function could be very useful for the implementation of pseudo frame rate independent simulation.
  by running the simulation at a fix rate and using linear interpolation between the last two simulation frames.
  to determine the exact fraction of the render step.

  See also: ::NewtonBodySetMatrix, ::NewtonBodyGetMatrix
*/
void NewtonBodyGetRotation(const NewtonBody* const bodyPtr, dFloat* const rotPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgQuaternion rot = body->GetRotation();
	rotPtr[0] = rot.m_q0;
	rotPtr[1] = rot.m_q1;
	rotPtr[2] = rot.m_q2;
	rotPtr[3] = rot.m_q3;
}


/*!
  Set the net force applied to a rigid body.

  @param *bodyPtr pointer to the body.
  @param *vectorPtr pointer to an array of 3 floats containing the net force to be applied to the body.

  @return Nothing.

  This function is only effective when called from *NewtonApplyForceAndTorque callback*

  See also: ::NewtonBodyAddForce, ::NewtonBodyGetForce
*/
void  NewtonBodySetForce(const NewtonBody* const bodyPtr, const dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (vectorPtr[0], vectorPtr[1], vectorPtr[2], dgFloat32 (0.0f));
	body->SetForce (vector);
}

/*!
  Add the net force applied to a rigid body.

  @param *bodyPtr pointer to the body to be destroyed.
  @param *vectorPtr pointer to an array of 3 floats containing the net force to be applied to the body.

  @return Nothing.

  This function is only effective when called from *NewtonApplyForceAndTorque callback*

  See also: ::NewtonBodySetForce, ::NewtonBodyGetForce
*/
void  NewtonBodyAddForce(const NewtonBody* const bodyPtr, const dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (vectorPtr[0], vectorPtr[1], vectorPtr[2], dgFloat32 (0.0f));

	body->AddForce (vector);
}




/*!
  Get the net force applied to a rigid body after the last NewtonUpdate.

  @param *bodyPtr pointer to the body.
  @param *vectorPtr pointer to an array of 3 floats to hold the net force of the body.

  @return Nothing.

  See also: ::NewtonBodyAddForce, ::NewtonBodyGetForce
*/
void NewtonBodyGetForce(const NewtonBody* const bodyPtr, dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (body->GetForce());
	vectorPtr[0] = vector.m_x;
	vectorPtr[1] = vector.m_y;
	vectorPtr[2] = vector.m_z;
}


/*!
  Calculate the next force that net to be applied to the body to archive the desired velocity in the current time step.

  @param *bodyPtr pointer to the body.
  @param timestep time step that the force will be applyed.
  @param *desiredVeloc pointer to an array of 3 floats containing the desired velocity.
  @param *forceOut pointer to an array of 3 floats to hold the calculated net force.

  this function can be useful when creating object for game play.

  this treat the body as a point mass and is uses the solver to calculates the net force that need to be applied to the body
  such that is reach the desired velocity in the net time step.
  In general the force should be calculated by the expression f = M * (dsiredVeloc - bodyVeloc) / timestep
  however due to algorithmic optimization and limitations if such equation is used then the solver will generate a different desired velocity.

  @return Nothing.

  See also: ::NewtonBodySetForce, ::NewtonBodyAddForce, ::NewtonBodyGetForce
*/
void NewtonBodyCalculateInverseDynamicsForce(const NewtonBody* const bodyPtr, dFloat timestep, const dFloat* const desiredVeloc, dFloat* const forceOut)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector veloc (desiredVeloc[0], desiredVeloc[1], desiredVeloc[2], dgFloat32 (0.0f));
	dgVector force (body->CalculateInverseDynamicForce (veloc, timestep));
	forceOut[0] = force[0];
	forceOut[1] = force[1];
	forceOut[2] = force[2];

}

/*!
  Set the net torque applied to a rigid body.

  @param *bodyPtr pointer to the body.
  @param *vectorPtr pointer to an array of 3 floats containing the net torque to be applied to the body.

  @return Nothing.

  This function is only effective when called from *NewtonApplyForceAndTorque callback*

  See also: ::NewtonBodyAddTorque, ::NewtonBodyGetTorque
*/
void  NewtonBodySetTorque(const NewtonBody* const bodyPtr, const dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (vectorPtr[0], vectorPtr[1], vectorPtr[2], dgFloat32 (0.0f));
	body->SetTorque (vector);
}


/*!
  Add the net torque applied to a rigid body.

  @param *bodyPtr pointer to the body.
  @param *vectorPtr pointer to an array of 3 floats containing the net torque to be applied to the body.

  @return Nothing.

  This function is only effective when called from *NewtonApplyForceAndTorque callback*

  See also: ::NewtonBodySetTorque, ::NewtonBodyGetTorque
*/
void  NewtonBodyAddTorque(const NewtonBody* const bodyPtr, const dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (vectorPtr[0], vectorPtr[1], vectorPtr[2], dgFloat32 (0.0f));
	body->AddTorque (vector);
}

/*!
  Get the net torque applied to a rigid body after the last NewtonUpdate.

  @param *bodyPtr pointer to the body.
  @param *vectorPtr pointer to an array of 3 floats to hold the net torque of the body.

  @return Nothing.

  See also: ::NewtonBodyAddTorque, ::NewtonBodyGetTorque
*/
void NewtonBodyGetTorque(const NewtonBody* const bodyPtr, dFloat* const vectorPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (body->GetTorque());
	vectorPtr[0] = vector.m_x;
	vectorPtr[1] = vector.m_y;
	vectorPtr[2] = vector.m_z;
}



/*!
  Set the relative position of the center of mass of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *comPtr pointer to an array of 3 floats containing the relative offset of the center of mass of the body.

  @return Nothing.

  This function can be used to set the relative offset of the center of mass of a rigid body.
  when a rigid body is created the center of mass is set the the point c(0, 0, 0), and normally this is
  the best setting for a rigid body. However the are situations in which and object does not have symmetry or
  simple some kind of special effect is desired, and this origin need to be changed.

  Care must be taken when offsetting the center of mass of a body.
  The application must make sure that the external torques resulting from forces applied at at point
  relative to the center of mass are calculated appropriately.
  this could be done Transform and Torque callback function as the follow pseudo code fragment shows:

  Matrix matrix;
  Vector center;

  NewtonGatMetrix(body, matrix)
  NewtonGetCentreOfMass(body, center);

  for global space torque.
  Vector localForce (fx, fy, fz);
  Vector localPosition (x, y, z);
  Vector localTroque (crossproduct ((localPosition - center). localForce);
  Vector globalTroque (matrix.RotateVector (localTroque));

  for global space torque.
  Vector globalCentre (matrix.TranformVector (center));
  Vector globalPosition (x, y, z);
  Vector globalForce (fx, fy, fz);
  Vector globalTroque (crossproduct ((globalPosition - globalCentre). globalForce);

  See also: ::NewtonConvexCollisionCalculateInertialMatrix, ::NewtonBodyGetCentreOfMass
*/
void NewtonBodySetCentreOfMass(const NewtonBody* const bodyPtr, const dFloat* const comPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (comPtr[0], comPtr[1], comPtr[2], dgFloat32 (1.0f));
	body->SetCentreOfMass (vector);
}

/*!
  Get the relative position of the center of mass of a rigid body.

  @param *bodyPtr pointer to the body.
  @param *comPtr pointer to an array of 3 floats to hold the relative offset of the center of mass of the body.

  @return Nothing.

  This function can be used to set the relative offset of the center of mass of a rigid body.
  when a rigid body is created the center of mass is set the the point c(0, 0, 0), and normally this is
  the best setting for a rigid body. However the are situations in which and object does not have symmetry or
  simple some kind of special effect is desired, and this origin need to be changed.

  This function can be used in conjunction with *NewtonConvexCollisionCalculateInertialMatrix*

  See also: ::NewtonConvexCollisionCalculateInertialMatrix, ::NewtonBodySetCentreOfMass
*/
void NewtonBodyGetCentreOfMass(const NewtonBody* const bodyPtr, dFloat* const comPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector vector (body->GetCentreOfMass ());
	comPtr[0] = vector.m_x;
	comPtr[1] = vector.m_y;
	comPtr[2] = vector.m_z;
}


/*!
  Return a pointer to the first joint attached to this rigid body.

  @param *bodyPtr pointer to the body.

  @return Joint if at least one is attached to the body, NULL if not joint is attached

  this function will only return the pointer to user defined joints, older build in constraints will be skipped by this function.

  this function can be used to implement recursive walk of complex articulated arrangement of rodid bodies.

  See also: ::NewtonBodyGetNextJoint
*/
NewtonJoint* NewtonBodyGetFirstJoint(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonJoint*)body->GetFirstJoint();
}

/*!
  Return a pointer to the next joint attached to this body.

  @param *bodyPtr pointer to the body.
  @param *jointPtr pointer to current joint.

  @return Joint is at least one joint is attached to the body, NULL if not joint is attached

  this function will only return the pointer to User defined joint, older build in constraints will be skipped by this function.

  this function can be used to implement recursive walk of complex articulated arrangement of rodid bodies.

  See also: ::NewtonBodyGetFirstJoint
*/
NewtonJoint* NewtonBodyGetNextJoint(const NewtonBody* const bodyPtr, const NewtonJoint* const jointPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonJoint*)body->GetNextJoint((dgConstraint*)jointPtr);
}


/*!
  Return a pointer to the first contact joint attached to this rigid body.

  @param *bodyPtr pointer to the body.

  @return Contact if the body is colliding with anther body, NULL otherwise

  See also: ::NewtonBodyGetNextContactJoint, ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact, ::NewtonContactJointRemoveContact
*/
NewtonJoint* NewtonBodyGetFirstContactJoint(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonJoint*)body->GetFirstContact();
}

/*!
  Return a pointer to the next contact joint attached to this rigid body.

  @param *bodyPtr pointer to the body.
  @param *contactPtr pointer to corrent contact joint.

  @return Contact if the body is colliding with anther body, NULL otherwise

  See also: ::NewtonBodyGetFirstContactJoint, ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact, ::NewtonContactJointRemoveContact
*/
NewtonJoint* NewtonBodyGetNextContactJoint(const NewtonBody* const bodyPtr, const NewtonJoint* const contactPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonJoint*)body->GetNextContact((dgConstraint*)contactPtr);
}


/*!
  Return a pointer to the next contact joint the connect these two bodies, if the are colliding

  @param *body0 pointer to one body.
  @param *body1 pointer to secund body.

  @return Contact if the body is colliding with anther body, NULL otherwise

  See also: ::NewtonBodyGetFirstContactJoint, ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact, ::NewtonContactJointRemoveContact
*/
NewtonJoint* NewtonBodyFindContact(const NewtonBody* const body0, const NewtonBody* const body1)
{
	TRACE_FUNCTION(__FUNCTION__);
	const dgBody* const bodyPtr0 = (dgBody*)body0;
	const dgBody* const bodyPtr1 = (dgBody*)body1;
	return (NewtonJoint*)bodyPtr0->GetWorld()->FindContactJoint(bodyPtr0, bodyPtr1);
}

int NewtonJointIsActive(const NewtonJoint* const jointPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const joint = (dgConstraint*) jointPtr;
	return joint->IsActive() ? 1 : 0;
}


NewtonInverseDynamics* NewtonCreateInverseDynamics(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return (NewtonInverseDynamics*)world->CreateInverseDynamics();
}

void NewtonInverseDynamicsDestroy(NewtonInverseDynamics* const inverseDynamics)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*) inverseDynamics;
	Newton* const world = (Newton*)ik->GetWorld();
	world->DestroyInverseDynamics(ik);
}

NewtonJoint* NewtonInverseDynamicsCreateEffector(NewtonInverseDynamics* const inverseDynamics, void* const ikNode, NewtonUserBilateralCallback callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;

	Newton* const world = (Newton *)ik->GetWorld();
	dgInverseDynamics::dgNode* const node = (dgInverseDynamics::dgNode*) ikNode;
	return (NewtonJoint*) new (world->dgWorld::GetAllocator()) NewtonUserJointInverseDynamicsEffector(ik, node, callback);
}

void NewtonInverseDynamicsDestroyEffector(NewtonJoint* const effector)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgAssert(0);
}


void NewtonInverseDynamicsEndBuild (NewtonInverseDynamics* const inverseDynamics)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	ik->Finalize();
}

void NewtonInverseDynamicsUpdate (NewtonInverseDynamics* const inverseDynamics, dFloat timestep, int threadIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	ik->Update(timestep, threadIndex);
}

void* NewtonInverseDynamicsGetRoot(NewtonInverseDynamics* const inverseDynamics)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return ik->GetRoot();
}


NewtonBody* NewtonInverseDynamicsGetBody(NewtonInverseDynamics* const inverseDynamics, void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return (NewtonBody*) ik->GetBody((dgInverseDynamics::dgNode*) node);
}

NewtonJoint* NewtonInverseDynamicsGetJoint(NewtonInverseDynamics* const inverseDynamics, void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return (NewtonJoint*) ik->GetJoint((dgInverseDynamics::dgNode*) node);
}

void* NewtonInverseDynamicsAddRoot(NewtonInverseDynamics* const inverseDynamics, NewtonBody* const root)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*) inverseDynamics;
	return ik->AddRoot((dgDynamicBody*)root);
}

void* NewtonInverseDynamicsAddChildNode(NewtonInverseDynamics* const inverseDynamics, void* const parentNode, NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*) inverseDynamics;
	return ik->AddChild((dgBilateralConstraint*)joint, (dgInverseDynamics::dgNode*) parentNode);
}

void* NewtonInverseDynamicsGetFirstChildNode(NewtonInverseDynamics* const inverseDynamics, void* const parentNode)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return ik->GetFirstChild ((dgInverseDynamics::dgNode*) parentNode);
}

void* NewtonInverseDynamicsGetNextChildNode(NewtonInverseDynamics* const inverseDynamics, void* const node)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return ik->GetNextSiblingChild((dgInverseDynamics::dgNode*) node);
}

bool NewtonInverseDynamicsAddLoopJoint(NewtonInverseDynamics* const inverseDynamics, NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgInverseDynamics* const ik = (dgInverseDynamics*)inverseDynamics;
	return ik->AddLoopJoint((dgBilateralConstraint*)joint);
}

/*!
  Return to number of contact in this contact joint.

  @param *contactJoint pointer to corrent contact joint.

  @return number of contacts.

  See also: ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact, ::NewtonContactJointRemoveContact
*/
int NewtonContactJointGetContactCount(const NewtonJoint* const contactJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;

	if ((joint->GetId() == dgConstraint::m_contactConstraint) && joint->GetCount()){
		return joint->GetCount();
	} else {
		return 0;
	}
}


/*!
  Return to pointer to the first contact from the contact array of the contact joint.

  @param *contactJoint pointer to a contact joint.

  @return a pointer to the first contact from the contact array, NULL if no contacts exist

  See also: ::NewtonContactJointGetNextContact, ::NewtonContactGetMaterial, ::NewtonContactJointRemoveContact
*/
void* NewtonContactJointGetFirstContact(const NewtonJoint* const contactJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;
	if ((joint->GetId() == dgConstraint::m_contactConstraint) && joint->GetCount() && joint->GetMaxDOF()){
		return joint->GetFirst();
	} else {
		return NULL;
	}
}

/*!
  Return a pointer to the next contact from the contact array of the contact joint.

  @param *contactJoint pointer to a contact joint.
  @param *contact pointer to current contact.

  @return a pointer to the next contact in the contact array,  NULL if no contacts exist.

  See also: ::NewtonContactJointGetFirstContact, ::NewtonContactGetMaterial, ::NewtonContactJointRemoveContact
*/
void* NewtonContactJointGetNextContact(const NewtonJoint* const contactJoint, void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;

	if ((joint->GetId() == dgConstraint::m_contactConstraint) && joint->GetCount()){
		dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
		return node->GetNext();
	} else {
		return NULL;
	}
}


/*!
  Return to the next contact from the contact array of the contact joint.

  @param *contactJoint pointer to corrent contact joint.
  @param *contact pointer to current contact.

  @return first contact contact array of the joint contact exist, NULL otherwise

  See also: ::NewtonBodyGetFirstContactJoint, ::NewtonBodyGetNextContactJoint, ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact
*/
void NewtonContactJointRemoveContact(const NewtonJoint* const contactJoint, void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;

	if ((joint->GetId() == dgConstraint::m_contactConstraint) && joint->GetCount()){
		dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;

		dgAssert (joint->GetBody0());
		dgAssert (joint->GetBody1());
		//dgBody* const body = joint->GetBody0() ? joint->GetBody0() : joint->GetBody1();
		//dgWorld* const world = body->GetWorld();
		dgWorld* const world = joint->GetBody0()->GetWorld();
		world->GlobalLock();
		joint->Remove(node);
		joint->GetBody0()->SetSleepState(false);
		joint->GetBody1()->SetSleepState(false);
		world->GlobalUnlock();
	}
}

dFloat NewtonContactJointGetClosestDistance(const NewtonJoint* const contactJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;
	return joint->GetClosestDistance();
}

void NewtonContactJointResetSelftJointCollision(const NewtonJoint* const contactJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;
	joint->ResetSkeletonSelftCollision();
}

void NewtonContactJointResetIntraJointCollision(const NewtonJoint* const contactJoint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgContact* const joint = (dgContact *)contactJoint;
	joint->ResetSkeletonIntraCollision();
}

/*!
  Return to the next contact from the contact array of the contact joint.

  @param *contact pointer to current contact.

  @return first contact contact array of the joint contact exist, NULL otherwise

  See also: ::NewtonContactJointGetFirstContact, ::NewtonContactJointGetNextContact
*/
NewtonMaterial* NewtonContactGetMaterial(const void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
	dgContactMaterial& contactMaterial = node->GetInfo();
	return (NewtonMaterial*) &contactMaterial;
}

NewtonCollision* NewtonContactGetCollision0(const void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
	dgContactMaterial& contactMaterial = node->GetInfo();
	return (NewtonCollision*) contactMaterial.m_collision0;
}

NewtonCollision* NewtonContactGetCollision1(const void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
	dgContactMaterial& contactMaterial = node->GetInfo();
	return (NewtonCollision*) contactMaterial.m_collision1;
}

void* NewtonContactGetCollisionID0(const void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
	dgContactMaterial& contactMaterial = node->GetInfo();
	return (void*) contactMaterial.m_shapeId0;
}

void* NewtonContactGetCollisionID1(const void* const contact)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgList<dgContactMaterial>::dgListNode* const node = (dgList<dgContactMaterial>::dgListNode*) contact;
	dgContactMaterial& contactMaterial = node->GetInfo();
	return (NewtonCollision*) contactMaterial.m_shapeId1;
}




/*!
  Assign a collision primitive to the body.

  @param *bodyPtr pointer to the body.
  @param *collisionPtr pointer to the new collision geometry.

  @return Nothing.

  This function replaces a collision geometry of a body with the new collision geometry.
  This function increments the reference count of the collision geometry and decrements the reference count
  of the old collision geometry. If the reference count of the old collision geometry reaches zero, the old collision geometry is destroyed.
  This function can be used to swap the collision geometry of bodies at runtime.

  See also: ::NewtonCreateDynamicBody, ::NewtonBodyGetCollision
*/
void NewtonBodySetCollision(const NewtonBody* const bodyPtr, const NewtonCollision* const collisionPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgCollisionInstance* const collision = (dgCollisionInstance*) collisionPtr;
	body->AttachCollision (collision);
}

void NewtonBodySetCollisionScale (const NewtonBody* const bodyPtr, dFloat scaleX, dFloat scaleY, dFloat scaleZ)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgWorld* const world = body->GetWorld();
	NewtonCollision* const collision = NewtonBodyGetCollision(bodyPtr);

	dgFloat32 mass = body->GetInvMass().m_w > dgFloat32 (0.0f) ? body->GetMass().m_w : dgFloat32 (0.0f);
	NewtonCollisionSetScale (collision, scaleX, scaleY, scaleZ);

	NewtonJoint* nextJoint;
	for (NewtonJoint* joint = NewtonBodyGetFirstContactJoint(bodyPtr); joint; joint = nextJoint) {
		dgConstraint* const contactJoint = (dgConstraint*)joint;
		nextJoint = NewtonBodyGetNextContactJoint(bodyPtr, joint);
		//world->DestroyConstraint (contactJoint);
		contactJoint->ResetMaxDOF();
	}
    NewtonBodySetMassProperties (bodyPtr, mass, collision);
	body->UpdateCollisionMatrix(dgFloat32(0.0f), 0);
	world->GetBroadPhase()->ResetEntropy ();
}


/*!
  Get the collision primitive of a body.

  @param *bodyPtr pointer to the body.

  @return Pointer to body collision geometry.

  This function does not increment the reference count of the collision geometry.

  See also: ::NewtonCreateDynamicBody, ::NewtonBodySetCollision
*/
NewtonCollision* NewtonBodyGetCollision(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return (NewtonCollision*) body->GetCollision();
}


/*!
  Assign a material group id to the body.

  @param *bodyPtr pointer to the body.
  @param id id of a previously created material group.

  @return Nothing.

  When the application creates a body, the default material group, *defaultGroupId*, is applied by default.

  See also: ::NewtonBodyGetMaterialGroupID, ::NewtonMaterialCreateGroupID, ::NewtonMaterialGetDefaultGroupID
*/
void NewtonBodySetMaterialGroupID(const NewtonBody* const bodyPtr, int id)
{
	dgBody* const body = (dgBody *)bodyPtr;

	TRACE_FUNCTION(__FUNCTION__);
	body->SetGroupID (dgUnsigned32 (id));
}


/*!
  Get the material group id of the body.

  @param *bodyPtr pointer to the body.

  @return Nothing.

  See also: ::NewtonBodySetMaterialGroupID
*/
int NewtonBodyGetMaterialGroupID(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return int (body->GetGroupID ());
}

/*!
  Set the continuous collision state mode for this rigid body.
  continuous collision flag is off by default in when bodies are created.

  @param *bodyPtr pointer to the body.
  @param state collision state. 1 indicates this body may tunnel through other objects while moving at high speed. 0 ignore high speed collision checks.

  @return Nothing.

  continuous collision mode enable allow the engine to predict colliding contact on rigid bodies
  Moving at high speed of subject to strong forces.

  continuous collision mode does not prevent rigid bodies from inter penetration instead it prevent bodies from
  passing trough each others by extrapolating contact points when the bodies normal contact calculation determine the bodies are not colliding.

  for performance reason the bodies angular velocities is only use on the broad face of the collision,
  but not on the contact calculation.

  continuous collision does not perform back tracking to determine time of contact, instead it extrapolate contact by incrementally
  extruding the collision geometries of the two colliding bodies along the linear velocity of the bodies during the time step,
  if during the extrusion colliding contact are found, a collision is declared and the normal contact resolution is called.

  for continuous collision to be active the continuous collision mode must on the material pair of the colliding bodies as well as on at least one of the two colliding bodies.

  Because there is penalty of about 40% to 80% depending of the shape complexity of the collision geometry, this feature is set
  off by default. It is the job of the application to determine what bodies need this feature on. Good guidelines are: very small objects,
  and bodies that move a height speed.

  See also: ::NewtonBodyGetContinuousCollisionMode, ::NewtonBodySetContinuousCollisionMode
*/
void NewtonBodySetContinuousCollisionMode(const NewtonBody* const bodyPtr, unsigned state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetContinueCollisionMode (state ? true : false);
}

int NewtonBodyGetSerializedID(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetSerializedID();
}

/*!
  Get the continuous collision state mode for this rigid body.

  @param *bodyPtr pointer to the body.

  @return Nothing.


  Because there is there is penalty of about 3 to 5 depending of the shape complexity of the collision geometry, this feature is set
  off by default. It is the job of the application to determine what bodies need this feature on. Good guidelines are: very small objects,
  and bodies that move a height speed.

  this feature is currently disabled:

  See also: ::NewtonBodySetContinuousCollisionMode, ::NewtonBodySetContinuousCollisionMode
*/
int NewtonBodyGetContinuousCollisionMode (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetContinueCollisionMode () ? 1 : false;
}



/*!
  Set the collision state flag of this body when the body is connected to another body by a hierarchy of joints.

  @param *bodyPtr pointer to the body.
  @param state collision state. 1 indicates this body will collide with any linked body. 0 disable collision with body connected to this one by joints.

  @return Nothing.

  sometimes when making complicated arrangements of linked bodies it is possible the collision geometry of these bodies is in the way of the
  joints work space. This could be a problem for the normal operation of the joints. When this situation happens the application can determine which bodies
  are the problem and disable collision for those bodies while they are linked by joints. For the collision to be disable for a pair of body,
  both bodies must have the collision disabled. If the joints connecting the bodies are destroyed these bodies become collidable automatically.
  This feature can also be achieved by making special material for the whole configuration of jointed bodies, however it is a lot easier just to set collision disable
  for jointed bodies.

  See also: ::NewtonBodySetMaterialGroupID
*/
void NewtonBodySetJointRecursiveCollision(const NewtonBody* const bodyPtr, unsigned state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetCollisionWithLinkedBodies (state ? true : false);
}

/*!
  Get the collision state flag when the body is joint.

  @param *bodyPtr pointer to the body.

  @return return the collision state flag for this body.

  See also: ::NewtonBodySetMaterialGroupID
*/
int NewtonBodyGetJointRecursiveCollision (const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	return body->GetCollisionWithLinkedBodies () ? 1 : 0;
}

/*!
  get the freeze state of this body

  @param *bodyPtr is the pointer to the body to be frozen

  @return 1 id the bode is frozen, 0 if bode is unfrozen.

  When a body is created it is automatically placed in the active simulation list. As an optimization
  for large scenes, you may use this function to put background bodies in an inactive equilibrium state.

  This function tells Newton that this body does not currently need to be simulated.
  However, if the body is part of a larger configuration it may be affected indirectly by the reaction forces
  of objects that it is connected to.

  See also: ::NewtonBodySetAutoSleep, ::NewtonBodyGetAutoSleep
*/
int NewtonBodyGetFreezeState(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetFreeze() ? 1 : 0;
}


/*!
  This function tells Newton to simulate or suspend simulation of this body and all other bodies in contact with it

  @param *bodyPtr is the pointer to the body to be activated
  @param state 1 teels newton to freeze the bode and allconceted bodiesm, 0 to unfreze it

  @return Nothing

  This function to no activate the body, is just lock or unlock the body for physics simulation.

  See also: ::NewtonBodyGetFreezeState, ::NewtonBodySetAutoSleep, ::NewtonBodyGetAutoSleep
*/
void NewtonBodySetFreezeState(const NewtonBody* const bodyPtr, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetFreeze(state ? true : false);
}

/*!
  Set the auto-activation mode for this body.

  @param *bodyPtr is the pointer to the body.
  @param state active mode: 1 = auto-activation on (controlled by Newton). 0 = auto-activation off and body is active all the time.

  @return Nothing.

  Bodies are created with auto-activation on by default.

  Auto activation enabled is the default state for the majority of bodies in a large scene.
  However, for player control, ai control or some other special circumstance, the application may want to control
  the activation/deactivation of the body.
  In that case, the application may call NewtonBodySetAutoSleep (body, 0) followed by
  NewtonBodySetFreezeState(body), this will make the body active forever.

  See also: ::NewtonBodyGetFreezeState, ::NewtonBodySetFreezeState, ::NewtonBodyGetAutoSleep
*/
void NewtonBodySetAutoSleep(const NewtonBody* const bodyPtr, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetAutoSleep (state ? true : false);
}

/*!
  Get the auto-activation state of the body.

  @param *bodyPtr is the pointer to the body.

  @return Auto activation state: 1 = auto-activation on. 0 = auto-activation off.

  See also: ::NewtonBodySetAutoSleep, ::NewtonBodyGetSleepState
*/
int NewtonBodyGetAutoSleep(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	return body->GetAutoSleep () ? 1 : 0;
}


/*!
  Return the sleep mode of a rigid body.

  @param *bodyPtr is the pointer to the body.

  @return Sleep state: 1 = active. 0 = sleeping.

  See also: ::NewtonBodySetAutoSleep
*/
int NewtonBodyGetSleepState(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	return body->GetSleepState() ? 1 : 0;
}

void NewtonBodySetSleepState(const NewtonBody* const bodyPtr, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->SetSleepState(state ? true : false);
}


/*!
  Get the world axis aligned bounding box (AABB) of the body.

  @param *bodyPtr is the pointer to the body.
  @param  *p0 - pointer to an array of at least three floats to hold minimum value for the AABB.
  @param  *p1 - pointer to an array of at least three floats to hold maximum value for the AABB.

*/
void NewtonBodyGetAABB(const NewtonBody* const bodyPtr, dFloat* const p0, dFloat* const p1)	
{
	TRACE_FUNCTION(__FUNCTION__);

	dgVector vector0;
	dgVector vector1;

	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	body->GetAABB (vector0, vector1);

	p0[0] = vector0.m_x;
	p0[1] = vector0.m_y;
	p0[2] = vector0.m_z;

	p1[0] = vector1.m_x;
	p1[1] = vector1.m_y;
	p1[2] = vector1.m_z;
}

/*!
  Set the global linear velocity of the body.

  @param *bodyPtr is the pointer to the body.
  @param *velocity pointer to an array of at least three floats containing the velocity vector.

  See also: ::NewtonBodyGetVelocity
*/
void NewtonBodySetVelocity(const NewtonBody* const bodyPtr, const dFloat* const velocity)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector (velocity[0], velocity[1], velocity[2], dgFloat32 (0.0f));
	body->SetVelocity (vector);
}

void NewtonBodySetVelocityNoSleep(const NewtonBody* const bodyPtr, const dFloat* const velocity)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector(velocity[0], velocity[1], velocity[2], dgFloat32(0.0f));
	body->SetVelocityNoSleep(vector);
}


/*!
  Get the global linear velocity of the body.

  @param *bodyPtr is the pointer to the body.
  @param *velocity pointer to an array of at least three floats to hold the velocity vector.

  See also: ::NewtonBodySetVelocity
*/
void NewtonBodyGetVelocity(const NewtonBody* const bodyPtr, dFloat* const velocity)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector (body->GetVelocity());
	velocity[0] = vector.m_x;
	velocity[1] = vector.m_y;
	velocity[2] = vector.m_z;
}

/*!
  Set the global angular velocity of the body.

  @param *bodyPtr is the pointer to the body.
  @param *omega pointer to an array of at least three floats containing the angular velocity vector.

  See also: ::NewtonBodyGetOmega
*/
void NewtonBodySetOmega(const NewtonBody* const bodyPtr, const dFloat* const omega)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector (omega[0], omega[1], omega[2], dgFloat32 (0.0f));
	body->SetOmega (vector);
}



void NewtonBodySetOmegaNoSleep(const NewtonBody* const bodyPtr, const dFloat* const omega)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector(omega[0], omega[1], omega[2], dgFloat32(0.0f));
	body->SetOmegaNoSleep(vector);
}


/*!
  Get the global angular velocity of the body.

  @param *bodyPtr is the pointer to the body
  @param *omega pointer to an array of at least three floats to hold the angular velocity vector.

  See also: ::NewtonBodySetOmega
*/
void NewtonBodyGetOmega(const NewtonBody* const bodyPtr, dFloat* const omega)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector (body->GetOmega());
	omega[0] = vector.m_x;
	omega[1] = vector.m_y;
	omega[2] = vector.m_z;
}


/*!
Get the global angular accelration of the body.

@param *bodyPtr is the pointer to the body
@param *omega pointer to an array of at least three floats to hold the angular acceleration vector.
*/
void NewtonBodyGetAlpha(const NewtonBody* const bodyPtr, dFloat* const alpha)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	const dgVector vector(body->GetAlpha());
	alpha[0] = vector.m_x;
	alpha[1] = vector.m_y;
	alpha[2] = vector.m_z;
}


/*!
Get the global linear Acceleration of the body.

@param *bodyPtr is the pointer to the body.
@param *acceleration pointer to an array of at least three floats to hold the acceleration vector.

See also: ::NewtonBodySetVelocity
*/
void NewtonBodyGetAcceleration(const NewtonBody* const bodyPtr, dFloat* const acceleration)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector(body->GetAccel());
	acceleration[0] = vector.m_x;
	acceleration[1] = vector.m_y;
	acceleration[2] = vector.m_z;
}

/*!
  Apply the linear viscous damping coefficient to the body.

  @param *bodyPtr is the pointer to the body.
  @param linearDamp linear damping coefficient.

  the default value of *linearDamp* is clamped to a value between 0.0 and 1.0; the default value is 0.1,
  There is a non zero implicit attenuation value of 0.0001 assume by the integrator.

  The dampening viscous friction force is added to the external force applied to the body every frame before going to the solver-integrator.
  This force is proportional to the square of the magnitude of the velocity to the body in the opposite direction of the velocity of the body.
  An application can set *linearDamp* to zero when the application takes control of the external forces and torque applied to the body, should the application
  desire to have absolute control of the forces over that body. However, it is recommended that the *linearDamp* coefficient is set to a non-zero
  value for the majority of background bodies. This saves the application from having to control these forces and also prevents the integrator from
  adding very large velocities to a body.

  See also: ::NewtonBodyGetLinearDamping
*/
void NewtonBodySetLinearDamping(const NewtonBody* const bodyPtr, dFloat linearDamp)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	body->SetLinearDamping (linearDamp);
}

/*!
  Get the linear viscous damping of the body.

  @param *bodyPtr is the pointer to the body.

  @return The linear damping coefficient.

  See also: ::NewtonBodySetLinearDamping
*/
dFloat NewtonBodyGetLinearDamping(const NewtonBody* const bodyPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	return body->GetLinearDamping();
}


/*!
  Apply the angular viscous damping coefficient to the body.

  @param *bodyPtr is the pointer to the body.
  @param *angularDamp pointer to an array of at least three floats containing the angular damping coefficients for the principal axis of the body.

  the default value of *angularDamp* is clamped to a value between 0.0 and 1.0; the default value is 0.1,
  There is a non zero implicit attenuation value of 0.0001 assumed by the integrator.

  The dampening viscous friction torque is added to the external torque applied to the body every frame before going to the solver-integrator.
  This torque is proportional to the square of the magnitude of the angular velocity to the body in the opposite direction of the angular velocity of the body.
  An application can set *angularDamp* to zero when the to take control of the external forces and torque applied to the body, should the application
  desire to have absolute control of the forces over that body. However, it is recommended that the *linearDamp* coefficient be set to a non-zero
  value for the majority of background bodies. This saves the application from needing to control these forces and also prevents the integrator from
  adding very large velocities to a body.

  See also: ::NewtonBodyGetAngularDamping
*/
void  NewtonBodySetAngularDamping(const NewtonBody* const bodyPtr, const dFloat* angularDamp)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	dgVector vector (angularDamp[0], angularDamp[1], angularDamp[2], dgFloat32 (0.0f));
	body->SetAngularDamping (vector);
}


/*!
  Get the linear viscous damping of the body.

  @param *bodyPtr is the pointer to the body.
  @param *angularDamp pointer to an array of at least three floats to hold the angular damping coefficient for the principal axis of the body.

  See also: ::NewtonBodySetAngularDamping
*/
void  NewtonBodyGetAngularDamping(const NewtonBody* const bodyPtr, dFloat* angularDamp)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

//	dgVector& vector = *((dgVector*) angularDamp);
//	vector = body->GetAngularDamping();

	dgVector vector (body->GetAngularDamping());
	angularDamp[0] = vector.m_x;
	angularDamp[1] = vector.m_y;
	angularDamp[2] = vector.m_z;
}



void NewtonBodyGetPointVelocity (const NewtonBody* const bodyPtr, const dFloat* const point, dFloat* const velocOut)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;
	dgVector veloc (body->GetVelocityAtPoint (dgVector (point[0], point[1], point[2], dgFloat32 (0.0f))));
	velocOut[0] = veloc[0];
	velocOut[1] = veloc[1];
	velocOut[2] = veloc[2];
}

/*!
  Add an impulse to a specific point on a body.

  @param *bodyPtr is the pointer to the body.
  @param pointDeltaVeloc pointer to an array of at least three floats containing the desired change in velocity to point pointPosit.
  @param  pointPosit	- pointer to an array of at least three floats containing the center of the impulse in global space.
  @param timestep - the update rate time step.

  @return Nothing.

  This function will activate the body.

  *pointPosit* and *pointDeltaVeloc* must be specified in global space.

  *pointDeltaVeloc* represent a change in velocity. For example, a value of *pointDeltaVeloc* of (1, 0, 0) changes the velocity
  of *bodyPtr* in such a way that the velocity of point *pointDeltaVeloc* will increase by (1, 0, 0)

  *the calculate impulse will be applied to the body on next frame update

  Because *pointDeltaVeloc* represents a change in velocity, this function must be used with care. Repeated calls
  to this function will result in an increase of the velocity of the body and may cause to integrator to lose stability.
*/
void NewtonBodyAddImpulse(const NewtonBody* const bodyPtr, const dFloat* const pointDeltaVeloc, const dFloat* const pointPosit, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	if (body->GetInvMass().m_w > dgFloat32 (0.0f)) {
		dgVector p (pointPosit[0], pointPosit[1], pointPosit[2], dgFloat32 (0.0f));
		dgVector v (pointDeltaVeloc[0], pointDeltaVeloc[1], pointDeltaVeloc[2], dgFloat32 (0.0f));
		body->AddImpulse (v, p, timestep);
	}
}


/*!
  Add an train of impulses to a specific point on a body.

  @param *bodyPtr is the pointer to the body.
  @param  impulseCount	- number of impulses and distances in the array distance
  @param  strideInByte	- sized in bytes of vector impulse and
  @param impulseArray pointer to an array containing the desired impulse to apply ate position point array.
  @param pointArray pointer to an array of at least three floats containing the center of the impulse in global space.
  @param timestep - the update rate time step.

  @return Nothing.

  This function will activate the body.

  *pointPosit* and *pointDeltaVeloc* must be specified in global space.

  *the calculate impulse will be applied to the body on next frame update

  this function apply at general impulse to a body a oppose to a desired change on velocity
  this mean that the body mass, and Inertia will determine the gain on velocity.
*/
void NewtonBodyApplyImpulseArray (const NewtonBody* const bodyPtr, int impulseCount, int strideInByte, const dFloat* const impulseArray, const dFloat* const pointArray, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	if (body->GetInvMass().m_w > dgFloat32 (0.0f)) {
		body->ApplyImpulsesAtPoint (impulseCount, strideInByte, impulseArray, pointArray, timestep);
	}
}

void NewtonBodyApplyImpulsePair (const NewtonBody* const bodyPtr, dFloat* const linearImpulse, dFloat* const angularImpulse, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	if (body->GetInvMass().m_w > dgFloat32 (0.0f)) {
		dgVector l (linearImpulse[0], linearImpulse[1], linearImpulse[2], dgFloat32 (0.0f));
		dgVector a (angularImpulse[0], angularImpulse[1], angularImpulse[2], dgFloat32 (0.0f));
		body->ApplyImpulsePair (l, a, timestep);
	}
}

void NewtonBodyIntegrateVelocity (const NewtonBody* const bodyPtr, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBody* const body = (dgBody *)bodyPtr;

	if (body->GetInvMass().m_w > dgFloat32 (0.0f)) {
		body->IntegrateVelocity(timestep);
	}
}

/*! @} */ // end of RigidBodyInterface

/*! @defgroup ConstraintBall ConstraintBall
Ball and Socket joint interface
@{
*/


/*!
  Create a ball an socket joint.

  @param *newtonWorld Pointer to the Newton world.
  @param *pivotPoint is origin of ball and socket in global space.
  @param *childBody is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.
  @param *parentBody is the pointer to the parent rigid body, this body can be NULL or any kind of rigid body.

  @return Pointer to the ball and socket joint.

  This function creates a ball and socket and add it to the world. By default joint disables collision with the linked bodies.
*/
NewtonJoint* NewtonConstraintCreateBall(const NewtonWorld* const newtonWorld, 
	const dFloat* pivotPoint, 
	const NewtonBody* const childBody, 
	const NewtonBody* const parentBody)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)childBody;
	dgBody* const body1 = (dgBody *)parentBody;
	dgVector pivot (pivotPoint[0], pivotPoint[1], pivotPoint[2], dgFloat32 (0.0f));
	return (NewtonJoint*) world->CreateBallConstraint (pivot, body0, body1);
}

/*!
  Set the ball and socket cone and twist limits.

  @param *ball is the pointer to a ball and socket joint.
  @param *pin pointer to a unit vector defining the cone axis in global space.
  @param maxConeAngle max angle in radians the attached body is allow to swing relative to the pin axis, a value of zero will disable this limits.
  @param maxTwistAngle max angle in radians the attached body is allow to twist relative to the pin axis, a value of zero will disable this limits.

  limits are disabled at creation time. A value of zero for *maxConeAngle* disable the cone limit, a value of zero for *maxTwistAngle* disable the twist limit
  all non-zero value for *maxConeAngle* are clamped between 5 degree and 175 degrees

  See also: ::NewtonConstraintCreateBall
*/
void NewtonBallSetConeLimits(const NewtonJoint* const ball, const dFloat* pin, dFloat maxConeAngle, dFloat maxTwistAngle)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBallConstraint* const joint = (dgBallConstraint*) ball;

	dgVector coneAxis (pin[0], pin[1], pin[2], dgFloat32 (0.0f)); 

	if (coneAxis.DotProduct3(coneAxis) < 1.0e-3f) {
		coneAxis.m_x = dgFloat32(1.0f);
	}
	dgVector tmp (dgFloat32 (1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f)); 
	if (dgAbs (tmp.DotProduct3(coneAxis)) > dgFloat32 (0.999f)) {
		tmp = dgVector (dgFloat32 (0.0f), dgFloat32(1.0f), dgFloat32 (0.0f), dgFloat32 (0.0f)); 
		if (dgAbs (tmp.DotProduct3(coneAxis)) > dgFloat32 (0.999f)) {
			tmp = dgVector (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32(1.0f), dgFloat32 (0.0f)); 
			dgAssert (dgAbs (tmp.DotProduct3(coneAxis)) < dgFloat32 (0.999f));
		}
	}
	dgVector lateral (tmp.CrossProduct(coneAxis)); 
	dgAssert(lateral.m_w = dgFloat32(0.0f));
	dgAssert(coneAxis.m_w = dgFloat32(0.0f));
	lateral = lateral.Normalize();
	coneAxis = coneAxis.Normalize();

	maxConeAngle = dgAbs (maxConeAngle);
	maxTwistAngle = dgAbs (maxTwistAngle);
	joint->SetConeLimitState ((maxConeAngle > dgDEG2RAD) ? true : false); 
	joint->SetTwistLimitState ((maxTwistAngle > dgDEG2RAD) ? true : false);
	joint->SetLatealLimitState (false); 
	joint->SetLimits (coneAxis, -maxConeAngle, maxConeAngle, maxTwistAngle, lateral, 0.0f, 0.0f);
}


/*!
  Set an update call back to be called when either of the two bodies linked by the joint is active.

  @param *ball pointer to the joint.
  @param callback pointer to the joint function call back.

  @return nothing.

  if the application wants to have some feedback from the joint simulation, the application can register a function
  update callback to be called every time any of the bodies linked by this joint is active. This is useful to provide special
  effects like particles, sound or even to simulate breakable moving parts.

  See also: ::NewtonJointSetUserData
*/
void NewtonBallSetUserCallback(const NewtonJoint* const ball, NewtonBallCallback callback)
{
	dgBallConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgBallConstraint*) ball;
	contraint->SetJointParameterCallback ((dgBallJointFriction)callback);
}


/*!
  Get the relative joint angle between the two bodies.

  @param *ball pointer to the joint.
  @param *angle pointer to an array of a least three floats to hold the joint relative Euler angles.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonBallSetUserCallback
*/
void NewtonBallGetJointAngle (const NewtonJoint* const ball, dFloat* angle)
{
	dgBallConstraint* contraint;

	contraint = (dgBallConstraint*) ball;
	dgVector angleVector (contraint->GetJointAngle ());

	TRACE_FUNCTION(__FUNCTION__);
	angle[0] = angleVector.m_x;
	angle[1] = angleVector.m_y;
	angle[2] = angleVector.m_z;
}

/*!
  Get the relative joint angular velocity between the two bodies.

  @param *ball pointer to the joint.
  @param *omega pointer to an array of a least three floats to hold the joint relative angular velocity.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonBallSetUserCallback
*/
void NewtonBallGetJointOmega(const NewtonJoint* const ball, dFloat* omega)
{
	dgBallConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgBallConstraint*) ball;
	dgVector omegaVector (contraint->GetJointOmega ());
	omega[0] = omegaVector.m_x;
	omega[1] = omegaVector.m_y;
	omega[2] = omegaVector.m_z;
}

/*!
  Get the total force asserted over the joint pivot point, to maintain the constraint.

  @param *ball pointer to the joint.
  @param *force pointer to an array of a least three floats to hold the force value of the joint.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can destroy the joint if the force exceeds some predefined value.

  See also: ::NewtonBallSetUserCallback
*/
void NewtonBallGetJointForce(const NewtonJoint* const ball, dFloat* const force)
{
  // fixme: type? "constraint" instead of "contraint"?
	dgBallConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgBallConstraint*) ball;
	dgVector forceVector (contraint->GetJointForce ());
	force[0] = forceVector.m_x;
	force[1] = forceVector.m_y;
	force[2] = forceVector.m_z;
}


/*! @} */ // end of ConstraintBall

/*! @defgroup JointSlider JointSlider
Slider joint interface
@{
*/

/*!
  Create a slider joint.

  @param *newtonWorld Pointer to the Newton world.
  @param *pivotPoint is origin of the slider in global space.
  @param *pinDir is the line of action of the slider in global space.
  @param *childBody is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.
  @param *parentBody is the pointer to the parent rigid body, this body can be NULL or any kind of rigid body.

  @return Pointer to the slider joint.

  This function creates a slider and add it to the world. By default joint disables collision with the linked bodies.
*/
NewtonJoint* NewtonConstraintCreateSlider(const NewtonWorld* const newtonWorld, const dFloat* pivotPoint, const dFloat* pinDir, const NewtonBody* const childBody, const NewtonBody* const parentBody)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)childBody;
	dgBody* const body1 = (dgBody *)parentBody;
	dgVector pin (pinDir[0], pinDir[1], pinDir[2], dgFloat32 (0.0f));
	dgVector pivot (pivotPoint[0], pivotPoint[1], pivotPoint[2], dgFloat32 (0.0f));
	return (NewtonJoint*) world->CreateSlidingConstraint (pivot, pin, body0, body1);
}


/*!
  Set an update call back to be called when either of the two body linked by the joint is active.

  @param *slider pointer to the joint.
  @param callback pointer to the joint function call back.

  @return nothing.

  if the application wants to have some feedback from the joint simulation, the application can register a function
  update callback to be call every time any of the bodies linked by this joint is active. This is useful to provide special
  effects like particles, sound or even to simulate breakable moving parts.

  See also: ::NewtonJointGetUserData, ::NewtonJointSetUserData
*/
void NewtonSliderSetUserCallback(const NewtonJoint* const slider, NewtonSliderCallback callback)
{
	dgSlidingConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgSlidingConstraint*) slider;
	contraint->SetJointParameterCallback ((dgSlidingJointAcceleration)callback);
}

/*!
  Get the relative joint angle between the two bodies.

  @param *Slider pointer to the joint.

  @return the joint angle relative to the hinge pin.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonSliderSetUserCallback
*/
dFloat NewtonSliderGetJointPosit (const NewtonJoint* Slider)
{
	dgSlidingConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgSlidingConstraint*) Slider;
	return contraint->GetJointPosit ();
}

/*!
  Get the relative joint angular velocity between the two bodies.

  @param *Slider pointer to the joint.

  @return the joint angular velocity relative to the pin axis.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonSliderSetUserCallback
*/
dFloat NewtonSliderGetJointVeloc(const NewtonJoint* Slider)
{
	dgSlidingConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgSlidingConstraint*) Slider;
	return contraint->GetJointVeloc ();
}


/*!
  Calculate the angular acceleration needed to stop the slider at the desired angle.

  @param *slider pointer to the joint.
  @param *desc is the pointer to the Slider or slide structure.
  @param distance desired stop distance relative to the pivot point

  fixme: inconsistent variable capitalisation; some functions use "slider", others "Slider".

  @return the relative linear acceleration needed to stop the slider.

  this function can only be called from a *NewtonSliderCallback* and it can be used by the application to implement slider limits.

  See also: ::NewtonSliderSetUserCallback
*/
dFloat NewtonSliderCalculateStopAccel(const NewtonJoint* const slider, const NewtonHingeSliderUpdateDesc* const desc, dFloat distance)
{
	dgSlidingConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgSlidingConstraint*) slider;
	return contraint->CalculateStopAccel (distance, (dgJointCallbackParam*) desc);
}

/*!
  Get the total force asserted over the joint pivot point, to maintain the constraint.

  @param *slider pointer to the joint.
  @param *force pointer to an array of a least three floats to hold the force value of the joint.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can destroy the joint if the force exceeds some predefined value.

  See also: ::NewtonSliderSetUserCallback
*/
void NewtonSliderGetJointForce(const NewtonJoint* const slider, dFloat* const force)
{
	dgSlidingConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgSlidingConstraint*) slider;
	dgVector forceVector (contraint->GetJointForce ());
	force[0] = forceVector.m_x;
	force[1] = forceVector.m_y;
	force[2] = forceVector.m_z;
}


/*! @} */ // end of JointSlider

/*! @defgroup JointCorkscrew JointCorkscrew
Corkscrew joint interface
@{
*/

/*!
  Create a corkscrew joint.

  @param *newtonWorld Pointer to the Newton world.
  @param *pivotPoint is origin of the corkscrew in global space.
  @param *pinDir is the line of action of the corkscrew in global space.
  @param *childBody is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.
  @param *parentBody is the pointer to the parent rigid body, this body can be NULL or any kind of rigid body.

  @return Pointer to the corkscrew joint.

  This function creates a corkscrew and add it to the world. By default joint disables collision with the linked bodies.
*/
NewtonJoint* NewtonConstraintCreateCorkscrew(const NewtonWorld* const newtonWorld, const dFloat* pivotPoint, const dFloat* pinDir, const NewtonBody* const childBody, const NewtonBody* const parentBody)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)childBody;
	dgBody* const body1 = (dgBody *)parentBody;
	dgVector pin (pinDir[0], pinDir[1], pinDir[2], dgFloat32 (0.0f));
	dgVector pivot (pivotPoint[0], pivotPoint[1], pivotPoint[2], dgFloat32 (0.0f));
	return (NewtonJoint*) world->CreateCorkscrewConstraint (pivot, pin, body0, body1);
}

/*!
  Set an update call back to be called when either of the two body linked by the joint is active.

  @param *corkscrew pointer to the joint.
  @param callback pointer to the joint function call back.

  @return nothing.

  if the application wants to have some feedback from the joint simulation, the application can register a function
  update callback to be call every time any of the bodies linked by this joint is active. This is useful to provide special
  effects like particles, sound or even to simulate breakable moving parts.

  the function *NewtonCorkscrewCallback callback* should return a bit field code.
  if the application does not want to set the joint acceleration the return code is zero
  if the application only wants to change the joint linear acceleration the return code is 1
  if the application only wants to change the joint angular acceleration the return code is 2
  if the application only wants to change the joint angular and linear acceleration the return code is 3

  See also: ::NewtonJointGetUserData, ::NewtonJointSetUserData
*/
void NewtonCorkscrewSetUserCallback(const NewtonJoint* const corkscrew, NewtonCorkscrewCallback callback)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	contraint->SetJointParameterCallback ((dgCorkscrewJointAcceleration)callback);
}

/*!
  Get the relative joint angle between the two bodies.

  @param *corkscrew pointer to the joint.

  @return the joint angle relative to the hinge pin.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewGetJointPosit (const NewtonJoint* const corkscrew)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->GetJointPosit ();
}

/*!
  Get the relative joint angular velocity between the two bodies.

  @param *corkscrew pointer to the joint.

  @return the joint angular velocity relative to the pin axis.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewGetJointVeloc(const NewtonJoint* const corkscrew)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->GetJointVeloc ();
}

/*!
  Get the relative joint angle between the two bodies.

  @param *corkscrew pointer to the joint.

  @return the joint angle relative to the corkscrew pin.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewGetJointAngle (const NewtonJoint* const corkscrew)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->GetJointAngle ();

}

/*!
  Get the relative joint angular velocity between the two bodies.

  @param *corkscrew pointer to the joint.

  @return the joint angular velocity relative to the pin axis.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewGetJointOmega(const NewtonJoint* const corkscrew)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->GetJointOmega ();
}


/*!
  Calculate the angular acceleration needed to stop the corkscrew at the desired angle.

  @param *corkscrew pointer to the joint.
  @param *desc is the pointer to the Corkscrew or slide structure.
  @param angle is the desired corkscrew stop angle

  @return the relative angular acceleration needed to stop the corkscrew.

  this function can only be called from a *NewtonCorkscrewCallback* and it can be used by the application to implement corkscrew limits.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewCalculateStopAlpha (const NewtonJoint* const corkscrew, const NewtonHingeSliderUpdateDesc* const desc, dFloat angle)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->CalculateStopAlpha (angle, (dgJointCallbackParam*) desc);
}


/*!
  Calculate the angular acceleration needed to stop the corkscrew at the desired angle.

  @param *corkscrew pointer to the joint.
  @param *desc is the pointer to the Corkscrew or slide structure.
  @param distance desired stop distance relative to the pivot point

  @return the relative linear acceleration needed to stop the corkscrew.

  this function can only be called from a *NewtonCorkscrewCallback* and it can be used by the application to implement corkscrew limits.

  See also: ::NewtonCorkscrewSetUserCallback
*/
dFloat NewtonCorkscrewCalculateStopAccel(const NewtonJoint* const corkscrew, const NewtonHingeSliderUpdateDesc* const desc, dFloat distance)
{
	dgCorkscrewConstraint* contraint;
	contraint = (dgCorkscrewConstraint*) corkscrew;
	return contraint->CalculateStopAccel (distance, (dgJointCallbackParam*) desc);
}

/*!
  Get the total force asserted over the joint pivot point, to maintain the constraint.

  @param *corkscrew pointer to the joint.
  @param *force pointer to an array of a least three floats to hold the force value of the joint.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can destroy the joint if the force exceeds some predefined value.

  See also: ::NewtonCorkscrewSetUserCallback
*/
void NewtonCorkscrewGetJointForce(const NewtonJoint* const corkscrew, dFloat* const force)
{
	dgCorkscrewConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgCorkscrewConstraint*) corkscrew;
	dgVector forceVector (contraint->GetJointForce ());
	force[0] = forceVector.m_x;
	force[1] = forceVector.m_y;
	force[2] = forceVector.m_z;
}


/*! @} */ // end of JointSlider

/*! @defgroup JointUniversal JointUniversal
Universal joint interface
@{
*/

/*!
  Create a universal joint.

  @param *newtonWorld Pointer to the Newton world.
  @param *pivotPoint is origin of the universal joint in global space.
  @param  *pinDir0 - first axis of rotation fixed on childBody body and perpendicular to pinDir1.
  @param  *pinDir1 - second axis of rotation fixed on parentBody body and perpendicular to pinDir0.
  @param *childBody is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.
  @param *parentBody is the pointer to the parent rigid body, this body can be NULL or any kind of rigid body.

  @return Pointer to the universal joint.

  This function creates a universal joint and add it to the world. By default joint disables collision with the linked bodies.

  a universal joint is a constraint that restricts twp rigid bodies to be connected to a point fixed on both bodies,
  while and allowing one body to spin around a fix axis in is own frame, and the other body to spin around another axis fixes on
  it own frame. Both axis must be mutually perpendicular.
*/
NewtonJoint* NewtonConstraintCreateUniversal(const NewtonWorld* const newtonWorld, const dFloat* pivotPoint, 
	const dFloat* pinDir0, const dFloat* pinDir1, const NewtonBody* const childBody, const NewtonBody* const parentBody)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)childBody;
	dgBody* const body1 = (dgBody *)parentBody;
	dgVector pin0 (pinDir0[0], pinDir0[1], pinDir0[2], dgFloat32 (0.0f));
	dgVector pin1 (pinDir1[0], pinDir1[1], pinDir1[2], dgFloat32 (0.0f));
	dgVector pivot (pivotPoint[0], pivotPoint[1], pivotPoint[2], dgFloat32 (0.0f));
	return (NewtonJoint*) world->CreateUniversalConstraint (pivot, pin0, pin1, body0, body1);
}


/*!
  Set an update call back to be called when either of the two body linked by the joint is active.

  @param *universal pointer to the joint.
  @param callback pointer to the joint function call back.

  @return nothing.

  if the application wants to have some feedback from the joint simulation, the application can register a function
  update callback to be called every time any of the bodies linked by this joint is active. This is useful to provide special
  effects like particles, sound or even to simulate breakable moving parts.

  the function *NewtonUniversalCallback callback* should return a bit field code.
  if the application does not want to set the joint acceleration the return code is zero
  if the application only wants to change the joint linear acceleration the return code is 1
  if the application only wants to change the joint angular acceleration the return code is 2
  if the application only wants to change the joint angular and linear acceleration the return code is 3

  See also: ::NewtonJointGetUserData, ::NewtonJointSetUserData
*/
void NewtonUniversalSetUserCallback(const NewtonJoint* const universal, NewtonUniversalCallback callback)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	contraint->SetJointParameterCallback ((dgUniversalJointAcceleration)callback);
}


/*!
  Get the relative joint angle between the two bodies.

  @param *universal pointer to the joint.

  @return the joint angle relative to the universal pin0.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalGetJointAngle0(const NewtonJoint* const universal)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->GetJointAngle0 ();
}

/*!
  Get the relative joint angle between the two bodies.

  @param *universal pointer to the joint.

  @return the joint angle relative to the universal pin1.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play a bell sound when the joint angle passes some max value.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalGetJointAngle1(const NewtonJoint* const universal)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->GetJointAngle1 ();
}


/*!
  Get the relative joint angular velocity between the two bodies.

  @param *universal pointer to the joint.

  @return the joint angular velocity relative to the pin0 axis.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalGetJointOmega0(const NewtonJoint* const universal)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->GetJointOmega0 ();
}


/*!
  Get the relative joint angular velocity between the two bodies.

  @param *universal pointer to the joint.

  @return the joint angular velocity relative to the pin1 axis.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can play the creaky noise of a hanging lamp.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalGetJointOmega1(const NewtonJoint* const universal)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->GetJointOmega1 ();
}



/*!
  Calculate the angular acceleration needed to stop the universal at the desired angle.

  @param *universal pointer to the joint.
  @param *desc is the pointer to the Universal or slide structure.
  @param angle is the desired universal stop angle rotation around pin0

  @return the relative angular acceleration needed to stop the universal.

  this function can only be called from a *NewtonUniversalCallback* and it can be used by the application to implement universal limits.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalCalculateStopAlpha0(const NewtonJoint* const universal, const NewtonHingeSliderUpdateDesc* const desc, dFloat angle)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->CalculateStopAlpha0 (angle, (dgJointCallbackParam*) desc);
}

/*!
  Calculate the angular acceleration needed to stop the universal at the desired angle.

  @param *universal pointer to the joint.
  @param *desc is the pointer to and the Universal or slide structure.
  @param angle is the desired universal stop angle rotation around pin1

  @return the relative angular acceleration needed to stop the universal.

  this function can only be called from a *NewtonUniversalCallback* and it can be used by the application to implement universal limits.

  See also: ::NewtonUniversalSetUserCallback
*/
dFloat NewtonUniversalCalculateStopAlpha1(const NewtonJoint* const universal, const NewtonHingeSliderUpdateDesc* const desc, dFloat angle)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	return contraint->CalculateStopAlpha1 (angle, (dgJointCallbackParam*) desc);
}



/*!
  Get the total force asserted over the joint pivot point, to maintain the constraint.

  @param *universal pointer to the joint.
  @param *force pointer to an array of a least three floats to hold the force value of the joint.

  @return nothing.

  this function can be used during a function update call back to provide the application with some special effect.
  for example the application can destroy the joint if the force exceeds some predefined value.

  See also: ::NewtonUniversalSetUserCallback
*/
void NewtonUniversalGetJointForce(const NewtonJoint* const universal, dFloat* const force)
{
	dgUniversalConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUniversalConstraint*) universal;
	dgVector forceVector (contraint->GetJointForce ());
	force[0] = forceVector.m_x;
	force[1] = forceVector.m_y;
	force[2] = forceVector.m_z;
}


/*! @} */ // end of JointUniversal

/*! @defgroup JointUpVector JointUpVector
UpVector joint Interface
@{
*/

/*!
  Create a UpVector joint.

  @param *newtonWorld Pointer to the Newton world.
  @param *pinDir is the aligning vector.
  @param *body is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.

  @return Pointer to the up vector joint.

  This function creates an up vector joint. An up vector joint is a constraint that allows a body to translate freely in 3d space,
  but it only allows the body to rotate around the pin direction vector. This could be use by the application to control a character
  with physics and collision.

  Since the UpVector joint is a unary constraint, there is not need to have user callback or user data assigned to it.
  The application can simple hold to the joint handle and update the pin on the force callback function of the rigid body owning the joint.
*/
NewtonJoint* NewtonConstraintCreateUpVector (const NewtonWorld* const newtonWorld, const dFloat* pinDir, const NewtonBody* const body)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)body;
	dgVector pin (pinDir[0], pinDir[1], pinDir[2], dgFloat32 (0.0f));
	return (NewtonJoint*) world->CreateUpVectorConstraint(pin, body0);
}


/*!
  Get the up vector pin of this joint in global space.

  @param *upVector pointer to the joint.
  @param *pin pointer to an array of a least three floats to hold the up vector direction in global space.

  @return nothing.

  the application ca call this function to read the up vector, this is useful to animate the up vector.
  if the application is going to animated the up vector, it must do so by applying only small rotation,
  too large rotation can cause vibration of the joint.

  See also: ::NewtonUpVectorSetPin
*/
void NewtonUpVectorGetPin(const NewtonJoint* const upVector, dFloat *pin)
{
	dgUpVectorConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUpVectorConstraint*) upVector;

	dgVector pinVector (contraint ->GetPinDir ());
	pin[0] = pinVector.m_x;
	pin[1] = pinVector.m_y;
	pin[2] = pinVector.m_z;
}


/*!
  Set the up vector pin of this joint in global space.

  @param *upVector pointer to the joint.
  @param *pin pointer to an array of a least three floats containing the up vector direction in global space.

  @return nothing.

  the application ca call this function to change the joint up vector, this is useful to animate the up vector.
  if the application is going to animated the up vector, it must do so by applying only small rotation,
  too large rotation can cause vibration of the joint.

  See also: ::NewtonUpVectorGetPin
*/
void NewtonUpVectorSetPin(const NewtonJoint* const upVector, const dFloat *pin)
{
	dgUpVectorConstraint* contraint;

	TRACE_FUNCTION(__FUNCTION__);
	contraint = (dgUpVectorConstraint*) upVector;

	dgVector pinVector (pin[0], pin[1], pin[2], dgFloat32 (0.0f));
	contraint->SetPinDir (pinVector);
}

/*! @} */ // end of JointUpVector

/*! @defgroup JointUser JointUser
User defined joint interface
@{
*/

/*!
  Create a user define bilateral joint.

  @param *newtonWorld Pointer to the Newton world.
  @param maxDOF is the maximum number of degree of freedom controlled by this joint.
  @param submitConstraints pointer to the joint constraint definition function call back.
  @param getInfo pointer to callback for collecting joint information.
  @param *childBody is the pointer to the attached rigid body, this body can not be NULL or it can not have an infinity (zero) mass.
  @param *parentBody is the pointer to the parent rigid body, this body can be NULL or any kind of rigid body.

  Bilateral joint are constraints that can have up to 6 degree of freedoms, 3 linear and 3 angular.
  By restricting the motion along any number of these degree of freedom a very large number of useful joint between
  two rigid bodies can be accomplished. Some of the degree of freedoms restriction makes no sense, and also some
  combinations are so rare that only make sense to a very specific application, the Newton engine implements the more
  commons combinations like, hinges, ball and socket, etc. However if and application is in the situation that any of
  the provided joints can achieve the desired effect, then the application can design it own joint.

  User defined joint is a very advance feature that should be look at, only for very especial situations.
  The designer must be a person with a very good understanding of constrained dynamics, and it may be the case
  that many trial have to be made before a good result can be accomplished.

  function *submitConstraints* is called before the solver state to get the jacobian derivatives and the righ hand acceleration
  for the definition of the constraint.

  maxDOF is and upper bound as to how many degrees of freedoms the joint can control, usually this value
  can be 6 for bilateral joints, but it can be higher for special joints like vehicles where by the used of friction clamping
  the number of rows can be higher.
  In general the application should determine maxDof correctly, passing an unnecessary excessive value will lead to performance decreased.

  See also: ::NewtonUserJointSetFeedbackCollectorCallback
*/
NewtonJoint* NewtonConstraintCreateUserJoint(const NewtonWorld* const newtonWorld, int maxDOF, 
											 NewtonUserBilateralCallback submitConstraints, 
											 const NewtonBody* const childBody, const NewtonBody* const parentBody)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgBody* const body0 = (dgBody *)childBody;
	dgBody* const body1 = (dgBody *)parentBody;
	dgAssert(body0);
	dgAssert(body0 != body1);
	return (NewtonJoint*) new (world->dgWorld::GetAllocator()) NewtonUserJoint (world, maxDOF, submitConstraints, body0, body1);
}

/*!
	Set the solver algorithm use to calculation the constraint forces.

	@param *joint pointer to the joint.
	@param  *model - solve model to choose.

	model = 0  zero is the default value and tells the solver to use the best possible algorithm
	model = 1 to signal the engine that is two joints form a kinematic loop 
	model = 2 to signal the engine this joint can be solved with a less accurate algorithm.
	In case multiple joints form a kinematic loop, joints with a lower model are preffered towards an exact solution.

	See also: NewtonUserJointGetSolverModel
*/
void NewtonUserJointSetSolverModel(const NewtonJoint* const joint, int model)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*)joint;
	contraint->SetSolverModel(model);
}

/*!
Get the solver algorthm use to calculation the constraint forces.
@param *joint pointer to the joint.

See also: NewtonUserJointGetSolverModel
*/
int NewtonUserJointGetSolverModel(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*)joint;
	return contraint->GetSolverModel();
}



/*!
  Add a linear restricted degree of freedom.

  @param *joint pointer to the joint.
  @param  *pivot0 - pointer of a vector in global space fixed on body zero.
  @param  *pivot1 - pointer of a vector in global space fixed on body one.
  @param *dir pointer of a unit vector in global space along which the relative position, velocity and acceleration between the bodies will be driven to zero.

  A linear constraint row calculates the Jacobian derivatives and relative acceleration required to enforce the constraint condition at
  the attachment point and the pin direction considered fixed to both bodies.

  The acceleration is calculated such that the relative linear motion between the two points is zero, the application can
  afterward override this value to create motors.

  after this function is call and internal DOF index will point to the current row entry in the constraint matrix.

  This function call only be called from inside a *NewtonUserBilateralCallback* callback.

  See also: ::NewtonUserJointAddAngularRow,
*/
void NewtonUserJointAddLinearRow(const NewtonJoint* const joint, const dFloat* const pivot0, const dFloat* const pivot1, const dFloat* const dir)
{
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;

	TRACE_FUNCTION(__FUNCTION__);
	dgVector direction (dir[0], dir[1], dir[2], dgFloat32 (0.0f)); 
	direction = direction.Normalize();
	dgAssert (dgAbs (direction.DotProduct3(direction) - dgFloat32 (1.0f)) < dgFloat32 (1.0e-4f));
	dgVector pivotPoint0 (pivot0[0], pivot0[1], pivot0[2], dgFloat32 (0.0f)); 
	dgVector pivotPoint1 (pivot1[0], pivot1[1], pivot1[2], dgFloat32 (0.0f)); 
	
	userJoint->AddLinearRowJacobian (pivotPoint0, pivotPoint1, direction);
}


/*!
  Add an angular restricted degree of freedom.

  @param *joint pointer to the joint.
  @param relativeAngleError relative angle error between both bodies around pin axis.
  @param *pin pointer of a unit vector in global space along which the relative position, velocity and acceleration between the bodies will be driven to zero.

  An angular constraint row calculates the Jacobian derivatives and relative acceleration required to enforce the constraint condition at
  pin direction considered fixed to both bodies.

  The acceleration is calculated such that the relative angular motion between the two points is zero, The application can
  afterward override this value to create motors.

  After this function is called and internal DOF index will point to the current row entry in the constraint matrix.

  This function call only be called from inside a *NewtonUserBilateralCallback* callback.

  This function is of not practical to enforce hard constraints, but it is very useful for making angular motors.

  See also: ::NewtonUserJointAddLinearRow
*/
void NewtonUserJointAddAngularRow(const NewtonJoint* const joint, dFloat relativeAngleError, const dFloat* const pin)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	dgVector direction (pin[0], pin[1], pin[2], dgFloat32 (0.0f));
	direction = direction.Normalize();
	dgAssert (dgAbs (direction.DotProduct3(direction) - dgFloat32 (1.0f)) < dgFloat32 (1.0e-3f));

	userJoint->AddAngularRowJacobian (direction, relativeAngleError);
}

/*!
  set the general linear and angular Jacobian for the desired degree of freedom

  @param *joint pointer to the joint.
  @param  *jacobian0 - pointer of a set of six values defining the linear and angular Jacobian for body0.
  @param  *jacobian1 - pointer of a set of six values defining the linear and angular Jacobian for body1.

  In general this function must be used for very special effects and in combination with other joints.
  it is expected that the user have a knowledge of Constrained dynamics to make a good used of this function.
  Must typical application of this function are the creation of synchronization or control joints like gears, pulleys,
  worm gear and some other mechanical control.

  this function set the relative acceleration for this degree of freedom to zero. It is the
  application responsibility to set the relative acceleration after a call to this function

  See also: ::NewtonUserJointAddLinearRow, ::NewtonUserJointAddAngularRow
*/
void NewtonUserJointAddGeneralRow(const NewtonJoint* const joint, const dFloat* const jacobian0, const dFloat* const jacobian1)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->AddGeneralRowJacobian (jacobian0, jacobian1);
}

int NewtonUserJoinRowsCount(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	return userJoint->GetJacobianCount();
}

void NewtonUserJointGetGeneralRow(const NewtonJoint* const joint, int index, dFloat* const jacobian0, dFloat* const jacobian1)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	userJoint->GetJacobianAt(index, jacobian0, jacobian1);
}

/*!
  Set the maximum friction value the solver is allow to apply to the joint row.

  @param *joint pointer to the joint.
  @param friction maximum friction value for this row. It must be a positive value between 0.0 and INFINITY.

  This function will override the default friction values set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngularRow.
  friction value is context sensitive, if for linear constraint friction is a Max friction force, for angular constraint friction is a
  max friction is a Max friction torque.

  See also: ::NewtonUserJointSetRowMinimumFriction, ::NewtonUserJointAddLinearRow, ::NewtonUserJointAddAngularRow
*/
void NewtonUserJointSetRowMaximumFriction(const NewtonJoint* const joint, dFloat friction)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->SetHighFriction (friction);
}

/*!
  Set the minimum friction value the solver is allow to apply to the joint row.

  @param *joint pointer to the joint.
  @param friction friction value for this row. It must be a negative value between 0.0 and -INFINITY.

  This function will override the default friction values set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngularRow.
  friction value is context sensitive, if for linear constraint friction is a Min friction force, for angular constraint friction is a
  friction is a Min friction torque.

  See also: ::NewtonUserJointSetRowMaximumFriction, ::NewtonUserJointAddLinearRow, ::NewtonUserJointAddAngularRow
*/
void NewtonUserJointSetRowMinimumFriction(const NewtonJoint* const joint, dFloat friction)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->SetLowerFriction (friction);
}

/*!
  Set the value for the desired acceleration for the current constraint row.

  @param *joint pointer to the joint.
  @param acceleration desired acceleration value for this row.

  This function will override the default acceleration values set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngularRow.
  friction value is context sensitive, if for linear constraint acceleration is a linear acceleration, for angular constraint acceleration is an
  angular acceleration.

  See also: ::NewtonUserJointAddLinearRow, ::NewtonUserJointAddAngularRow
*/
void NewtonUserJointSetRowAcceleration(const NewtonJoint* const joint, dFloat acceleration)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->SetAcceleration (acceleration);
}

dFloat NewtonUserJointGetRowAcceleration (const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	return userJoint->GetAcceleration();
}

/*
dFloat NewtonUserJointGetRowInverseDynamicsAcceleration (const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	return userJoint->GetInverseDynamicsAcceleration();
}
*/

void NewtonUserJointSetRowAsInverseDynamics (const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	userJoint->SetAsInverseDynamicsRow();
}

dFloat NewtonUserJointCalculateRowZeroAccelaration (const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	return userJoint->CalculateZeroMotorAcceleration();
}

/*!
  Calculates the row acceleration to satisfy the specified the spring damper system.

  @param *joint pointer to the joint.
  @param rowStiffness fraction of the row reaction forces used a sspring damper penalty.
  @param spring desired spring stiffness, it must be a positive value.
  @param damper desired damper coefficient, it must be a positive value.

  This function will override the default acceleration values set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngularRow.
  friction value is context sensitive, if for linear constraint acceleration is a linear acceleration, for angular constraint acceleration is an
  angular acceleration.

  the acceleration calculated by this function represent the mass, spring system of the form
  a = -ks * x - kd * v.

  for this function to take place the joint stiffness must be set to a values lower than 1.0

  See also: ::NewtonUserJointSetRowAcceleration, ::NewtonUserJointSetRowStiffness
*/
void NewtonUserJointSetRowSpringDamperAcceleration(const NewtonJoint* const joint, dFloat rowStiffness, dFloat spring, dFloat damper)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->SetSpringDamperAcceleration (rowStiffness, spring, damper);
}


/*!
  Set the maximum percentage of the constraint force that will be applied to the constraint row.

  @param *joint pointer to the joint.
  @param stiffness row stiffness, it must be a values between 0.0 and 1.0, the default is 0.9.

  This function will override the default stiffness value set after a call to NewtonUserJointAddLinearRow or NewtonUserJointAddAngularRow.
  the row stiffness is the percentage of the constraint force that will be applied to the rigid bodies. Ideally the value should be
  1.0 (100% stiff) but dues to numerical integration error this could be the joint a little unstable, and lower values are preferred.

  See also: ::NewtonUserJointAddLinearRow, ::NewtonUserJointAddAngularRow, ::NewtonUserJointSetRowSpringDamperAcceleration
*/
void NewtonUserJointSetRowStiffness(const NewtonJoint* const joint, dFloat stiffness)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	userJoint->SetRowStiffness (stiffness);
}

/*!
  Return the magnitude previews force or torque value calculated by the solver for this constraint row.

  @param *joint pointer to the joint.
  @param row index to the constraint row.

  This function can be call for any of the previews row for this particular joint, The application must keep track of the meaning of the row.

  This function can be used to produce special effects like breakable or malleable joints, fro example a hinge can turn into ball and socket
  after the force in some of the row exceed  certain high value.
*/
dFloat NewtonUserJointGetRowForce(const NewtonJoint* const joint, int row)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	return userJoint->GetRowForce (row);
}


/*!
  Set a constrain callback to collect the force calculated by the solver to enforce this constraint

  @param *joint pointer to the joint.
  @param getFeedback pointer to the joint constraint definition function call back.

  See also: ::NewtonUserJointGetRowForce
*/
void NewtonUserJointSetFeedbackCollectorCallback(const NewtonJoint* const joint, NewtonUserBilateralCallback getFeedback)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*) joint;
	return userJoint->SetUpdateFeedbackFunction (getFeedback);
}

int NewtonUserJointSubmitImmediateModeConstraint(const NewtonJoint* const joint, NewtonImmediateModeConstraint* const descriptor, dFloat timestep)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonUserJoint* const userJoint = (NewtonUserJoint*)joint;
	return userJoint->SubmitImmediateModeConstraint(descriptor, timestep);
}



/*! @} */ // end of JointUser

/*! @defgroup JointCommon JointCommon
Joint common function s
@{
*/

/*!
  Store a user defined data value with the joint.

  @param *joint pointer to the joint.
  @param *userData pointer to the user defined user data value.

  @return Nothing.

  The application can store a user defined value with the Joint. This value can be the pointer to a structure containing some application data for special effect.
  if the application allocate some resource to store the user data, the application can register a joint destructor to get rid of the allocated resource when the Joint is destroyed

  See also: ::NewtonJointSetDestructor
*/
void NewtonJointSetUserData(const NewtonJoint* const joint, void* const userData)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	contraint->SetUserData (userData);
}

/*!
  Retrieve a user defined data value stored with the joint.

  @param *joint pointer to the joint.

  @return The user defined data.

  The application can store a user defined value with a joint. This value can be the pointer
  to a structure to store some game play data for special effect.

  See also: ::NewtonJointSetUserData
*/
void* NewtonJointGetUserData(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	return contraint->GetUserData();
}

/*!
  Get creation parameters for this joint.

  @param joint is the pointer to a convex collision primitive.
  @param *jointInfo pointer to a collision information record.

  This function can be used by the application for writing file format and for serialization.

  See also: ::// See also:
*/
void NewtonJointGetInfo(const NewtonJoint* const joint, NewtonJointRecord* const jointInfo)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	contraint->GetInfo ((dgConstraintInfo*) jointInfo);
}

/*!
  Get the first body connected by this joint.

  @param *joint is the pointer to a convex collision primitive.


  See also: ::// See also:
*/
NewtonBody* NewtonJointGetBody0(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	dgBody* const body = contraint->GetBody0();
	dgWorld* const world = body->GetWorld();
	return (world->GetSentinelBody() != body) ? (NewtonBody*) body : NULL;
}


/*!
  Get the second body connected by this joint.

  @param *joint is the pointer to a convex collision primitive.

  See also: ::// See also:
*/
NewtonBody* NewtonJointGetBody1(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	dgBody* const body = contraint->GetBody1();
	dgWorld* const world = body->GetWorld();
	return (world->GetSentinelBody() != body) ? (NewtonBody*) body : NULL;
}


/*!
  Enable or disable collision between the two bodies linked by this joint. The default state is collision disable when the joint is created.

  @param *joint pointer to the joint.
  @param state collision state, zero mean disable collision, non zero enable collision between linked bodies.

  @return nothing.

  usually when two bodies are linked by a joint, the application wants collision between this two bodies to be disabled.
  This is the default behavior of joints when they are created, however when this behavior is not desired the application can change
  it by setting collision on. If the application decides to enable collision between jointed bodies, the application should make sure the
  collision geometry do not collide in the work space of the joint.

  if the joint is destroyed the collision state of the two bodies linked by this joint is determined by the material pair assigned to each body.

  See also: ::NewtonJointGetCollisionState, ::NewtonBodySetJointRecursiveCollision
*/
void NewtonJointSetCollisionState(const NewtonJoint* const joint, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	return contraint->SetCollidable (state ? true : false);
}

/*!
  Get the collision state of the two bodies linked by the joint.

  @param *joint pointer to the joint.

  @return the collision state.

  usually when two bodies are linked by a joint, the application wants collision between this two bodies to be disabled.
  This is the default behavior of joints when they are created, however when this behavior is not desired the application can change
  it by setting collision on. If the application decides to enable collision between jointed bodies, the application should make sure the
  collision geometry do not collide in the work space of the joint.

  See also: ::NewtonJointSetCollisionState
*/
int NewtonJointGetCollisionState(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	return contraint->IsCollidable () ? 1 : 0;
}


/*!
  Set the strength coefficient to be applied to the joint reaction forces.

  @param *joint pointer to the joint.
  @param stiffness stiffness coefficient, a value between 0, and 1.0, the default value for most joint is 0.9

  @return nothing.

  Constraint keep bodies together by calculating the exact force necessary to cancel the relative acceleration between one or
  more common points fixed in the two bodies. The problem is that when the bodies drift apart due to numerical integration inaccuracies,
  the reaction force work to pull eliminated the error but at the expense of adding extra energy to the system, does violating the rule
  that constraint forces must be work less. This is a inevitable situation and the only think we can do is to minimize the effect of the
  extra energy by dampening the force by some amount. In essence the stiffness coefficient tell Newton calculate the precise reaction force
  by only apply a fraction of it to the joint point. And value of 1.0 will apply the exact force, and a value of zero will apply only
  10 percent.

  The stiffness is set to a all around value that work well for most situation, however the application can play with these
  parameter to make finals adjustment. A high value will make the joint stronger but more prompt to vibration of instability; a low
  value will make the joint more stable but weaker.

  See also: ::NewtonJointGetStiffness
*/
void NewtonJointSetStiffness(const NewtonJoint* const joint, dFloat stiffness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	contraint->SetStiffness(stiffness);
}

/*!
  Get the strength coefficient bing applied to the joint reaction forces.

  @param *joint pointer to the joint.

  @return stiffness coefficient.

  Constraint keep bodies together by calculating the exact force necessary to cancel the relative acceleration between one or
  more common points fixed in the two bodies. The problem is that when the bodies drift apart due to numerical integration inaccuracies,
  the reaction force work to pull eliminated the error but at the expense of adding extra energy to the system, does violating the rule
  that constraint forces must be work less. This is a inevitable situation and the only think we can do is to minimize the effect of the
  extra energy by dampening the force by some amount. In essence the stiffness coefficient tell Newton calculate the precise reaction force
  by only apply a fraction of it to the joint point. And value of 1.0 will apply the exact force, and a value of zero will apply only
  10 percent.

  The stiffness is set to a all around value that work well for most situation, however the application can play with these
  parameter to make finals adjustment. A high value will make the joint stronger but more prompt to vibration of instability; a low
  value will make the joint more stable but weaker.

  See also: ::NewtonJointSetStiffness
*/
dFloat NewtonJointGetStiffness(const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	return contraint->GetStiffness();
}

/*!
  Register a destructor callback to be called when the joint is about to be destroyed.

  @param *joint pointer to the joint.
  @param destructor pointer to the joint destructor callback.

  @return nothing.

  If application stores any resource with the joint, or the application wants to be notified when the
  joint is about to be destroyed. The application can register a destructor call back with the joint.

  See also: ::NewtonJointSetUserData
*/
void NewtonJointSetDestructor(const NewtonJoint* const joint, NewtonConstraintDestructor destructor)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgConstraint* const contraint = (dgConstraint*) joint;
	contraint->SetDestructorCallback ((OnConstraintDestroy) destructor);
}


/*!
  destroy a joint.

  @param *newtonWorld is the pointer to the body.
  @param *joint pointer to joint to be destroyed

  @return nothing

  The application can call this function when it wants to destroy a joint. This function can be used by the application to simulate
  breakable joints

  See also: ::NewtonConstraintCreateSlider
*/
void NewtonDestroyJoint(const NewtonWorld* const newtonWorld, const NewtonJoint* const joint)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	world->DestroyJoint ((dgConstraint*) joint);
}

/*! @} */ // end of JointCommon

/*! @defgroup SpecialEffectMesh SpecialEffectMesh
Special effect mesh interface
@{
*/

NewtonMesh* NewtonMeshCreate(const NewtonWorld* const newtonWorld)
{
	TRACE_FUNCTION(__FUNCTION__);

	Newton* const world = (Newton *) newtonWorld;
	dgMeshEffect* const mesh = new (world->dgWorld::GetAllocator()) dgMeshEffect (world->dgWorld::GetAllocator());
	return (NewtonMesh*) mesh;
}

NewtonMesh* NewtonMeshCreateFromMesh(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const srcMesh = (dgMeshEffect*) mesh;

	dgMeshEffect* const clone = new (srcMesh->GetAllocator()) dgMeshEffect (*srcMesh);
	return (NewtonMesh*) clone;
}

NewtonMesh* NewtonMeshCreateFromCollision(const NewtonCollision* const collision)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgCollisionInstance* const shape = (dgCollisionInstance*) collision;
	dgMeshEffect* const mesh = new (shape->GetAllocator()) dgMeshEffect (shape);
	return (NewtonMesh*) mesh;
}

NewtonMesh* NewtonMeshCreateConvexHull (const NewtonWorld* const newtonWorld, int count, const dFloat* const vertexCloud, int strideInBytes, dFloat tolerance)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	dgStack<dgBigVector> pool (count);

	dgInt32 stride = strideInBytes / sizeof (dgFloat32);
	for (dgInt32 i = 0; i < count; i ++) {
		pool[i].m_x = vertexCloud[i * stride + 0];
		pool[i].m_y = vertexCloud[i * stride + 1];
		pool[i].m_z = vertexCloud[i * stride + 2];
		pool[i].m_w = dgFloat64 (0.0);
	}
	dgMeshEffect* const mesh = new (world->dgWorld::GetAllocator()) dgMeshEffect (world->dgWorld::GetAllocator(), &pool[0].m_x, count, sizeof (dgBigVector), tolerance);
	return (NewtonMesh*) mesh;
}

NewtonMesh* NewtonMeshCreateTetrahedraIsoSurface(const NewtonMesh* const closeManifoldMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) closeManifoldMesh;
	return (NewtonMesh*)meshEffect->CreateTetrahedraIsoSurface();
}

void NewtonCreateTetrahedraLinearBlendSkinWeightsChannel(const NewtonMesh* const tetrahedraMesh, NewtonMesh* const skinMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)skinMesh;
	meshEffect->CreateTetrahedraLinearBlendSkinWeightsChannel((const dgMeshEffect*)tetrahedraMesh);
}

NewtonMesh* NewtonMeshCreateVoronoiConvexDecomposition (const NewtonWorld* const newtonWorld, int pointCount, const dFloat* const vertexCloud, int strideInBytes, int materialID, const dFloat* const textureMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return (NewtonMesh*) dgMeshEffect::CreateVoronoiConvexDecomposition (world->dgWorld::GetAllocator(), pointCount, strideInBytes, vertexCloud, materialID, dgMatrix (textureMatrix));
}

NewtonMesh* NewtonMeshCreateFromSerialization (const NewtonWorld* const newtonWorld, NewtonDeserializeCallback deserializeFunction, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	return (NewtonMesh*) dgMeshEffect::CreateFromSerialization (world->dgWorld::GetAllocator(), (dgDeserialize) deserializeFunction, serializeHandle);
}

void NewtonMeshDestroy(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	delete meshEffect;
}

void NewtonMeshSerialize (const NewtonMesh* const mesh, NewtonSerializeCallback serializeFunction, void* const serializeHandle)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->Serialize ((dgSerialize) serializeFunction, serializeHandle);
}

void NewtonMeshSaveOFF(const NewtonMesh* const mesh, const char* const filename)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->SaveOFF(filename);
}


NewtonMesh* NewtonMeshLoadOFF(const NewtonWorld* const newtonWorld, const char* const filename)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *) newtonWorld;
	dgMemoryAllocator* const allocator = world->dgWorld::GetAllocator();
	dgMeshEffect* const mesh = new (allocator) dgMeshEffect (allocator);
	mesh->LoadOffMesh(filename);
	return (NewtonMesh*) mesh;
}

NewtonMesh* NewtonMeshLoadTetrahedraMesh(const NewtonWorld* const newtonWorld, const char* const filename)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	dgMemoryAllocator* const allocator = world->dgWorld::GetAllocator();
	dgMeshEffect* const mesh = new (allocator) dgMeshEffect(allocator);
	mesh->LoadTetraMesh (filename);
	return (NewtonMesh*)mesh;
}

void NewtonMeshApplyTransform (const NewtonMesh* const mesh, const dFloat* const matrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	meshEffect->ApplyTransform(dgMatrix (matrix));
}

void NewtonMeshCalculateOOBB(const NewtonMesh* const mesh, dFloat* const matrix, dFloat* const x, dFloat* const y, dFloat* const z)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	dgBigVector size;
	dgMatrix alignMatrix (meshEffect->CalculateOOBB (size));

//	*((dgMatrix *)matrix) = alignMatrix; 
	memcpy (matrix, &alignMatrix[0][0], sizeof (dgMatrix));
	*x = dgFloat32 (size.m_x);
	*y = dgFloat32 (size.m_y);
	*z = dgFloat32 (size.m_z);
}

void NewtonMeshCalculateVertexNormals(const NewtonMesh* const mesh, dFloat angleInRadians)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->CalculateNormals (angleInRadians);
}

void NewtonMeshApplyAngleBasedMapping(const NewtonMesh* const mesh, int material, NewtonReportProgress reportPrograssCallback, void* const reportPrgressUserData, dFloat* const aligmentMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMatrix matrix(aligmentMatrix);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->AngleBaseFlatteningMapping(material, (dgReportProgress) reportPrograssCallback, reportPrgressUserData);
}

void NewtonMeshApplySphericalMapping(const NewtonMesh* const mesh, int material, const dFloat* const aligmentMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMatrix matrix(aligmentMatrix);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->SphericalMapping (material, matrix);
}

void NewtonMeshApplyBoxMapping(const NewtonMesh* const mesh, int front, int side, int top, const dFloat* const aligmentMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMatrix matrix(aligmentMatrix);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->BoxMapping (front, side, top);
}

void NewtonMeshApplyCylindricalMapping(const NewtonMesh* const mesh, int cylinderMaterial, int capMaterial, const dFloat* const aligmentMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMatrix matrix(aligmentMatrix);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->CylindricalMapping (cylinderMaterial, capMaterial, matrix);
}

void NewtonMeshTriangulate (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*) mesh)->Triangulate ();
}

void NewtonMeshPolygonize (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*) mesh)->ConvertToPolygons ();
}


int NewtonMeshIsOpenMesh (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);

	return ((dgMeshEffect*) mesh)->HasOpenEdges () ? 1 : 0;
}

void NewtonMeshFixTJoints (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);

	return ((dgMeshEffect*) mesh)->RepairTJoints();
}


void NewtonMeshClip (const NewtonMesh* const mesh, const NewtonMesh* const clipper, const dFloat* const clipperMatrix, NewtonMesh** const topMesh, NewtonMesh** const bottomMesh)
{
	TRACE_FUNCTION(__FUNCTION__);

	*topMesh = NULL;
	*bottomMesh = NULL;
	((dgMeshEffect*) mesh)->ClipMesh (dgMatrix (clipperMatrix), (dgMeshEffect*)clipper, (dgMeshEffect**) topMesh, (dgMeshEffect**) bottomMesh);
}


NewtonMesh* NewtonMeshSimplify (const NewtonMesh* const mesh, int maxVertexCount, NewtonReportProgress progressReportCallback, void* const reportPrgressUserData)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->CreateSimplification (maxVertexCount, (dgReportProgress) progressReportCallback, reportPrgressUserData);
}

NewtonMesh* NewtonMeshApproximateConvexDecomposition (const NewtonMesh* const mesh, dFloat maxConcavity, dFloat backFaceDistanceFactor, int maxCount, int maxVertexPerHull, NewtonReportProgress progressReportCallback, void* const reportProgressUserData)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->CreateConvexApproximation (maxConcavity, backFaceDistanceFactor, maxCount, maxVertexPerHull, (dgReportProgress) progressReportCallback, reportProgressUserData);
}



NewtonMesh* NewtonMeshUnion (const NewtonMesh* const mesh, const NewtonMesh* const clipper, const dFloat* const clipperMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->Union (dgMatrix (clipperMatrix), (dgMeshEffect*)clipper);
}


NewtonMesh* NewtonMeshDifference (const NewtonMesh* const mesh, const NewtonMesh* const clipper, const dFloat* const clipperMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->Difference (dgMatrix (clipperMatrix), (dgMeshEffect*)clipper);
}

NewtonMesh* NewtonMeshIntersection (const NewtonMesh* const mesh, const NewtonMesh* const clipper, const dFloat* const clipperMatrix)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->Intersection (dgMatrix (clipperMatrix), (dgMeshEffect*)clipper);
}

NewtonMesh* NewtonMeshConvexMeshIntersection (const NewtonMesh* const mesh, const NewtonMesh* const convexMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return (NewtonMesh*) ((dgMeshEffect*) mesh)->ConvexMeshIntersection ((dgMeshEffect*)convexMesh);
}

void NewtonRemoveUnusedVertices(const NewtonMesh* const mesh, int* const vertexRemapTable)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*) mesh)->RemoveUnusedVertices (vertexRemapTable);
}


void NewtonMeshBeginBuild(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->BeginBuild();
}

void NewtonMeshBeginFace (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->BeginBuildFace();
}

void NewtonMeshEndFace(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->EndBuildFace();
}

void NewtonMeshAddPoint(const NewtonMesh* const mesh, dFloat64 x, dFloat64 y, dFloat64 z)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddPoint (x, y, z);
}

void NewtonMeshAddMaterial(const NewtonMesh* const mesh, int materialIndex)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddMaterial(materialIndex);
}

void NewtonMeshAddLayer(const NewtonMesh* const mesh, int layer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddLayer(layer);
}

void NewtonMeshAddNormal(const NewtonMesh* const mesh, dFloat x, dFloat y, dFloat z)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddNormal(x, y, z);
}

void NewtonMeshAddBinormal(const NewtonMesh* const mesh, dFloat x, dFloat y, dFloat z)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddBinormal(x, y, z);
}

void NewtonMeshAddUV0(const NewtonMesh* const mesh, dFloat u, dFloat v)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddUV0(u, v);
}

void NewtonMeshAddUV1(const NewtonMesh* const mesh, dFloat u, dFloat v)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddUV1(u, v);
}

void NewtonMeshAddVertexColor(const NewtonMesh* const mesh, dFloat32 r, dFloat32 g, dFloat32 b, dFloat32 a)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->AddVertexColor(r, g, b, a);
}

void NewtonMeshAddVertexWeight(const NewtonMesh* const mesh, int matrixIndex[4], dFloat32 weights[4])
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;

	dgMeshEffect::dgPointFormat::dgWeightSet weightSet;
	for (int i = 0; i < 4; i ++) {
		weightSet.m_weightPair[i].m_weight = weights[i];
		weightSet.m_weightPair[i].m_controlIndex = matrixIndex[i];
	}

	meshEffect->AddWeights (weightSet);
}

void NewtonMeshEndBuild(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	meshEffect->EndBuild(dgFloat64 (1.0e-8f));
}

void NewtonMeshClearVertexFormat (NewtonMeshVertexFormat* const format)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect::dgMeshVertexFormat* const vertexFormat = (dgMeshEffect::dgMeshVertexFormat*) format;
	vertexFormat->Clear ();
}

void NewtonMeshBuildFromVertexListIndexList (const NewtonMesh* const mesh, const NewtonMeshVertexFormat* const format)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->BuildFromIndexList((dgMeshEffect::dgMeshVertexFormat*) format);
}

void NewtonMeshOptimizePoints(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->OptimizePoints();
}

void NewtonMeshOptimizeVertex(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->OptimizeAttibutes();
}

void NewtonMeshOptimize(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	NewtonMeshOptimizePoints(mesh);
	NewtonMeshOptimizeVertex(mesh);
}


int NewtonMeshGetVertexCount(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

//	return meshEffect->GetPropertiesCount();
	return meshEffect->GetVertexCount();
}

int NewtonMeshGetVertexStrideInByte(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->GetVertexStrideInByte();
}

const dFloat64* NewtonMeshGetVertexArray (const NewtonMesh* const mesh) 
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->GetVertexPool (); 
}


int NewtonMeshGetPointCount (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	return meshEffect->GetPropertiesCount();
}

const int* NewtonMeshGetIndexToVertexMap(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->GetIndexToVertexMap();
}

int NewtonMeshGetVertexWeights(const NewtonMesh* const mesh, int vertexIndex, int* const weightIndices, dFloat* const weightFactors)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->GetVertexWeights (vertexIndex, weightIndices, weightFactors);
}

int NewtonMeshHasNormalChannel(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->HasNormalChannel() ? 1 : 0;
}

int NewtonMeshHasBinormalChannel(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->HasBinormalChannel() ? 1 : 0;
}

int NewtonMeshHasUV0Channel(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->HasUV0Channel() ? 1 : 0;
}

int NewtonMeshHasUV1Channel(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->HasUV1Channel() ? 1 : 0;
}

int NewtonMeshHasVertexColorChannel(const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	return meshEffect->HasVertexColorChannel() ? 1 : 0;
}


void NewtonMeshGetVertexDoubleChannel (const NewtonMesh* const mesh, int vertexStrideInByte, dFloat64* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetVertexChannel64(vertexStrideInByte, (dgFloat64*)outBuffer);
}

void NewtonMeshGetVertexChannel (const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetVertexChannel(vertexStrideInByte, (dgFloat32*)outBuffer);
}

void NewtonMeshGetNormalChannel(const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetNormalChannel(vertexStrideInByte, (dgFloat32*)outBuffer);
}

void NewtonMeshGetBinormalChannel(const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetBinormalChannel(vertexStrideInByte, (dgFloat32*)outBuffer);
}

void NewtonMeshGetUV0Channel(const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetUV0Channel(vertexStrideInByte, (dgFloat32*)outBuffer);
}

void NewtonMeshGetUV1Channel(const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetUV1Channel(vertexStrideInByte, (dgFloat32*)outBuffer);
}

void NewtonMeshGetVertexColorChannel(const NewtonMesh* const mesh, int vertexStrideInByte, dFloat* const outBuffer)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*)mesh;
	meshEffect->GetVertexColorChannel(vertexStrideInByte, (dgFloat32*)outBuffer);
}


void* NewtonMeshBeginHandle (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->MaterialGeometryBegin();
}

void NewtonMeshEndHandle (const NewtonMesh* const mesh, void* const handle)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;
	meshEffect->MaterialGeomteryEnd((dgMeshEffect::dgIndexArray*) handle);
}



int NewtonMeshFirstMaterial (const NewtonMesh* const mesh, void* const handle)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->GetFirstMaterial((dgMeshEffect::dgIndexArray*) handle);
}

int NewtonMeshNextMaterial (const NewtonMesh* const mesh, void* const handle, int materialId)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->GetNextMaterial((dgMeshEffect::dgIndexArray*) handle, materialId);
}

int NewtonMeshMaterialGetMaterial (const NewtonMesh* const mesh, void* const handle, int materialId)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return  meshEffect->GetMaterialID ((dgMeshEffect::dgIndexArray*) handle, materialId);	
}

int NewtonMeshMaterialGetIndexCount (const NewtonMesh* const mesh, void* const handle, int materialId)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	return meshEffect->GetMaterialIndexCount ((dgMeshEffect::dgIndexArray*) handle, materialId);		
}

void NewtonMeshMaterialGetIndexStream (const NewtonMesh* const mesh, void* const handle, int materialId, int* const index)
{
	TRACE_FUNCTION(__FUNCTION__);	
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	meshEffect->GetMaterialGetIndexStream ((dgMeshEffect::dgIndexArray*) handle, materialId, index);		
}

void NewtonMeshMaterialGetIndexStreamShort (const NewtonMesh* const mesh, void* const handle, int materialId, short int* const index)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgMeshEffect* const meshEffect = (dgMeshEffect*) mesh;

	meshEffect->GetMaterialGetIndexStreamShort ((dgMeshEffect::dgIndexArray*) handle, materialId, index);		
}


NewtonMesh* NewtonMeshCreateFirstSingleSegment (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgMeshEffect* const effectMesh = (dgMeshEffect*)mesh;
	dgPolyhedra segment(effectMesh->GetAllocator());

	effectMesh->BeginConectedSurface();
	if (effectMesh->GetConectedSurface (segment)) {
		dgMeshEffect* const solid = new (effectMesh->GetAllocator()) dgMeshEffect(segment, *((dgMeshEffect*)mesh));
	return (NewtonMesh*)solid;
	} else {
		return NULL;
	}
}

NewtonMesh* NewtonMeshCreateNextSingleSegment (const NewtonMesh* const mesh, const NewtonMesh* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgMeshEffect* const effectMesh = (dgMeshEffect*)mesh;
	dgPolyhedra nextSegment(effectMesh->GetAllocator());

	dgAssert (segment);
	dgInt32 moreSegments = effectMesh->GetConectedSurface (nextSegment);

	dgMeshEffect* solid;
	if (moreSegments) {
		solid = new (effectMesh->GetAllocator()) dgMeshEffect(nextSegment, *effectMesh);
	} else {
		solid = NULL;
		effectMesh->EndConectedSurface();
	}

	return (NewtonMesh*)solid;
}

NewtonMesh* NewtonMeshCreateFirstLayer (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgMeshEffect* const effectMesh = (dgMeshEffect*)mesh;
	return (NewtonMesh*) effectMesh->GetFirstLayer ();
}

NewtonMesh* NewtonMeshCreateNextLayer (const NewtonMesh* const mesh, const NewtonMesh* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);

	dgMeshEffect* const effectMesh = (dgMeshEffect*)mesh;
	return (NewtonMesh*) effectMesh->GetNextLayer ((dgMeshEffect*)segment);
}



int NewtonMeshGetTotalFaceCount (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetTotalFaceCount();
}

int NewtonMeshGetTotalIndexCount (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetTotalIndexCount();
}

void NewtonMeshGetFaces (const NewtonMesh* const mesh, int* const faceIndexCount, int* const faceMaterial, void** const faceIndices)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*)mesh)->GetFaces (faceIndexCount, faceMaterial, faceIndices);
}


void* NewtonMeshGetFirstVertex (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFirstVertex ();
}

void* NewtonMeshGetNextVertex (const NewtonMesh* const mesh, const void* const vertex)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetNextVertex (vertex);
}

int NewtonMeshGetVertexIndex (const NewtonMesh* const mesh, const void* const vertex)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetVertexIndex (vertex);
}

void* NewtonMeshGetFirstPoint (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFirstPoint ();
}
void* NewtonMeshGetNextPoint (const NewtonMesh* const mesh, const void* const point)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetNextPoint (point);
}

int NewtonMeshGetPointIndex (const NewtonMesh* const mesh, const void* const point)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetPointIndex (point);
}

int NewtonMeshGetVertexIndexFromPoint (const NewtonMesh* const mesh, const void* const point)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetVertexIndexFromPoint (point);
}

void* NewtonMeshGetFirstEdge (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFirstEdge ();
}

void* NewtonMeshGetNextEdge (const NewtonMesh* const mesh, const void* const edge)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetNextEdge (edge);
}

void NewtonMeshGetEdgeIndices (const NewtonMesh* const mesh, const void* const edge, int* const v0, int* const v1)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetEdgeIndex (edge, *v0, *v1);
}


//void NewtonMeshGetEdgePointIndices (const NewtonMesh* const mesh, const void* const edge, int* const v0, int* const v1)
//{
//	return ((dgMeshEffect*)mesh)->GetEdgeAttributeIndex (edge, *v0, *v1);
//}

void* NewtonMeshGetFirstFace (const NewtonMesh* const mesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFirstFace ();
}

void* NewtonMeshGetNextFace (const NewtonMesh* const mesh, const void* const face)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetNextFace (face);
}

int NewtonMeshIsFaceOpen (const NewtonMesh* const mesh, const void* const face)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->IsFaceOpen (face);
}

int NewtonMeshGetFaceIndexCount (const NewtonMesh* const mesh, const void* const face)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFaceIndexCount (face);
}

int NewtonMeshGetFaceMaterial (const NewtonMesh* const mesh, const void* const face)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->GetFaceMaterial (face);
}

void NewtonMeshSetFaceMaterial (const NewtonMesh* const mesh, const void* const face, int matId)
{
	TRACE_FUNCTION(__FUNCTION__);
	return ((dgMeshEffect*)mesh)->SetFaceMaterial (face, matId);
}

void NewtonMeshGetFaceIndices (const NewtonMesh* const mesh, const void* const face, int* const indices)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*)mesh)->GetFaceIndex (face, indices);
}

void NewtonMeshGetFacePointIndices (const NewtonMesh* const mesh, const void* const face, int* const indices)
{
	TRACE_FUNCTION(__FUNCTION__);
	((dgMeshEffect*)mesh)->GetFaceAttributeIndex (face, indices);
}

void NewtonMeshCalculateFaceNormal (const NewtonMesh* const mesh, const void* const face, dFloat64* const normal)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBigVector n (((dgMeshEffect*)mesh)->CalculateFaceNormal (face));
	normal[0] = n.m_x;
	normal[1] = n.m_y;
	normal[2] = n.m_z;
}

NewtonCollision* NewtonCreateDeformableSolid(const NewtonWorld* const newtonWorld, const NewtonMesh* const mesh, int shapeID)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return (NewtonCollision*) world->CreateDeformableSolid ((dgMeshEffect*)mesh, shapeID);
}

NewtonCollision* NewtonCreateMassSpringDamperSystem (const NewtonWorld* const newtonWorld, int shapeID,
													 const dFloat* const points, int pointCount, int strideInBytes, const dFloat* const pointMass, 
													 const int* const links, int linksCount, const dFloat* const linksSpring, const dFloat* const linksDamper)
{
	TRACE_FUNCTION(__FUNCTION__);
	Newton* const world = (Newton *)newtonWorld;
	return (NewtonCollision*)world->CreateMassSpringDamperSystem (shapeID, pointCount, points, strideInBytes, pointMass, linksCount, links, linksSpring, linksDamper);
}

int NewtonDeformableMeshGetParticleCount(const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
		dgCollisionLumpedMassParticles* const deformableShape = (dgCollisionLumpedMassParticles*)collision->GetChildShape();
		return deformableShape->GetCount();
	}
	return 0;
}


const dFloat* NewtonDeformableMeshGetParticleArray(const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
		dgCollisionLumpedMassParticles* const deformableShape = (dgCollisionLumpedMassParticles*)collision->GetChildShape();
		const dgVector* const posit = deformableShape->GetPositions();
		return &posit[0].m_x;
	}
	return NULL;
}


int NewtonDeformableMeshGetParticleStrideInBytes(const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionLumpedMass_RTTI)) {
		dgCollisionLumpedMassParticles* const deformableShape = (dgCollisionLumpedMassParticles*)collision->GetChildShape();
		return deformableShape->GetStrideInByte();
	}
	return 0;
}



/*
void NewtonDeformableMeshConstraintParticle(NewtonCollision* const deformableMesh, int particleIndex, const dFloat* const posit, const NewtonBody* const body)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*)deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*)collision->GetChildShape();
		dgVector position(posit[0], posit[1], posit[2], dgFloat32(0.0f));
		deformableShape->ConstraintParticle(particleIndex, position, (dgBody*)body);
	}
}



void NewtonDeformableMeshCreateClusters (NewtonCollision* const deformableMesh, int clunsterCount, dFloat overlapingWidth)
{
	dgAssert(0);

	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->CreateClusters(clunsterCount, overlapingWidth);
	}

}

void NewtonDeformableMeshSetDebugCallback (NewtonCollision* const deformableMesh, NewtonCollisionIterator callback)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->SetOnDebugDisplay((dgCollision::OnDebugCollisionMeshCallback)callback); 
	}
}

void NewtonDeformableMeshGetParticlePosition (NewtonCollision* const deformableMesh, int particleIndex, dFloat* const posit)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		dgVector p (deformableShape->GetParticlePosition(particleIndex));
		posit[0] = p[0];
		posit[1] = p[1];
		posit[2] = p[2];
	}
}

void NewtonDeformableMeshBeginConfiguration (const NewtonCollision* const deformableMesh)
{
}

void NewtonDeformableMeshEndConfiguration (const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->EndConfiguration();
	}
}

void NewtonDeformableMeshUnconstraintParticle (NewtonCollision* const deformableMesh, int partivleIndex)
{
}



void NewtonDeformableMeshSetSkinThickness (NewtonCollision* const deformableMesh, dFloat skinThickness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->SetSkinThickness(skinThickness);
	}
}

void NewtonDeformableMeshSetPlasticity (NewtonCollision* const deformableMesh, dFloat plasticity)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgAssert (0);

	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformable = (dgCollisionDeformableMesh*) collision;
		deformable->SetPlasticity (plasticity);
	}
}

void NewtonDeformableMeshSetStiffness (NewtonCollision* const deformableMesh, dFloat stiffness)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgAssert (0);

	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformable = (dgCollisionDeformableMesh*) collision;
		deformable->SetStiffness(stiffness);
	}
}


int NewtonDeformableMeshGetVertexCount (const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return deformableShape->GetVisualPointsCount();
	}
	return 0;
}

void NewtonDeformableMeshUpdateRenderNormals (const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->UpdateVisualNormals();
	}
}

void NewtonDeformableMeshGetVertexStreams (const NewtonCollision* const deformableMesh, int vertexStrideInByte, dFloat* const vertex, int normalStrideInByte, dFloat* const normal, int uvStrideInByte0, dFloat* const uv0)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		deformableShape->GetVisualVertexData(vertexStrideInByte, vertex, normalStrideInByte, normal, uvStrideInByte0, uv0);
	}
}

NewtonDeformableMeshSegment* NewtonDeformableMeshGetFirstSegment (const NewtonCollision* const deformableMesh)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return (NewtonDeformableMeshSegment*) deformableShape->GetFirtVisualSegment();
	}
	return NULL;
}

NewtonDeformableMeshSegment* NewtonDeformableMeshGetNextSegment (const NewtonCollision* const deformableMesh, const NewtonDeformableMeshSegment* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return (NewtonDeformableMeshSegment*) deformableShape->GetNextVisualSegment((void*)segment);
	}
	return NULL;
}

int NewtonDeformableMeshSegmentGetMaterialID (const NewtonCollision* const deformableMesh, const NewtonDeformableMeshSegment* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return deformableShape->GetSegmentMaterial((void*)segment);
	}
	return 0;
}

int NewtonDeformableMeshSegmentGetIndexCount (const NewtonCollision* const deformableMesh, const NewtonDeformableMeshSegment* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return deformableShape->GetSegmentIndexCount((void*)segment);
	}
	return 0;
}

const int* NewtonDeformableMeshSegmentGetIndexList (const NewtonCollision* const deformableMesh, const NewtonDeformableMeshSegment* const segment)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgCollisionInstance* const collision = (dgCollisionInstance*) deformableMesh;
	if (collision->IsType(dgCollision::dgCollisionDeformableMesh_RTTI)) {
		dgCollisionDeformableMesh* const deformableShape = (dgCollisionDeformableMesh*) collision->GetChildShape();
		return deformableShape->GetSegmentIndexList((void*)segment);
	}
	return NULL;
}
*/

/*! @} */ // end of


void* NewtonCollisionAggregateCreate(NewtonWorld* const worldPtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgWorld* const world = (dgWorld*) worldPtr;
	return world->CreateAggreGate();
}

void NewtonCollisionAggregateDestroy(void* const aggregatePtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*) aggregatePtr;
	aggregate->m_broadPhase->GetWorld()->DestroyAggregate(aggregate);
}

void NewtonCollisionAggregateAddBody(void* const aggregatePtr, const NewtonBody* const body)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*) aggregatePtr;
	aggregate->AddBody((dgBody*)body);
}

void NewtonCollisionAggregateRemoveBody(void* const aggregatePtr, const NewtonBody* const body)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*) aggregatePtr;
	aggregate->RemoveBody((dgBody*)body);
}

int NewtonCollisionAggregateGetSelfCollision(void* const aggregatePtr)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*) aggregatePtr;
	return aggregate->GetSelfCollision() ? true : false;
}

void NewtonCollisionAggregateSetSelfCollision(void* const aggregatePtr, int state)
{
	TRACE_FUNCTION(__FUNCTION__);
	dgBroadPhaseAggregate* const aggregate = (dgBroadPhaseAggregate*) aggregatePtr;
	aggregate->SetSelfCollision(state ? true : false);
}
/*! @} */ // end of


