/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/


// dCustomControllerManager.h: interface for the dCustomControllerManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_CONTROLLER_MANAGER_H_
#define D_CUSTOM_CONTROLLER_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomAlloc.h"


class dCustomControllerConvexCastPreFilter
{	
	public:
	dCustomControllerConvexCastPreFilter ()
		:m_me(NULL)
	{
	}

	dCustomControllerConvexCastPreFilter (const NewtonBody* const me)
		:m_me(me)
	{
	}

	~dCustomControllerConvexCastPreFilter ()
	{
	}

	virtual unsigned Prefilter(const NewtonBody* const body, const NewtonCollision* const myCollision)
	{
		return NewtonCollisionGetMode(myCollision);
	}

	private:
	CUSTOM_JOINTS_API static unsigned Prefilter(const NewtonBody* const body, const NewtonCollision* const myCollision, void* const userData)
	{
		dCustomControllerConvexCastPreFilter* const filter = (dCustomControllerConvexCastPreFilter*) userData;
		return (body != filter->m_me) ? filter->Prefilter (body, myCollision) : 0;
	}

	protected:
	const NewtonBody* m_me;
	friend class dCustomPlayerController;
	friend class dCustomVehicleController;
};

class dCustomControllerConvexRayFilter: public dCustomControllerConvexCastPreFilter
{	
	public:
	CUSTOM_JOINTS_API dCustomControllerConvexRayFilter (const NewtonBody* const me)
		:dCustomControllerConvexCastPreFilter()
		,m_hitBody(NULL)
		,m_shapeHit(NULL)
		,m_collisionID(0) 
	{
		m_me = me;
	}

	CUSTOM_JOINTS_API static dFloat Filter (const NewtonBody* const body, const NewtonCollision* const shapeHit, const dFloat* const hitContact, const dFloat* const hitNormal, dLong collisionID, void* const userData, dFloat intersectParam)
	{
		dCustomControllerConvexRayFilter* const filter = (dCustomControllerConvexRayFilter*) userData;
		dAssert (body != filter->m_me);
		if (intersectParam < filter->m_intersectParam) {
			filter->m_hitBody = body;	
			filter->m_shapeHit = shapeHit;
			filter->m_collisionID = collisionID;
			filter->m_intersectParam = intersectParam;
			filter->m_hitContact = dVector(hitContact[0], hitContact[1], hitContact[2], 0.0f); 
			filter->m_hitNormal = dVector(hitNormal[0], hitNormal[1], hitNormal[2], 0.0f); 
		}
		return intersectParam;
	}
	
	dVector m_hitContact; 
	dVector m_hitNormal;
	const NewtonBody* m_hitBody;
	const NewtonCollision* m_shapeHit;
	dLong m_collisionID; 
	dFloat m_intersectParam;
};


class dCustomControllerManagerBase
{
	public:
	dCustomControllerManagerBase(NewtonWorld* const world)
		:m_world(world)
		,m_curTimestep(0.0f)
	{
	}

	NewtonWorld* GetWorld() const
	{
		return m_world;
	}

	dFloat GetTimeStep() const
	{
		return m_curTimestep;
	}

	NewtonWorld* m_world;
	dFloat m_curTimestep;
};


class dCustomControllerBase
{
	public:
	dCustomControllerBase ()
		:m_userData(NULL)
		,m_body(NULL)
		,m_manager(NULL)
	{
	}

	virtual ~dCustomControllerBase ()
	{
	}

	void* GetUserData () const 
	{
		return m_userData;
	}

	void SetUserData (void* const userData)
	{
		m_userData = userData;
	}

	NewtonBody* GetBody() const 
	{
		return m_body;
	}

	dCustomControllerManagerBase* GetManager() const
	{
		return m_manager;
	}

	bool operator== (const dCustomControllerBase& copy) const
	{
		dAssert (0);
		return false;
	}

	virtual void PreUpdate (dFloat timestep, int threadIndex) = 0;
	virtual void PostUpdate (dFloat timestep, int threadIndex) = 0;
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const
	{
	}

	void* m_userData;
	NewtonBody* m_body;
	dCustomControllerManagerBase* m_manager;
};

template<class CONTROLLER_BASE>
class dCustomControllerManager: public dCustomControllerManagerBase, public dList<CONTROLLER_BASE>
{
	public:
	dCustomControllerManager(NewtonWorld* const world, const char* const managerName);
	~dCustomControllerManager();

	virtual CONTROLLER_BASE* CreateController ();
	virtual void DestroyController (CONTROLLER_BASE* const controller);
	virtual void OnDestroyBody (NewtonBody* const body) 
	{
	}

	public:
	virtual void PreUpdate(dFloat timestep);
	virtual void PostUpdate(dFloat timestep);
	virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext);

	private:
	void DestroyAllController ();

	static void Destroy (const NewtonWorld* const world, void* const listenerUserData);
	static void Debug (const NewtonWorld* const world, void* const listenerUserData, void* const debugContext);
	static void PreUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep);
	static void PostUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep);
	static void OnBodyDestroy (const NewtonWorld* const world, void* const listener, NewtonBody* const body);

	static void PreUpdateKernel (NewtonWorld* const world, void* const context, int threadIndex);
	static void PostUpdateKernel (NewtonWorld* const world, void* const context, int threadIndex);
};


