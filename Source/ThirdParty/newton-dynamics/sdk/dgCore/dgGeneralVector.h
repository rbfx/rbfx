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

#ifndef __dgGeneralVector__
#define __dgGeneralVector__

#include "dgStdafx.h"
#include "dgDebug.h"
#include "dgMemory.h"

template <class T>
DG_INLINE T dgSQRH(const T num, const T den)
{
	T r(num / den);
	return T(sqrt(T(1.0f) + r * r));
}

template <class T>
DG_INLINE T dgPythag(const T a, const T b)
{
	T absa(dgAbs(a));
	T absb(dgAbs(b));
	return (absa > absb) ? (absa * dgSQRH(absb, absa)) : ((absb == T(0.0f) ? T(0.0f) : (absb * dgSQRH(absa, absb))));
}

template <class T>
DG_INLINE T dgSign(const T a, const T b)
{
	return (b >= T(0.0f)) ? (a >= T(0.0f) ? a : -a) : (a >= T(0.0f) ? -a : a);
}


// return dot product
template<class T>
DG_INLINE T dgDotProduct(dgInt32 size, const T* const A, const T* const B)
{
	T val(0.0f);
	for (dgInt32 i = 0; i < size; i++) {
		val = val + A[i] * B[i];
	}
	return val;
}



#endif


