/////////////////////////////////////////////////////////////////////////////
// Name:        dMeshNodeInfo.h
// Purpose:     
// Author:      Julio Jerez
// Modified by: 
// Created:     22/05/2010 08:02:08
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
#include "dScene.h"
#include "dSceneRender.h"
#include "dMeshNodeInfo.h"
#include "dTextureNodeInfo.h"
#include "dMaterialNodeInfo.h"
#include "dGeometryNodeModifierInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dMeshNodeInfo);


dMeshNodeInfo::dMeshNodeInfo()
	:dGeometryNodeInfo (NULL), m_mesh (NULL)
{
}

dMeshNodeInfo::dMeshNodeInfo(dScene* const world)
	:dGeometryNodeInfo (world), m_mesh (NewtonMeshCreate(world->GetNewtonWorld()))
{
}

dMeshNodeInfo::dMeshNodeInfo(NewtonMesh* const mesh)
	:dGeometryNodeInfo (), m_mesh (mesh)
{
	SetName ("mesh");
}

dMeshNodeInfo::dMeshNodeInfo(const dMeshNodeInfo& me)
	:dGeometryNodeInfo (me), m_mesh (NewtonMeshCreateFromMesh(me.m_mesh))
{
}

dMeshNodeInfo::~dMeshNodeInfo(void)
{
	ReplaceMesh (NULL);
}

NewtonMesh* dMeshNodeInfo::GetMesh () const 
{
	return m_mesh;
}

void dMeshNodeInfo::SetMesh (NewtonMesh* const mesh)
{
	m_mesh = mesh;
}

void dMeshNodeInfo::ReplaceMesh (NewtonMesh* const mesh)
{
	if (m_mesh) {
		NewtonMeshDestroy(m_mesh);
	}
	SetMesh (mesh);
}




void dMeshNodeInfo::BeginBuild ()
{
	NewtonMeshBeginBuild(m_mesh);
}


void dMeshNodeInfo::AddPolygon (int pointsCount, const neMeshInfoFlatPoint* const points, int materialID)
{
	dAssert (0);
//	NewtonMeshAddFace(m_mesh, pointsCount, &points[0].m_x, sizeof (neMeshInfoFlatPoint), materialID);
}

void dMeshNodeInfo::EndBuild ()
{
	NewtonMeshEndBuild(m_mesh);
}

void dMeshNodeInfo::ConvertToTriangles()
{
	NewtonMeshTriangulate (m_mesh);
}

void dMeshNodeInfo::ConvertToPolygons()
{
	NewtonMeshPolygonize(m_mesh);
}
void dMeshNodeInfo::RepairTJoints ()
{
	NewtonMeshFixTJoints (m_mesh);
}


void dMeshNodeInfo::SmoothNormals (dFloat angleInRadiants)
{
	NewtonMeshCalculateVertexNormals(m_mesh, angleInRadiants);
}

void dMeshNodeInfo::BakeTransform (const dMatrix& transform)
{
	dVector scale; 
	dMatrix stretchMatrix;

	//dMatrix tmp (m_matrix);
	dMatrix matrix (transform.Inverse4x4() * m_matrix * transform);
	matrix.PolarDecomposition (m_matrix, scale, stretchMatrix);
	matrix = transform * dMatrix (dGetIdentityMatrix(), scale, stretchMatrix);

	NewtonMeshApplyTransform (m_mesh, &matrix[0][0]);
}
/*
void dMeshNodeInfo::BuildFromVertexListIndexList(int faceCount, const int* const faceIndexCount, const int* faceMaterialIndex, 
	const dFloat* const vertex, int vertexStrideInBytes, const int* vertexIndex,
	const dFloat* const normal, int normalStrideInBytes, const int* normalIndex,
	const dFloat* const uv0, int uv0StrideInBytes, const int* uv0Index,
	const dFloat* const uv1, int uv1StrideInBytes, const int* uv1Index)
*/
void dMeshNodeInfo::BuildFromVertexListIndexList(const NewtonMeshVertexFormat* const format)
{
	NewtonMeshBuildFromVertexListIndexList (m_mesh, format);
}

