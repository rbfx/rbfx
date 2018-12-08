/////////////////////////////////////////////////////////////////////////////
// Name:        dPluginStdafx.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 07:45:05
// RCS-ID:      
// Copyright:   Copyright (c) <2010> <Newton Game Dynamics>
// License:     
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
// 
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely
/////////////////////////////////////////////////////////////////////////////



#include "dSceneStdafx.h"
#include <tinyxml.h>

#ifdef _DSCENE_DLL
	void* operator new (size_t size) 
	{ 
		return dContainersAlloc::Alloc(size);
	}

	void operator delete (void* ptr) 
	{ 
		dContainersAlloc::Free (ptr);
	}

	BOOL APIENTRY DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
	{
		switch (ul_reason_for_call)
		{
			case DLL_PROCESS_ATTACH:
			case DLL_THREAD_ATTACH:
				// check for memory leaks
				#if defined(_DEBUG) && defined(_MSC_VER)
					// Track all memory leaks at the operating system level.
					// make sure no Newton tool or utility leaves leaks behind.
					_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF|_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF));
				#endif

			case DLL_THREAD_DETACH:
			case DLL_PROCESS_DETACH:
				break;
		}
		return TRUE;
	}
#endif


void dIntArrayToString (const int* const array, int count, char* const string, int maxSixeInBytes)
{
	if (count) {
		char* ptr = string;
		sprintf (string, " ");
		for (int i = 0; i < count; i ++) {
			sprintf (ptr, "%d ", array[i]);
			ptr += strlen (ptr);
			dAssert ((ptr - string) < maxSixeInBytes);
		}
		string[ptr - string - 1] = 0;
	}
}

void dStringToIntArray(const char* const string, int* const array, int maxCount)
{
	const char* ptr = string;
	for (int i = 0; i < maxCount; i++) {
		char value[128];
		while (*ptr == ' ') {
			ptr++;
		}
		int j = 0;
		while (*ptr != ' ' && *ptr) {
			value[j] = *ptr;
			ptr++;
			j++;
		}
		value[j] = 0;
		int val = atoi(value);
		array[i] = val;
	}
}

void dFloatArrayToString (const dFloat* const array, int count, char* const string, int maxSixeInBytes)
{
	if (count) {
		char* ptr = string;
		sprintf (string, " ");
		for (int i = 0; i < count; i ++) {
			sprintf (ptr, "%f ", array[i]);
			ptr += strlen (ptr);
			dAssert ((ptr - string) < maxSixeInBytes);
		}
		string[ptr - string - 1] = 0;
	}
}

void dStringToFloatArray (const char* const string, dFloat* const array, int maxCount)
{
	const char* ptr = string;
	for (int i = 0; i < maxCount; i ++) {
		char value[128];
		while (*ptr == ' ') {
			ptr ++;
		}
		int j = 0;
		while (*ptr != ' ' && *ptr) {
			value[j] = *ptr;
			ptr ++;
			j ++;
		}
		value[j] = 0;
		dFloat val = dFloat (atof (value));
		array[i] = val;
	}
}

#ifndef _NEWTON_USE_DOUBLE
void dFloatArrayToString (const dFloat64* const array, int count, char* const string, int maxSixeInBytes)
{
	if (count) {
		char* ptr = string;
		sprintf (string, " ");
		for (int i = 0; i < count; i ++) {
			sprintf (ptr, "%f ", array[i]);
			ptr += strlen (ptr);
			dAssert ((ptr - string) < maxSixeInBytes);
		}
		string[ptr - string - 1] = 0;
	}
}

void dStringToFloatArray (const char* const string, dFloat64* const array, int maxCount)
{
	const char* ptr = string;
	for (int i = 0; i < maxCount; i ++) {
		char value[128];
		while (*ptr == ' ') {
			ptr ++;
		}
		int j = 0;
		while (*ptr != ' ' && *ptr) {
			value[j] = *ptr;
			ptr ++;
			j ++;
		}
		value[j] = 0;
		dFloat64 val = atof (value);
		array[i] = val;
	}
}
#endif

static int SortVertexArray (const void *A, const void *B) 
{
	const dFloat* const vertexA = (dFloat*) A;
	const dFloat* const vertexB = (dFloat*) B;

	if (vertexA[0] < vertexB[0]) {
		return -1;
	} else if (vertexA[0] > vertexB[0]) {
		return 1;
	} else {
		return 0;
	}
}


