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


#ifndef __D_ANIM_IK_CONTROLLER_H__
#define __D_ANIM_IK_CONTROLLER_H__

#include "dAnimationStdAfx.h"
#include "dAnimPose.h"
#include "dAnimIKRigJoint.h"

class dAnimIKBlendNodeRoot;

class dAnimIKController: public dCustomControllerBase, public dAnimIKRigJoint
{
	public:
	dAnimIKController ();
	~dAnimIKController ();
	
//	NewtonBody* GetNewtonBody() const;
//	virtual void Finalize();
//	dAnimationEffectorBlendRoot* GetAnimationTree () const;
//	dMatrix GetBasePoseMatrix() const;
//	const dMatrix& GetLocalFrame() const {return m_localFrame;}
//	dList<dAnimIDRigEffector*>& GetEffectors() {return m_effectors;}

	void* GetUserData() const;
	void SetUserData(void* const userData);

	dAnimPose& GetBasePose() {return m_basePose;}
	dAnimIKBlendNodeRoot* GetAnimationTree() const;
	void SetAnimationTree(dAnimIKBlendNodeRoot* const animTree);

	protected:
	virtual void Init(const dMatrix& localFrameInGlobalSpace);
	virtual void PreUpdate(dFloat timestep, int threadIndex);
	virtual void PostUpdate(dFloat timestep, int threadIndex);
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;
	
//	dAnimationAcyclicJoint* GetStaticWorld() {return &m_staticWorld;}

//	dMatrix m_localFrame;
//	dAnimationAcyclicJoint m_staticWorld;
//	dAnimationAcyclicSolver m_solver;
//	dList<dAnimIDRigEffector*> m_effectors;
	dAnimPose m_basePose;
	dAnimIKBlendNodeRoot* m_animationTree;
//	friend class dAnimIDRigEffector;
	friend class dAnimIKManager;
};


#endif 

