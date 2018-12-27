/////////////////////////////////////////////////////////////////////////////
// Name:        dGeometryNodeSkinModifierInfo.h
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
#include "dBoneNodeInfo.h"
#include "dMeshNodeInfo.h"
#include "dSceneNodeInfo.h"
#include "dGeometryNodeModifierInfo.h"
#include "dGeometryNodeSkinModifierInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dGeometryNodeSkinModifierInfo);


dGeometryNodeSkinModifierInfo::dGeometryNodeSkinModifierInfo()
	:dGeometryNodeModifierInfo () 
	,m_boneCount(0)
	,m_vertexCount(0)
	,m_shapeBindMatrix(dGetIdentityMatrix()) 
	,m_vertexWeights(NULL)
	,m_boneBindingMatrix(NULL)
	,m_boneWeightIndex(NULL)
{
	SetName ("Skin Modifier");
}

dGeometryNodeSkinModifierInfo::dGeometryNodeSkinModifierInfo(dScene* const world)
	:dGeometryNodeModifierInfo ()
	,m_boneCount(0)
	,m_vertexCount(0)
	,m_shapeBindMatrix(dGetIdentityMatrix()) 
	,m_vertexWeights(NULL)
	,m_boneBindingMatrix(NULL)
	,m_boneWeightIndex(NULL)
{
	SetName ("Skin Modifier");
}


dGeometryNodeSkinModifierInfo::dGeometryNodeSkinModifierInfo(const dGeometryNodeSkinModifierInfo& me)
	:dGeometryNodeModifierInfo (me)
	,m_boneCount(0)
	,m_vertexCount(0)
	,m_shapeBindMatrix(dGetIdentityMatrix()) 
	,m_vertexWeights(NULL)
	,m_boneBindingMatrix(NULL)
	,m_boneWeightIndex(NULL)
{
	dAssert (0);
	SetName ("Skin Modifier");
}

dGeometryNodeSkinModifierInfo::~dGeometryNodeSkinModifierInfo(void)
{
	if (m_vertexWeights) {
		delete[] m_vertexWeights;
	}
	if (m_boneBindingMatrix) {
		delete[] m_boneBindingMatrix;
	}
	if (m_boneWeightIndex) {
		delete[] m_boneWeightIndex;
	}
}

void dGeometryNodeSkinModifierInfo::RemoveUnusedVertices(const int* const vertexMap)
{
	int vertexCount = 0;
	dVector* const vertexWeights = new dVector[m_vertexCount];
	dBoneWeightIndex* const boneWeightIndex = new dBoneWeightIndex[m_vertexCount];
	memcpy (vertexWeights, m_vertexWeights, m_vertexCount * sizeof(dVector));
	memcpy (boneWeightIndex, m_boneWeightIndex, m_vertexCount * sizeof(dBoneWeightIndex));
	for (int i = 0; i < m_vertexCount; i ++) {
		int index = vertexMap[i];
		if (index >= 0) {
			m_vertexWeights[index] = vertexWeights[i];
			m_boneWeightIndex[index] = boneWeightIndex[i];
			vertexCount = dMax (index + 1, vertexCount);
		}
	}
	m_vertexCount = vertexCount;

	delete[] boneWeightIndex;
	delete[] vertexWeights;
}


//void dGeometryNodeSkinModifierInfo::BakeTransform (const dMatrix& transform)
//{
//	dAssert (0);
//}



