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

#include "dgPhysicsStdafx.h"
#include "dgBody.h"
#include "dgWorld.h"
#include "dgCollisionHeightField.h"


#define DG_HIGHTFIELD_DATA_ID 0x45AF5E07

dgVector dgCollisionHeightField::m_yMask (0xffffffff, 0, 0xffffffff, 0);
dgVector dgCollisionHeightField::m_padding (dgFloat32 (0.25f), dgFloat32 (0.25f), dgFloat32 (0.25f), dgFloat32 (0.0f));
dgVector dgCollisionHeightField::m_elevationPadding (dgFloat32 (0.0f), dgFloat32 (1.0e10f), dgFloat32 (0.0f), dgFloat32 (0.0f));

dgInt32 dgCollisionHeightField::m_cellIndices[][4] =
{
	{0, 1, 2, 3},
	{1, 3, 0, 2}
};

dgInt32 dgCollisionHeightField::m_verticalEdgeMap[][7] = 
{
	{1 * 9 + 1, 0 * 9 + 0, 1 * 9 + 4, 1 * 9 + 6, 0 * 9 + 6, 1 * 9 + 4, 0 * 9 + 4},
	{1 * 9 + 2, 0 * 9 + 1, 1 * 9 + 4, 1 * 9 + 6, 0 * 9 + 7, 1 * 9 + 4, 0 * 9 + 4},
	{1 * 9 + 2, 0 * 9 + 0, 1 * 9 + 4, 1 * 9 + 7, 0 * 9 + 6, 1 * 9 + 4, 0 * 9 + 4},
	{1 * 9 + 0, 0 * 9 + 1, 1 * 9 + 4, 1 * 9 + 7, 0 * 9 + 7, 1 * 9 + 4, 0 * 9 + 4}
};

dgInt32 dgCollisionHeightField::m_horizontalEdgeMap[][7] =
{
	{1 * 9 + 0, 2 * 9 + 1, 1 * 9 + 4, 1 * 9 + 7, 2 * 9 + 7, 1 * 9 + 4, 2 * 9 + 4},
	{1 * 9 + 0, 3 * 9 + 0, 1 * 9 + 4, 1 * 9 + 7, 3 * 9 + 6, 1 * 9 + 4, 3 * 9 + 4},
	{0 * 9 + 1, 2 * 9 + 1, 0 * 9 + 4, 1 * 9 + 7, 2 * 9 + 7, 0 * 9 + 4, 2 * 9 + 4},
	{0 * 9 + 1, 3 * 9 + 0, 0 * 9 + 4, 0 * 9 + 6, 3 * 9 + 6, 0 * 9 + 4, 3 * 9 + 4}
};


dgCollisionHeightField::dgCollisionHeightField(
	dgWorld* const world, dgInt32 width, dgInt32 height, dgInt32 contructionMode, 
	const void* const elevationMap, dgElevationType elevationDataType, dgFloat32 verticalScale, 
	const dgInt8* const atributeMap, dgFloat32 horizontalScale_x, dgFloat32 horizontalScale_z)
	:dgCollisionMesh (world, m_heightField)
	,m_width(width)
	,m_height(height)
	,m_diagonalMode (dgCollisionHeightFieldGridConstruction  (dgClamp (contructionMode, dgInt32 (m_normalDiagonals), dgInt32 (m_starInvertexDiagonals))))
	,m_horizontalDisplacement(NULL)
	,m_verticalScale(verticalScale)
	,m_horizontalScale_x(horizontalScale_x)
	,m_horizontalScaleInv_x (dgFloat32 (1.0f) / m_horizontalScale_x)
	,m_horizontalDisplacementScale_x(dgFloat32 (1.0f))
	,m_horizontalScale_z(horizontalScale_z)
	,m_horizontalScaleInv_z(dgFloat32(1.0f) / m_horizontalScale_z)
	,m_horizontalDisplacementScale_z(dgFloat32(1.0f))
	,m_userRayCastCallback(NULL)
	,m_elevationDataType(elevationDataType)
{
	m_rtti |= dgCollisionHeightField_RTTI;

	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			m_elevationMap = dgMallocStack(m_width * m_height * sizeof (dgFloat32));
			memcpy (m_elevationMap, elevationMap, m_width * m_height * sizeof (dgFloat32));
			break;
		}

		case m_unsigned16Bit:
		{
			m_elevationMap = dgMallocStack(m_width * m_height * sizeof (dgUnsigned16));
			memcpy (m_elevationMap, elevationMap, m_width * m_height * sizeof (dgUnsigned16));
		}
	}

	dgInt32 attibutePaddedMapSize = (m_width * m_height + 4) & -4; 
	m_atributeMap = (dgInt8 *)dgMallocStack(attibutePaddedMapSize * sizeof (dgInt8));
	m_diagonals = (dgInt8 *)dgMallocStack(attibutePaddedMapSize * sizeof (dgInt8));

	switch (m_diagonalMode)
	{
		case m_normalDiagonals:
		{
			memset (m_diagonals, 0, m_width * m_height * sizeof (dgInt8));
			break;
		}
		case m_invertedDiagonals:
		{
			memset (m_diagonals, 1, m_width * m_height * sizeof (dgInt8));
			break;
		}

		case m_alternateOddRowsDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i ++) {
					m_diagonals[index + i] = 0;
				}
			}

			for (dgInt32 j = 1; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i ++) {
					m_diagonals[index + i] = 1;
				}
			}
			break;
		}

		case m_alternateEvenRowsDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i ++) {
					m_diagonals[index + i] = 1;
				}
			}

			for (dgInt32 j = 1; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i ++) {
					m_diagonals[index + i] = 0;
				}
			}
			break;
		}


		case m_alternateOddColumsDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j ++) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}

				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}
			}
			break;
		}

		case m_alternateEvenColumsDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j ++) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}

				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}
			}
			break;
		}

		case m_starDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}
				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}
			}

			for (dgInt32 j = 1; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}
				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}
			}
			break;
		}

		case m_starInvertexDiagonals:
		{
			for (dgInt32 j = 0; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}
				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}
			}

			for (dgInt32 j = 1; j < m_height; j += 2) {
				dgInt32 index = j * m_width;
				for (dgInt32 i = 0; i < m_width; i += 2) {
					m_diagonals[index + i] = 0;
				}
				for (dgInt32 i = 1; i < m_width; i += 2) {
					m_diagonals[index + i] = 1;
				}
			}

			break;
		}

		default:
			dgAssert (0);
		
	}
	memcpy (m_atributeMap, atributeMap, m_width * m_height * sizeof (dgInt8));

	dgTree<void*, unsigned>::dgTreeNode* nodeData = world->m_perInstanceData.Find(DG_HIGHTFIELD_DATA_ID);
	if (!nodeData) {
		m_instanceData = (dgPerIntanceData*) new dgPerIntanceData();
		m_instanceData->m_refCount = 0;
		m_instanceData->m_world = world;
		for (dgInt32 i = 0 ; i < DG_MAX_THREADS_HIVE_COUNT; i ++) {
			m_instanceData->m_vertex[i] = NULL;
			m_instanceData->m_vertexCount[i] = 0;
			m_instanceData->m_vertex[i].SetAllocator(world->GetAllocator());
			AllocateVertex(world, i);
		}
		nodeData = world->m_perInstanceData.Insert (m_instanceData, DG_HIGHTFIELD_DATA_ID);
	}
	m_instanceData = (dgPerIntanceData*) nodeData->GetInfo();

	m_instanceData->m_refCount ++;

	CalculateAABB();
	SetCollisionBBox(m_minBox, m_maxBox);
}

