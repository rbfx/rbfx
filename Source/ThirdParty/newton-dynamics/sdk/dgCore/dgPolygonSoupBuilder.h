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

/****************************************************************************
*
*  Visual C++ 6.0 created by: Julio Jerez
*
****************************************************************************/
#ifndef __dgPolygonSoupDatabaseBuilder0x23413452233__
#define __dgPolygonSoupDatabaseBuilder0x23413452233__


#include "dgStdafx.h"
#include "dgRef.h"
#include "dgArray.h"
#include "dgIntersections.h"


class AdjacentdFace
{
	public:
	dgPlane m_normal;
	dgInt32 m_count;
	dgInt32 *m_index;
	dgInt64 m_edgeMap[256];
};

class dgPolygonSoupDatabaseBuilder 
{
	class dgFaceMap;
	class dgFaceInfo;
	class dgFaceBucket;
	class dgPolySoupFilterAllocator;
	public:

	dgPolygonSoupDatabaseBuilder (dgMemoryAllocator* const allocator);
	dgPolygonSoupDatabaseBuilder (const dgPolygonSoupDatabaseBuilder& sopurce);
	~dgPolygonSoupDatabaseBuilder ();

	DG_CLASS_ALLOCATOR(allocator)

	void Begin();
	void End(bool optimize);
	void AddMesh (const dgFloat32* const vertex, dgInt32 vertexCount, dgInt32 strideInBytes, dgInt32 faceCount, 
		          const dgInt32* const faceArray, const dgInt32* const indexArray, const dgInt32* const faceTagsData, const dgMatrix& worldMatrix); 

	private:
	void Optimize(dgInt32 faceId, const dgFaceBucket& faceBucket, const dgPolygonSoupDatabaseBuilder& source);

	void Finalize();
	void FinalizeAndOptimize();
	void OptimizeByIndividualFaces();
	dgInt32 FilterFace (dgInt32 count, dgInt32* const indexArray);
	dgInt32 AddConvexFace (dgInt32 count, dgInt32* const indexArray, dgInt32* const  facesArray);
	void PackArray();



	public:
	class dgVertexArray: public dgArray<dgBigVector>
	{	
		public:
		dgVertexArray(dgMemoryAllocator* const allocator)
			:dgArray<dgBigVector>(allocator)
		{
		}
	};

	class dgIndexArray: public dgArray<dgInt32>
	{
		public:
		dgIndexArray(dgMemoryAllocator* const allocator)
			:dgArray<dgInt32>(allocator)
		{
		}
	};

	dgInt32 m_run;
	dgInt32 m_faceCount;
	dgInt32 m_indexCount;
	dgInt32 m_vertexCount;
	dgInt32 m_normalCount;
	dgIndexArray m_faceVertexCount;
	dgIndexArray m_vertexIndex;
	dgIndexArray m_normalIndex;
	dgVertexArray m_vertexPoints;
	dgVertexArray m_normalPoints;
	dgMemoryAllocator* m_allocator;

};





#endif