void dGeometryNodeSkinModifierInfo::SkinMesh(dScene::dTreeNode* const skinNode, dScene* const scene, dBoneVertexWeightData* const skinData, int skinDataCount)  
{
	dAssert (scene->GetInfoFromNode(skinNode) == this);

	if (m_vertexWeights) {
		delete[] m_vertexWeights;
	}
	if (m_boneWeightIndex) {
		delete[] m_boneWeightIndex;
	}

	if (m_boneBindingMatrix) {
		delete[] m_boneBindingMatrix;
	}

	while (scene->GetFirstChildLink(skinNode)) {
		dScene::dTreeNode* const bone = scene->GetNodeFromLink(scene->GetFirstChildLink(skinNode));
		dAssert (bone);
		scene->RemoveReference (skinNode, bone);
	}

	dScene::dTreeNode* const meshNode = scene->FindParentByType(skinNode, dMeshNodeInfo::GetRttiType());
	dAssert (meshNode);

	dMeshNodeInfo* const meshInfo = (dMeshNodeInfo*) scene->GetInfoFromNode (meshNode);
	NewtonMesh* const mesh = meshInfo->GetMesh();

	m_vertexCount = NewtonMeshGetVertexCount(mesh);

	m_vertexWeights = new dVector[m_vertexCount];
	m_boneWeightIndex = new dBoneWeightIndex[m_vertexCount];
	memset (m_vertexWeights, 0, m_vertexCount * sizeof (dVector));
	memset (m_boneWeightIndex, 0, m_vertexCount * sizeof (dBoneWeightIndex));

	int skinBoneCount = 0;
	dTree<int,dScene::dTreeNode*> boneMap;
	for (int i = 0; i < skinDataCount; i ++) {
		dAssert (skinData[i].m_weight > 0.0f);
		dScene::dTreeNode* const boneNode = skinData[i].m_boneNode;
		dTree<int,dScene::dTreeNode*>::dTreeNode* mapNode = boneMap.Find(boneNode);
		if (!mapNode){
			mapNode = boneMap.Insert(skinBoneCount, boneNode);
			scene->AddReference(skinNode, boneNode);
			skinBoneCount ++;
		}

		dBoneNodeInfo* const boneInfo = (dBoneNodeInfo*)scene->GetInfoFromNode(boneNode);
		dAssert(boneInfo->GetTypeId() == dBoneNodeInfo::GetRttiType());
		int vertexIndex = skinData[i].m_vertexIndex;
		for (int j = 0; j < 4; j ++) {
			if (m_vertexWeights[vertexIndex][j] == 0.0f) {
				m_vertexWeights[vertexIndex][j] = skinData[i].m_weight;
				m_boneWeightIndex[vertexIndex].m_index[j] = boneInfo->GetId();
				break;
			}
		}
	}

	m_boneCount = skinBoneCount;
	m_boneBindingMatrix = new dMatrix [skinBoneCount];

	dScene::dTreeNode* const parentBone = scene->FindParentByType(meshNode, dSceneNodeInfo::GetRttiType());
	dAssert (parentBone);
	dSceneNodeInfo* const sceneNode = (dSceneNodeInfo*) scene->GetInfoFromNode(parentBone);
	m_shapeBindMatrix = meshInfo->m_matrix * sceneNode->GetTransform();
	for (void* boneLink = scene->GetFirstChildLink(skinNode); boneLink; boneLink = scene->GetNextChildLink(skinNode, boneLink)) {
		dScene::dTreeNode* const boneNode = scene->GetNodeFromLink(boneLink);
		dScene::dTreeNode* const transformNode = scene->FindParentByType(boneNode, dSceneNodeInfo::GetRttiType());
		dAssert(transformNode);
		dBoneNodeInfo* const boneInfo = (dBoneNodeInfo*)scene->GetInfoFromNode(boneNode);
		dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*) scene->GetInfoFromNode(transformNode);
		dMatrix matrix (sceneInfo->GetTransform());
		m_boneBindingMatrix[boneInfo->GetId()] = matrix.Inverse4x4();
	}
}

void dGeometryNodeSkinModifierInfo::Serialize (TiXmlElement* const rootNode) const
{
	SerialiseBase(dGeometryNodeModifierInfo, rootNode);

	TiXmlElement* vertexCount = new TiXmlElement ("vertexCount");
	rootNode->LinkEndChild(vertexCount);
	vertexCount->SetAttribute("count", m_vertexCount);

	int size0 = sizeof (dMatrix) * m_boneCount / sizeof (dFloat);
	int size1 = sizeof (dVector) * m_vertexCount / sizeof (dFloat);
	int bufferSizeInBytes = dMax (size0, size1) * 10;
	char* const buffer = new char[bufferSizeInBytes];

	TiXmlElement* const matrix = new TiXmlElement ("bindingMatrix");
	rootNode->LinkEndChild(matrix);
	dFloatArrayToString (&m_shapeBindMatrix[0][0], 16, buffer, bufferSizeInBytes);
	matrix->SetAttribute("float16", buffer);

	TiXmlElement* const bindMatrices = new TiXmlElement ("bonesBindMatrices");
	rootNode->LinkEndChild(bindMatrices);

	dFloatArrayToString (&m_boneBindingMatrix[0][0][0], size0, buffer, bufferSizeInBytes);
	bindMatrices->SetAttribute("count", m_boneCount);
	bindMatrices->SetAttribute("float16", buffer);

	int weightCount = 0;
	for (int i = 0; i < m_vertexCount; i ++) {
		for (int j = 0; j < 4; j ++) {
			weightCount += (m_vertexWeights[i][j] > 0.0f) ? 1 : 0;
		}
	}

	int count = 0;
	dFloat* const weights = new dFloat[weightCount];
	int* const vertexIndex = new int[weightCount];
	int* const boneIndex = new int[weightCount];
	for (int i = 0; i < m_vertexCount; i ++) {
		for (int j = 0; j < 4; j ++) {
			if (m_vertexWeights[i][j] > 0.0f) {
				weights[count] = m_vertexWeights[i][j];
				vertexIndex[count] = i;
				boneIndex[count] = m_boneWeightIndex[i].m_index[j];
				count++;
			}
		}
	}

	dAssert (count == weightCount);

	TiXmlElement* const vertexWeight = new TiXmlElement ("vertexWeights");
	rootNode->LinkEndChild(vertexWeight);
	
	vertexWeight->SetAttribute("count", weightCount);

	TiXmlElement* const vIndices = new TiXmlElement ("vertexIndices");
	vertexWeight->LinkEndChild(vIndices);
	dIntArrayToString (vertexIndex, weightCount, buffer, bufferSizeInBytes);
	vIndices->SetAttribute("indices", buffer);

	TiXmlElement* const bIndices = new TiXmlElement ("boneIndices");
	vertexWeight->LinkEndChild(bIndices);
	dIntArrayToString (boneIndex, weightCount, buffer, bufferSizeInBytes);
	bIndices->SetAttribute("indices", buffer);

	TiXmlElement* const vWeights = new TiXmlElement ("weights");
	vertexWeight->LinkEndChild(vWeights);
	dFloatArrayToString (weights, weightCount, buffer, bufferSizeInBytes);
	vWeights->SetAttribute("floats", buffer);

	delete[] boneIndex;
	delete[] vertexIndex;
	delete[] weights;
	delete[] buffer;
}

bool dGeometryNodeSkinModifierInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dGeometryNodeModifierInfo, rootNode);

	TiXmlElement* const vertexCount = (TiXmlElement*) rootNode->FirstChild ("vertexCount");
	vertexCount->Attribute("count", &m_vertexCount);

	TiXmlElement* const matrixNode = (TiXmlElement*) rootNode->FirstChild ("bindingMatrix");
	dStringToFloatArray (matrixNode->Attribute("float16"), &m_shapeBindMatrix[0][0], 16);

	TiXmlElement* const bindMatrices = (TiXmlElement*) rootNode->FirstChild ("bonesBindMatrices");
	bindMatrices->Attribute("count", &m_boneCount);

	m_boneBindingMatrix = new dMatrix[m_boneCount];
	dStringToFloatArray (bindMatrices->Attribute("float16"), &m_boneBindingMatrix[0][0][0], 16 * m_boneCount);

	int weightCount = 0; 
	TiXmlElement* const vertexWeight = (TiXmlElement*) rootNode->FirstChild ("vertexWeights");
	vertexWeight->Attribute("count", &weightCount);

	int* const boneIndex = new int[weightCount];
	int* const vertexIndex = new int[weightCount];
	dFloat* const weights = new dFloat[weightCount];

	TiXmlElement* const vIndices = (TiXmlElement*) vertexWeight->FirstChild ("vertexIndices");
	dStringToIntArray (vIndices->Attribute("indices"), vertexIndex, weightCount);

	TiXmlElement* const bIndices = (TiXmlElement*) vertexWeight->FirstChild ("boneIndices");
	dStringToIntArray (bIndices->Attribute("indices"), boneIndex, weightCount);

	TiXmlElement* const vWeights = (TiXmlElement*) vertexWeight->FirstChild ("weights");
	dStringToFloatArray (vWeights->Attribute("floats"), &weights[0], weightCount);

	m_vertexWeights = new dVector[m_vertexCount];
	m_boneWeightIndex = new dBoneWeightIndex[m_vertexCount];
	memset (m_vertexWeights, 0, m_vertexCount * sizeof (dVector));
	memset (m_boneWeightIndex, 0, m_vertexCount * sizeof (dBoneWeightIndex));

	for (int i = 0; i < weightCount; i ++) {
		int index = vertexIndex[i];
		for (int j = 0; j < 4; j ++) {
			if (m_vertexWeights[index][j] == 0.0f) {
				m_vertexWeights[index][j] = weights[i];
				m_boneWeightIndex[index].m_index[j] = boneIndex[i];
				break;
			}
		}
	}

	delete[] boneIndex;
	delete[] vertexIndex;
	delete[] weights;
	return true;
}


void dGeometryNodeSkinModifierInfo::BakeTransform (const dMatrix& transform)
{
	dMatrix inverse (transform.Inverse4x4());
	m_shapeBindMatrix = inverse * m_shapeBindMatrix * transform;

	for (int i = 0; i < m_boneCount; i ++) {
		m_boneBindingMatrix[i] = inverse * m_boneBindingMatrix[i] * transform;
	}
}