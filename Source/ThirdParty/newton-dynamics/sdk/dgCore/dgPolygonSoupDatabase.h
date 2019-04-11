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
#ifndef __dgPolygonSoupDatabase_H_
#define __dgPolygonSoupDatabase_H_


#include "dgStdafx.h"
#include "dgRef.h"
#include "dgArray.h"
#include "dgIntersections.h"

class dgMatrix;



class dgPolygonSoupDatabase
{
	public:
	dgFloat32 GetRadius() const;
	dgInt32 GetVertexCount() const;
	dgInt32 GetStrideInBytes() const;
	dgFloat32* GetLocalVertexPool() const;

	dgUnsigned32 GetTagId(const dgInt32* const face, dgInt32 indexCount) const;
	void SetTagId(const dgInt32* const face, dgInt32 indexCount, dgUnsigned32 newID) const;

	virtual void Serialize (dgSerialize callback, void* const userData) const = 0;
	virtual void Deserialize (dgDeserialize callback, void* const userData, dgInt32 revisionNumber) = 0;
		
	protected:
	dgPolygonSoupDatabase(const char* const name = NULL);
	virtual ~dgPolygonSoupDatabase ();

	dgInt32 m_vertexCount;
	dgInt32 m_strideInBytes;
	dgFloat32* m_localVertex;
};



#endif