void dMeshNodeInfo::RemoveUnusedVertices(dScene* const world, dScene::dTreeNode* const myNode)
{
	dAssert (world->GetInfoFromNode(myNode) == this);

	int vertexCount = NewtonMeshGetVertexCount(m_mesh);
	int* vertexRemapArray = new int [vertexCount];

	dTrace(("fix this shit here\n"));
//	NewtonRemoveUnusedVertices(m_mesh, vertexRemapArray);
	NewtonRemoveUnusedVertices(m_mesh, NULL);

	for (void* ptr0 = world->GetFirstChildLink (myNode); ptr0; ptr0 = world->GetNextChildLink(myNode, ptr0)) {
		dScene::dTreeNode* node = world->GetNodeFromLink(ptr0);
		dNodeInfo* info = world->GetInfoFromNode(node);
		if (info->IsType(dGeometryNodeModifierInfo::GetRttiType())) {
			dGeometryNodeModifierInfo* const modifier = (dGeometryNodeModifierInfo*) info;
			modifier->RemoveUnusedVertices(vertexRemapArray);
		}
	}

	delete vertexRemapArray;
}


void dMeshNodeInfo::Serialize (TiXmlElement* const rootNode) const
{
 	SerialiseBase(dGeometryNodeInfo, rootNode);
	SerializeMesh (m_mesh, rootNode);
}

bool dMeshNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dGeometryNodeInfo, rootNode);
	DeserializeMesh (m_mesh, rootNode); 
	return true;
}

void dMeshNodeInfo::CalcutateAABB (dVector& p0, dVector& p1) const
{
//	int strideInBytes = NewtonMeshGetVertexStrideInByte(m_mesh);
//	dFloat64* const vertexList = NewtonMeshGetVertexArray(m_mesh);
//	dFloat t = 1.2f;
//	for (void* face = NewtonMeshGetFirstFace (m_mesh); face; face = NewtonMeshGetNextFace (m_mesh, face)) {
//		if (!NewtonMeshIsFaceOpen (m_mesh, face)) {

//	dMatrix matrix (GetIdentityMatrix());
//	NewtonMeshCalculateOOBB(const NewtonMesh* const mesh, dFloat* const matrix, dFloat* const x, dFloat* const y, dFloat* const z);

	p0 = dVector (1.0e10f, 1.0e10f, 1.0e10f, 0.0f);
	p1 = dVector (-1.0e10f, -1.0e10f, -1.0e10f, 0.0f);

	int strideInBytes = NewtonMeshGetVertexStrideInByte(m_mesh);
	int stride = strideInBytes / sizeof (dFloat64) ;
	const dFloat64* const vertexList = NewtonMeshGetVertexArray(m_mesh);
	for (void* ptr = NewtonMeshGetFirstVertex(m_mesh); ptr; ptr = NewtonMeshGetNextVertex(m_mesh, ptr)) {
		int index = NewtonMeshGetVertexIndex (m_mesh, ptr);

		dFloat x = dFloat (vertexList[index * stride + 0]);
		dFloat y = dFloat (vertexList[index * stride + 1]);
		dFloat z = dFloat (vertexList[index * stride + 2]);
		dVector v (m_matrix.TransformVector(dVector (x, y, z, 0.0f)));

		p0[0] = dMin(v[0], p0[0]);
		p0[1] = dMin(v[1], p0[1]);
		p0[2] = dMin(v[2], p0[2]);
					   
		p1[0] = dMax(v[0], p1[0]);
		p1[1] = dMax(v[1], p1[1]);
		p1[2] = dMax(v[2], p1[2]);
	}
}


