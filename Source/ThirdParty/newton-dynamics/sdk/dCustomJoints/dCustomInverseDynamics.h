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


#ifndef _D_CUSTOM_RAG_DOLL_MOTOR_H_
#define _D_CUSTOM_RAG_DOLL_MOTOR_H_

#include "dCustomJoint.h"


/*
class dCustomRagdollMotor_1dof: public dCustomInverseDynamics
{
	public:
	CUSTOM_JOINTS_API dCustomRagdollMotor_1dof(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent);

	CUSTOM_JOINTS_API void SetTwistAngle(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void GetTwistAngle(dFloat& minAngle, dFloat& maxAngle) const;

	protected:
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);

	dFloat m_minTwistAngle;
	dFloat m_maxTwistAngle;
	DECLARE_CUSTOM_JOINT(dCustomRagdollMotor_1dof, dCustomInverseDynamics)
};


class dCustomRagdollMotor_2dof: public dCustomInverseDynamics
{
	public:
	CUSTOM_JOINTS_API dCustomRagdollMotor_2dof(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent);

	CUSTOM_JOINTS_API void SetConeAngle(dFloat angle);
	CUSTOM_JOINTS_API dFloat GetConeAngle() const;

	protected:
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);

	dFloat m_coneAngle;
	DECLARE_CUSTOM_JOINT(dCustomRagdollMotor_2dof, dCustomInverseDynamics)
};


class dCustomRagdollMotor_3dof: public dCustomInverseDynamics
{
	public:
	CUSTOM_JOINTS_API dCustomRagdollMotor_3dof(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent);

	CUSTOM_JOINTS_API void SetConeAngle(dFloat angle);
	CUSTOM_JOINTS_API dFloat GetConeAngle() const;

	CUSTOM_JOINTS_API void SetTwistAngle(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void GetTwistAngle(dFloat& minAngle, dFloat& maxAngle) const;

	protected:
	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);

	dFloat m_coneAngle;
	dFloat m_minTwistAngle;
	dFloat m_maxTwistAngle;
	DECLARE_CUSTOM_JOINT(dCustomRagdollMotor_3dof, dCustomInverseDynamics)
};
*/

class dCustomInverseDynamicsEffector: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dCustomInverseDynamicsEffector(NewtonBody* const body, NewtonBody* const referenceBody, const dMatrix& attachmentPointInGlobalSpace);
	CUSTOM_JOINTS_API dCustomInverseDynamicsEffector(NewtonInverseDynamics* const invDynSolver, void* const invDynNode, NewtonBody* const referenceBody, const dMatrix& attachmentPointInGlobalSpace);
	CUSTOM_JOINTS_API virtual ~dCustomInverseDynamicsEffector();

	CUSTOM_JOINTS_API void SetAsSixdof (); 
	CUSTOM_JOINTS_API void SetAsThreedof (); 

	CUSTOM_JOINTS_API void SetLinearSpeed(dFloat speed);
	CUSTOM_JOINTS_API void SetAngularSpeed(dFloat speed);

	CUSTOM_JOINTS_API void SetMaxLinearFriction(dFloat friction);
	CUSTOM_JOINTS_API void SetMaxAngularFriction(dFloat friction);

	CUSTOM_JOINTS_API void SetTargetPosit(const dVector& posit);
	CUSTOM_JOINTS_API void SetTargetRotation(const dQuaternion& rotation);
	
	CUSTOM_JOINTS_API dMatrix GetBodyMatrix() const;
	CUSTOM_JOINTS_API dMatrix GetTargetMatrix() const;
	CUSTOM_JOINTS_API void SetTargetMatrix(const dMatrix& matrix);

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void Deserialize (NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;

	dMatrix m_targetMatrix;	
	NewtonBody* m_referenceBody;
	dFloat m_linearSpeed;
	dFloat m_angularSpeed;
	dFloat m_linearFriction;
	dFloat m_angularFriction;
	bool m_isSixdof;

	DECLARE_CUSTOM_JOINT(dCustomInverseDynamicsEffector, dCustomJoint)
};

class dEffectorTreeInterface: public dCustomAlloc
{
	public:
	class dEffectorTransform
	{
		public:
		dVector m_posit;
		dQuaternion m_rotation;
		dCustomInverseDynamicsEffector* m_effector;
	};

