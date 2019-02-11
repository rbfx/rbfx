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

#ifndef __dgMeshEffect_H__
#define __dgMeshEffect_H__

#include <dgRefCounter.h>

class dgWorld;
class dgMeshEffect;
class dgCollisionInstance;

#define DG_MESH_EFFECT_PRECISION_BITS		48
#define DG_MESH_EFFECT_PRECISION_SCALE		dgFloat64(dgInt64(1)<<DG_MESH_EFFECT_PRECISION_BITS)
#define DG_MESH_EFFECT_PRECISION_SCALE_INV	(dgFloat64 (1.0f) / DG_MESH_EFFECT_PRECISION_SCALE)

#define DG_VERTEXLIST_INDEXLIST_TOL			(dgFloat64 (0.0f))
#define DG_MESH_EFFECT_POINT_SPLITED		512
#define DG_MESH_EFFECT_BVH_STACK_DEPTH		256

class dgMeshEffect: public dgPolyhedra, public dgRefCounter
{
	public:
	enum dgChannelType
	{
		m_vertex,
		m_normal,
		m_binormal,
		m_uv0,
		m_uv1,
		m_color,
		m_material,
		m_layer,
		m_point,
	};

	class dgMeshVertexFormat
	{
		public:
		class dgDoubleData
		{
			public:
			const dgFloat64* m_data;
			const dgInt32* m_indexList;
			dgInt32 m_strideInBytes;
		};

		class dgFloatData
		{
			public:
			const dgFloat32* m_data;
			const dgInt32* m_indexList;
			dgInt32 m_strideInBytes;
		};

		dgMeshVertexFormat ()
		{
			Clear ();
		}

		void Clear ()
		{
			memset (this, 0, sizeof (dgMeshVertexFormat));
		}

		dgInt32 m_faceCount;
		const dgInt32* m_faceIndexCount;
		const dgInt32* m_faceMaterial;
		dgDoubleData m_vertex;
		dgFloatData m_normal;
		dgFloatData m_binormal;
		dgFloatData m_uv0;
		dgFloatData m_uv1;
		dgFloatData m_vertexColor;
	};

	template<class T, dgChannelType type>
	class dgChannel: public dgArray<T>
	{
		public:
		dgChannel(dgMemoryAllocator* const allocator)
			:dgArray<T>(allocator)
			,m_count(0)
			,m_type(type)
		{
		}

		dgChannel(const dgChannel& source)
			:dgArray<T>(source, source.m_count)
			,m_count(source.m_count)
			,m_type(source.m_type)
		{
		}

		~dgChannel()
		{
		}

		void CopyFrom (const dgChannel<T, type>& source)
		{
			dgArray<T>& me = *this;
			dgChannel& src = *((dgChannel*)&source);

			Clear();
			m_count = src.m_count;
			dgAssert (m_type == src.m_type);
			for (dgInt32 i = 0; i < m_count; i++) {
				me[i] = src[i];
			}
		}

		void Clear()
		{
			m_count = 0;
			dgArray<T>::Clear ();
		}

		void Reserve (dgInt32 size)
		{
			dgArray<T>::Resize(size);
			m_count = size;
		}

		void PushBack (const T& element) 
		{
			T tmp (element);
			dgArray<T>& me = *this;
			me[m_count] = tmp;
			m_count ++;
		}

		void SetCount (dgInt32 count) 
		{
			if (m_count) {
				dgAssert (count >= 0);
				dgAssert (m_count >= count);
				m_count = count;
			}
		}

		dgInt32 m_count;
		dgChannelType m_type;
	};

	class dgFormat
	{
		public:
		class dgSortKey
		{
			public:
			dgInt32 m_mask;
			dgInt32 m_ordinal;
			dgInt32 m_vertexIndex;
			dgInt32 m_attibuteIndex;
		};
		class VertexSortData
		{
			public:
			const dgChannel<dgBigVector, m_point>* m_points;
			dgInt32 m_vertexSortIndex;
		};

		dgInt32 GetSortIndex (const dgChannel<dgBigVector, m_point>& points, dgFloat64& dist) const;
		static dgInt32 CompareVertex(const dgSortKey* const ptr0, const dgSortKey* const ptr1, void* const context);
	};

	class dgPointFormat: public dgFormat
	{
		public:
		dgPointFormat(dgMemoryAllocator* const allocator);
		dgPointFormat(const dgPointFormat& source);
		~dgPointFormat();

		void Clear();
		void SetCount (dgInt32 count);
		void CompressData(dgInt32* const indexList);