dFloat dMeshNodeInfo::RayCast (const dVector& q0, const dVector& q1) const
{
	//	int vertexCount = NewtonMeshGetVertexCount(m_mesh);
	int strideInBytes = NewtonMeshGetVertexStrideInByte(m_mesh);
	const dFloat64* const vertexList = NewtonMeshGetVertexArray(m_mesh);
	dFloat t = 1.2f;

	dVector p0 = m_matrix.UntransformVector(q0);
	dVector p1 = m_matrix.UntransformVector(q1);
	for (void* face = NewtonMeshGetFirstFace (m_mesh); face; face = NewtonMeshGetNextFace (m_mesh, face)) {
		if (!NewtonMeshIsFaceOpen (m_mesh, face)) {

			int indices[1024];
			int vertexCount = NewtonMeshGetFaceIndexCount (m_mesh, face);
			NewtonMeshGetFaceIndices (m_mesh, face, indices);

			dFloat t1 = dPolygonRayCast (p0, p1, vertexCount, vertexList, strideInBytes, indices);
			if (t1 < t) {
				t = t1;		
			}
		}
	}
	return t;
}

dCRCTYPE dMeshNodeInfo::CalculateSignature() const
{
	dCRCTYPE signature = 0;

	int vertexCount = NewtonMeshGetVertexCount (m_mesh); 
	int vertexStride = NewtonMeshGetVertexStrideInByte(m_mesh);
	signature = dCRC64 (NewtonMeshGetVertexArray (m_mesh), vertexStride * vertexCount, signature);

	// for now just compare the vertex array, do no forget to add more text using the face winding and material indexed  

/*
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
		int index = NewtonMeshGetVertexIndex (mesh, indexArray[i]);
		remapedIndexArray[i] = vertexIndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const positionIndex = new TiXmlElement ("position");
	polygons->LinkEndChild(positionIndex);
	positionIndex->SetAttribute("index", buffer);


	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = normalIndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const normalIndex = new TiXmlElement ("normal");
	polygons->LinkEndChild(normalIndex);
	normalIndex->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = uv0IndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const uv0Index = new TiXmlElement ("uv0");
	polygons->LinkEndChild(uv0Index);
	uv0Index->SetAttribute("index", buffer);

	for (int i = 0; i < indexCount; i ++) {
		int index = NewtonMeshGetPointIndex(mesh, indexArray[i]);
		remapedIndexArray[i] = uv1IndexList[index];
	}
	dIntArrayToString (remapedIndexArray, indexCount, buffer, bufferSizeInBytes);
	TiXmlElement* const uv1Index = new TiXmlElement ("uv1");
	polygons->LinkEndChild(uv1Index);
	uv1Index->SetAttribute("index", buffer);
*/
	return signature;
}

void dMeshNodeInfo::DrawWireFrame(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (myNode == scene->Find(GetUniqueID()));
	dAssert (scene->GetInfoFromNode(myNode) == this);


	int displayList = render->GetCachedWireframeDisplayList(m_mesh);
	dAssert (displayList > 0);
	
	render->PushMatrix(&m_matrix[0][0]);
	if (GetEditorFlags() & m_selected) {
		dVector color (render->GetColor());
		render->SetColor(dVector (1.0f, 1.0f, 0.0f, 0.0f));
		render->DrawDisplayList(displayList);
		render->SetColor(color);
	} else {
		render->DrawDisplayList(displayList);
	}
	render->PopMatrix();
}

void dMeshNodeInfo::DrawFlatShaded(dSceneRender* const render, dScene* const scene, dScene::dTreeNode* const myNode) const
{
	dAssert (myNode == scene->Find(GetUniqueID()));
	dAssert (scene->GetInfoFromNode(myNode) == this);

	int displayList = render->GetCachedFlatShadedDisplayList(m_mesh);
	dAssert (displayList > 0);

	render->PushMatrix(&m_matrix[0][0]);
//render->SetColor(dVector(0, 0, 0, 0));
	render->DrawDisplayList(displayList);
	render->PopMatrix();
}