dgCollisionHeightField::dgCollisionHeightField (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revisionNumber)
	:dgCollisionMesh (world, deserialization, userData, revisionNumber)
{
	dgAssert (m_rtti | dgCollisionHeightField_RTTI);
	
	dgInt32 elevationDataType;

	m_userRayCastCallback = NULL;
	m_horizontalDisplacement = NULL;
	deserialization (userData, &m_width, sizeof (dgInt32));
	deserialization (userData, &m_height, sizeof (dgInt32));
	deserialization (userData, &m_diagonalMode, sizeof (dgInt32));
	deserialization (userData, &elevationDataType, sizeof (dgInt32));
	deserialization (userData, &m_verticalScale, sizeof (dgFloat32));
	deserialization (userData, &m_horizontalScale_x, sizeof (dgFloat32));
	deserialization (userData, &m_horizontalDisplacementScale_x, sizeof (dgFloat32));
	deserialization (userData, &m_horizontalScale_z, sizeof (dgFloat32));
	deserialization (userData, &m_horizontalDisplacementScale_z, sizeof (dgFloat32));
	deserialization (userData, &m_minBox.m_x, sizeof (dgVector)); 
	deserialization (userData, &m_maxBox.m_x, sizeof (dgVector)); 

	m_elevationDataType = dgElevationType (elevationDataType);

	dgInt32 attibutePaddedMapSize = (m_width * m_height + 4) & -4; 
	m_atributeMap = (dgInt8 *)dgMallocStack(attibutePaddedMapSize * sizeof (dgInt8));
	m_diagonals = (dgInt8 *)dgMallocStack(attibutePaddedMapSize * sizeof (dgInt8));

	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			m_elevationMap = dgMallocStack(m_width * m_height * sizeof (dgFloat32));
			deserialization (userData, m_elevationMap, m_width * m_height * sizeof (dgFloat32));
			break;
		}

		case m_unsigned16Bit:
		{
			m_elevationMap = dgMallocStack(m_width * m_height * sizeof (dgUnsigned16));
			deserialization (userData, m_elevationMap, m_width * m_height * sizeof (dgUnsigned16));
			break;
		}
	}
	deserialization (userData, m_atributeMap, attibutePaddedMapSize * sizeof (dgInt8));
	deserialization (userData, m_diagonals, attibutePaddedMapSize * sizeof (dgInt8));

	dgInt32 hasDisplacement = m_horizontalDisplacement ? 1 : 0;
	deserialization (userData, &hasDisplacement, sizeof (hasDisplacement));
	if (hasDisplacement) {
		m_horizontalDisplacement = (dgUnsigned16*) dgMallocStack(m_width * m_height * sizeof (dgUnsigned16));
		deserialization (userData, m_horizontalDisplacement, m_width * m_height * sizeof (dgUnsigned16));
	}

	m_horizontalScaleInv_x = dgFloat32 (1.0f) / m_horizontalScale_x;
	m_horizontalScaleInv_z = dgFloat32 (1.0f) / m_horizontalScale_z;

	dgTree<void*, unsigned>::dgTreeNode* nodeData = world->m_perInstanceData.Find(DG_HIGHTFIELD_DATA_ID);
	if (!nodeData) {
		m_instanceData = (dgPerIntanceData*) new dgPerIntanceData();
		m_instanceData->m_refCount = 0;
		m_instanceData->m_world = world;
		for (dgInt32 i = 0; i < DG_MAX_THREADS_HIVE_COUNT; i++) {
			m_instanceData->m_vertex[i] = NULL;
			m_instanceData->m_vertexCount[i] = 0;
			m_instanceData->m_vertex[i].SetAllocator(world->GetAllocator());
			AllocateVertex(world, i);
		}
		nodeData = world->m_perInstanceData.Insert(m_instanceData, DG_HIGHTFIELD_DATA_ID);
	}
	m_instanceData = (dgPerIntanceData*)nodeData->GetInfo();

	m_instanceData->m_refCount ++;
	SetCollisionBBox(m_minBox, m_maxBox);
}

dgCollisionHeightField::~dgCollisionHeightField(void)
{
	m_instanceData->m_refCount --;
	if (!m_instanceData->m_refCount) {
		dgWorld* const world = m_instanceData->m_world;
		delete m_instanceData;
		world->m_perInstanceData.Remove(DG_HIGHTFIELD_DATA_ID);
	}
	dgFreeStack(m_elevationMap);
	dgFreeStack(m_atributeMap);
	dgFreeStack(m_diagonals);

	if (m_horizontalDisplacement) {
		dgFreeStack(m_horizontalDisplacement);
	}
}

void dgCollisionHeightField::Serialize(dgSerialize callback, void* const userData) const
{
	SerializeLow(callback, userData);

	dgInt32 elevationDataType = m_elevationDataType;

	callback (userData, &m_width, sizeof (dgInt32));
	callback (userData, &m_height, sizeof (dgInt32));
	callback (userData, &m_diagonalMode, sizeof (dgInt32));
	callback (userData, &elevationDataType, sizeof (dgInt32));
	callback (userData, &m_verticalScale, sizeof (dgFloat32));
	callback (userData, &m_horizontalScale_x, sizeof (dgFloat32));
	callback (userData, &m_horizontalDisplacementScale_x, sizeof (dgFloat32));
	callback (userData, &m_horizontalScale_z, sizeof (dgFloat32));
	callback (userData, &m_horizontalDisplacementScale_z, sizeof (dgFloat32));
	callback (userData, &m_minBox.m_x, sizeof (dgVector)); 
	callback (userData, &m_maxBox.m_x, sizeof (dgVector)); 

	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			callback (userData, m_elevationMap, m_width * m_height * sizeof (dgFloat32));
			break;
		}
		case m_unsigned16Bit:
		{
			callback (userData, m_elevationMap, m_width * m_height * sizeof (dgUnsigned16));
			break;
		}
	}

	dgInt32 attibutePaddedMapSize = (m_width * m_height + 4) & -4; 
	callback (userData, m_atributeMap, attibutePaddedMapSize * sizeof (dgInt8));
	callback (userData, m_diagonals, attibutePaddedMapSize * sizeof (dgInt8));
	
	dgInt32 hasDisplacement = m_horizontalDisplacement ? 1 : 0;
	callback (userData, &hasDisplacement, sizeof (hasDisplacement));
	if (hasDisplacement) {
		callback (userData, m_horizontalDisplacement, m_width * m_height * sizeof (dgUnsigned16));
	}
}

void dgCollisionHeightField::SetCollisionRayCastCallback (dgCollisionHeightFieldRayCastCallback rayCastCallback)
{
	m_userRayCastCallback = rayCastCallback;
}