		dgChannel<dgInt32, m_layer> m_layers;
		dgChannel <dgBigVector, m_point> m_vertex;
	};

	class dgAttibutFormat: public dgFormat
	{
		public:
		class dgUV
		{
			public:
			dgFloat32 m_u;
			dgFloat32 m_v;
		};

		dgAttibutFormat(dgMemoryAllocator* const allocator);
		dgAttibutFormat(const dgAttibutFormat& source);
		~dgAttibutFormat();

		void Clear();
		void SetCount (dgInt32 count);
		void CopyFrom (const dgAttibutFormat& source);
		void CopyEntryFrom (dgInt32 index, const dgAttibutFormat& source, dgInt32 sourceIndex);
		void CompressData (const dgPointFormat& points, dgInt32* const indexList);

		dgChannel<dgInt32, m_vertex> m_pointChannel;
		dgChannel<dgInt32, m_material> m_materialChannel;
		dgChannel<dgTriplex, m_normal> m_normalChannel;
		dgChannel<dgTriplex, m_binormal> m_binormalChannel;
		dgChannel<dgVector, m_color> m_colorChannel;
		dgChannel<dgUV, m_uv0> m_uv0Channel;
		dgChannel<dgUV, m_uv1> m_uv1Channel;
	};

	class dgIndexArray 
	{
		public:
		dgInt32 m_materialCount;
		dgInt32 m_indexCount;
		dgInt32 m_materials[256];
		dgInt32 m_materialsIndexCount[256];
		dgInt32* m_indexList;
	};

	class dgMeshBVH
	{
		public:
		class dgMeshBVHNode
		{
			public:
			dgMeshBVHNode (const dgMeshEffect* const mesh, dgEdge* const face, void* const userData);
			dgMeshBVHNode (dgMeshBVHNode* const left, dgMeshBVHNode* const right);
			~dgMeshBVHNode ();
			void SetBox (const dgVector& p0, const dgVector& p1);

			DG_CLASS_ALLOCATOR(allocator)
			dgVector m_p0;
			dgVector m_p1;

			dgFloat32 m_area;
			dgEdge* m_face;
			void* m_userData;
			dgMeshBVHNode* m_left;
			dgMeshBVHNode* m_right;
			dgMeshBVHNode* m_parent;
		};

		class dgFitnessList: public dgTree <dgMeshBVHNode*, dgMeshBVHNode*>
		{
			public:
			dgFitnessList (dgMemoryAllocator* const allocator);
			dgFloat64 TotalCost () const;
		};

		
		dgMeshBVH (const dgMeshEffect* const mesh);
		virtual ~dgMeshBVH();

		virtual void Build ();
		virtual void Cleanup ();
		
		void FaceRayCast (const dgBigVector& l0, const dgBigVector& l1, void* const userData) const;
		void GetOverlapNodes (dgList<dgMeshBVHNode*>& overlapNodes, const dgBigVector& p0, const dgBigVector& p1) const;

		protected:
		virtual dgMeshBVHNode* CreateLeafNode (dgEdge* const face, void* const userData) = 0;

		dgMeshBVHNode* AddFaceNode (dgEdge* const face, void* const userData);
		void RemoveNode (dgMeshBVHNode* const treeNode);
		void ImproveNodeFitness ();
		void ImproveNodeFitness (dgMeshBVHNode* const node);
		dgFloat32 CalculateSurfaceArea (dgMeshBVHNode* const node0, dgMeshBVHNode* const node1, dgVector& minBox, dgVector& maxBox) const;
		virtual bool SanityCheck() const;

		virtual dgFloat64 RayFaceIntersect (const dgMeshBVHNode* const face, const dgBigVector& p0, const dgBigVector& p1, void* const userData) const;
//		virtual dgFloat64 VertexRayCast (const dgBigVector& l0, const dgBigVector& l1) const;
//		virtual bool RayRayIntersect (dgEdge* const edge, const dgMeshEffect* const otherMesh, dgEdge* const otherEdge, dgFloat64& param, dgFloat64& otherParam) const;
		
		const dgMeshEffect* m_mesh;
		dgMeshBVHNode* m_rootNode;
		dgFitnessList m_fitness;
		friend class dgMeshEffect;
	};

	dgMeshEffect ();
	dgMeshEffect(dgMemoryAllocator* const allocator);
	dgMeshEffect(dgCollisionInstance* const collision);
	dgMeshEffect(const dgMeshEffect& source);
	dgMeshEffect(dgPolyhedra& mesh, const dgMeshEffect& source);
	dgMeshEffect (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData);

