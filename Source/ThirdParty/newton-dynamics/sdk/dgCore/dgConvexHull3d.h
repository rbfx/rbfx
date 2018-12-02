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

#ifndef __DG_CONVEXHULL_3D__
#define __DG_CONVEXHULL_3D__

#include "dgStdafx.h"
#include "dgList.h"
#include "dgArray.h"
#include "dgPlane.h"
#include "dgVector.h"
#include "dgMatrix.h"
#include "dgQuaternion.h"

#define DG_OLD_CONVEXHULL_3D

class dgMemoryAllocator;
class dgConvexHull3DVertex;
class dgConvexHull3dAABBTreeNode;

class dgConvexHull3DFace
{
	public:
	dgConvexHull3DFace();
	
	void SetMark(dgInt32 mark) {m_mark = mark;}
	dgInt32 GetMark() const {return m_mark;}
	dgList<dgConvexHull3DFace>::dgListNode* GetTwin(dgInt32 index) const { return m_twin[index];}

	private:
	dgFloat64 Evalue (const dgBigVector* const pointArray, const dgBigVector& point) const;
	dgBigPlane GetPlaneEquation (const dgBigVector* const pointArray) const;

	public:
	dgInt32 m_index[3]; 
	private:
	dgInt32 m_mark;
	dgList<dgConvexHull3DFace>::dgListNode* m_twin[3];
	friend class dgConvexHull3d;
};

class dgConvexHull3d: public dgList<dgConvexHull3DFace>
{
#ifdef	DG_OLD_CONVEXHULL_3D
	class dgNormalMap;
#endif

	public:
	dgConvexHull3d(const dgConvexHull3d& source);
	dgConvexHull3d(dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount = 0x7fffffff);
	virtual ~dgConvexHull3d();

	dgInt32 GetVertexCount() const;
	const dgBigVector* GetVertexPool() const;
	const dgBigVector& GetVertex(dgInt32 i) const;

	dgFloat64 GetDiagonal() const;
	void GetAABB (dgBigVector& boxP0, dgBigVector& boxP1) const;
	dgFloat64 RayCast (const dgBigVector& localP0, const dgBigVector& localP1) const;
	void CalculateVolumeAndSurfaceArea (dgFloat64& volume, dgFloat64& surcafeArea) const;

	protected:
	dgConvexHull3d(dgMemoryAllocator* const allocator);
	void BuildHull (const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount);

	virtual dgListNode* AddFace (dgInt32 i0, dgInt32 i1, dgInt32 i2);
	virtual void DeleteFace (dgListNode* const node) ;
	virtual dgInt32 InitVertexArray(dgConvexHull3DVertex* const points, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, void* const memoryPool, dgInt32 maxMemSize);

	bool CheckFlatSurface(dgConvexHull3dAABBTreeNode* vertexTree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount);
	void CalculateConvexHull2d (dgConvexHull3dAABBTreeNode* vertexTree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount);
	void CalculateConvexHull3d (dgConvexHull3dAABBTreeNode* vertexTree, dgConvexHull3DVertex* const points, dgInt32 count, dgFloat64 distTol, dgInt32 maxVertexCount);
	
	dgInt32 SupportVertex (dgConvexHull3dAABBTreeNode** const tree, const dgConvexHull3DVertex* const points, const dgBigVector& dir, const bool removeEntry = true) const;
	dgFloat64 TetrahedrumVolume (const dgBigVector& p0, const dgBigVector& p1, const dgBigVector& p2, const dgBigVector& p3) const;

	dgInt32 GetUniquePoints(dgConvexHull3DVertex* const points, const dgFloat64* const vertexCloud, dgInt32 strideInBytes, dgInt32 count, void* const memoryPool, dgInt32 maxMemSize);
	dgConvexHull3dAABBTreeNode* BuildTree (dgConvexHull3dAABBTreeNode* const parent, dgConvexHull3DVertex* const points, dgInt32 count, dgInt32 baseIndex, dgInt8** const memoryPool, dgInt32& maxMemSize) const;
	static dgInt32 ConvexCompareVertex(const dgConvexHull3DVertex* const A, const dgConvexHull3DVertex* const B, void* const context);
	bool Sanity() const;
	void Save (const char* const filename) const;

	dgInt32 m_count;
	dgFloat64 m_diag;
	dgBigVector m_aabbP0;
	dgBigVector m_aabbP1;
	dgArray<dgBigVector> m_points;
};


inline dgInt32 dgConvexHull3d::GetVertexCount() const
{
	return m_count;
}

inline const dgBigVector* dgConvexHull3d::GetVertexPool() const
{
	return &m_points[0];
}

inline const dgBigVector& dgConvexHull3d::GetVertex(dgInt32 index) const
{
	return m_points[index];
}

inline dgFloat64 dgConvexHull3d::GetDiagonal() const
{
	return m_diag;
}


inline void dgConvexHull3d::GetAABB (dgBigVector& boxP0, dgBigVector& boxP1) const
{
	boxP0 = m_aabbP0;
	boxP1 = m_aabbP1;
}

#endif