void dgCollisionHeightField::SetHorizontalDisplacement (const dgUnsigned16* const displacemnet, dgFloat32 scale)
{
	if (m_horizontalDisplacement) {
		dgFreeStack(m_horizontalDisplacement);
		m_horizontalDisplacement = NULL;
	}

	m_horizontalDisplacementScale_x = scale;
	if (displacemnet) {
		m_horizontalDisplacement = (dgUnsigned16*)dgMallocStack(m_width * m_height * sizeof (dgUnsigned16));
		memcpy (m_horizontalDisplacement, displacemnet, m_width * m_height * sizeof (dgUnsigned16));
	}
}

void dgCollisionHeightField::AllocateVertex(dgWorld* const world, dgInt32 threadIndex) const
{
	m_instanceData->m_vertex[threadIndex].Resize (m_instanceData->m_vertex[threadIndex].GetElementsCapacity() * 2);
	m_instanceData->m_vertexCount[threadIndex] = m_instanceData->m_vertex[threadIndex].GetElementsCapacity();
}



DG_INLINE void dgCollisionHeightField::CalculateMinExtend2d(const dgVector& p0, const dgVector& p1, dgVector& boxP0, dgVector& boxP1) const
{
	dgVector scale (m_horizontalScale_x, dgFloat32 (0.0f), m_horizontalScale_z, dgFloat32 (0.0f));
	dgVector q0(p0.GetMin(p1) - m_padding);
	dgVector q1(p0.GetMax(p1) + scale + m_padding);

	//dgVector elevationPadding (dgVector(dgFloat32(1.0e10f)).AndNot(m_yMask));
	dgVector invScale (m_horizontalScaleInv_x, dgFloat32 (0.0f), m_horizontalScaleInv_z, dgFloat32 (0.0f));
	boxP0 = (((q0 * invScale).Floor() * scale)         & m_yMask) - m_elevationPadding;
	boxP1 = (((q1 * invScale).Floor() * scale + scale) & m_yMask) + m_elevationPadding;
	dgAssert (boxP0.m_w == dgFloat32 (0.0f));
	dgAssert (boxP1.m_w == dgFloat32 (0.0f));

	dgVector minBox((m_minBox & m_yMask) + boxP0.AndNot(m_yMask));
	dgVector maxBox((m_maxBox & m_yMask) + boxP1.AndNot(m_yMask));
	boxP0 = boxP0.GetMax(minBox);
	boxP1 = boxP1.GetMin(maxBox);
}

DG_INLINE void dgCollisionHeightField::CalculateMinExtend3d(const dgVector& p0, const dgVector& p1, dgVector& boxP0, dgVector& boxP1) const
{
	dgAssert(p0.m_x <= p1.m_x);
	dgAssert(p0.m_y <= p1.m_y);
	dgAssert(p0.m_z <= p1.m_z);
	dgAssert(p0.m_w == dgFloat32(0.0f));
	dgAssert(p1.m_w == dgFloat32(0.0f));

	dgVector scale (m_horizontalScale_x, dgFloat32 (0.0f), m_horizontalScale_z, dgFloat32 (0.0f));
	dgVector q0(p0.GetMin(p1) - m_padding);
	dgVector q1(p0.GetMax(p1) + scale + m_padding);

	dgVector invScale(m_horizontalScaleInv_x, dgFloat32(0.0f), m_horizontalScaleInv_z, dgFloat32(0.0f));
	boxP0 = (((q0 * invScale).Floor() * scale        ) & m_yMask) + q0.AndNot(m_yMask);
	boxP1 = (((q1 * invScale).Floor() * scale + scale) & m_yMask) + q1.AndNot(m_yMask);

	if (m_horizontalDisplacement) {
		boxP0 -= dgVector(m_horizontalScale_x, dgFloat32(0.0f), m_horizontalScale_z, dgFloat32(0.0f));
		boxP1 += dgVector(m_horizontalScale_x, dgFloat32(0.0f), m_horizontalScale_z, dgFloat32(0.0f));
	}

	dgVector minBox((m_minBox & m_yMask) + boxP0.AndNot(m_yMask));
	dgVector maxBox((m_maxBox & m_yMask) + boxP1.AndNot(m_yMask));
	boxP0 = boxP0.GetMax(minBox);
	boxP1 = boxP1.GetMin(maxBox);
}

void dgCollisionHeightField::CalculateAABB()
{
	dgFloat32 y0 = dgFloat32 (dgFloat32 (1.0e10f));
	dgFloat32 y1 = dgFloat32 (-dgFloat32 (1.0e10f));
	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
			for (dgInt32 i = 0; i < m_width * m_height; i ++) {
				y0 = dgMin(y0, elevation[i]);
				y1 = dgMax(y1, elevation[i]);
			}
			break;
		}

		case m_unsigned16Bit:
		{
			const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
			for (dgInt32 i = 0; i < m_width * m_height; i ++) {
				y0 = dgMin(y0, dgFloat32 (elevation[i]));
				y1 = dgMax(y1, dgFloat32 (elevation[i]));
			}
		}
	}

	m_minBox = dgVector (dgFloat32 (dgFloat32 (0.0f)),                  y0 * m_verticalScale, dgFloat32 (dgFloat32 (0.0f)),               dgFloat32 (0.0f)); 
	m_maxBox = dgVector (dgFloat32 (m_width - 1) * m_horizontalScale_x, y1 * m_verticalScale, dgFloat32 (m_height-1) * m_horizontalScale_z, dgFloat32 (0.0f)); 
}

void dgCollisionHeightField::GetCollisionInfo(dgCollisionInfo* const info) const
{
	dgCollision::GetCollisionInfo(info);

	dgCollisionInfo::dgHeightMapCollisionData& data = info->m_heightFieldCollision;
	data.m_width = m_width;
	data.m_height = m_height;
	data.m_gridsDiagonals = m_diagonalMode;
	data.m_elevationDataType = m_elevationDataType;
	data.m_horizotalDisplacement = m_horizontalDisplacement;
	data.m_verticalScale = m_verticalScale;
	data.m_horizonalScale_x = m_horizontalScale_x;
	data.m_horizonalScale_z = m_horizontalScale_z;
	data.m_horizonalDisplacementScale_x = m_horizontalDisplacementScale_x;
	data.m_horizonalDisplacementScale_z = m_horizontalDisplacementScale_z;
	data.m_atributes = m_atributeMap;
	data.m_elevation = m_elevationMap;
}