	// create from OFF or PLY file format
	dgMeshEffect(dgMemoryAllocator* const allocator, const char* const fileName);

	// Create a convex hull Mesh form point cloud
	dgMeshEffect (dgMemoryAllocator* const allocator, const dgFloat64* const vertexCloud, dgInt32 count, dgInt32 strideInByte, dgFloat64 distTol);

	// create a planar Mesh
	dgMeshEffect(dgMemoryAllocator* const allocator, const dgMatrix& planeMatrix, dgFloat32 witdth, dgFloat32 breadth, dgInt32 material, const dgMatrix& textureMatrix0, const dgMatrix& textureMatrix1);
	virtual ~dgMeshEffect(void);

	void Init();

	void Trace () const;
	void SaveOFF (const char* const fileName) const;

	void ApplyTransform (const dgMatrix& matrix);
	dgMatrix CalculateOOBB (dgBigVector& size) const;
	void CalculateAABB (dgBigVector& min, dgBigVector& max) const;

	void CalculateNormals (dgFloat64 angleInRadians);
	void SphericalMapping (dgInt32 material, const dgMatrix& uvAligment);
	void BoxMapping (dgInt32 front, dgInt32 side, dgInt32 top);
	void UniformBoxMapping (dgInt32 material, const dgMatrix& textruMatrix);
	void CylindricalMapping (dgInt32 cylinderMaterial, dgInt32 capMaterial, const dgMatrix& uvAligment);
	void AngleBaseFlatteningMapping (dgInt32 cylinderMaterial, dgReportProgress progressReportCallback, void* const userData);

	dgEdge* InsertEdgeVertex (dgEdge* const edge, dgFloat64 param);

	dgMeshEffect* Union (const dgMatrix& matrix, const dgMeshEffect* const clipper) const;
	dgMeshEffect* Difference (const dgMatrix& matrix, const dgMeshEffect* const clipper) const;
	dgMeshEffect* Intersection (const dgMatrix& matrix, const dgMeshEffect* const clipper) const;
	void ClipMesh (const dgMatrix& matrix, const dgMeshEffect* const clipper, dgMeshEffect** const top, dgMeshEffect** const bottom) const;

	//bool PlaneClip (const dgBigPlane& plane);
	
	dgMeshEffect* ConvexMeshIntersection (const dgMeshEffect* const convexMesh) const;

	dgMeshEffect* GetFirstLayer ();
	dgMeshEffect* GetNextLayer (dgMeshEffect* const layer);

	void Triangulate ();
	void ConvertToPolygons ();
	void RemoveUnusedVertices(dgInt32* const vertexRemapTable);
	
	void BeginBuild ();
		void BeginBuildFace ();
			void AddPoint (dgFloat64 x, dgFloat64 y, dgFloat64 z);
			void AddLayer (dgInt32 layer);
			void AddMaterial (dgInt32 materialIndex);
			void AddNormal (dgFloat32 x, dgFloat32 y, dgFloat32 z);
			void AddBinormal (dgFloat32 x, dgFloat32 y, dgFloat32 z);
			void AddVertexColor (dgFloat32 x, dgFloat32 y, dgFloat32 z, dgFloat32 w);
			void AddUV0 (dgFloat32 u, dgFloat32 v);
			void AddUV1 (dgFloat32 u, dgFloat32 v);
		void EndBuildFace ();
	void EndBuild (dgFloat64 tol, bool fixTjoint = true);

	dgInt32 GetVertexCount() const;
	dgInt32 GetVertexStrideInByte() const;
	const dgFloat64* GetVertexPool () const;

	dgInt32 GetVertexBaseCount() const;
	void SetVertexBaseCount(dgInt32 count);
	
	dgEdge* SpliteFace (dgInt32 v0, dgInt32 v1);

	dgInt32 GetTotalFaceCount() const;
	dgInt32 GetTotalIndexCount() const;
	void GetFaces (dgInt32* const faceCount, dgInt32* const materials, void** const faceNodeList) const;

	void RepairTJoints ();
	bool SeparateDuplicateLoops (dgEdge* const face);

	bool HasOpenEdges () const;

	dgFloat64 CalculateVolume () const;

	void OptimizePoints();
	void OptimizeAttibutes();
	void BuildFromIndexList(const dgMeshVertexFormat* const format);

	dgInt32 GetPropertiesCount() const;
	const dgInt32* GetIndexToVertexMap() const;

	bool HasLayersChannel() const;
	bool HasNormalChannel() const;
	bool HasBinormalChannel() const;
	bool HasUV0Channel() const;
	bool HasUV1Channel() const;
	bool HasVertexColorChannel() const;
	
