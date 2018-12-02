/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_BODY_H_
#define _D_NEWTON_BODY_H_

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"
#include "dNewtonTransformLerp.h"

class dNewton;
class dNewtonCollision;

class dNewtonBody: public dNewtonAlloc, public dNewtonTransformLerp
{
	public:
	enum dBodyType
	{
		m_dynamic,
		m_kinematic,
		m_unknown,
	};

	CNEWTON_API dNewtonBody (dNewton* const world, dFloat mass, const dNewtonCollision* const collision, void* const userData, const dFloat* const matrix, dBodyType m_type, dNewtonBody* const parent);
	CNEWTON_API virtual ~dNewtonBody();

	dBodyType GetType() const {return m_bodyType;}

	CNEWTON_API bool GetSleepState() const;
	CNEWTON_API void SetSleepState(bool state) const;

	CNEWTON_API bool GetAutoSleepMode() const;
	CNEWTON_API void SetAutoSleepMode(bool mode);

	CNEWTON_API void SetMatrix (const dFloat* const matrix);
	CNEWTON_API void GetMatrix (dFloat* const matrix) const;

	CNEWTON_API void GetVisualMatrix (dFloat param, dFloat* const matrix) const;

	CNEWTON_API void SetVeloc (const dFloat* const veloc);
	CNEWTON_API void GetVeloc (dFloat* const veloc) const;

	CNEWTON_API void SetOmega (const dFloat* const omega);
	CNEWTON_API void GetOmega (dFloat* const omega) const;

	CNEWTON_API void SetForce (const dFloat* const force);
	CNEWTON_API void AddForce (const dFloat* const force);
	//CNEWTON_API void GetForce (dFloat* const force) const;

	CNEWTON_API void SetTorque (const dFloat* const torque);
	CNEWTON_API void AddTorque (const dFloat* const torque);
	//CNEWTON_API void GetTorque (dFloat* const torque) const;

	CNEWTON_API dFloat GetLinearDrag () const;
	CNEWTON_API void SetLinearDrag (const dFloat drag);

	CNEWTON_API void GetAngularDrag (dFloat* const drag) const;
	CNEWTON_API void SetAngularDrag (const dFloat* const drag);
	
//	NEWTON_API void  NewtonBodySetLinearDamping (const NewtonBody* const body, dFloat linearDamp);
//	NEWTON_API void  NewtonBodySetAngularDamping (const NewtonBody* const body, const dFloat* const angularDamp);

	CNEWTON_API void GetCenterOfMass (dFloat* const com) const;
	CNEWTON_API void SetCenterOfMass (const dFloat* const com);

	 
    CNEWTON_API void SetMassAndInertia (dFloat mass, dFloat Ixx, dFloat Iyy, dFloat Izz);
	CNEWTON_API void SetMassAndInertia (dFloat mass, const dNewtonCollision* const collision);
	CNEWTON_API void GetMassAndInertia (dFloat& mass, dFloat& Ixx, dFloat& Iyy, dFloat& Izz) const;

	CNEWTON_API void SetCCDMode (bool mode);
	CNEWTON_API bool GetCCDMode () const;

	// applications can overload this to update game object information each time there transformation changes 
	CNEWTON_API virtual void OnApplicationPostTransform (dFloat timestep) {};

	// call each time the physics update the body transformation 
	CNEWTON_API virtual void OnBodyTransform (const dFloat* const matrix, int threadIndex);

	CNEWTON_API virtual void OnContactProcess (dNewtonContactMaterial* const contactMaterial, dFloat timestep, int threadIndex) const {}

	CNEWTON_API void* GetUserData() const;
	CNEWTON_API void SetUserData(void* const userData);

	
	CNEWTON_API dNewtonBody* GetParent() const;
	CNEWTON_API dNewtonBody* GetChild() const;
	CNEWTON_API dNewtonBody* GetSibling() const;
	CNEWTON_API void AttachChild (dNewtonBody* const child);

	CNEWTON_API void* GetBoneArticulation() const;
	CNEWTON_API void SetBoneArticulation(void* const boneArticulation);
	
	CNEWTON_API dNewton* GetNewton () const;

	CNEWTON_API NewtonBody* GetNewtonBody () const;
	CNEWTON_API dNewtonCollision* GetCollision() const;
	CNEWTON_API void SetCollision(const dNewtonCollision* const collision) const;

	protected:
	CNEWTON_API dNewtonBody(dBodyType type, dNewtonBody* const parent);
	CNEWTON_API virtual void SetBody  (NewtonBody* const body);

	private: 
	CNEWTON_API static void OnBodyDestroy (const NewtonBody* const body);
	//CNEWTON_API static void OnBodyTransform (const NewtonBody* const body, const dFloat* const matrix, int threadIndex);

	protected:
	NewtonBody* m_body;
	dNewtonBody* m_child;
	dNewtonBody* m_sibling;
	dNewtonBody* m_parent;
	void* m_boneArticulation; 
	void* m_userData;
	dBodyType m_bodyType;
};

#endif
