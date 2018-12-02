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

#ifndef __DG_DEALUNAY_TETRAHEDRALIZAION_4D__
#define __DG_DEALUNAY_TETRAHEDRALIZAION_4D__

#include "dgStdafx.h"
#include "dgConvexHull4d.h"

class dgDelaunayTetrahedralization: public dgConvexHull4d
{
	public:
	dgDelaunayTetrahedralization(dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 count, dgInt32 strideInByte, dgFloat64 distTol);
	virtual ~dgDelaunayTetrahedralization();
	void RemoveUpperHull ();

	dgInt32 AddVertex (const dgBigVector& vertex);

	protected:
	virtual void DeleteFace (dgListNode* const node) ;

	void SortVertexArray ();
	static dgInt32 CompareVertexByIndex(const dgConvexHull4dVector* const  A, const dgConvexHull4dVector* const B, void* const context);

};

#endif
