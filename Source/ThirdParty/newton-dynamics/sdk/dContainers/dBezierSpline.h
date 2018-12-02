/* Copyright (c) <2003-2016> <Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

#include "dContainersStdAfx.h"
#include "dContainersAlloc.h"

#ifndef __D_BEZIER_SPLINE_H__
#define __D_BEZIER_SPLINE_H__

class dBezierSpline: public dContainersAlloc
{
	public:

	// empty spline
	DCONTAINERS_API dBezierSpline ();
	DCONTAINERS_API dBezierSpline (const dBezierSpline& src);

	// create from knot vector and control points
	DCONTAINERS_API virtual ~dBezierSpline ();

	DCONTAINERS_API dBezierSpline& operator = (const dBezierSpline &copy) ;

	DCONTAINERS_API int GetDegree () const;

	DCONTAINERS_API dBigVector CurvePoint (dFloat64 u) const;
	DCONTAINERS_API dBigVector CurveDerivative (dFloat64 u, int index = 1) const;
	DCONTAINERS_API int CurveAllDerivatives (dFloat64 u, dBigVector* const defivatives) const;

	DCONTAINERS_API dFloat64 CalculateLength (dFloat64 tol) const;

	DCONTAINERS_API void GlobalCubicInterpolation (int count, const dBigVector* const points, const dBigVector& firstTangent, const dBigVector& lastTangent);
	DCONTAINERS_API void CreateFromKnotVectorAndControlPoints (int degree, int knotCount, const dFloat64* const knotVector, const dBigVector* const controlPoints);

	DCONTAINERS_API void InsertKnot (dFloat64 u);
	DCONTAINERS_API bool RemoveKnot (dFloat64 u, dFloat64 tol);

	DCONTAINERS_API int GetControlPointCount() const;
	DCONTAINERS_API dBigVector* GetControlPointArray();
	DCONTAINERS_API const dBigVector* GetControlPointArray() const;
	DCONTAINERS_API dBigVector GetControlPoint(int i) const;
	DCONTAINERS_API void SetControlPoint(int i, const dBigVector& point);
	
	DCONTAINERS_API int GetKnotCount() const;
	DCONTAINERS_API dFloat64* GetKnotArray();
	DCONTAINERS_API const dFloat64* GetKnotArray() const;
	DCONTAINERS_API dFloat64 GetKnot(int i) const;

	DCONTAINERS_API dFloat64 FindClosestKnot (dBigVector& closestPointOnCurve, const dBigVector& point, int subdivitionSteps = 2) const;

	private:
	void Clear();
	int GetSpan(dFloat64 u) const;

	dBigVector CurvePoint (dFloat64 u, int span) const;
	void CreateCubicKnotVector(int count, const dBigVector* const points);
	void CreateCubicControlPoints(int count, const dBigVector* const points, const dBigVector& firstTangent, const dBigVector& lastTangent);

	void BasicsFunctions (dFloat64 u, int span, dFloat64* const functionOut) const;
	void BasicsFunctionsDerivatives (dFloat64 u, int span, dFloat64* const derivatyivesOut) const;
	

	dFloat64* m_knotVector;
	dBigVector* m_controlPoints; 

	int m_degree;
	int m_knotsCount;
	int m_controlPointsCount;
};
#endif