	void GetVertexChannel64(dgInt32 strideInByte, dgFloat64* const bufferOut) const;
	void GetVertexChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
	void GetNormalChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
	void GetBinormalChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
	void GetUV0Channel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
	void GetUV1Channel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
	void GetVertexColorChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
//	void GetWeightBlendChannel(dgInt32 strideInByte, dgFloat32* const bufferOut) const;
//	void GetWeightIndexChannel(dgInt32 strideInByte, dgInt32* const bufferOut) const;

	dgIndexArray* MaterialGeometryBegin();
	void MaterialGeomteryEnd(dgIndexArray* const handle);
	dgInt32 GetFirstMaterial (dgIndexArray* const handle) const;
	dgInt32 GetNextMaterial (dgIndexArray* const handle, dgInt32 materialHandle) const;
	dgInt32 GetMaterialID (dgIndexArray* const handle, dgInt32 materialHandle) const;
	dgInt32 GetMaterialIndexCount (dgIndexArray* const handle, dgInt32 materialHandle) const;
	void GetMaterialGetIndexStream (dgIndexArray* const handle, dgInt32 materialHandle, dgInt32* const index) const;
	void GetMaterialGetIndexStreamShort (dgIndexArray* const handle, dgInt32 materialHandle, dgInt16* const index) const;
	
	dgCollisionInstance* CreateCollisionTree(dgWorld* const world, dgInt32 shapeID) const;
	dgCollisionInstance* CreateConvexCollision(dgWorld* const world, dgFloat64 tolerance, dgInt32 shapeID, const dgMatrix& matrix = dgGetIdentityMatrix()) const;

	dgMeshEffect* CreateSimplification (dgInt32 maxVertexCount, dgReportProgress reportProgressCallback, void* const userData) const;
	dgMeshEffect* CreateConvexApproximation (dgFloat32 maxConcavity, dgFloat32 backFaceDistanceFactor, dgInt32 maxHullOuputCount, dgInt32 maxVertexPerHull, dgReportProgress reportProgressCallback, void* const userData) const;

	dgMeshEffect* CreateTetrahedraIsoSurface() const;
	void CreateTetrahedraLinearBlendSkinWeightsChannel (const dgMeshEffect* const tetrahedraMesh);

	static dgMeshEffect* CreateVoronoiConvexDecomposition (dgMemoryAllocator* const allocator, dgInt32 pointCount, dgInt32 pointStrideInBytes, const dgFloat32* const pointCloud, dgInt32 materialId, const dgMatrix& textureProjectionMatrix);
	static dgMeshEffect* CreateFromSerialization (dgMemoryAllocator* const allocator, dgDeserialize deserialization, void* const userData);

	void LoadOffMesh (const char* const filename);
	void LoadTetraMesh (const char* const filename);
	void Serialize (dgSerialize callback, void* const userData) const;

	dgBigVector GetVertex (dgInt32 index) const;
	dgInt32 GetVertexLayer (dgInt32 index) const;

	void TransformMesh (const dgMatrix& matrix);

	void* GetFirstVertex () const;
	void* GetNextVertex (const void* const vertex) const;
	int GetVertexIndex (const void* const vertex) const;

	void* GetFirstPoint () const;
	void* GetNextPoint (const void* const point) const;
	dgInt32 GetPointIndex (const void* const point) const;
	dgInt32 GetVertexIndexFromPoint (const void* const point) const;

	void* GetFirstEdge () const;
	void* GetNextEdge (const void* const edge) const;
//	void* FindEdge (dgInt32 v0, dgInt32 v1) const;
	void GetEdgeIndex (const void* const edge, dgInt32& v0, dgInt32& v1) const;
//	void GetEdgeAttributeIndex (const void* edge, dgInt32& v0, dgInt32& v1) const;

	const dgEdge* GetPolyhedraEdgeFromNode(const void* const edge) const;

	void* GetFirstFace () const;
	void* GetNextFace (const void* const face) const;
	dgInt32 IsFaceOpen (const void* const face) const;
	dgInt32 GetFaceMaterial (const void* const face) const;
	dgInt32 GetFaceIndexCount (const void* const face) const;
	void GetFaceIndex (const void* const face, dgInt32* const indices) const;
	void GetFaceAttributeIndex (const void* const face, dgInt32* const indices) const;
	dgBigVector CalculateFaceNormal (const void* const face) const;

	void SetFaceMaterial (const void* const face, int materialID);
	void AddInterpolatedEdgeAttribute (dgEdge* const edge, dgFloat64 param);
	dgInt32 AddInterpolatedHalfAttribute(dgEdge* const edge, dgInt32 midPoint);
	dgInt32 InterpolateVertex (const dgBigVector& point, const dgEdge* const face) const;