	class dEffectorPose: public dList<dEffectorTransform>
	{
		public:
		dEffectorPose(): dList<dEffectorTransform>(), m_childNode(NULL)	{}
		dEffectorTreeInterface* m_childNode;
	};

	dEffectorTreeInterface(NewtonBody* const rootBody)
		:dCustomAlloc()
		,m_rootBody(rootBody)
	{
	}

	virtual ~dEffectorTreeInterface()
	{
	}

	virtual void Evaluate(dEffectorPose& output, dFloat timestep)
	{
	}

	NewtonBody* GetRootBody() const 
	{
		return m_rootBody;
	}

	NewtonBody* m_rootBody;
};


class dEffectorTreeRoot: public dEffectorTreeInterface
{
	public:
	dEffectorTreeRoot(NewtonBody* const rootBody, dEffectorTreeInterface* const childNode)
		:dEffectorTreeInterface(rootBody)
	{
		m_pose.m_childNode = childNode;
	}
	CUSTOM_JOINTS_API virtual ~dEffectorTreeRoot();

	dEffectorPose& GetPose() {return m_pose;}

	CUSTOM_JOINTS_API void Update(dFloat timestep);
	CUSTOM_JOINTS_API void Evaluate(dEffectorPose& output, dFloat timestep);

	protected:
	dEffectorPose m_pose;
};

class dEffectorTreePose: public dEffectorTreeInterface
{
	public:
	dEffectorTreePose(NewtonBody* const rootBody)
		:dEffectorTreeInterface(rootBody)
	{
	}

	void Evaluate(dEffectorPose& output, dFloat timestep)
	{
		dAssert(0);
	}
};


class dEffectorTreeFixPose: public dEffectorTreePose
{
	public:
	dEffectorTreeFixPose(NewtonBody* const rootBody)
		:dEffectorTreePose(rootBody)
	{
	}

	dEffectorPose& GetPose() {return m_pose;}

	protected:
	CUSTOM_JOINTS_API void Evaluate(dEffectorPose& output, dFloat timestep);
	dEffectorPose m_pose;
};


class dEffectorTreeTwoWayBlender: public dEffectorTreeInterface
{
	public:
	dEffectorTreeTwoWayBlender(NewtonBody* const rootBody, dEffectorTreeInterface* const node0, dEffectorTreeInterface* const node1)
		:dEffectorTreeInterface(rootBody)
		,m_node0(node0)
		,m_node1(node1)
		,m_param(1.0f)
	{
	}

	~dEffectorTreeTwoWayBlender()
	{
		delete m_node0;
		delete m_node1;
	}

	protected:
	CUSTOM_JOINTS_API void Evaluate(dEffectorPose& output, dFloat timestep);

	dEffectorTreeInterface* m_node0;
	dEffectorTreeInterface* m_node1;
	dFloat m_param;
};


// this joint is for controlling rag dolls muscles
class dCustomInverseDynamics: public dCustomJoint
{
	public:
	CUSTOM_JOINTS_API dCustomInverseDynamics(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent);
	CUSTOM_JOINTS_API dCustomInverseDynamics (const dMatrix& pinAndPivotFrameChild, const dMatrix& pinAndPivotFrameParent, NewtonBody* const child, NewtonBody* const parent);
	CUSTOM_JOINTS_API virtual ~dCustomInverseDynamics();

	CUSTOM_JOINTS_API dFloat GetJointTorque() const;
	CUSTOM_JOINTS_API void SetJointTorque(dFloat torque);

	CUSTOM_JOINTS_API void SetTwistAngle(dFloat minAngle, dFloat maxAngle);
	CUSTOM_JOINTS_API void GetTwistAngle(dFloat& minAngle, dFloat& maxAngle) const;

	CUSTOM_JOINTS_API virtual void Debug(dDebugDisplay* const debugDisplay) const;

	protected:
	CUSTOM_JOINTS_API virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);
	CUSTOM_JOINTS_API virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	CUSTOM_JOINTS_API virtual void SubmitConstraints(dFloat timestep, int threadIndex);
	CUSTOM_JOINTS_API virtual void SubmitHingeConstraints(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep);

	dFloat m_torque;
	dFloat m_coneAngle;
	dFloat m_minTwistAngle;
	dFloat m_maxTwistAngle;
	DECLARE_CUSTOM_JOINT(dCustomInverseDynamics, dCustomJoint)
};


#endif 


