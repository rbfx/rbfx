/* Copyright (c) <2003-2013> <Julio Jerez, Newton Game Dynamics>
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

#ifndef _D_NEWTON_MESH_H_
#define _D_NEWTON_MESH_H_

#include "dStdAfxNewton.h"
#include "dNewtonAlloc.h"

class dNewton;
class dNewtonCollision;

class dNewtonMesh: public dNewtonAlloc
{
	public:
	class dUV
	{
		public:
		dFloat m_u;
		dFloat m_v;
	};

	class dPoint
	{
		public:
		dFloat m_x;
		dFloat m_y;
		dFloat m_z;
	};

	CNEWTON_API dNewtonMesh(dNewton* const world);
	CNEWTON_API dNewtonMesh(NewtonWorld* const world);
	CNEWTON_API dNewtonMesh(const dNewtonMesh& clone);
	CNEWTON_API dNewtonMesh(const dNewtonCollision& collision);
	CNEWTON_API dNewtonMesh(dNewton* const world, int pointCount, const dFloat* const vertexCloud, int strideInBytes, dFloat tolerance);
	CNEWTON_API virtual ~dNewtonMesh();

	CNEWTON_API NewtonMesh* GetMesh() const;

	// special construction functions
	CNEWTON_API void CreateApproximateConvexDecomposition (const dNewtonMesh& sourceMesh, dFloat maxConcavity, dFloat backFaceDistanceFactor, int maxCount, int maxVertexPerHull);

	CNEWTON_API void BeginBuild();
		CNEWTON_API void BeginPolygon();
			CNEWTON_API void AddPoint (dFloat64 x, dFloat64 y, dFloat64 z);
			CNEWTON_API void AddMaterial (int materialIndex);
			CNEWTON_API void AddLayer (int layer);
			CNEWTON_API void AddNormal (dFloat32 nx, dFloat32 ny, dFloat32 nz);
			CNEWTON_API void AddBiNormal (dFloat32 nx, dFloat32 ny, dFloat32 nz);
			CNEWTON_API void AddUV0 (dFloat32 u, dFloat32 v);
			CNEWTON_API void AddUV1 (dFloat32 u, dFloat32 v);
			CNEWTON_API void AddVertexColor (dFloat32 r, dFloat32 g, dFloat32 b, dFloat32 a);
			CNEWTON_API void AddVertexWeight (int matrixIndex[4], dFloat32 weights[4]);
		CNEWTON_API void EndPolygon();
	CNEWTON_API void EndBuild();

	CNEWTON_API void ApplyTransform (const dFloat* const matrix);

	CNEWTON_API void CreateVoronoiConvexDecomposition (const dNewtonMesh& convexMesh);
	CNEWTON_API int GetPointCount() const;
	CNEWTON_API void GetVertexStreams(dPoint* const posit, dPoint* const normal, dUV* const uv0, dUV* const uv1) const;

	CNEWTON_API int GetTotalIndexCount() const;
	CNEWTON_API int GetTotalFaceCount() const;

	CNEWTON_API void* BeginMaterialHandle () const; 
	CNEWTON_API void EndMaterialHandle (void* const materialHandle) const; 

	CNEWTON_API void ApplySphericalMapping (int matId, const dMatrix& aligment); 
	CNEWTON_API void ApplyCylindricalMapping (int cylinderMatId, int capMatId, const dMatrix& aligment); 
	CNEWTON_API void ApplyBoxMapping (int topMatId, int sideMatId, int frontMatId, const dMatrix& aligment); 

	CNEWTON_API void CalculateVertexNormals (dFloat angleInRadians); 
	
	CNEWTON_API int GetMaterialIndex (void* const materialHandle) const; 
	CNEWTON_API int GetNextMaterialIndex (void* const materialHandle, int materialIndex) const; 

	CNEWTON_API int MaterialGetMaterial (void* const materialHandle, int materialIndex) const; 
	CNEWTON_API int MaterialGetIndexCount (void* const materialHandle, int materialIndex) const; 
	CNEWTON_API void MaterialGetIndexStream (void* const materialHandle, int materialIndex, int* const indexes) const; 

	CNEWTON_API void Polygonize ();
	CNEWTON_API void Triangulate ();

	protected:
	NewtonMesh* m_mesh;
};

#endif
