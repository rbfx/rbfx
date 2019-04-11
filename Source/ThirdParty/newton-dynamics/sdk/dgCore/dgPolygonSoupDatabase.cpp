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

#include "dgStdafx.h"
#include "dgPolygonSoupDatabase.h"


dgPolygonSoupDatabase::dgPolygonSoupDatabase(const char* const name)
{
	m_vertexCount = 0;
	m_strideInBytes = 0;
	m_localVertex = NULL;
}

dgPolygonSoupDatabase::~dgPolygonSoupDatabase ()
{
	if (m_localVertex) {
		dgFreeStack (m_localVertex);
	}
}


dgUnsigned32 dgPolygonSoupDatabase::GetTagId(const dgInt32* const face, dgInt32 indexCount) const
{
	return dgUnsigned32 (face[indexCount]);
}

void dgPolygonSoupDatabase::SetTagId(const dgInt32* const facePtr, dgInt32 indexCount, dgUnsigned32 newID) const
{
	dgUnsigned32* const face = (dgUnsigned32*) facePtr;
	face[indexCount] = newID;
}

dgInt32 dgPolygonSoupDatabase::GetVertexCount()	const
{
	return m_vertexCount;
}

dgFloat32* dgPolygonSoupDatabase::GetLocalVertexPool() const
{
	return m_localVertex;
}

dgInt32 dgPolygonSoupDatabase::GetStrideInBytes() const
{
	return m_strideInBytes;
}

dgFloat32 dgPolygonSoupDatabase::GetRadius() const
{
	return 0.0f;
}



