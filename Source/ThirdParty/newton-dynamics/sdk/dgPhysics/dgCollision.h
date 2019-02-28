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

#ifndef _DG_COLLISION_H_ 
#define _DG_COLLISION_H_ 


class dgBody;
class dgWorld;
class dgCollision;
class dgMeshEffect;
class dgContactPoint;
class dgPolygonSoupDesc;
class dgCollisionConvex;
class dgPolygonMeshDesc;
class dgCollisionInstance;
class dgCollisionConvexHull;
class dgPolygonSoupRayHitDesc;

#ifdef _DEBUG
//	#define DG_DEBUG_AABB
#endif

#define PREFILTER_RAYCAST(filter,body,collision,userData) (filter && !filter(body,collision,userData)) 

typedef dgInt32(dgApi *OnBodiesInAABB) (dgBody* body, void* const userData);
typedef dgUnsigned32(dgApi *OnRayPrecastAction) (const dgBody* const body, const dgCollisionInstance* const collision, void* const userData);
typedef dgFloat32(dgApi *OnRayCastAction) (const dgBody* const body, const dgCollisionInstance* const collision, const dgVector& contact, const dgVector& normal, dgInt64 collisionID, void* const userData, dgFloat32 intersetParam);


enum dgCollisionID
{
	// do not change the order of these enum
	m_sphereCollision = 0,
	m_capsuleCollision,
	m_cylinderCollision,
	m_chamferCylinderCollision,
	m_boxCollision,
	m_coneCollision,
	m_convexHullCollision,
	// this must be the last convex shape ID
	m_nullCollision,					
	
	m_compoundCollision,
	m_boundingBoxHierachy,
	m_heightField,
	m_deformableClothPatch,
	m_deformableSolidMesh,
	m_userMesh,
	m_sceneCollision,
	m_compoundFracturedCollision,

	// these are for internal use only	
	m_contactCloud,
	m_polygonCollision,
	m_lumpedMassCollision
};


DG_MSC_VECTOR_ALIGMENT
class dgCollisionInfo
{
	public:
	class dgInstanceMaterial
	{
		public:
		dgInstanceMaterial()
		{
			memset(this, 0, sizeof (dgInstanceMaterial));
		}

		void* m_userData;
		dgInt32 m_userId;
		dgInt32 m_userFlags;
		dgFloat32 m_userParam[8];
	};

	struct dgBoxData
	{
		dgFloat32 m_x;
		dgFloat32 m_y;
		dgFloat32 m_z;
	} ;

	struct dgSphereData
	{
		dgFloat32 m_radius;
	} ;

	struct dgCylinderData
	{
		dgFloat32 m_radio0;
		dgFloat32 m_radio1;
		dgFloat32 m_height;
	};

	struct dgCapsuleData
	{
		dgFloat32 m_radio0;
		dgFloat32 m_radio1;
		dgFloat32 m_height;
	};

	struct dgConeData
	{
		dgFloat32 m_r;
		dgFloat32 m_height;
	};

	struct dgChamferCylinderData
	{
		dgFloat32 m_r;
		dgFloat32 m_height;
	};

	struct dgConvexHullData
	{
		dgInt32 m_vertexCount;
		dgInt32 m_strideInBytes;
		dgInt32 m_faceCount;
		dgVector* m_vertex;
	};

	struct dgConvexModifierData
	{
		dgCollision* m_child;
	};

	struct dgCoumpountCollisionData
	{
		dgInt32 m_chidrenCount;
		//dgCollision** m_chidren;
	};

	struct dgCollisionBVHData
	{
		dgInt32 m_vertexCount;
		dgInt32 m_indexCount;
	};

	struct dgDeformableMeshData
	{
		dgInt32 m_vertexCount;
		dgInt32 m_triangleCount;
		dgInt32 m_vertexStrideInBytes;
		dgUnsigned16* m_indexList;
		dgFloat32* m_vertexList;
	};

	struct dgHeightMapCollisionData
	{
		dgInt32 m_width;
		dgInt32 m_height;
		dgInt32 m_gridsDiagonals;
		dgInt32 m_elevationDataType;		// 0 = 32 bit floats, 1 = unsigned 16 bit intergers
		dgFloat32 m_verticalScale;
		dgFloat32 m_horizonalScale_x;
		dgFloat32 m_horizonalScale_z;
		dgFloat32 m_horizonalDisplacementScale_x;
		dgFloat32 m_horizonalDisplacementScale_z;
		void* m_elevation;
		dgUnsigned16* m_horizotalDisplacement;
		dgInt8* m_atributes;
		
	};

