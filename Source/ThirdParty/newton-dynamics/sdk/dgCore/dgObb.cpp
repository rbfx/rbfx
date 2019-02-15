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

#include "dgStdafx.h"
#include "dgObb.h"
#include "dgDebug.h"
#include "dgPlane.h"
#include "dgMatrix.h"


namespace InternalSphere
{
	const dgFloat32 SPHERE_TOL = 0.002f;	

	static dgFloat32 AspectRatio (dgFloat32 x, dgFloat32 y)
	{
		dgFloat32 tmp;
		x = dgAbs (x);
		y = dgAbs (y);

		if (y < x) {
			tmp = y;
			y = x;
			x = tmp;
		}
		if (y < 1.0e-12)	{
			y = 1.0e-12f;
		}
		return x / y;
	}
	

	static void BoundingBox (const dgMatrix& matrix, const dgFloat32 vertex[], dgInt32 stride, const dgInt32 index[], dgInt32 indexCount, dgVector &min, dgVector &max)
	{
	   dgFloat32 xmin = dgFloat32 (1.0e10f);
	   dgFloat32 ymin = dgFloat32 (1.0e10f);
	   dgFloat32 zmin = dgFloat32 (1.0e10f);

	   dgFloat32 xmax = dgFloat32 (-1.0e10f);
	   dgFloat32 ymax = dgFloat32 (-1.0e10f);
	   dgFloat32 zmax = dgFloat32 (-1.0e10f);

	   const dgFloat32*	const ptr = vertex;
	   for (dgInt32 j = 0 ; j < indexCount; j ++ ) {
			dgInt32 i = index[j] * stride;
			dgVector tmp (ptr[i + 0], ptr[i + 1], ptr[i + 2], dgFloat32 (0.0f));
			tmp = matrix.UnrotateVector (tmp);
			if (tmp.m_x < xmin) xmin = tmp.m_x;
			if (tmp.m_y < ymin) ymin = tmp.m_y;
			if (tmp.m_z < zmin) zmin = tmp.m_z;
			if (tmp.m_x > xmax) xmax = tmp.m_x;
			if (tmp.m_y > ymax) ymax = tmp.m_y;
			if (tmp.m_z > zmax) zmax = tmp.m_z;
	   }
		
	   min = dgVector (xmin, ymin, zmin, dgFloat32 (0.0f));
	   max = dgVector (xmax, ymax, zmax, dgFloat32 (0.0f));
	}

	// Compute axis aligned box
	static void BoundingBox (const dgMatrix &Mat, const dgFloat32 vertex[], dgInt32 vertexCount, dgInt32 stride, dgVector &min, dgVector &max)
	{
		dgFloat32 xmin = dgFloat32 (1.0e10f);
		dgFloat32 ymin = dgFloat32 (1.0e10f);
		dgFloat32 zmin = dgFloat32 (1.0e10f);

		dgFloat32 xmax = dgFloat32 (-1.0e10f);
		dgFloat32 ymax = dgFloat32 (-1.0e10f);
		dgFloat32 zmax = dgFloat32 (-1.0e10f);

		const dgFloat32* ptr = vertex;
		for (dgInt32 i = 0 ; i < vertexCount; i ++ ) {
			dgVector tmp (ptr[0], ptr[1], ptr[2], dgFloat32 (0.0f));
			ptr += stride;
			tmp = Mat.UnrotateVector (tmp);
			if (tmp.m_x < xmin) xmin = tmp.m_x;
			if (tmp.m_y < ymin) ymin = tmp.m_y;
			if (tmp.m_z < zmin) zmin = tmp.m_z;
			if (tmp.m_x > xmax) xmax = tmp.m_x;
			if (tmp.m_y > ymax) ymax = tmp.m_y;
			if (tmp.m_z > zmax) zmax = tmp.m_z;
		}

		min = dgVector (xmin, ymin, zmin, dgFloat32 (0.0f));
		max = dgVector (xmax, ymax, zmax, dgFloat32 (0.0f));
	}


