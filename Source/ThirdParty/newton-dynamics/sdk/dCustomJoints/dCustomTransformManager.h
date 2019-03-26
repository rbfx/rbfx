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


//////////////////////////////////////////////////////////////////////

#ifndef D_CUSTOM_ARTICULATED_TRANSFORM_MANAGER_H_
#define D_CUSTOM_ARTICULATED_TRANSFORM_MANAGER_H_

#include "dCustomJointLibraryStdAfx.h"
#include "dCustomListener.h"


#define HIERACHICAL_ARTICULATED_PLUGIN_NAME	"__articulatedTransformManager__"


class dCustomTransformManager;

class dSkeletonBone: public dList<dSkeletonBone>
{
	public:
	dSkeletonBone()
		:dList<dSkeletonBone>()
		,m_bindMatrix(dGetIdentityMatrix())
		,m_body(NULL)
		,m_parent(NULL)
	{
	}

	~dSkeletonBone()
	{
	}

	NewtonBody* GetBody() const { return m_body; }
	const dMatrix& GetBindMatrix() const {return m_bindMatrix;}
	
	void* GetUserData() const { return m_userData; }
	void SetUserData(void* const data) { m_userData = data; }

	CUSTOM_JOINTS_API dCustomJoint* GetParentJoint() const;

	protected:
	dMatrix m_bindMatrix;
	NewtonBody* m_body;
	dSkeletonBone* m_parent;
	void* m_userData; 
	

	friend class dCustomTransformController;
};

class dCustomTransformController: public dSkeletonBone
{
	public:
	dCustomTransformController()
		:dSkeletonBone()
		,m_calculateLocalTransform(true)
	{
	}

	~dCustomTransformController()
	{
	}

	CUSTOM_JOINTS_API dSkeletonBone* AddBone (NewtonBody* const bone, const dMatrix& bindMatrix, dSkeletonBone* const parentBone);

	void SetCalculateLocalTransforms(bool val) { m_calculateLocalTransform = val; }
	bool GetCalculateLocalTransforms() const { return m_calculateLocalTransform; }

	private:
	void PostUpdate(dCustomTransformManager* const manager, dFloat timestep) const;

	bool m_calculateLocalTransform;

	friend class dCustomTransformManager;
};

class dCustomTransformManager: public dCustomListener
{
	public:
	CUSTOM_JOINTS_API dCustomTransformManager(NewtonWorld* const world, const char* const name = HIERACHICAL_ARTICULATED_PLUGIN_NAME);
	CUSTOM_JOINTS_API virtual ~dCustomTransformManager();

	CUSTOM_JOINTS_API virtual dCustomTransformController* CreateController (NewtonBody* const bone, const dMatrix& bindMatrix);
	CUSTOM_JOINTS_API virtual void DestroyController (dCustomTransformController* const controller);

	CUSTOM_JOINTS_API virtual void OnDebug(dCustomJoint::dDebugDisplay* const debugContext) = 0;
	CUSTOM_JOINTS_API virtual void OnPreUpdate (dCustomTransformController* const controller, dFloat timestep, int threadIndex) const = 0;
	CUSTOM_JOINTS_API virtual void OnUpdateTransform (const dCustomTransformController::dSkeletonBone* const bone, const dMatrix& localMatrix) const = 0;

	protected:
	CUSTOM_JOINTS_API virtual void OnDestroy();
	CUSTOM_JOINTS_API virtual void PreUpdate(dFloat timestep);
	CUSTOM_JOINTS_API virtual void PostUpdate(dFloat timestep);

	private: 
	dList<dCustomTransformController> m_controllerList;
};


#endif 

