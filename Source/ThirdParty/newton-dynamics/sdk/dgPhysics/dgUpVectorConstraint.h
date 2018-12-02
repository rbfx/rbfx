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

#ifndef __DGUPVECTORCONSTRAINT__
#define __DGUPVECTORCONSTRAINT__
#include "dgBilateralConstraint.h"

//template<class T> class dgPool;

class dgUpVectorConstraint;

typedef dgUnsigned32 (dgApi *dgUpVectorJointCallback) (const dgUpVectorConstraint& upVector);

class dgUpVectorConstraint: public dgBilateralConstraint
{
	public:
	void SetJointParameterCallback (dgUpVectorJointCallback callback);
	void InitPinDir (const dgVector& pin);
	
	dgVector GetPinDir () const;
	void SetPinDir (const dgVector& pin);

	private:
	dgUpVectorConstraint();
	virtual ~dgUpVectorConstraint();

	virtual dgUnsigned32 JacobianDerivative (dgContraintDescritor& params); 
	virtual void Serialize (dgSerialize serializeCallback, void* const userData) {dgAssert (0);}

	dgUpVectorJointCallback m_callBack;

	dgMatrix m_localMatrix0;
	dgMatrix m_localMatrix1;
//	dgUnsigned32 m_reserve[3];

	friend class dgWorld;
//	friend class dgPool<dgUpVectorConstraint>;
};

/*
class dgUpVectorConstraintArray: public dgPoolContainer<dgUpVectorConstraint>
{
};
*/

#endif // !defined(__DGUPVECTORCONSTRAINT_563GFT35684GT_H)