template<class CONTROLLER_BASE>
dCustomControllerManager<CONTROLLER_BASE>::dCustomControllerManager(NewtonWorld* const world, const char* const managerName)
	:dCustomControllerManagerBase(world)
	,dList<CONTROLLER_BASE>()
{
	void* const listener = NewtonWorldAddListener(world, managerName, this);

	NewtonWorldListenerSetDebugCallback(world, listener, Debug);
	NewtonWorldListenerSetDestructorCallback(world, listener, Destroy);
	NewtonWorldListenerSetPreUpdateCallback(world, listener, PreUpdate);
	NewtonWorldListenerSetPostUpdateCallback(world, listener, PostUpdate);
	NewtonWorldListenerSetBodyDestroyCallback(world, listener, OnBodyDestroy);
}

template<class CONTROLLER_BASE>
dCustomControllerManager<CONTROLLER_BASE>::~dCustomControllerManager()
{
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::DestroyAllController ()
{
	while (dCustomControllerManager<CONTROLLER_BASE>::GetCount()) {
		DestroyController (&dCustomControllerManager<CONTROLLER_BASE>::GetLast()->GetInfo());
	}
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PreUpdate(dFloat timestep)
{
	for (typename dCustomControllerManager<CONTROLLER_BASE>::dListNode* node = dCustomControllerManager<CONTROLLER_BASE>::GetFirst(); node; node = node->GetNext()) {
		NewtonDispachThreadJob(m_world, PreUpdateKernel, &node->GetInfo(), __FUNCTION__);
	}
	NewtonSyncThreadJobs(m_world);
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PostUpdate(dFloat timestep)
{
	for (typename dList<CONTROLLER_BASE>::dListNode* node = dList<CONTROLLER_BASE>::GetFirst(); node; node = node->GetNext()) {
		NewtonDispachThreadJob(m_world, PostUpdateKernel, &node->GetInfo(), __FUNCTION__);
	}
	NewtonSyncThreadJobs(m_world);
}


template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PreUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep)
{
	dCustomControllerManager* const me = (dCustomControllerManager*) listenerUserData;
	me->m_curTimestep = timestep;
	me->PreUpdate(timestep);
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PostUpdate (const NewtonWorld* const world, void* const listenerUserData, dFloat timestep)
{
	dCustomControllerManager* const me = (dCustomControllerManager*) listenerUserData;
	me->m_curTimestep = timestep;
	me->PostUpdate(timestep);
}


template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::OnDebug(dCustomJoint::dDebugDisplay* const debugContext)
{
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::Debug (const NewtonWorld* const world, void* const listenerUserData, void* const debugContext)
{
	dCustomControllerManager* const me = (dCustomControllerManager*) listenerUserData;
	me->OnDebug((dCustomJoint::dDebugDisplay*) debugContext);
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::Destroy (const NewtonWorld* const world, void* const listenerUserData)
{
	dCustomControllerManager* const me = (dCustomControllerManager*) listenerUserData;
	me->DestroyAllController ();
	delete me;
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::OnBodyDestroy (const NewtonWorld* const world, void* const listener, NewtonBody* const body)
{
	dCustomControllerManager* const me = (dCustomControllerManager*) NewtonWorldGetListenerUserData(world, listener);
	me->OnDestroyBody(body);
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PreUpdateKernel (NewtonWorld* const world, void* const context, int threadIndex)
{
	dCustomControllerBase* const controller = (dCustomControllerBase*) context;
	controller->PreUpdate(((dCustomControllerManager*)controller->m_manager)->GetTimeStep(), threadIndex);
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::PostUpdateKernel (NewtonWorld* const world, void* const context, int threadIndex)
{
	class dCustomControllerBase* const controller = (dCustomControllerBase*) context;
	controller->PostUpdate(((dCustomControllerManager*)controller->m_manager)->GetTimeStep(), threadIndex);
}

template<class CONTROLLER_BASE>
CONTROLLER_BASE* dCustomControllerManager<CONTROLLER_BASE>::CreateController ()
{
	CONTROLLER_BASE* const controller = &dCustomControllerManager<CONTROLLER_BASE>::Append()->GetInfo();
	controller->m_manager = this;
	return controller;
}

template<class CONTROLLER_BASE>
void dCustomControllerManager<CONTROLLER_BASE>::DestroyController (CONTROLLER_BASE* const controller)
{
	dAssert (dCustomControllerManager<CONTROLLER_BASE>::GetNodeFromInfo (*controller));
	typename dCustomControllerManager<CONTROLLER_BASE>::dListNode* const node = dCustomControllerManager<CONTROLLER_BASE>::GetNodeFromInfo (*controller);
	dCustomControllerManager<CONTROLLER_BASE>::Remove (node);
}



#endif 

