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


#ifndef __D_ANIM_ID_CONTROLLER_H__
#define __D_ANIM_ID_CONTROLLER_H__

#include "dAnimationStdAfx.h"
#include "dAnimIDRigJoint.h"
#include "dAnimAcyclicSolver.h"

class dAnimIDRigEffector;
class dAnimIDBlendNodeRoot;

class dAnimIDController: public dCustomControllerBase, public dAnimIDRigJoint
{
	public:
	dAnimIDController ();
	~dAnimIDController ();
	
	NewtonBody* GetNewtonBody() const;
	virtual void Finalize();

	dAnimIDBlendNodeRoot* GetAnimationTree () const;
	void SetAnimationTree (dAnimIDBlendNodeRoot* const animTree);

	dMatrix GetBasePoseMatrix() const;
	const dMatrix& GetLocalFrame() const {return m_localFrame;}

	dList<dAnimIDRigEffector*>& GetEffectors() {return m_effectors;}

	protected:
	virtual void Init(NewtonBody* const body, const dMatrix& localFrameInGlobalSpace);
	virtual void PreUpdate(dFloat timestep, int threadIndex);
	virtual void PostUpdate(dFloat timestep, int threadIndex);
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;
	
	dAnimAcyclicJoint* GetStaticWorld() {return &m_staticWorld;}

	dMatrix m_localFrame;
	dAnimAcyclicJoint m_staticWorld;
	dAnimAcyclicSolver m_solver;
	dList<dAnimIDRigEffector*> m_effectors;
	dAnimIDBlendNodeRoot* m_animationTree;

	friend class dAnimIDRigEffector;
	friend class dAnimIDManager;
};


#endif 