	static void Statistics (dgObb &sphere, dgVector &eigenValues, const dgVector &scale, const dgFloat32 vertex[], const dgInt32 faceIndex[], dgInt32 indexCount, dgInt32 stride)
	{
		dgVector var (dgFloat32 (0.0f));
		dgVector cov (dgFloat32 (0.0f));
		dgVector centre (dgFloat32 (0.0f));
		dgVector massCenter (dgFloat32 (0.0f));

		dgVector scaleVector (scale & dgVector::m_triplexMask);

		dgFloat64 totalArea = dgFloat32 (0.0f);
		const dgFloat32* const ptr = vertex;
		for (dgInt32 i = 0; i < indexCount; i += 3) {
			dgInt32 index = faceIndex[i] * stride;
			dgVector p0 (&ptr[index]);
			p0 = p0 & dgVector::m_triplexMask;
			p0 = p0 * scaleVector;

			index = faceIndex[i + 1] * stride;;
			dgVector p1 (&ptr[index]);
			p1 = p1 & dgVector::m_triplexMask;
			p1 = p1 * scaleVector;

			index = faceIndex[i + 2] * stride;;
			dgVector p2 (&ptr[index]);
			p2 = p2 & dgVector::m_triplexMask;
			p2 = p2 * scaleVector;

			dgVector normal ((p1 - p0).CrossProduct(p2 - p0));
			dgAssert(normal.m_w == dgFloat32(0.0f));
			dgFloat64 area = dgFloat32 (0.5f) * sqrt (normal.DotProduct(normal).GetScalar());

			centre = p0 + p1 + p2;
			centre = centre.Scale (dgFloat32  (1.0f / 3.0f));

			// Inertia of each point in the triangle
			dgFloat64 Ixx = p0.m_x * p0.m_x + p1.m_x * p1.m_x + p2.m_x * p2.m_x;	
			dgFloat64 Iyy = p0.m_y * p0.m_y + p1.m_y * p1.m_y + p2.m_y * p2.m_y;	
			dgFloat64 Izz = p0.m_z * p0.m_z + p1.m_z * p1.m_z + p2.m_z * p2.m_z;	

			dgFloat64 Ixy = p0.m_x * p0.m_y + p1.m_x * p1.m_y + p2.m_x * p2.m_y;	
			dgFloat64 Iyz = p0.m_y * p0.m_z + p1.m_y * p1.m_z + p2.m_y * p2.m_z;	
			dgFloat64 Ixz = p0.m_x * p0.m_z + p1.m_x * p1.m_z + p2.m_x * p2.m_z;	

			if (area > dgEPSILON * 10.0) {
				dgFloat64 K = area / dgFloat64 (12.0);
				//Coriolis theorem for Inertia of a triangle in an arbitrary orientation
				Ixx = K * (Ixx + 9.0 * centre.m_x * centre.m_x);
				Iyy = K * (Iyy + 9.0 * centre.m_y * centre.m_y);
				Izz = K * (Izz + 9.0 * centre.m_z * centre.m_z);

				Ixy = K * (Ixy + 9.0 * centre.m_x * centre.m_y);
				Ixz = K * (Ixz + 9.0 * centre.m_x * centre.m_z);
				Iyz = K * (Iyz + 9.0 * centre.m_y * centre.m_z);
				centre = centre.Scale ((dgFloat32)area);
			} 

			totalArea += area;
			massCenter += centre;
			var += dgVector ((dgFloat32)Ixx, (dgFloat32)Iyy, (dgFloat32)Izz, dgFloat32 (0.0f));
			cov += dgVector ((dgFloat32)Ixy, (dgFloat32)Ixz, (dgFloat32)Iyz, dgFloat32 (0.0f));
		}

		if (totalArea > dgEPSILON * 10.0) {
			dgFloat64 K = dgFloat64 (1.0) / totalArea; 
			var = var.Scale ((dgFloat32)K);
			cov = cov.Scale ((dgFloat32)K);
			massCenter = massCenter.Scale ((dgFloat32)K);
		}

		dgFloat64 Ixx = var.m_x - massCenter.m_x * massCenter.m_x;
		dgFloat64 Iyy = var.m_y - massCenter.m_y * massCenter.m_y;
		dgFloat64 Izz = var.m_z - massCenter.m_z * massCenter.m_z;

		dgFloat64 Ixy = cov.m_x - massCenter.m_x * massCenter.m_y;
		dgFloat64 Ixz = cov.m_y - massCenter.m_x * massCenter.m_z;
		dgFloat64 Iyz = cov.m_z - massCenter.m_y * massCenter.m_z;

		sphere.m_front = dgVector ((dgFloat32)Ixx, (dgFloat32)Ixy, (dgFloat32)Ixz, dgFloat32 (0.0f));
		sphere.m_up    = dgVector ((dgFloat32)Ixy, (dgFloat32)Iyy, (dgFloat32)Iyz, dgFloat32 (0.0f));
		sphere.m_right = dgVector ((dgFloat32)Ixz, (dgFloat32)Iyz, (dgFloat32)Izz, dgFloat32 (0.0f));
		sphere.EigenVectors(eigenValues);
	}

	
	static void Statistics (dgObb& sphere, dgVector &eigenValues, dgVector &scaleVector, const dgFloat32 vertex[], dgInt32 vertexCount, dgInt32 stride)
	{
		dgBigVector var (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		dgBigVector cov (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		dgBigVector massCenter (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
	
		const dgFloat32* ptr = vertex;
		for (dgInt32 i = 0; i < vertexCount; i ++) {
			dgFloat32 x = ptr[0] * scaleVector.m_x;
			dgFloat32 y = ptr[1] * scaleVector.m_y;
			dgFloat32 z = ptr[2] * scaleVector.m_z;
			ptr += stride;
			massCenter += dgBigVector (x, y, z, dgFloat32 (0.0f));
			var += dgBigVector (x * x, y * y, z * z, dgFloat32 (0.0f));
			cov += dgBigVector (x * y, x * z, y * z, dgFloat32 (0.0f));
		}
	
		dgFloat64 k = dgFloat64 (1.0) / vertexCount;
		var = var.Scale (k);
		cov = cov.Scale (k);
		massCenter = massCenter.Scale (k);
	
		dgFloat64 Ixx = var.m_x - massCenter.m_x * massCenter.m_x;
		dgFloat64 Iyy = var.m_y - massCenter.m_y * massCenter.m_y;
		dgFloat64 Izz = var.m_z - massCenter.m_z * massCenter.m_z;
	
		dgFloat64 Ixy = cov.m_x - massCenter.m_x * massCenter.m_y;
		dgFloat64 Ixz = cov.m_y - massCenter.m_x * massCenter.m_z;
		dgFloat64 Iyz = cov.m_z - massCenter.m_y * massCenter.m_z;
	
		sphere.m_front = dgVector (dgFloat32(Ixx), dgFloat32(Ixy), dgFloat32(Ixz), dgFloat32 (0.0f));
		sphere.m_up    = dgVector (dgFloat32(Ixy), dgFloat32(Iyy), dgFloat32(Iyz), dgFloat32 (0.0f));
		sphere.m_right = dgVector (dgFloat32(Ixz), dgFloat32(Iyz), dgFloat32(Izz), dgFloat32 (0.0f));
 		sphere.EigenVectors (eigenValues);
	}

/*
	static void Statistics (
		dgObb &sphere,
		dgVector &eigenValues,
		const dgVector &scaleVector,
		const dgFloat32 vertex[],
		dgInt32 stride,
		const dgFace face[],
		dgInt32 faceCount)
	{
		dgAssert (0);

		dgInt32 i;
		dgInt32 index;
		const dgFloat32 *ptr;
		dgFloat64 K;
		dgFloat64 Ixx;
		dgFloat64 Iyy;
		dgFloat64 Izz;
		dgFloat64 Ixy;
		dgFloat64 Ixz;
		dgFloat64 Iyz;
		dgFloat64 area;
		dgFloat64 totalArea;
		const dgFace *Face;
	
		dgVector var (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		dgVector cov (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		dgVector centre (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		dgVector massCenter (dgFloat32 (0.0f), dgFloat32 (0.0f), dgFloat32 (0.0f));
		
		totalArea = 0.0;
		ptr = vertex;
		for (i = 0; i < faceCount; i ++) {
			 Face = &face[i];
		
			 index = Face->m_wireFrame[0] * stride;
			 dgVector p0 (&ptr[index]);
			 p0 = p0 * scaleVector;
	
			 index = Face->m_wireFrame[1] * stride;
			 dgVector p1 (&ptr[index]);
			 p1 = p1 * scaleVector;
		
			 index = Face->m_wireFrame[2] * stride;
			 dgVector p2 (&ptr[index]);
			 p2 = p2 * scaleVector;
		
			 dgVector normal ((p1 - p0) * (p2 - p0));
		
			 area = 0.5 * sqrt (normal % normal);
		
			 centre = p0 + p1 + p2;
			 centre = centre.Scale (1.0f / 3.0f);
		
			 // Inercia of each point in the triangle
			 Ixx = p0.m_x * p0.m_x + p1.m_x * p1.m_x + p2.m_x * p2.m_x;	
			 Iyy = p0.m_y * p0.m_y + p1.m_y * p1.m_y + p2.m_y * p2.m_y;	
			 Izz = p0.m_z * p0.m_z + p1.m_z * p1.m_z + p2.m_z * p2.m_z;	
		
			 Ixy = p0.m_x * p0.m_y + p1.m_x * p1.m_y + p2.m_x * p2.m_y;	
			 Iyz = p0.m_y * p0.m_z + p1.m_y * p1.m_z + p2.m_y * p2.m_z;	
			 Ixz = p0.m_x * p0.m_z + p1.m_x * p1.m_z + p2.m_x * p2.m_z;	
		
			 if (area > dgEPSILON * 10.0) {
			     K = area / 12.0;
			     //Coriolis theorem for Inertia of a triangle in an arbitrary orientation
			     Ixx = K * (Ixx + 9.0 * centre.m_x * centre.m_x);
			     Iyy = K * (Iyy + 9.0 * centre.m_y * centre.m_y);
			     Izz = K * (Izz + 9.0 * centre.m_z * centre.m_z);
		
			     Ixy = K * (Ixy + 9.0 * centre.m_x * centre.m_y);
			     Ixz = K * (Ixz + 9.0 * centre.m_x * centre.m_z);
			     Iyz = K * (Iyz + 9.0 * centre.m_y * centre.m_z);
			     centre = centre.Scale ((dgFloat32)area);
			 } 
		
			 totalArea += area;
			 massCenter += centre;
			 var += dgVector ((dgFloat32)Ixx, (dgFloat32)Iyy, (dgFloat32)Izz);
			 cov += dgVector ((dgFloat32)Ixy, (dgFloat32)Ixz, (dgFloat32)Iyz);
		}
		
		if (totalArea > dgEPSILON * 10.0) {
			K = 1.0 / totalArea; 
			var = var.Scale ((dgFloat32)K);
			cov = cov.Scale ((dgFloat32)K);
			massCenter = massCenter.Scale ((dgFloat32)K);
		}
	
		Ixx = var.m_x - massCenter.m_x * massCenter.m_x;
		Iyy = var.m_y - massCenter.m_y * massCenter.m_y;
		Izz = var.m_z - massCenter.m_z * massCenter.m_z;
		
		Ixy = cov.m_x - massCenter.m_x * massCenter.m_y;
		Ixz = cov.m_y - massCenter.m_x * massCenter.m_z;
		Iyz = cov.m_z - massCenter.m_y * massCenter.m_z;
	
		sphere.m_front = dgVector ((dgFloat32)Ixx, (dgFloat32)Ixy, (dgFloat32)Ixz);
		sphere.m_up    = dgVector ((dgFloat32)Ixy, (dgFloat32)Iyy, (dgFloat32)Iyz);
		sphere.m_right = dgVector ((dgFloat32)Ixz, (dgFloat32)Iyz, (dgFloat32)Izz);
		sphere.EigenVectors(eigenValues);
	}
*/
}



void dgObb::SetDimensions (const dgFloat32 vertex[], dgInt32 strideInBytes, const dgInt32 triangles[], dgInt32 indexCount, const dgMatrix *basis)
{
	dgVector eigen;
	dgVector scaleVector (dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (1.0f), dgFloat32 (0.0f));

	if (indexCount < 3) {
		return;
	}

	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));
	if (!basis)	{

		InternalSphere::Statistics (*this, eigen, scaleVector, vertex, triangles, indexCount, stride);

		dgInt32 k = 0;
		for (dgInt32 i = 0; i < 3; i ++) {
			if (k >= 6) {
				 break;
			}
			for (dgInt32 j = i + 1; j < 3; j ++) {
				dgFloat32 aspect = InternalSphere::AspectRatio (eigen[i], eigen[j]);
				if (aspect > dgFloat32 (0.9f)) {
					scaleVector[i] *= dgFloat32 (2.0f); 
					InternalSphere::Statistics (*this, eigen, scaleVector, vertex, triangles, indexCount, stride);
					k ++;
					i = -1;
					break;
				}
			}
		}
	} else {
		*this = *basis;
	}

	dgVector min; 
	dgVector max;
	InternalSphere::BoundingBox (*this, vertex, stride, triangles, indexCount, min, max);

	dgVector massCenter	((max + min) * dgVector::m_half);
	//massCenter = massCenter.Scale (dgFloat32 (0.5f));
	m_posit = TransformVector (massCenter);

	dgVector dim ((max - min) * dgVector::m_half);
	//dim = dim.Scale (dgFloat32(0.5f));
	SetDimensions (dim.m_x, dim.m_y, dim.m_z);
}


void dgObb::SetDimensions (const dgFloat32 vertex[], dgInt32 strideInBytes, dgInt32 count, const dgMatrix *basis) 
{
	dgVector eigen;
	dgVector scaleVector (dgFloat32(1.0f), dgFloat32(1.0f), dgFloat32(1.0f), dgFloat32 (0.0f));

	dgInt32 stride = dgInt32 (strideInBytes / sizeof (dgFloat32));
	if (!basis)	{
		InternalSphere::Statistics (*this, eigen, scaleVector, vertex, count, stride);

		dgInt32 k = 0;
		for (dgInt32 i = 0; i < 3; i ++) {
			if (k >= 6) {
				 break;
			}
			for (dgInt32 j = i + 1; j < 3; j ++) {
				dgFloat32 aspect = InternalSphere::AspectRatio (eigen[i], eigen[j]);
				if (aspect > dgFloat32 (0.9f)) {
					scaleVector[i] *= dgFloat32 (2.0f); 
					InternalSphere::Statistics (*this, eigen, scaleVector, vertex, count, stride);
					k ++;
					i = -1;
					break;
				}
			}
		}
	} else {
		*this = *basis;
	}
		
   dgVector min; 
   dgVector max;
   InternalSphere::BoundingBox (*this, vertex, count, stride, min, max);

	dgVector massCenter ((max + min) * dgVector::m_half);
	//massCenter = massCenter.Scale (0.5);
	m_posit = TransformVector (massCenter);

	dgVector dim ((max - min) * dgVector::m_half);
	//dim = dim.Scale (dgFloat32(0.5f));
	SetDimensions (dim.m_x + InternalSphere::SPHERE_TOL, 
				   dim.m_y + InternalSphere::SPHERE_TOL, 
				   dim.m_z + InternalSphere::SPHERE_TOL);
}

/*
void dgObb::SetDimensions (
	const dgFloat32 vertex[],  
	dgInt32 strideInBytes,
	const dgInt32 index[],
	dgInt32 indexCount,
	const dgMatrix *basis)
{
	dgInt32 i;
	dgInt32 j;
	dgInt32 k;
	dgInt32 stride;
	dgFloat32 aspect;
	dgVector eigen;
	dgVector scaleVector (dgFloat32(1.0f), dgFloat32(1.0f), dgFloat32(1.0f), dgFloat32 (0.0f));

	stride = strideInBytes / sizeof (dgFloat32);
	if (!basis)	{
		InternalSphere::Statistics (*this, eigen, scaleVector, vertex, index, indexCount, stride);

		k = 0;
		for (i = 0; i < 3; i ++) {
			if (k >= 6) {
				 break;
			}
			for (j = i + 1; j < 3; j ++) {
				aspect = InternalSphere::AspectRatio (eigen[i], eigen[j]);
				if (aspect > dgFloat32 (0.9f)) {
					scaleVector[i] *= dgFloat32 (2.0f); 
					InternalSphere::Statistics (*this, eigen, scaleVector, vertex, index, indexCount, stride);
					i = -1;
					k ++;
					break;
				}
			}
		}
	} else {
		*this = *basis;
	}
		
   dgVector min; 
   dgVector max;
   InternalSphere::BoundingBox (*this, vertex, stride, index, indexCount, min, max);

	dgVector massCenter (max + min);
	massCenter = massCenter.Scale (dgFloat32(0.5f));
	m_posit = TransformVector (massCenter);

	dgVector dim (max - min);
	dim = dim.Scale (dgFloat32(0.5f));
	SetDimensions (dim.m_x + InternalSphere::SPHERE_TOL, 
				   dim.m_y + InternalSphere::SPHERE_TOL, 
				   dim.m_z + InternalSphere::SPHERE_TOL);
}
*/



/*
dgObb::dgObb (
	const dgObb &dgObb, 
	const dgVector &Dir)
{
	if ((Dir % Dir) < EPSILON * 0.01f) {
		*this = dgObb;
		return;
	}

	front = Dir;
	front.Fast_Normalize(); 

	if (dgAbs (front % dgObb.right) < 0.995) {
		up = front * dgObb.right;
		up.Fast_Normalize(); 
	} else {
		up	= dgObb.up;
	}
	right = up * front;

	dgVector Step (Dir.Scale(0.5));
	size.m_x = (dgFloat32)(dgObb.size.m_x * dgAbs (right % dgObb.right) +
						 dgObb.size.m_y * dgAbs (right % dgObb.up) +
						 dgObb.size.m_z * dgAbs (right % dgObb.front));

	size.m_y = (dgFloat32)(dgObb.size.m_x * dgAbs (up % dgObb.right) +
						  dgObb.size.m_y * dgAbs (up % dgObb.up) +
						  dgObb.size.m_z * dgAbs (up % dgObb.front));

	size.m_z = (dgFloat32)(sqrt (Step % Step) +
							dgObb.size.m_x * dgAbs (front % dgObb.right) +
							dgObb.size.m_y * dgAbs (front % dgObb.up) +
							dgObb.size.m_z * dgAbs (front % dgObb.front));
	posit = dgObb.posit + Step;

}



bool dgObb::dgObb_Overlap_Test (const dgObb &dgObb)
{
	dgFloat64 R;
	dgVector Dir (dgObb.posit - posit);

  	R = size.m_x * dgAbs (right % Dir) + dgObb.size.m_x * dgAbs (dgObb.right % Dir) + 
	    size.m_y * dgAbs (up % Dir) + dgObb.size.m_y * dgAbs (dgObb.up % Dir) + 
		 size.m_z * dgAbs (front %Dir) + dgObb.size.m_z * dgAbs (dgObb.front % Dir);
	if (R	< (Dir % Dir)) {
		return false;
	}

	R = size.m_x * dgAbs (right % dgObb.right) + 
	    size.m_y * dgAbs (up % dgObb.right) + 
		 size.m_z * dgAbs (front % dgObb.right) + dgObb.size.m_x;
	if (R	< dgAbs (Dir % dgObb.right)) {
		return false;
	}

	R = size.m_x * dgAbs (right % dgObb.up) + 
	    size.m_y * dgAbs (up % dgObb.up) + 
		 size.m_z * dgAbs (front % dgObb.up) + dgObb.size.m_y;
	if (R	< dgAbs (Dir % dgObb.up)) {
		return false;
	}

	R = size.m_x * dgAbs (right % dgObb.front) + 
	    size.m_y * dgAbs (up % dgObb.front) + 
		 size.m_z * dgAbs (front % dgObb.front) + dgObb.size.m_z; 
	if (R	< dgAbs (Dir % dgObb.front)) {
		return false;
	}

	R = dgObb.size.m_x * dgAbs (dgObb.right % right) + 
	    dgObb.size.m_y * dgAbs (dgObb.up % right) + 
		 dgObb.size.m_z * dgAbs (dgObb.front % right) + size.m_x;
	if (R	< dgAbs (Dir % right)) {
		return false;
	}

	R = dgObb.size.m_x * dgAbs (dgObb.right % up) + 
	    dgObb.size.m_y * dgAbs (dgObb.up % up) + 
		 dgObb.size.m_z * dgAbs (dgObb.front % up) + size.m_y;
	if (R	< dgAbs (Dir % up)) {
		return false;
	}

	R = dgObb.size.m_x * dgAbs (dgObb.right % front) + 
	    dgObb.size.m_y * dgAbs (dgObb.up % front) + 
		 dgObb.size.m_z * dgAbs (dgObb.front % front) + size.m_z;
	if (R	< dgAbs (Dir % front)) {
		return false;
	}

	return true;
}


void dgObb::Swept_Volume (
	dgVector &min, 
	dgVector &max)
{
	dgFloat32 w;
	dgFloat32 h;
	dgFloat32 b;

	w = (dgFloat32)(size.m_x * dgAbs(right.m_x) + size.m_y * dgAbs(up.m_x) + size.m_z * dgAbs(front.m_x));  
	h = (dgFloat32)(size.m_x * dgAbs(right.m_y) + size.m_y * dgAbs(up.m_y) + size.m_z * dgAbs(front.m_y));  
	b = (dgFloat32)(size.m_x * dgAbs(right.m_z) + size.m_y * dgAbs(up.m_z) + size.m_z * dgAbs(front.m_z));  

	min.m_x = posit.m_x - w;
	min.m_y = posit.m_y - h;
	min.m_z = posit.m_z - b;

	max.m_x = posit.m_x + w;
	max.m_y = posit.m_y + h;
	max.m_z = posit.m_z + b;
}
*/


/*
dgInt32 dgObb::FrontTest (
	const dgMatrix& matrix, 
	const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x) + m_size.m_y * dgAbs (matrix.m_up.m_x) + m_size.m_z * dgAbs (matrix.m_right.m_x);
	R = plane[5].m_x * matrix.m_posit.m_x + plane[5].m_w; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = flag0.i >> 30 & 2;
	flag1.i = flag1.i >> 31 & 1;
	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}

dgInt32 dgObb::RearTest (const dgMatrix& matrix, const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x) + m_size.m_y * dgAbs (matrix.m_up.m_x) + m_size.m_z * dgAbs (matrix.m_right.m_x);
	R = plane[4].m_x * matrix.m_posit.m_x + plane[4].m_w; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = flag0.i >> 30 & 2;
	flag1.i = flag1.i >> 31 & 1;
	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}


dgInt32 dgObb::LeftTest (const dgMatrix& matrix, const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x * plane[0].m_x + matrix.m_front.m_z * plane[0].m_z) + 
		 m_size.m_y * dgAbs (matrix.m_up.m_x    * plane[0].m_x + matrix.m_up.m_z    * plane[0].m_z) +
		 m_size.m_z * dgAbs (matrix.m_right.m_x * plane[0].m_x + matrix.m_right.m_z * plane[0].m_z);
	R = plane[0].m_x * matrix.m_posit.m_x + plane[0].m_z * matrix.m_posit.m_z; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = (flag0.i >> 30) & 2;
	flag1.i = (flag1.i >> 31) & 1;
	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}

dgInt32 dgObb::RightTest (const dgMatrix& matrix, const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x * plane[1].m_x + matrix.m_front.m_z * plane[1].m_z) + 
		 m_size.m_y * dgAbs (matrix.m_up.m_x    * plane[1].m_x + matrix.m_up.m_z    * plane[1].m_z) +
		 m_size.m_z * dgAbs (matrix.m_right.m_x * plane[1].m_x + matrix.m_right.m_z * plane[1].m_z);
	R = plane[1].m_x * matrix.m_posit.m_x + plane[1].m_z * matrix.m_posit.m_z; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = (flag0.i >> 30) & 2;
	flag1.i = (flag1.i >> 31) & 1;
	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}

dgInt32 dgObb::BottomTest (const dgMatrix& matrix, const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x * plane[2].m_x + matrix.m_front.m_y * plane[2].m_y) + 
		 m_size.m_y * dgAbs (matrix.m_up.m_x    * plane[2].m_x + matrix.m_up.m_y    * plane[2].m_y) +
		 m_size.m_z * dgAbs (matrix.m_right.m_x * plane[2].m_x + matrix.m_right.m_y * plane[2].m_y);

	R = plane[2].m_x * matrix.m_posit.m_x + plane[2].m_y * matrix.m_posit.m_y; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = (flag0.i >> 30) & 2;
	flag1.i = (flag1.i >> 31) & 1;

	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}

dgInt32 dgObb::TopTest (const dgMatrix& matrix, const dgPlane* plane) const
{
	dgFloat32 R;
	dgFloat32 dR;
	InternalSphere::dgFloatSign flag0;
	InternalSphere::dgFloatSign flag1;

	dR = m_size.m_x * dgAbs (matrix.m_front.m_x * plane[3].m_x + matrix.m_front.m_y * plane[3].m_y) + 
		 m_size.m_y * dgAbs (matrix.m_up.m_x    * plane[3].m_x + matrix.m_up.m_y    * plane[3].m_y) +
		 m_size.m_z * dgAbs (matrix.m_right.m_x * plane[3].m_x + matrix.m_right.m_y * plane[3].m_y);

	R = plane[3].m_x * matrix.m_posit.m_x + plane[3].m_y * matrix.m_posit.m_y; 

	flag0.f = R + dR;
	flag1.f = R - dR;
	flag0.i = (flag0.i >> 30) & 2;
	flag1.i = (flag1.i >> 31) & 1;
	return InternalSphere::CodeTbl[flag0.i | flag1.i];
}



dgInt32 dgObb::VisibilityTestLow (
	const dgCamera* camera,
	const dgMatrix& matrix) const
{
	dgInt32 i;
	dgInt32 code;
	const dgPlane* planes;
	const dgPlane* guardPlanes;

	planes = camera->GetViewVolume();

	code = (this->*planeTest) (matrix, planes);
	if (code != -1) {
		for (i = 0; i < 6; i ++) {
			code |= (this->*planeTestArray[i]) (matrix, planes);
			if (code == -1) {
				planeTest = planeTestArray[i];
				return -1;
			}
		}

		if (code) {
			guardPlanes = camera->GetGuardViewVolume();
			if (guardPlanes) {
				code = 0;
				for (i = 0; i < 6; i ++) {
					code |= (this->*planeTestArray[i]) (matrix, guardPlanes);
					dgAssert (code >= 0);
					if (code) {
						return code;
					}
				}
			}
		}
	}

   return code;
}


dgInt32 dgObb::VisibilityTest (const dgCamera* camera) const
{
	dgMatrix viewMatrix (*this * camera->GetViewMatrix());
	return VisibilityTestLow (camera, viewMatrix);
}

dgInt32 dgObb::VisibilityTest (const dgCamera* camera, const dgMatrix &worldMatrix) const 
{
	dgMatrix viewMatrix (*this * worldMatrix * camera->GetViewMatrix());
	return VisibilityTestLow (camera, viewMatrix);
}

void dgObb::Render (
	const dgCamera* camera, 
	const dgMatrix &worldMatrix, 
	dgUnsigned32 rgb) const
{
	dgInt32 i;
	struct ColorVertex
	{
		dgFloat32 m_x;
		dgFloat32 m_y;
		dgFloat32 m_z;
		dgColor m_color;
	};

	dgUnsigned32 index [][2] = {
		{0, 4}, {1, 5}, {2, 6}, {3, 7},
		{0, 1}, {4, 5}, {7, 6}, {3, 2},
		{1, 2}, {5, 6}, {4, 7}, {0, 3},
	};

	ColorVertex* ptr;
	ColorVertex box[8];

	box[0].m_x = -size.m_x;
	box[0].m_y = -size.m_y;
	box[0].m_z = -size.m_z;
	box[0].m_color.m_val = rgb;

	box[1].m_x =  size.m_x;
	box[1].m_y = -size.m_y;
	box[1].m_z = -size.m_z;
	box[1].m_color.m_val = rgb;

	box[2].m_x =  size.m_x;
	box[2].m_y = -size.m_y;
	box[2].m_z =  size.m_z;
	box[2].m_color.m_val = rgb;

	box[3].m_x = -size.m_x;
	box[3].m_y = -size.m_y;
	box[3].m_z =  size.m_z;
	box[3].m_color.m_val = rgb;

	box[4].m_x = -size.m_x;
	box[4].m_y =  size.m_y;
	box[4].m_z = -size.m_z;
	box[4].m_color.m_val = rgb;

	box[5].m_x =  size.m_x;
	box[5].m_y =  size.m_y;
	box[5].m_z = -size.m_z;
	box[5].m_color.m_val = rgb;

	box[6].m_x =  size.m_x;
	box[6].m_y =  size.m_y;
	box[6].m_z =  size.m_z;
	box[6].m_color.m_val = rgb;

	box[7].m_x = -size.m_x;
	box[7].m_y =  size.m_y;
	box[7].m_z =  size.m_z;
	box[7].m_color.m_val = rgb;

	dgRenderDescriptorParams param;
	param.m_indexCount = 0;
	param.m_vertexCount = sizeof (index) / sizeof (dgInt32); 
	param.m_descType = dgDynamicVertex;
	param.m_primitiveType = RENDER_LINELIST;
	param.m_vertexFlags = VERTEX_ENABLE_XYZ | COLOR_ENABLE;

	dgRenderDescriptor desc (param);

	dgMatrix tmpMat (*this * worldMatrix);
	camera->SetWorldMatrix (&tmpMat);

	desc.m_material = dgMaterial::UseDebugMaterial();

  	dgVertexRecord vertexRecord (desc.LockVertex());
	ptr = (ColorVertex*) vertexRecord.vertex.ptr;
	for (i = 0; i < (sizeof (index) / (2 * sizeof (dgUnsigned32))); i ++) {
		 ptr[0] = box[index[i][0]]; 
		 ptr[1] = box[index[i][1]]; 
		 ptr += 2;
	}
	desc.UnlockVertex();

	camera->Render (desc);

	desc.m_material->Release();
}
*/