	struct dgSceneData
	{
		dgInt32 m_childrenProxyCount;
	};

	dgMatrix m_offsetMatrix;
	dgInstanceMaterial m_collisionMaterial;
	dgInt32 m_collisionType;
	union 
	{
		dgBoxData m_box;
		dgConeData m_cone;
		dgSphereData m_sphere;
		dgCapsuleData m_capsule;
		dgCylinderData m_cylinder;
		dgChamferCylinderData m_chamferCylinder;
		dgConvexHullData m_convexHull;
		dgDeformableMeshData m_deformableMesh;
		dgConvexModifierData m_convexModifierData;
		dgCoumpountCollisionData m_compoundCollision;
		dgCollisionBVHData m_bvhCollision;
		dgHeightMapCollisionData m_heightFieldCollision;
		dgSceneData m_sceneCollision;
		dgFloat32 m_paramArray[32];
	};
}DG_GCC_VECTOR_ALIGMENT;




DG_MSC_VECTOR_ALIGMENT
class dgCollision
{
	public:
	typedef void (dgApi *OnDebugCollisionMeshCallback) (void* userData, int vertexCount, const dgFloat32* FaceArray, int faceId);

	enum dgRTTI {
		dgCollisionNull_RTTI						= 1<<0,
		dgCollisionBox_RTTI							= 1<<1,
		dgCollisionCone_RTTI						= 1<<2,
		dgCollisionSphere_RTTI						= 1<<3,
		dgCollisionCapsule_RTTI						= 1<<4,
		dgCollisionCylinder_RTTI					= 1<<5,
		dgCollisionConvexHull_RTTI					= 1<<6,
		dgCollisionChamferCylinder_RTTI 			= 1<<7,
		dgCollisionConvexPolygon_RTTI				= 1<<8,
		dgCollisionConvexShape_RTTI					= 1<<9,
		dgCollisionCompound_RTTI					= 1<<10,
		dgCollisionBVH_RTTI							= 1<<11,
		dgCollisionMesh_RTTI						= 1<<12,
		dgCollisionLumpedMass_RTTI					= 1<<13,
		dgCollisionDeformableMesh_RTTI				= 1<<14,
		dgCollisionDeformableSolidMesh_RTTI			= 1<<15,
		dgCollisionMassSpringDamperSystem_RTTI		= 1<<16,
		dgCollisionIncompressibleParticles_RTTI		= 1<<17,
		dgCollisionUserMesh_RTTI					= 1<<18,
		dgCollisionHeightField_RTTI					= 1<<19,
		dgCollisionScene_RTTI						= 1<<20,
		dgCollisionCompoundBreakable_RTTI			= 1<<21,
	};													 
	
	DG_CLASS_ALLOCATOR(allocator)
	static dgUnsigned32 Quantize (dgFloat32 value);
	static dgUnsigned32 Quantize(const void* const buffer, int size);

	// these function should be be virtual
	dgInt32 IsType (dgRTTI type) const; 
	dgUnsigned32 GetSignature () const;
	dgCollisionID GetCollisionPrimityType () const;

	virtual dgVector SupportVertex (const dgVector& dir, dgInt32* const vertexIndex) const = 0;
	virtual dgVector SupportVertexSpecialProjectPoint (const dgVector& point, const dgVector& dir) const = 0;
	virtual dgVector SupportVertexSpecial (const dgVector& dir, dgFloat32 skinSkinThickness, dgInt32* const vertexIndex) const = 0;

	virtual dgInt32 CalculatePlaneIntersection (const dgVector& normal, const dgVector& point, dgVector* const contactsOut) const = 0;

	virtual void SetCollisionBBox (const dgVector& p0, const dgVector& p1) = 0;
	virtual void CalcAABB (const dgMatrix& matrix, dgVector& p0, dgVector& p1) const = 0;

	virtual void DebugCollision  (const dgMatrix& matrix, dgCollision::OnDebugCollisionMeshCallback callback, void* const userData) const = 0;
	virtual dgFloat32 RayCast (const dgVector& localP0, const dgVector& localP1, dgFloat32 maxT, dgContactPoint& contactOut, const dgBody* const body, void* const userData, OnRayPrecastAction preFilter) const = 0;