int dPackVertexArray (dFloat* const vertexList, int compareElements, int strideInBytes, int vertexCount, int* const indexListOut)
{
	int stride = strideInBytes / sizeof (dFloat);
	dFloat errorTol = dFloat (1.0e-4f);

	dFloat* const array = new dFloat[(stride + 2) * vertexCount];
	for (int i = 0; i < vertexCount; i ++) {
		memcpy (&array[i * (stride + 2)], &vertexList[i * stride], strideInBytes);
		array[i * (stride + 2) + stride + 0] = dFloat(i);
		array[i * (stride + 2) + stride + 1] = 0.0f;
	}

	qsort(array, vertexCount, (stride + 2) * sizeof (dFloat), SortVertexArray);
	int indexCount = 0;
	for (int i = 0; i < vertexCount; i ++) {
		int index = i * (stride + 2);
		if (array[index + stride + 1] == 0.0f) {
			dFloat window = array[index] + errorTol; 
			for (int j = i + 1; j < vertexCount; j ++) {
				int index2 = j * (stride + 2);
				if (array[index2] >= window) {
					break;
				}
				if (array[index2 + stride + 1] == 0.0f) {
					int k;
					for (k = 0; k < compareElements; k ++) {
						dFloat error;
						error = array[index + k] - array[index2+ k];
						if (dAbs (error) >= errorTol) {
							break;
						}
					}
					if (k >= compareElements) {
						int m = int (array[index2 + stride]);
						memcpy (&array[indexCount * (stride + 2)], &array[index2], sizeof (dFloat) * stride);
						indexListOut[m] = indexCount;
						array[index2 + stride + 1] = 1.0f;
					}
				}
			}
			int m = int (array[index + stride]);
			memcpy (&array[indexCount * (stride + 2)], &array[index], sizeof (dFloat) * stride);
			indexListOut[m] = indexCount;
			array[indexCount * (stride + 2) + stride + 1] = 1.0f;
			indexCount ++;
		}
	}

	for (int i = 0; i < indexCount; i ++) {
		memcpy (&vertexList[i * stride], &array[i * (stride + 2)], sizeof (dFloat) * stride);
	}

	delete[] array;
	return indexCount;
}

const char* dGetNameFromPath (const char* const fullName)
{
	const char* ptr = strrchr ((char*) fullName, '\\');
	if (!ptr) {
		ptr = strrchr ((char*) fullName, '/');
	}
	if (ptr) {
		ptr ++;
	} else {
		ptr = fullName;
	}
	return ptr;
}

void dExtractPathFromFullName (const char* const fullName, char* const path)
{
	strcpy (path, fullName);
	char* ptr = (char*)dGetNameFromPath(path);
	if (ptr != path) {
		ptr --;
	}
	ptr[0] = 0;
}


dFloat dBoxRayCast (const dVector& line0, const dVector& line1, const dVector& boxP0, const dVector& boxP1) 
{	
	int index = 0;
	dFloat signDir = dFloat (0.0f);
	dFloat tmin = dFloat (0.0f);
	dFloat tmax = dFloat (1.0f);

	for (int i = 0; i < 3; i++) {
		dFloat dp = line1[i] - line0[i];

		if (dAbs (dp) < dFloat (1.0e-8f)) {
			if (line0[i] <= boxP0[i] || line0[i] >= boxP1[i]) {
				return dFloat (1.2f);
			}
		} else {

			dp = dFloat (1.0f) / dp; 
			dFloat t1 = (boxP1[i] - line0[i]) * dp;
			dFloat t2 = (boxP0[i] - line0[i]) * dp;

			dFloat sign = dFloat (-1.0f);
			if (t1 > t2) {
				dFloat t = t1;
				t1 = t2;
				t2 = t;
				sign = 1;
			}

			if (t1 > tmin) {
				signDir = sign;
				index = i;
				tmin = t1;
			}
			if (t2 < tmax) {
				tmax = t2;
			}
			if (tmin > tmax) {
				return dFloat (1.2f);
			}
		}
	}

	if (tmin <= 0.0f) {
		tmin = 1.2f;
	}

	return tmin;
}