	bool Sanity () const;

	protected:
	virtual void BeginFace();
	virtual bool EndFace ();

	dgBigVector GetOrigin ()const;
	dgInt32 CalculateMaxAttributes () const;
	dgFloat64 QuantizeCordinade(dgFloat64 val) const;

	void MergeFaces (const dgMeshEffect* const source);
//	void ReverseMergeFaces (dgMeshEffect* const source);

	bool PlaneClip (const dgMeshEffect& convexMesh, const dgEdge* const face);

	dgMeshEffect* GetNextLayer (dgInt32 mark);
	dgMeshEffect* CreateVoronoiConvex (const dgBigVector* const conevexPointCloud, dgInt32 count, dgInt32 materialId, const dgMatrix& textureProjectionMatrix, dgFloat32 normalAngleInRadians) const;

	void PackAttibuteData ();
	void UnpackAttibuteData ();

	void PackPoints (dgFloat64 tol);
	void UnpackPoints();

	dgPointFormat m_points;
	dgAttibutFormat m_attrib;
	dgInt32 m_vertexBaseCount;
	dgInt32 m_constructionIndex;
	
	friend class dgConvexHull3d;
	friend class dgConvexHull4d;
	friend class dgBooleanMeshBVH;
	friend class dgHACDClusterGraph;
	friend class dgTriangleAnglesToUV;
	friend class dgTetraIsoSufaceStuffing;
	friend class dgCollisionCompoundFractured;
};

DG_INLINE dgInt32 dgMeshEffect::GetVertexCount() const
{
	return m_points.m_vertex.m_count;
}

DG_INLINE dgInt32 dgMeshEffect::GetVertexBaseCount() const
{
	return m_vertexBaseCount;
}

DG_INLINE void dgMeshEffect::SetVertexBaseCount(dgInt32 count)
{
	m_vertexBaseCount = count;
}


DG_INLINE dgInt32 dgMeshEffect::GetPropertiesCount() const
{
	return m_attrib.m_pointChannel.m_count;
}

DG_INLINE const dgInt32* dgMeshEffect::GetIndexToVertexMap() const
{
	return &m_attrib.m_pointChannel[0];
}

DG_INLINE dgInt32 dgMeshEffect::GetMaterialID (dgIndexArray* const handle, dgInt32 materialHandle) const
{
	return handle->m_materials[materialHandle];
}

DG_INLINE dgInt32 dgMeshEffect::GetMaterialIndexCount (dgIndexArray* const handle, dgInt32 materialHandle) const
{
	return handle->m_materialsIndexCount[materialHandle];
}

DG_INLINE dgBigVector dgMeshEffect::GetVertex (dgInt32 index) const
{
	dgAssert(index >= 0);
	dgAssert(index < m_points.m_vertex.m_count);
	return m_points.m_vertex[index];
}

DG_INLINE bool dgMeshEffect::HasLayersChannel() const
{
	return m_points.m_layers.m_count != 0;
}

DG_INLINE dgInt32 dgMeshEffect::GetVertexLayer(dgInt32 index) const
{
	dgAssert(index >= 0);
	dgAssert(index < m_points.m_vertex.m_count);
	return (m_points.m_layers.m_count) ? m_points.m_layers[index] : 0;
}


DG_INLINE dgInt32 dgMeshEffect::GetVertexStrideInByte() const 
{
	return sizeof (dgBigVector);
}

DG_INLINE const dgFloat64* dgMeshEffect::GetVertexPool () const 
{
	return &m_points.m_vertex[0].m_x;
}

DG_INLINE dgMeshEffect* dgMeshEffect::GetFirstLayer ()
{
	return GetNextLayer (IncLRU());
}

DG_INLINE dgMeshEffect* dgMeshEffect::GetNextLayer (dgMeshEffect* const layerSegment)
{
	if (!layerSegment) {
		return NULL;
	}
	return GetNextLayer (layerSegment->IncLRU() - 1);
}


DG_INLINE dgFloat64 dgMeshEffect::QuantizeCordinade(dgFloat64 x) const
{
	dgInt32 exp;
	dgFloat64 mantissa = frexp(x, &exp);
	mantissa = DG_MESH_EFFECT_PRECISION_SCALE_INV * floor (mantissa * DG_MESH_EFFECT_PRECISION_SCALE);

	dgFloat64 x1 = ldexp(mantissa, exp);
	return x1;
}

#endif
