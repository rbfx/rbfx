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

#ifndef _D_NEWTON_H_
#define _D_NEWTON_H_

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"

class dNewtonBody;
class dNewtonMaterial;
class dNewtonCollision;
class dNewtonContactMaterial;

class dNewton: public dNewtonAlloc
{
	public:
	class ScopeLock
	{
		public:
		CNEWTON_API ScopeLock (unsigned* const lock);
		CNEWTON_API ~ScopeLock();

		unsigned* m_atomicLock;
	};

	CNEWTON_API dNewton();
	CNEWTON_API virtual ~dNewton();

	CNEWTON_API virtual void Update (dFloat timestepInSecunds);
	CNEWTON_API virtual void UpdateAsync (dFloat timestepInSecunds);
	CNEWTON_API virtual void UpdateOffLine (dFloat timestepInSecunds);
	CNEWTON_API void WaitForUpdateToFinish ();

	CNEWTON_API int GetNumberOfThreads() const;
	CNEWTON_API void SetNumberOfThreads(int threadCount);

	CNEWTON_API dFloat GetInterpolationParam(dFloat timestepInSecunds) const;

	CNEWTON_API void SetMaxUpdatesPerIterations (int update);
	CNEWTON_API NewtonWorld* GetNewton () const;
	
	CNEWTON_API dNewtonBody* GetFirstBody() const;
	CNEWTON_API dNewtonBody* GetNextBody(const dNewtonBody* const body) const;

	CNEWTON_API int GetBodyCount() const;
	CNEWTON_API void DestroyAllBodies();

	//CNEWTON_API virtual bool OnBodiesAABBOverlap (const dNewtonBody* const body0, const dNewtonBody* const body1, int threadIndex) const
	CNEWTON_API virtual bool OnBodiesAABBOverlap(dNewtonContactMaterial* const contactMaterial, dFloat timestep, int threadIndex) const
	{
		return true;
	}

	CNEWTON_API virtual bool OnCompoundSubCollisionAABBOverlap (const dNewtonBody* const body0, const dNewtonCollision* const subShape0, const dNewtonBody* const body1, const dNewtonCollision* const subShape1, int threadIndex) const
	{
		return true;
	}

	CNEWTON_API virtual void OnContactProcess (dNewtonContactMaterial* const contactMaterial, dFloat timestep, int threadIndex) const;

	CNEWTON_API void ResetTimer();
	CNEWTON_API dLong GetTimeInMicrosenconds() const; 

	private:
	CNEWTON_API static void OnCollisionDestructorCallback (const NewtonWorld* const newtonWorld, const NewtonCollision* const collision);
	CNEWTON_API static void OnCollisionCopyConstruct (const NewtonWorld* const newtonWorld, NewtonCollision* const collision, const NewtonCollision* const sourceCollision);
	
	//CNEWTON_API static int OnBodiesAABBOverlap(const NewtonMaterial* const material, const NewtonBody* const body0, const NewtonBody* const body1, int threadIndex);
	CNEWTON_API static int OnBodiesAABBOverlap(const NewtonJoint* const contact, dFloat timestep, int threadIndex);
	CNEWTON_API static void OnContactProcess (const NewtonJoint* const contact, dFloat timestep, int threadIndex);
	//CNEWTON_API static int OnCompoundSubCollisionAABBOverlap(const NewtonMaterial* const material, const NewtonBody* const body0, const void* const collsionNode0, const NewtonBody* const body1, const void* const collsionNode1, int threadIndex);
	CNEWTON_API static int OnCompoundSubCollisionAABBOverlap(const NewtonJoint* const contact, dFloat timestep, const NewtonBody* const body0, const void* const collsionNode0, const NewtonBody* const body1, const void* const collsionNode1, int threadIndex);

	NewtonWorld* m_world;
	dLong m_frequency;
	dLong m_baseCount;
	dLong m_microseconds;
	int m_maxUpdatePerIterations;
};

#endif
