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

#ifndef _D_NEWTON_TRANSFORM_LERP_H_
#define _D_NEWTON_TRANSFORM_LERP_H_

#include "dStdAfxNewton.h"

class dNewtonTransformLerp
{
	public:
	CNEWTON_API dNewtonTransformLerp ();
	CNEWTON_API dNewtonTransformLerp (const dFloat* const matrix);
	CNEWTON_API virtual ~dNewtonTransformLerp();

	CNEWTON_API void Update (const dFloat* const matrix);
	CNEWTON_API void ResetMatrix (const dFloat* const matrix);

	CNEWTON_API void GetTargetMatrix (dFloat* const matrix) const;
	CNEWTON_API void SetTargetMatrix (const dFloat* const matrix);
	CNEWTON_API void GetBaseMatrix (dFloat* const matrix) const;

	CNEWTON_API void InterpolateMatrix (dFloat param, dFloat* const matrix) const;

	private:
	void SetTargetMatrixLow (const dFloat* const matrix);

	dVector m_posit0;
	dVector m_posit1;
	dQuaternion m_rotat0;
	dQuaternion m_rotat1;
	mutable unsigned m_lock;
};

#endif