dBigVector dPolygonNormal (int indexCount, const dFloat64* const vertex, int strideInBytes, const int* const indices)
{
	int stride = strideInBytes / sizeof (dFloat64);
	int index = indices[0] * stride;
	dBigVector p0 (vertex[index], vertex[index + 1], vertex[index + 2], dFloat64(0.0));

	index = indices[1] * stride;
	dBigVector p1 (vertex[index], vertex[index + 1], vertex[index + 2], dFloat64(0.0));

	dBigVector e0 (p1 - p0);
	dBigVector normal (0.0, 0.0, 0.0, 0.0);
	for (int i = 2; i < indexCount; i ++) {
		index = indices[i] * stride;
		dBigVector p2 (vertex[index], vertex[index + 1], vertex[index + 2], dFloat64(0.0));
		dBigVector e1 (p2 - p0);
		normal += e0.CrossProduct(e1);
		e0 = e1;
	}
	return normal;
}


dFloat dPolygonRayCast (const dVector& l0, const dVector& l1, int indexCount, const dFloat64* const vertex, int strideInBytes, const int* const indices)
{
	int stride = strideInBytes / sizeof (dFloat64);

	dBigVector line0 (l0);
	dBigVector line1(l1);
	dBigVector segment (line1 - line0);
	dBigVector normal (dPolygonNormal (indexCount, vertex, strideInBytes, indices));
	double den = normal.DotProduct3(segment);
	if (dAbs(den) < 1.0e-15) {
		return 1.2f;
	}
	
	double sign = (den < 0.0f) ? 1.0f : -1.0f;

	int index = indices[indexCount - 1] * stride;
	dBigVector v0 (vertex[index], vertex[index + 1], vertex[index + 2], dFloat64(0.0));
	dBigVector p0v0 (v0 - line0);
	for (int i = 0; i < indexCount; i ++) {
		index = indices[i] * stride;
		dBigVector v1 (vertex[index], vertex[index + 1], vertex[index + 2], dFloat64(0.0));
		dBigVector p0v1 (v1 - line0);
		//double alpha = sign * ((p0v1.CrossProduct(p0v0)).DotProduct3(segment));
		double alpha = sign * segment.DotProduct3(p0v1.CrossProduct(p0v0));
		if (alpha < 1.0e-15f) {
			return 1.2f;
		}
		p0v0 = p0v1;
	}

	double t = - ((line0 - v0).DotProduct3(normal)) / den;
	if ((t < 0.0f) || (t > 1.0f)) {
		return 1.2f;
	}
	return dFloat (t);
}