dgFloat32 dgCollisionHeightField::RayCastCell (const dgFastRayTest& ray, dgInt32 xIndex0, dgInt32 zIndex0, dgVector& normalOut, dgFloat32 maxT) const
{
	dgVector points[4];
	dgInt32 triangle[3];

	// get the 3d point at the corner of the cell
	if ((xIndex0 < 0) || (zIndex0 < 0) || (xIndex0 >= (m_width - 1)) || (zIndex0 >= (m_height - 1))) {
		return dgFloat32 (1.2f);
	}
	
	dgAssert (maxT <= 1.0);

	dgInt32 base = zIndex0 * m_width + xIndex0;
	
	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
			points[0 * 2 + 0] = dgVector ((xIndex0 + 0) * m_horizontalScale_x, m_verticalScale * elevation[base],			      (zIndex0 + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[0 * 2 + 1] = dgVector ((xIndex0 + 1) * m_horizontalScale_x, m_verticalScale * elevation[base + 1],           (zIndex0 + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[1 * 2 + 1] = dgVector ((xIndex0 + 1) * m_horizontalScale_x, m_verticalScale * elevation[base + m_width + 1], (zIndex0 + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[1 * 2 + 0] = dgVector ((xIndex0 + 0) * m_horizontalScale_x, m_verticalScale * elevation[base + m_width + 0], (zIndex0 + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
			break;
		}

		case m_unsigned16Bit:
		default:
		{
			const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
			points[0 * 2 + 0] = dgVector ((xIndex0 + 0) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base]),			   (zIndex0 + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[0 * 2 + 1] = dgVector ((xIndex0 + 1) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + 1]),           (zIndex0 + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[1 * 2 + 1] = dgVector ((xIndex0 + 1) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + m_width + 1]), (zIndex0 + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
			points[1 * 2 + 0] = dgVector ((xIndex0 + 0) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + m_width + 0]), (zIndex0 + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
			break;
		}
	}
	
	dgFloat32 t = dgFloat32 (1.2f);
	if (!m_diagonals[base]) {
		triangle[0] = 1;
		triangle[1] = 2;
		triangle[2] = 3;

		dgVector e10 (points[2] - points[1]);
		dgVector e20 (points[3] - points[1]);
		dgVector normal (e10.CrossProduct(e20));
		normal = normal.Normalize();
		t = ray.PolygonIntersect (normal, maxT, &points[0].m_x, sizeof (dgVector), triangle, 3);
		if (t < maxT){
			normalOut = normal;
			return t;
		}

		triangle[0] = 1;
		triangle[1] = 0;
		triangle[2] = 2;

		dgVector e30 (points[0] - points[1]);
		normal = e30.CrossProduct(e10);
		normal = normal.Normalize();
		t = ray.PolygonIntersect (normal, maxT, &points[0].m_x, sizeof (dgVector), triangle, 3);
		if (t < maxT){
			normalOut = normal;
			return t;
		}

	} else {
		triangle[0] = 0;
		triangle[1] = 2;
		triangle[2] = 3;

		dgVector e10 (points[2] - points[0]);
		dgVector e20 (points[3] - points[0]);
		dgVector normal (e10.CrossProduct(e20));
		normal = normal.Normalize();
		t = ray.PolygonIntersect (normal, maxT, &points[0].m_x, sizeof (dgVector), triangle, 3);
		if (t < maxT){
			normalOut = normal;
			return t;
		}

		triangle[0] = 0;
		triangle[1] = 3;
		triangle[2] = 1;

		dgVector e30 (points[1] - points[0]);
		normal = e20.CrossProduct(e30);
		normal = normal.Normalize();
		t = ray.PolygonIntersect (normal, maxT, &points[0].m_x, sizeof (dgVector), triangle, 3);
		if (t < maxT){
			normalOut = normal;
			return t;
		}
	}
	return t;
}

dgFloat32 dgCollisionHeightField::RayCast (const dgVector& q0, const dgVector& q1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const
{
	dgVector boxP0;
	dgVector boxP1;

	// calculate the ray bounding box
	CalculateMinExtend2d (q0, q1, boxP0, boxP1);

	dgVector p0 (q0);
	dgVector p1 (q1);

	// clip the line against the bounding box
	if (dgRayBoxClip (p0, p1, boxP0, boxP1)) { 
		dgVector dp (p1 - p0);
		dgVector normalOut (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));

		dgFloat32 scale_x = m_horizontalScale_x;
		dgFloat32 invScale_x = m_horizontalScaleInv_x;
		dgFloat32 scale_z = m_horizontalScale_z;
		dgFloat32 invScale_z = m_horizontalScaleInv_z;
		dgInt32 ix0 = dgFastInt (p0.m_x * invScale_x);
		dgInt32 iz0 = dgFastInt (p0.m_z * invScale_z);

		// implement a 3ddda line algorithm 
		dgInt32 xInc;
		dgFloat32 tx;
		dgFloat32 stepX;
		if (dp.m_x > dgFloat32 (0.0f)) {
			xInc = 1;
			dgFloat32 val = dgFloat32 (1.0f) / dp.m_x;
			stepX = scale_x * val;
			tx = (scale_x * (ix0 + dgFloat32 (1.0f)) - p0.m_x) * val;
		} else if (dp.m_x < dgFloat32 (0.0f)) {
			xInc = -1;
			dgFloat32 val = -dgFloat32 (1.0f) / dp.m_x;
			stepX = scale_x * val;
			tx = -(scale_x * ix0 - p0.m_x) * val;
		} else {
			xInc = 0;
			stepX = dgFloat32 (0.0f);
			tx = dgFloat32 (1.0e10f);
		}

		dgInt32 zInc;
		dgFloat32 tz;
		dgFloat32 stepZ;
		if (dp.m_z > dgFloat32 (0.0f)) {
			zInc = 1;
			dgFloat32 val = dgFloat32 (1.0f) / dp.m_z;
			stepZ = scale_z * val;
			tz = (scale_z * (iz0 + dgFloat32 (1.0f)) - p0.m_z) * val;
		} else if (dp.m_z < dgFloat32 (0.0f)) {
			zInc = -1;
			dgFloat32 val = -dgFloat32 (1.0f) / dp.m_z;
			stepZ = scale_z * val;
			tz = -(scale_z * iz0 - p0.m_z) * val;
		} else {
			zInc = 0;
			stepZ = dgFloat32 (0.0f);
			tz = dgFloat32 (1.0e10f);
		}

		dgFloat32 txAcc = tx;
		dgFloat32 tzAcc = tz;
		dgInt32 xIndex0 = ix0;
		dgInt32 zIndex0 = iz0;
		dgFastRayTest ray (q0, q1); 

		// for each cell touched by the line
		do {
			dgFloat32 t = RayCastCell (ray, xIndex0, zIndex0, normalOut, maxT);
			if (t < maxT) {
				// bail out at the first intersection and copy the data into the descriptor
				dgAssert (normalOut.m_w == dgFloat32 (0.0f));
				contactOut.m_normal = normalOut.Normalize();
				contactOut.m_shapeId0 = m_atributeMap[zIndex0 * m_width + xIndex0];
				contactOut.m_shapeId1 = m_atributeMap[zIndex0 * m_width + xIndex0];

				if (m_userRayCastCallback) {
					dgVector normal (body->GetCollision()->GetGlobalMatrix().RotateVector (contactOut.m_normal));
					m_userRayCastCallback (body, this, t, xIndex0, zIndex0, &normal, dgInt32 (contactOut.m_shapeId0), userData);
				}

				return t;
			}

			if (txAcc < tzAcc) {
				xIndex0 += xInc;
				tx = txAcc;
				txAcc += stepX;
			} else {
				zIndex0 += zInc;
				tz = tzAcc;
				tzAcc += stepZ;
			}
		} while ((tx <= dgFloat32 (1.0f)) || (tz <= dgFloat32 (1.0f)));
	}

	// if no cell was hit, return a large value
	return dgFloat32 (1.2f);
}


void dgCollisionHeightField::GetVertexListIndexList (const dgVector& p0, const dgVector& p1, dgMeshVertexListIndexList &data) const
{
	dgAssert (0);
	data.m_vertexCount = 0;
}

struct dgCollisionHeightFieldShowPolyContext
{
	dgMatrix m_matrix;
	void* m_userData;
	dgCollision::OnDebugCollisionMeshCallback m_callback;
};

dgVector dgCollisionHeightField::SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const
{
	dgFloat32 maxProject (dgFloat32 (-1.e-20f));
	dgVector support (dgFloat32 (0.0f));
	if (m_elevationDataType == m_float32Bit)  {
		const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
		for (dgInt32 z = 0; z < m_height - 1; z ++) {
			dgInt32 base = z * m_width;
			dgFloat32 zVal = m_horizontalScale_z * z;
			for (dgInt32 x = 0; x < m_width; x ++) {
				dgVector p (m_horizontalScale_x * x, m_verticalScale * elevation[base + x], zVal, dgFloat32 (0.0f));
				dgFloat32 project = dir.DotProduct(p).m_x;
				if (project > maxProject) {
					maxProject = project;
					support = p;
				}
			}
		}

	} else {
		const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
		for (dgInt32 z = 0; z < m_height - 1; z ++) {
			dgInt32 base = z * m_width;
			dgFloat32 zVal = m_horizontalScale_z * z;
			for (dgInt32 x = 0; x < m_width; x ++) {
				dgVector p (m_horizontalScale_x * x, m_verticalScale * elevation[base + x], zVal, dgFloat32 (0.0f));
				dgFloat32 project = dir.DotProduct(p).m_x;
				if (project > maxProject) {
					maxProject = project;
					support = p;
				}
			}
		}
	}
	return support;
}

dgVector dgCollisionHeightField::SupportVertexSpecial (const dgVector& dir, dgFloat32 skinThickness, dgInt32* const vertexIndex) const
{
	dgAssert (0);
	return SupportVertex (dir, vertexIndex);
}

void dgCollisionHeightField::DebugCollision (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const
{
	dgVector points[4];

	dgInt32 base = 0;
	for (dgInt32 z = 0; z < m_height - 1; z ++) {
		switch (m_elevationDataType) 
		{
			case m_float32Bit:
			{
				const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
				points[0 * 2 + 0] = dgVector ((0 + 0) * m_horizontalScale_x, m_verticalScale * elevation[base + 0              ], (z + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
				points[1 * 2 + 0] = dgVector ((0 + 0) * m_horizontalScale_x, m_verticalScale * elevation[base + 0 + m_width + 0], (z + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
				break;
			}

			case m_unsigned16Bit:
			{
				const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
				points[0 * 2 + 0] = dgVector ((0 + 0) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + 0              ]), (z + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
				points[1 * 2 + 0] = dgVector ((0 + 0) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + 0 + m_width + 0]), (z + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
				break;
			}
		}

		if (m_horizontalDisplacement) {
			dgUnsigned16 val = m_horizontalDisplacement[base];
			dgInt8 hor_x = val & 0xff;
			dgInt8 hor_z = (val >> 8);
			points[0 * 2 + 0] += dgVector(hor_x * m_horizontalDisplacementScale_x, dgFloat32(0.0f), hor_z * m_horizontalDisplacementScale_z, dgFloat32(0.0f));

			val = m_horizontalDisplacement[base + m_width];
			hor_x = val & 0xff;
			hor_z = (val >> 8);
			points[1 * 2 + 0] += dgVector(hor_x * m_horizontalDisplacementScale_x, dgFloat32(0.0f), hor_z * m_horizontalDisplacementScale_z, dgFloat32(0.0f));
		}

		points[0 * 2 + 0] = matrix.TransformVector(points[0 * 2 + 0]);
		points[1 * 2 + 0] = matrix.TransformVector(points[1 * 2 + 0]);

		for (dgInt32 x = 0; x < m_width - 1; x ++) {
			dgTriplex triangle[3];
			switch (m_elevationDataType) 
			{
				case m_float32Bit:
				{
					const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
					points[0 * 2 + 1] = dgVector ((x + 1) * m_horizontalScale_x, m_verticalScale * elevation[base + x +           1], (z + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
					points[1 * 2 + 1] = dgVector ((x + 1) * m_horizontalScale_x, m_verticalScale * elevation[base + x + m_width + 1], (z + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
					break;
				}

				case m_unsigned16Bit:
				{
					const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
					points[0 * 2 + 1] = dgVector ((x + 1) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + x +           1]), (z + 0) * m_horizontalScale_z, dgFloat32 (0.0f));
					points[1 * 2 + 1] = dgVector ((x + 1) * m_horizontalScale_x, m_verticalScale * dgFloat32 (elevation[base + x + m_width + 1]), (z + 1) * m_horizontalScale_z, dgFloat32 (0.0f));
					break;
				}
			}

			if (m_horizontalDisplacement) {
				dgUnsigned16 val = m_horizontalDisplacement[base + x + 1];
				dgInt8 hor_x = val & 0xff;
				dgInt8 hor_z = (val >> 8);
				points[0 * 2 + 1] += dgVector(hor_x * m_horizontalDisplacementScale_x, dgFloat32(0.0f), hor_z * m_horizontalDisplacementScale_z, dgFloat32(0.0f));

				val = m_horizontalDisplacement[base + m_width + x + 1];
				hor_x = val & 0xff;
				hor_z = (val >> 8);
				points[1 * 2 + 1] += dgVector(hor_x * m_horizontalDisplacementScale_x, dgFloat32(0.0f), hor_z * m_horizontalDisplacementScale_z, dgFloat32(0.0f));
			}

			points[0 * 2 + 1] = matrix.TransformVector(points[0 * 2 + 1]);
			points[1 * 2 + 1] = matrix.TransformVector(points[1 * 2 + 1]);

			const dgInt32* const indirectIndex = &m_cellIndices[dgInt32 (m_diagonals[z * m_width + x])][0];

			dgInt32 i0 = indirectIndex[0];
			dgInt32 i1 = indirectIndex[1];
			dgInt32 i2 = indirectIndex[2];
			dgInt32 i3 = indirectIndex[3];

			triangle[0].m_x = points[i1].m_x;
			triangle[0].m_y = points[i1].m_y;
			triangle[0].m_z = points[i1].m_z;

			triangle[1].m_x = points[i0].m_x;
			triangle[1].m_y = points[i0].m_y;
			triangle[1].m_z = points[i0].m_z;

			triangle[2].m_x = points[i2].m_x;
			triangle[2].m_y = points[i2].m_y;
			triangle[2].m_z = points[i2].m_z;
			callback (userData, 3, &triangle[0].m_x, m_atributeMap[base]);

			triangle[0].m_x = points[i1].m_x;
			triangle[0].m_y = points[i1].m_y;
			triangle[0].m_z = points[i1].m_z;

			triangle[1].m_x = points[i2].m_x;
			triangle[1].m_y = points[i2].m_y;
			triangle[1].m_z = points[i2].m_z;

			triangle[2].m_x = points[i3].m_x;
			triangle[2].m_y = points[i3].m_y;
			triangle[2].m_z = points[i3].m_z;
			callback (userData, 3, &triangle[0].m_x, m_atributeMap[base]);

			points[0 * 2 + 0] = points[0 * 2 + 1];
			points[1 * 2 + 0] = points[1 * 2 + 1];
		}
		base += m_width;
	}
}

void dgCollisionHeightField::CalculateMinAndMaxElevation(dgInt32 x0, dgInt32 x1, dgInt32 z0, dgInt32 z1, const dgFloat32* const elevation, dgFloat32& minHeight, dgFloat32& maxHeight) const
{
	dgInt32 base = z0 * m_width;
	for (dgInt32 z = z0; z <= z1; z++) {
		for (dgInt32 x = x0; x <= x1; x++) {
			dgFloat32 high = elevation[base + x];
			minHeight = dgMin(high, minHeight);
			maxHeight = dgMax(high, maxHeight);
		}
		base += m_width;
	}
}

void dgCollisionHeightField::CalculateMinAndMaxElevation(dgInt32 x0, dgInt32 x1, dgInt32 z0, dgInt32 z1, const dgUnsigned16* const elevation, dgFloat32& minHeight, dgFloat32& maxHeight) const
{
	dgInt32 base = z0 * m_width;
	for (dgInt32 z = z0; z <= z1; z++) {
		for (dgInt32 x = x0; x <= x1; x++) {
			dgFloat32 high = dgFloat32 (elevation[base + x]);
			minHeight = dgMin(high, minHeight);
			maxHeight = dgMax(high, maxHeight);
		}
		base += m_width;
	}
}


void dgCollisionHeightField::GetLocalAABB (const dgVector& q0, const dgVector& q1, dgVector& boxP0, dgVector& boxP1) const
{
	// the user data is the pointer to the collision geometry
	CalculateMinExtend3d (q0, q1, boxP0, boxP1);

	dgVector p0 (boxP0.Scale(m_horizontalScaleInv_x).GetInt());
	dgVector p1 (boxP1.Scale(m_horizontalScaleInv_x).GetInt());

	dgAssert (p0.m_ix == dgFastInt (boxP0.m_x * m_horizontalScaleInv_x));
	dgAssert (p0.m_iz == dgFastInt (boxP0.m_z * m_horizontalScaleInv_x));
	dgAssert (p1.m_ix == dgFastInt (boxP1.m_x * m_horizontalScaleInv_x));
	dgAssert (p1.m_iz == dgFastInt (boxP1.m_z * m_horizontalScaleInv_x));

	dgInt32 x0 = dgInt32 (p0.m_ix);
	dgInt32 x1 = dgInt32 (p1.m_ix);
	dgInt32 z0 = dgInt32 (p0.m_iz);
	dgInt32 z1 = dgInt32 (p1.m_iz);

	dgFloat32 minHeight = dgFloat32 (1.0e10f);
	dgFloat32 maxHeight = dgFloat32 (-1.0e10f);
	//dgInt32 base = z0 * m_width;
	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			CalculateMinAndMaxElevation(x0, x1, z0, z1, (dgFloat32*)m_elevationMap, minHeight, maxHeight);
			break;
		}

		case m_unsigned16Bit:
		{
			CalculateMinAndMaxElevation(x0, x1, z0, z1, (dgUnsigned16*)m_elevationMap, minHeight, maxHeight);
			break;
		}
	}

	boxP0.m_y = m_verticalScale * minHeight;
	boxP1.m_y = m_verticalScale * maxHeight;
}

void dgCollisionHeightField::AddDisplacement (dgVector* const vertex, dgInt32 x0, dgInt32 x1, dgInt32 z0, dgInt32 z1) const
{
	const dgUnsigned16* const displacement = m_horizontalDisplacement;
	dgInt32 vertexIndex = 0;
	dgInt32 base = z0 * m_width;
	for (dgInt32 z = z0; z <= z1; z++) {
		for (dgInt32 x = x0; x <= x1; x++) {
			dgUnsigned16 val = displacement[base + x];
			dgInt8 hor_x = val & 0xff; 
			dgInt8 hor_z = (val >> 8); 
			vertex[vertexIndex] += dgVector(hor_x * m_horizontalDisplacementScale_x, dgFloat32 (0.0f), hor_z * m_horizontalDisplacementScale_x, dgFloat32(0.0f));
			vertexIndex++;
		}
		base += m_width;
	}
}

void dgCollisionHeightField::GetCollidingFaces (dgPolygonMeshDesc* const data) const
{
	dgVector boxP0;
	dgVector boxP1;

	dgWorld* const world = data->m_objBody->GetWorld();

	// the user data is the pointer to the collision geometry
	CalculateMinExtend3d (data->m_p0, data->m_p1, boxP0, boxP1);
	boxP0 += data->m_boxDistanceTravelInMeshSpace & (data->m_boxDistanceTravelInMeshSpace < dgVector (dgFloat32 (0.0f)));  
	boxP1 += data->m_boxDistanceTravelInMeshSpace & (data->m_boxDistanceTravelInMeshSpace > dgVector (dgFloat32 (0.0f)));  

	boxP0 = (boxP0.GetMax(dgVector (dgFloat32 (0.0f))) & m_yMask) + boxP0.AndNot(m_yMask);
	boxP1 = (boxP1.GetMax(dgVector (dgFloat32 (0.0f))) & m_yMask) + boxP1.AndNot(m_yMask);

	dgVector p0 (boxP0.Scale(m_horizontalScaleInv_x).GetInt());
	dgVector p1 (boxP1.Scale(m_horizontalScaleInv_x).GetInt());

	dgAssert (p0.m_ix == dgFastInt (boxP0.m_x * m_horizontalScaleInv_x));
	dgAssert (p0.m_iz == dgFastInt (boxP0.m_z * m_horizontalScaleInv_x));
	dgAssert (p1.m_ix == dgFastInt (boxP1.m_x * m_horizontalScaleInv_x));
	dgAssert (p1.m_iz == dgFastInt (boxP1.m_z * m_horizontalScaleInv_x));

	dgInt32 x0 = dgInt32 (p0.m_ix);
	dgInt32 x1 = dgInt32 (p1.m_ix);
	dgInt32 z0 = dgInt32 (p0.m_iz);
	dgInt32 z1 = dgInt32 (p1.m_iz);

	data->m_separationDistance = dgFloat32 (0.0f);
	dgFloat32 minHeight = dgFloat32 (1.0e10f);
	dgFloat32 maxHeight = dgFloat32 (-1.0e10f);
//	dgInt32 base = z0 * m_width;
	switch (m_elevationDataType) 
	{
		case m_float32Bit:
		{
			CalculateMinAndMaxElevation(x0, x1, z0, z1, (dgFloat32*)m_elevationMap, minHeight, maxHeight);
			break;
		}

		case m_unsigned16Bit:
		{
			CalculateMinAndMaxElevation(x0, x1, z0, z1, (dgUnsigned16*)m_elevationMap, minHeight, maxHeight);
			break;
		}
	}

	minHeight *= m_verticalScale;
	maxHeight *= m_verticalScale;

	if (!((maxHeight < boxP0.m_y) || (minHeight > boxP1.m_y))) {
		// scan the vertices's intersected by the box extend
		dgInt32 base = (z1 - z0 + 1) * (x1 - x0 + 1) + 2 * (z1 - z0) * (x1 - x0);
		while (base > m_instanceData->m_vertexCount[data->m_threadNumber]) {
			AllocateVertex(world, data->m_threadNumber);
		}

		dgInt32 vertexIndex = 0;
		base = z0 * m_width;
		dgVector* const vertex = &m_instanceData->m_vertex[data->m_threadNumber][0];

		switch (m_elevationDataType) 
		{
			case m_float32Bit:
			{
				const dgFloat32* const elevation = (dgFloat32*)m_elevationMap;
				for (dgInt32 z = z0; z <= z1; z ++) {
					dgFloat32 zVal = m_horizontalScale_z * z;
					for (dgInt32 x = x0; x <= x1; x ++) {
						vertex[vertexIndex] = dgVector(m_horizontalScale_x * x, m_verticalScale * elevation[base + x], zVal, dgFloat32 (0.0f));
						vertexIndex ++;
						dgAssert (vertexIndex <= m_instanceData->m_vertexCount[data->m_threadNumber]); 
					}
					base += m_width;
				}
				if (m_horizontalDisplacement) {
					AddDisplacement (vertex, x0, x1, z0, z1);
				}
				break;
			}

			case m_unsigned16Bit:
			{
				const dgUnsigned16* const elevation = (dgUnsigned16*)m_elevationMap;
				for (dgInt32 z = z0; z <= z1; z ++) {
					dgFloat32 zVal = m_horizontalScale_z * z;
					for (dgInt32 x = x0; x <= x1; x ++) {
						vertex[vertexIndex] = dgVector(m_horizontalScale_x * x, m_verticalScale * dgFloat32 (elevation[base + x]), zVal, dgFloat32 (0.0f));
						vertexIndex ++;
						dgAssert (vertexIndex <= m_instanceData->m_vertexCount[data->m_threadNumber]); 
					}
					base += m_width;
				}
				if (m_horizontalDisplacement) {
					AddDisplacement(vertex, x0, x1, z0, z1);
				}
				break;
			}
		}
	
		dgInt32 normalBase = vertexIndex;
		vertexIndex = 0;
		dgInt32 index = 0;
		dgInt32 faceCount = 0;
		dgInt32 step = x1 - x0 + 1;
		dgInt32* const indices = data->m_globalFaceVertexIndex;
		dgInt32* const faceIndexCount = data->m_meshData.m_globalFaceIndexCount;
		dgInt32 faceSize = dgInt32 (dgMax (m_horizontalScale_x, m_horizontalScale_z) * dgFloat32 (2.0f)); 

		for (dgInt32 z = z0; (z < z1) && (faceCount < DG_MAX_COLLIDING_FACES); z ++) {
			dgInt32 zStep = z * m_width;
			for (dgInt32 x = x0; (x < x1) && (faceCount < DG_MAX_COLLIDING_FACES); x ++) {
				const dgInt32* const indirectIndex = &m_cellIndices[dgInt32 (m_diagonals[zStep + x])][0];

				dgInt32 vIndex[4];
				vIndex[0] = vertexIndex;
				vIndex[1] = vertexIndex + 1;
				vIndex[2] = vertexIndex + step;
				vIndex[3] = vertexIndex + step + 1;

				const dgInt32 i0 = vIndex[indirectIndex[0]];
				const dgInt32 i1 = vIndex[indirectIndex[1]];
				const dgInt32 i2 = vIndex[indirectIndex[2]];
				const dgInt32 i3 = vIndex[indirectIndex[3]];

				const dgVector e0 (vertex[i0] - vertex[i1]);
				const dgVector e1 (vertex[i2] - vertex[i1]);
				const dgVector e2 (vertex[i3] - vertex[i1]);
				dgVector n0 (e0.CrossProduct(e1));
				dgVector n1 (e1.CrossProduct(e2));
				dgAssert (n0.m_w == dgFloat32 (0.0f));
				dgAssert (n1.m_w == dgFloat32 (0.0f));

				dgAssert  (n0.DotProduct(n0).GetScalar() > dgFloat32 (0.0f));
				dgAssert  (n1.DotProduct(n1).GetScalar() > dgFloat32 (0.0f));

				//normalBase 
				const dgInt32 normalIndex0 = normalBase;
				const dgInt32 normalIndex1 = normalBase + 1;
				vertex[normalIndex0] = n0.Normalize();
				vertex[normalIndex1] = n1.Normalize();

				faceIndexCount[faceCount] = 3;
				indices[index + 0 + 0] = i2;
				indices[index + 0 + 1] = i1;
				indices[index + 0 + 2] = i0;
				indices[index + 0 + 3] = m_atributeMap[zStep + x];
				indices[index + 0 + 4] = normalIndex0;
				indices[index + 0 + 5] = normalIndex0;
				indices[index + 0 + 6] = normalIndex0;
				indices[index + 0 + 7] = normalIndex0;
				indices[index + 0 + 8] = faceSize;

				faceIndexCount[faceCount + 1] = 3;
				indices[index + 9 + 0] = i1;
				indices[index + 9 + 1] = i2;
				indices[index + 9 + 2] = i3;
				indices[index + 9 + 3] = m_atributeMap[zStep + x];
				indices[index + 9 + 4] = normalIndex1;
				indices[index + 9 + 5] = normalIndex1;
				indices[index + 9 + 6] = normalIndex1;
				indices[index + 9 + 7] = normalIndex1;
				indices[index + 9 + 8] = faceSize;

				dgVector dp (vertex[i3] - vertex[i1]);
				dgAssert (dp.m_w == dgFloat32 (0.0f));
				dgFloat32 dist (vertex[normalIndex0].DotProduct(dp).GetScalar());
				if (dist < -dgFloat32 (1.0e-3f)) {
					indices[index + 0 + 5] = normalIndex1;
					indices[index + 9 + 5] = normalIndex0;
				}

				index += 9 * 2;
				normalBase += 2;
				faceCount += 2;
				vertexIndex ++;
			}
			vertexIndex ++;
		}

		#ifdef _DEBUG
		dgAssert (faceCount < DG_MAX_COLLIDING_FACES);
		if (faceCount >= DG_MAX_COLLIDING_FACES) {
			dgTrace (("buffer Over float, try using a lower resolution mesh for collision\n"));
		}
		#endif

		const int maxIndex = index;
		dgInt32 stepBase = (x1 - x0) * (2 * 9);
		for (dgInt32 z = z0; z < z1; z ++) {
			const dgInt32 diagBase = m_width * z;
			const dgInt32 triangleIndexBase = (z - z0) * stepBase;
			for (dgInt32 x = x0; x < (x1 - 1); x ++) {
				dgInt32 index1 = (x - x0) * (2 * 9) + triangleIndexBase;
				if (index1 < maxIndex) {
					const dgInt32 code = (m_diagonals[diagBase + x] << 1) + m_diagonals[diagBase + x + 1];
					const dgInt32* const edgeMap = &m_horizontalEdgeMap[code][0];
				
					dgInt32* const triangles = &indices[index1];
					const dgInt32 i0 = triangles[edgeMap[0]];
					const dgInt32 i1 = triangles[edgeMap[1]];
					const dgInt32 i2 = triangles[edgeMap[2]];

					const dgVector& origin = vertex[i0];
					const dgVector& testPoint = vertex[i1];
					const dgVector& normal = vertex[i2];
					dgAssert (normal.m_w == dgFloat32 (0.0f));
					dgFloat32 dist (normal.DotProduct(testPoint - origin).GetScalar());

					if (dist < -dgFloat32 (1.0e-3f)) {
						const dgInt32 i3 = edgeMap[3];
						const dgInt32 i4 = edgeMap[4];
						const dgInt32 i5 = edgeMap[5];
						const dgInt32 i6 = edgeMap[6];
						triangles[i3] = triangles[i6];
						triangles[i4] = triangles[i5];
					}
				}
			}
		}

		for (dgInt32 x = x0; x < x1; x ++) {
			const dgInt32 triangleIndexBase = (x - x0) * (2 * 9);
			for (dgInt32 z = z0; z < (z1 - 1); z ++) {	
				dgInt32 index1 = (z - z0) * stepBase + triangleIndexBase;
				if (index1 < maxIndex) {
					const dgInt32 diagBase = m_width * z;
					const dgInt32 code = (m_diagonals[diagBase + x] << 1) + m_diagonals[diagBase + m_width + x];
					const dgInt32* const edgeMap = &m_verticalEdgeMap[code][0];

					dgInt32* const triangles = &indices[index1];
					const dgInt32 i0 = triangles[edgeMap[0]];
					const dgInt32 i1 = triangles[edgeMap[1] + stepBase];
					const dgInt32 i2 = triangles[edgeMap[2]];

					const dgVector& origin = vertex[i0];
					const dgVector& testPoint = vertex[i1];
					const dgVector& normal = vertex[i2];
					dgAssert (normal.m_w == dgFloat32 (0.0f));
					dgFloat32 dist (normal.DotProduct(testPoint - origin).GetScalar());

					if (dist < -dgFloat32 (1.0e-3f)) {
						const dgInt32 i3 = edgeMap[3];
						const dgInt32 i4 = edgeMap[4] + stepBase;
						const dgInt32 i5 = edgeMap[5];
						const dgInt32 i6 = edgeMap[6] + stepBase;
						triangles[i3] = triangles[i6];
						triangles[i4] = triangles[i5];
					}
				}
			}
		}

		dgInt32 stride = sizeof (dgVector) / sizeof (dgFloat32);
		dgInt32 faceCount0 = 0; 
		dgInt32 faceIndexCount0 = 0; 
		dgInt32 faceIndexCount1 = 0; 

		dgInt32* const address = data->m_meshData.m_globalFaceIndexStart;
		dgFloat32* const hitDistance = data->m_meshData.m_globalHitDistance;
		
		if (data->m_doContinuesCollisionTest) {
			dgFastRayTest ray (dgVector (dgFloat32 (0.0f)), data->m_boxDistanceTravelInMeshSpace);
			for (dgInt32 i = 0; i < faceCount; i ++) {
				const dgInt32* const indexArray = &indices[faceIndexCount1]; 
				const dgVector& faceNormal = vertex[indexArray[4]];
				dgFloat32 dist = data->PolygonBoxRayDistance (faceNormal, 3, indexArray, stride, &vertex[0].m_x, ray);
				if (dist < dgFloat32 (1.0f)) {
					hitDistance[faceCount0] = dist;
					address[faceCount0] = faceIndexCount0;
					memcpy (&indices[faceIndexCount0], indexArray, 9 * sizeof (dgInt32));
					faceCount0 ++;
					faceIndexCount0 += 9;
				}
				faceIndexCount1 += 9;
			}
		} else {
			for (dgInt32 i = 0; i < faceCount; i ++) {
				const dgInt32* const indexArray = &indices[faceIndexCount1]; 
				const dgVector& faceNormal = vertex[indexArray[4]];
				dgFloat32 dist = data->PolygonBoxDistance (faceNormal, 3, indexArray, stride, &vertex[0].m_x);
				if (dist > dgFloat32 (0.0f)) {
					hitDistance[faceCount0] = dist;
					address[faceCount0] = faceIndexCount0;
					memcpy (&indices[faceIndexCount0], indexArray, 9 * sizeof (dgInt32));
					faceCount0 ++;
					faceIndexCount0 += 9;
				}
				faceIndexCount1 += 9;
			}
		}

		if (faceCount0) {
			// initialize the callback data structure
			data->m_faceCount = faceCount0;
			data->m_vertex = &vertex[0].m_x;
			data->m_faceVertexIndex = indices;
			data->m_faceIndexStart = address;
			data->m_hitDistance = hitDistance;
			data->m_faceIndexCount = faceIndexCount;
			data->m_vertexStrideInBytes = sizeof (dgVector);

			if (GetDebugCollisionCallback()) { 
				dgTriplex triplex[3];
				const dgVector scale = data->m_polySoupInstance->GetScale();
				dgMatrix matrix(data->m_polySoupInstance->GetLocalMatrix() * data->m_polySoupBody->GetMatrix());

				for (dgInt32 i = 0; i < data->m_faceCount; i ++) {
					dgInt32 base1 = address[i];
					for (dgInt32 j = 0; j < 3; j ++) {
						dgInt32 index1 = data->m_faceVertexIndex[base1 + j];
						dgVector p (matrix.TransformVector(scale * dgVector(vertex[index1]))); 
						triplex[j].m_x = p.m_x;
						triplex[j].m_y = p.m_y;
						triplex[j].m_z = p.m_z;
					}
					GetDebugCollisionCallback() (data->m_polySoupBody, data->m_objBody, data->m_faceVertexIndex[base1 + 4], 3, &triplex[0].m_x, sizeof (dgTriplex));
				}
			}
		}

		#ifdef _DEBUG
		for (dgInt32 i = 0; i < data->m_faceCount; i ++) {
			dgInt32 base1 = address[i];
			const dgInt32* const localIndexArray = &data->m_faceVertexIndex[base1];

			dgInt32 index1 = data->GetNormalIndex (localIndexArray, 3);
			dgVector n (vertex[index1]);
			dgVector q0 (vertex[data->m_faceVertexIndex[base1 + 0]]);
			dgVector q1 (vertex[data->m_faceVertexIndex[base1 + 1]]);
			dgVector q2 (vertex[data->m_faceVertexIndex[base1 + 2]]);

			dgMatrix polygonMatrix;
			polygonMatrix[0] = q1 - q0;
			polygonMatrix[0] = polygonMatrix[0].Normalize();
			polygonMatrix[1] = n.CrossProduct(polygonMatrix[0]);
			polygonMatrix[2] = n;
			polygonMatrix[3] = dgVector::m_wOne;
			dgAssert (polygonMatrix.TestOrthogonal());
		}
		#endif
	}
}
