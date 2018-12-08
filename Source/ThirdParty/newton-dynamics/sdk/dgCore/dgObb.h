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

#ifndef __dgOOBB_H__
#define __dgOOBB_H__

#include "dgStdafx.h"
#include "dgTypes.h"
#include "dgVector.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"


class dgPlane;

DG_MSC_VECTOR_ALIGMENT
class dgObb: public dgMatrix
{
	public:
	DG_INLINE dgObb (){};
	dgObb (const dgQuaternion &quat, const dgVector &position, const dgVector& dim = dgVector(0.0f));
	dgObb (const dgMatrix& matrix, const dgVector& dim = dgVector(0.0f));

	dgObb &operator= (const dgMatrix &arg);
	void Scale (dgFloat32 Ws, dgFloat32 Hs, dgFloat32 Bs) ;
	void SetDimensions (dgFloat32 W, dgFloat32 H, dgFloat32 B);
	void SetDimensions (const dgFloat32 vertex[], dgInt32 strideInBytes, dgInt32 vertexCount, const dgMatrix *basis = NULL);
	void SetDimensions (const dgFloat32 vertex[], dgInt32 strideInBytes, const dgInt32 triangles[], dgInt32 indexCount, const dgMatrix *basis);
//	void SetDimensions (const dgFloat32 vertex[], dgInt32 strideInBytes, const dgInt32 index[], dgInt32 indexCount, const dgMatrix *basis = NULL);

	// return:  0 if the sphere is wholly inside the view port
	//          1 if the sphere is partially inside the view port
	//         -1 if the sphere is wholly outside the view port
//	dgInt32 VisibilityTest (const dgCamera* camera) const;
//	dgInt32 VisibilityTest (const dgCamera* camera, const dgMatrix &worldMatrix) const; 
//	void Render (const dgCamera* camera, const dgMatrix &transform, unsigned rgb) const;

	private:
/*
 	typedef dgInt32 (dgSphere::*CachedVisibilityTest) (const dgMatrix &point, const dgPlane* plane) const;

	mutable CachedVisibilityTest planeTest;
	static CachedVisibilityTest planeTestArray[6];

	void ChangeCachedVisibilityTest (CachedVisibilityTest	fnt);
	dgInt32 FrontTest  (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 RearTest   (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 LeftTest   (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 RightTest  (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 TopTest    (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 BottomTest (const dgMatrix &point, const dgPlane* plane) const;
	dgInt32 VisibilityTestLow (const dgCamera* camera, const dgMatrix& viewMNatrix) const;
*/

	public:
	dgVector m_size;
} DG_GCC_VECTOR_ALIGMENT; 


inline dgObb::dgObb (const dgQuaternion &quat, const dgVector &position, const dgVector& dim)
	:dgMatrix(quat, position)
{
	SetDimensions (dim.m_x, dim.m_y, dim.m_z);
	dgAssert (0);
}

inline dgObb::dgObb(const dgMatrix& matrix, const dgVector& dim)
	:dgMatrix(matrix)
{
	SetDimensions (dim.m_x, dim.m_y, dim.m_z);
}




inline dgObb &dgObb::operator= (const dgMatrix &arg)
{
	m_front = arg.m_front;
	m_up = arg.m_up;
	m_right = arg.m_right;
	m_posit = arg.m_posit;
	return *this;
}

inline void dgObb::SetDimensions (dgFloat32 W, dgFloat32 H, dgFloat32 B)
{
	m_size = dgVector (dgAbs(W), dgAbs(H), dgAbs(B), dgSqrt (W * W + H * H + B * B));
}

inline void dgObb::Scale (dgFloat32 Ws, dgFloat32 Hs, dgFloat32 Bs) 
{
	SetDimensions (m_size.m_x * Ws, m_size.m_y * Hs, m_size.m_z * Bs);
}


#endif

