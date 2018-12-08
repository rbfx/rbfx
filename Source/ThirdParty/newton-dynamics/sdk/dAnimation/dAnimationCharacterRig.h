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


#ifndef __D_ANIMATION_CHARACTER_RIG_H__
#define __D_ANIMATION_CHARACTER_RIG_H__

#include "dAnimationStdAfx.h"
#include "dAnimationRigJoint.h"
#include "dAnimationAcyclicSolver.h"

class dAnimationRigEffector;
class dAnimationEffectorBlendRoot;

class dAnimationCharacterRig: public dCustomControllerBase, public dAnimationRigJoint
{
	public:
	dAnimationCharacterRig ();
	~dAnimationCharacterRig ();
	
	NewtonBody* GetNewtonBody() const;
	virtual void Finalize();

	dAnimationEffectorBlendRoot* GetAnimationTree () const;
	void SetAnimationTree (dAnimationEffectorBlendRoot* const animTree);

	dMatrix GetBasePoseMatrix() const;
	const dMatrix& GetLocalFrame() const {return m_localFrame;}

	dList<dAnimationRigEffector*>& GetEffectors() {return m_effectors;}

	protected:
	virtual void Init(NewtonBody* const body, const dMatrix& localFrameInGlobalSpace);
	virtual void PreUpdate(dFloat timestep, int threadIndex);
	virtual void PostUpdate(dFloat timestep, int threadIndex);
	virtual void Debug(dCustomJoint::dDebugDisplay* const debugContext) const;
	
	dAnimationAcyclicJoint* GetStaticWorld() {return &m_staticWorld;}

	dMatrix m_localFrame;
	dAnimationAcyclicJoint m_staticWorld;
	dAnimationAcyclicSolver m_solver;
	dList<dAnimationRigEffector*> m_effectors;
	dAnimationEffectorBlendRoot* m_animationTree;

	friend class dAnimationRigEffector;
	friend class dAnimationCharacterRigManager;
};


#endif 