void dRayToRayCast (const dVector& ray_p0, const dVector& ray_p1, const dVector& ray_q0, const dVector& ray_q1, dVector& pOut, dVector& qOut)
{
	dVector u (ray_p1 - ray_p0);
	dVector v (ray_q1 - ray_q0);
	dVector w (ray_p0 - ray_q0);

	dFloat a = u.DotProduct3(u);        // always >= 0
	dFloat b = u.DotProduct3(v);
	dFloat c = v.DotProduct3(v);        // always >= 0
	dFloat d = u.DotProduct3(w);
	dFloat e = v.DotProduct3(w);
	dFloat D = a*c - b*b;   // always >= 0
	dFloat sD = D;			// sc = sN / sD, default sD = D >= 0
	dFloat tD = D;			// tc = tN / tD, default tD = D >= 0

	// compute the line parameters of the two closest points
	dFloat sN;
	dFloat tN;
	if (D < dFloat (1.0e-8f)) { 
		// the lines are almost parallel
		sN = dFloat (0.0f);  // force using point P0 on segment S1
		sD = dFloat (1.0f);  // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	} else {                
		// get the closest points on the infinite lines
		sN = (b*e - c*d);
		tN = (a*e - b*d);
		if (sN < dFloat (0.0f)) {       
			// sc < 0 => the s=0 edge is visible
			sN = dFloat (0.0f);
			tN = e;
			tD = c;
		}
		else if (sN > sD) {  // sc > 1 => the s=1 edge is visible
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}


	if (tN < dFloat (0.0f)) {           // tc < 0 => the t=0 edge is visible
		tN = dFloat (0.0f);
		// recompute sc for this edge
		if (-d < dFloat (0.0f))
			sN = dFloat (0.0f);
		else if (-d > a)
			sN = sD;
		else {
			sN = -d;
			sD = a;
		}
	}
	else if (tN > tD) {      // tc > 1 => the t=1 edge is visible
		tN = tD;
		// recompute sc for this edge
		if ((-d + b) < dFloat (0.0f))
			sN = dFloat (0.0f);
		else if ((-d + b) > a)
			sN = sD;
		else {
			sN = (-d + b);
			sD = a;
		}
	}

	// finally do the division to get sc and tc
	dFloat sc = (dAbs(sN) < dFloat(1.0e-8f) ? dFloat (0.0f) : sN / sD);
	dFloat tc = (dAbs(tN) < dFloat(1.0e-8f) ? dFloat (0.0f) : tN / tD);

	pOut = ray_p0 + u.Scale (sc);
	qOut = ray_q0 + v.Scale (tc);
}


void SerializeMesh (const NewtonMesh* const mesh, TiXmlElement* const rootNode)
{
	TiXmlElement* pointElement = new TiXmlElement ("points");
	rootNode->LinkEndChild(pointElement);

	int pointCount = NewtonMeshGetPointCount(mesh);
	int vertexCount = NewtonMeshGetVertexCount(mesh);

	int bufferCount = dMax (vertexCount, pointCount);
	int bufferSizeInBytes = bufferCount * sizeof (dFloat) * 4 * 12;
	char* const buffer = new char[bufferSizeInBytes];
	dFloat* const points = new dFloat[4 * bufferCount];
	int* const vertexIndexList = new int[bufferCount];
	int* const normalIndexList = new int[bufferCount];
	int* const uv0IndexList = new int[bufferCount];
	int* const uv1IndexList = new int[bufferCount];
	
	// pack the vertex Array
	{
		NewtonMeshGetVertexChannel(mesh, 4 * sizeof(dFloat), points);
		for (int i = 0; i < pointCount; i++) {
			points[4 * i + 3] = 0.0f;
			//vertexIndexList[i] = i;
		}
		int count = dPackVertexArray(points, 4, 4 * sizeof(dFloat), pointCount, vertexIndexList);
		dFloatArrayToString(points, count * 4, buffer, bufferSizeInBytes);

		TiXmlElement* const position = new TiXmlElement("position");
		pointElement->LinkEndChild(position);
		position->SetAttribute("float4", count);
		position->SetAttribute("floats", buffer);
	}

	// pack the normal array
	if (NewtonMeshHasNormalChannel(mesh)) {
		NewtonMeshGetNormalChannel(mesh, 3 * sizeof(dFloat), points);
		int count = dPackVertexArray(points, 3, 3 * sizeof(dFloat), pointCount, normalIndexList);
		dFloatArrayToString(points, count * 3, buffer, bufferSizeInBytes);

		TiXmlElement* const normal = new TiXmlElement("normal");
		pointElement->LinkEndChild(normal);
		normal->SetAttribute("float3", count);
		normal->SetAttribute("floats", buffer);
	}

	if (NewtonMeshHasUV0Channel(mesh)) {
		memset(points, 0, 3 * sizeof(dFloat) * pointCount);
		NewtonMeshGetUV0Channel(mesh, 3 * sizeof(dFloat), points);
		int count = dPackVertexArray(points, 3, 3 * sizeof(dFloat), pointCount, uv0IndexList);
		for (int i = 0; i < pointCount; i++) {
			points[i * 2 + 0] = points[i * 3 + 0];
			points[i * 2 + 1] = points[i * 3 + 1];
		}
		dFloatArrayToString(points, count * 2, buffer, bufferSizeInBytes);

		TiXmlElement* const uv0 = new TiXmlElement("uv0");
		pointElement->LinkEndChild(uv0);
		uv0->SetAttribute("float2", count);
		uv0->SetAttribute("floats", buffer);
	}

	if (NewtonMeshHasUV1Channel(mesh)) {
		memset(points, 0, 3 * sizeof(dFloat) * pointCount);
		NewtonMeshGetUV1Channel(mesh, 3 * sizeof(dFloat), points);
		int count = dPackVertexArray(points, 3, 3 * sizeof(dFloat), pointCount, uv1IndexList);
		for (int i = 0; i < pointCount; i++) {
			points[i * 2 + 0] = points[i * 3 + 0];
			points[i * 2 + 1] = points[i * 3 + 1];
		}
		dFloatArrayToString(points, count * 2, buffer, bufferSizeInBytes);

		TiXmlElement* const uv1 = new TiXmlElement("uv1");
		pointElement->LinkEndChild(uv1);
		uv1->SetAttribute("float2", count);
		uv1->SetAttribute("floats", buffer);
	}

	if (NewtonMeshHasVertexColorChannel(mesh)) {
		dAssert(0);
	}

	if (NewtonMeshHasBinormalChannel(mesh)) {
		dAssert(0);
	}

	// save the polygon array
	int faceCount = NewtonMeshGetTotalFaceCount (mesh); 
	int indexCount = NewtonMeshGetTotalIndexCount (mesh); 

	int* const faceArray = new int [faceCount];
	void** const indexArray = new void* [indexCount];
	int* const materialIndexArray = new int [faceCount];
	int* const remapedIndexArray = new int [indexCount];

	NewtonMeshGetFaces (mesh, faceArray, materialIndexArray, indexArray); 

	// save the faces vertex Count
	dIntArrayToString (faceArray, faceCount, buffer, bufferSizeInBytes);
	TiXmlElement* const polygons = new TiXmlElement ("polygons");
	rootNode->LinkEndChild(polygons);
	polygons->SetAttribute("count", faceCount);
	polygons->SetAttribute("faceIndexCount", buffer);

	dIntArrayToString (materialIndexArray, faceCount, buffer, bufferSizeInBytes);
	TiXmlElement* const faceMaterial = new TiXmlElement ("faceMaterial");
	polygons->LinkEndChild(faceMaterial);
	faceMaterial->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
//		int index = NewtonMeshGetVertexIndex (mesh, indexArray[i]);
//		dTrace(("%d ", index));
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = vertexIndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const positionIndex = new TiXmlElement ("position");
	polygons->LinkEndChild(positionIndex);
	positionIndex->SetAttribute("index", buffer);

	if (NewtonMeshHasNormalChannel(mesh)) {
		for (int i = 0; i < indexCount; i++) {
			int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
			remapedIndexArray[i] = normalIndexList[index];
		}
		dIntArrayToString(remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
		TiXmlElement* const normalIndex = new TiXmlElement("normal");
		polygons->LinkEndChild(normalIndex);
		normalIndex->SetAttribute("index", buffer);
	}

	if (NewtonMeshHasUV0Channel(mesh)) {
		for (int i = 0; i < indexCount; i++) {
			int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
			remapedIndexArray[i] = uv0IndexList[index];
		}
		dIntArrayToString(remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
		TiXmlElement* const uv0Index = new TiXmlElement("uv0");
		polygons->LinkEndChild(uv0Index);
		uv0Index->SetAttribute("index", buffer);
	}

	if (NewtonMeshHasUV1Channel(mesh)) {
		for (int i = 0; i < indexCount; i++) {
			int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
			remapedIndexArray[i] = uv1IndexList[index];
		}
		dIntArrayToString(remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
		TiXmlElement* const uv1Index = new TiXmlElement("uv1");
		polygons->LinkEndChild(uv1Index);
		uv1Index->SetAttribute("index", buffer);
	}

	delete[] remapedIndexArray;
	delete[] faceArray;
	delete[] indexArray;
	delete[] materialIndexArray;
	delete[] uv1IndexList;
	delete[] uv0IndexList;
	delete[] normalIndexList;
	delete[] vertexIndexList;
	delete[] buffer;
}


bool DeserializeMesh (const NewtonMesh* const mesh, TiXmlElement* const rootNode) 
{
	// load all the vertexData
	TiXmlElement* const pointElement = (TiXmlElement*) rootNode->FirstChild ("points");

	//load face informations
	TiXmlElement* const polygonsElement = (TiXmlElement*)rootNode->FirstChild("polygons");

	NewtonMeshVertexFormat vertexFormat;
	NewtonMeshClearVertexFormat(&vertexFormat);

	int faceCount;
	polygonsElement->Attribute("count", &faceCount);
	int* const faceIndexCount = new int[faceCount];
	dStringToIntArray(polygonsElement->Attribute("faceIndexCount"), faceIndexCount, faceCount);

	int* const faceMaterials = new int[faceCount];
	TiXmlElement* materialElement = (TiXmlElement*)polygonsElement->FirstChild("faceMaterial");
	dStringToIntArray(materialElement->Attribute("index"), faceMaterials, faceCount);

	int indexCount = 0;
	for (int i = 0; i < faceCount; i++) {
		indexCount += faceIndexCount[i];
	}

	int* const positionVertexIndex = new int[indexCount];
	TiXmlElement* const positionVertexIndexElement = (TiXmlElement*)polygonsElement->FirstChild("position");
	dStringToIntArray(positionVertexIndexElement->Attribute("index"), positionVertexIndex, indexCount);

	int positionCount;
	TiXmlElement* const positionsElement = (TiXmlElement*) pointElement->FirstChild ("position");
	positionsElement->Attribute("float4", &positionCount);
	dFloat64* const positions = new dFloat64[4 * positionCount];
	dStringToFloatArray (positionsElement->Attribute("floats"), positions, 4 * positionCount);

	TiXmlElement* const normalsElement = (TiXmlElement*) pointElement->FirstChild ("normal");
	if (normalsElement) {
		int normalCount;
		normalsElement->Attribute("float3", &normalCount);
		dFloat* const normals = new dFloat[3 * normalCount];
		dStringToFloatArray (normalsElement->Attribute("floats"), normals, 3 * normalCount);

		int* const normalVertexIndex = new int[indexCount];
		TiXmlElement* const normalVertexIndexElement = (TiXmlElement*)polygonsElement->FirstChild("normal");
		dStringToIntArray(normalVertexIndexElement->Attribute("index"), normalVertexIndex, indexCount);

		vertexFormat.m_normal.m_data = normals;
		vertexFormat.m_normal.m_indexList = normalVertexIndex;
		vertexFormat.m_normal.m_strideInBytes = 3 * sizeof (dFloat);
	}
	
	TiXmlElement* const uv0Element = (TiXmlElement*) pointElement->FirstChild ("uv0");
	if (uv0Element) {
		int uv0Count;
		uv0Element->Attribute("float2", &uv0Count);
		dFloat* const uv0 = new dFloat[2 * uv0Count];
		dStringToFloatArray (uv0Element->Attribute("floats"), uv0, 2 * uv0Count);

		int* const uv0VertexIndex = new int[indexCount];
		TiXmlElement* const uv0VertexIndexElement = (TiXmlElement*)polygonsElement->FirstChild("uv0");
		dStringToIntArray(uv0VertexIndexElement->Attribute("index"), uv0VertexIndex, indexCount);

		vertexFormat.m_uv0.m_data = uv0;
		vertexFormat.m_uv0.m_indexList = uv0VertexIndex;
		vertexFormat.m_uv0.m_strideInBytes = 2 * sizeof (dFloat);
	}

	TiXmlElement* const uv1Element = (TiXmlElement*) pointElement->FirstChild ("uv1");
	if (uv1Element) {
		int uv1Count;
		uv1Element->Attribute("float2", &uv1Count);
		dFloat* const uv1 = new dFloat[2 * uv1Count];
		dStringToFloatArray (uv1Element->Attribute("floats"), uv1, 2 * uv1Count);

		int* const uv1VertexIndex = new int [indexCount]; 
		TiXmlElement* const uv1VertexIndexElement = (TiXmlElement*) polygonsElement->FirstChild ("uv1");
		dStringToIntArray (uv1VertexIndexElement->Attribute("index"), uv1VertexIndex, indexCount);

		vertexFormat.m_uv1.m_data = uv1;
		vertexFormat.m_uv1.m_indexList = uv1VertexIndex;
		vertexFormat.m_uv1.m_strideInBytes = 2 * sizeof (dFloat);
	}


	vertexFormat.m_faceCount = faceCount;
	vertexFormat.m_faceIndexCount = faceIndexCount;
	vertexFormat.m_faceMaterial = faceMaterials;

	vertexFormat.m_vertex.m_data = positions;
	vertexFormat.m_vertex.m_indexList = positionVertexIndex;
	vertexFormat.m_vertex.m_strideInBytes = 4 * sizeof (dFloat64);

	NewtonMeshBuildFromVertexListIndexList (mesh, &vertexFormat);

	if (vertexFormat.m_normal.m_data) {
		delete[] vertexFormat.m_normal.m_data;
		delete[] vertexFormat.m_normal.m_indexList;
	}

	if (vertexFormat.m_uv0.m_data) {
		delete[] vertexFormat.m_uv0.m_data;
		delete[] vertexFormat.m_uv0.m_indexList;
	}

	if (vertexFormat.m_uv1.m_data) {
		delete[] vertexFormat.m_uv1.m_data;
		delete[] vertexFormat.m_uv1.m_indexList;
	}


	delete[] positions;	
	delete[] positionVertexIndex;
	delete[] faceMaterials;
	delete[] faceIndexCount;
	return true;
}
