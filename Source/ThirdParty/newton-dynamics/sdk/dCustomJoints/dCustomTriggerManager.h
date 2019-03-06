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


// NewtonPlayerControllerManager.h: interface for the NewtonPlayerControllerManager class.
//
//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_TRIGGER_MANAGER_H_
#define D_CUSTOM_TRIGGER_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomListener.h"


#define TRIGGER_PLUGIN_NAME				"__triggerManager__"
// a trigger is volume of space that is there to send a message 
// to other objects when and object enter of leave the trigger region  
// they are not visible and do not collide with bodies, but the generate contacts


class dCustomTriggerManager;

class dCustomTriggerController
{
	public:
	dCustomTriggerController()
		:m_manifest()
		,m_userData(NULL)
		,m_kinematicBody(NULL)
		,m_manager(NULL)
	{
	}

	~dCustomTriggerController()
	{
		NewtonDestroyBody(m_kinematicBody);
	}
	
	void* GetUserData() const {return m_userData;}
	void SetUserData(void* const data) {m_userData = data;}

	NewtonBody* GetBody() const {return m_kinematicBody; }

	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void PostUpdate(dFloat timestep, int threadIndex);

	CUSTOM_JOINTS_API virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;

	private:
	dTree<unsigned, NewtonBody*> m_manifest;
	void* m_userData;
	NewtonBody* m_kinematicBody;
	dCustomTriggerManager* m_manager;

	friend class dCustomTriggerManager;
};

class dCustomTriggerManager: public dCustomListener
{
	public:
	enum dTriggerEventType
	{
		m_inTrigger,
		m_exitTrigger,
		m_enterTrigger,
	};

	CUSTOM_JOINTS_API dCustomTriggerManager (NewtonWorld* const world);
	CUSTOM_JOINTS_API virtual ~dCustomTriggerManager();
	
	CUSTOM_JOINTS_API virtual dCustomTriggerController* CreateTrigger (const dMatrix& matrix, NewtonCollision* const convexShape, void* const userData);
	CUSTOM_JOINTS_API virtual void DestroyTrigger (dCustomTriggerController* const trigger);
	CUSTOM_JOINTS_API virtual void EventCallback (const dCustomTriggerController* const me, dTriggerEventType eventType, NewtonBody* const guess) const = 0;

	protected:
	virtual void Debug () const {};
	CUSTOM_JOINTS_API virtual void OnDestroyBody (NewtonBody* const body); 
	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep);

	CUSTOM_JOINTS_API virtual void OnDestroy();

	virtual void PostUpdate(dFloat timestep)
	{
		// bypass the entire Post Update call by not calling the base class
	}

	private:
	void UpdateTrigger (dCustomTriggerController* const controller);
	static void UpdateTrigger (NewtonWorld* const world, void* const context, int threadIndex);

	dList<dCustomTriggerController> m_triggerList;
	dFloat m_timestep;
	unsigned m_lock;
	unsigned m_lru;
};


#endif 