	virtual dgFloat32 GetVolume () const = 0;
	
	virtual void MassProperties ();
	virtual dgFloat32 CalculateMassProperties (const dgMatrix& offset, dgVector& inertia, dgVector& crossInertia, dgVector& centerOfMass) const {dgAssert (0); return 0;}
	virtual dgMatrix CalculateInertiaAndCenterOfMass (const dgMatrix& m_alignMatrix, const dgVector& localScale, const dgMatrix& matrix) const {dgAssert (0); return dgGetZeroMatrix();}

	virtual dgFloat32 GetBoxMinRadius () const = 0; 
	virtual dgFloat32 GetBoxMaxRadius () const = 0; 
	
	virtual dgVector CalculateVolumeIntegral (const dgMatrix& globalMatrix, const dgVector& globalPlane, const dgCollisionInstance& parentScale) const = 0;
	virtual void Serialize(dgSerialize callback, void* const userData) const = 0;

	virtual void GetCollisionInfo(dgCollisionInfo* const info) const;
	virtual void SerializeLow(dgSerialize callback, void* const userData) const;

	virtual dgInt32 GetConvexVertexCount() const; 

	const dgCollision* AddRef () const;
	dgInt32 GetRefCount() const;
	virtual dgInt32 Release () const;
	
	const dgVector& GetObbOrigin() const; 
	virtual dgVector GetObbSize() const; 

	dgFloat32 GetUmbraClipSize () const;
	dgMemoryAllocator* GetAllocator() const;

	protected:
	dgCollision (const dgCollision& source);
	dgCollision (dgMemoryAllocator* const allocator, dgUnsigned32 signature, dgCollisionID id);
	dgCollision (dgWorld* const world, dgDeserialize deserialization, void* const userData, dgInt32 revision);
	virtual ~dgCollision();
	
	void SetSignature (dgInt32 signature);
	virtual dgInt32 CalculateSignature () const = 0;

	dgVector m_inertia;	
	dgVector m_crossInertia;	
	dgVector m_centerOfMass;	
	dgVector m_boxSize;
	dgVector m_boxOrigin;
	dgInt32 m_rtti;
	mutable dgInt32 m_refCount;
	dgUnsigned32 m_signature;
	dgCollisionID m_collisionId;
	dgMemoryAllocator* m_allocator;

	static dgVector m_flushZero;
	friend class dgBody;
	friend class dgWorld;
	friend class dgMinkowskiConv;
	friend class dgCollisionInstance;
	friend class dgCollisionCompound;
}DG_GCC_VECTOR_ALIGMENT;

DG_INLINE dgCollisionID dgCollision::GetCollisionPrimityType () const
{
	return m_collisionId;
}

DG_INLINE dgUnsigned32 dgCollision::GetSignature () const
{
	return m_signature;
}

DG_INLINE void dgCollision::SetSignature (dgInt32 signature)
{
	m_signature = dgUnsigned32 (signature);
}


DG_INLINE const dgCollision* dgCollision::AddRef () const
{
	dgAtomicExchangeAndAdd (&m_refCount, 1);
	return this;
}

DG_INLINE dgInt32 dgCollision::Release () const
{
	dgAtomicExchangeAndAdd (&m_refCount, -1);
	if (m_refCount) {
		return m_refCount;
	}
	delete this;
	return 0;
}

DG_INLINE dgInt32 dgCollision::GetRefCount() const
{
	dgAssert (0);
	return m_refCount;
}


DG_INLINE dgMemoryAllocator* dgCollision::GetAllocator() const
{
	return m_allocator;
}


DG_INLINE dgFloat32 dgCollision::GetUmbraClipSize () const
{
//	return GetMax (GetBoxMaxRadius() * dgFloat32 (2.0f) + dgFloat32 (1.0f), dgFloat32 (16.0f));
	return dgFloat32 (3.0f) * GetBoxMaxRadius();
}

DG_INLINE dgInt32 dgCollision::GetConvexVertexCount() const 
{ 
	return 0;
}

DG_INLINE dgInt32 dgCollision::IsType (dgRTTI type) const 
{
	return type & m_rtti;
}

DG_INLINE dgVector dgCollision::GetObbSize() const
{
	return m_boxSize;
}

DG_INLINE const dgVector& dgCollision::GetObbOrigin() const
{
	return m_boxOrigin;
}


#endif 


