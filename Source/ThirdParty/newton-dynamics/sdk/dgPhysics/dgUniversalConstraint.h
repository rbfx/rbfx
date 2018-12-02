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

#if !defined(AFX_DGUNIVERSALCONSTRAINT_H__3FA456TR_INCLUDED)
#define AFX_DGUNIVERSALCONSTRAINT_H__3FA456TR_INCLUDED
#include "dgBilateralConstraint.h"

//template<class T> class dgPool;

class dgUniversalConstraint;


typedef dgUnsigned32 (dgApi *dgUniversalJointAcceleration) (const dgUniversalConstraint& hinge, dgJointCallbackParam* param);

class dgUniversalConstraint: public dgBilateralConstraint
{
	public:
	dgFloat32 GetJointAngle0 () const;
	dgFloat32 GetJointAngle1 () const;
	dgFloat32 GetJointOmega0 () const;
	dgFloat32 GetJointOmega1 () const;

	dgVector GetJointForce () const;
	dgFloat32 CalculateStopAlpha0 (dgFloat32 angle, const dgJointCallbackParam* param) const;
	dgFloat32 CalculateStopAlpha1 (dgFloat32 angle, const dgJointCallbackParam* param) const;
	void SetJointParameterCallback (dgUniversalJointAcceleration callback);

	private:
	virtual dgUnsigned32 JacobianDerivative (dgContraintDescritor& params); 
	virtual void Serialize (dgSerialize serializeCallback, void* const userData) {dgAssert (0);}

	dgUniversalConstraint();
	virtual ~dgUniversalConstraint();

	dgMatrix m_localMatrix0;
	dgMatrix m_localMatrix1;

	dgFloat32 m_angle0;
	dgFloat32 m_angle1;
	dgUniversalJointAcceleration m_jointAccelFnt;

//#ifdef _NEWTON_USE_DOUBLE
//	dgUnsigned32 m_reserve[3];
//#else
//	dgUnsigned32 m_reserve[1];
//#endif


	friend class dgWorld;
//	friend class dgPool<dgUniversalConstraint>;
};

//class dgUniversalConstraintArray: public dgPoolContainer<dgUniversalConstraint>
//{
//};

#endif // !defined(AFX_DGUNIVERSALCONSTRAINT_H__3FA456TR_INCLUDED)

