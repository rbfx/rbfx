/////////////////////////////////////////////////////////////////////////////
// Name:        dScene.h
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
#include "dLineNodeInfo.h"
#include "dRootNodeInfo.h"
#include "dBoneNodeInfo.h"
#include "dMeshNodeInfo.h"
#include "dSceneNodeInfo.h"
#include "dSceneCacheInfo.h"
#include "dSceneModelInfo.h"
#include "dTextureNodeInfo.h"
#include "dMaterialNodeInfo.h"
#include "dRigidbodyNodeInfo.h"
#include "dCollisionBoxNodeInfo.h"
#include "dCollisionConeNodeInfo.h"
#include "dCollisionTreeNodeInfo.h"
#include "dCollisionSphereNodeInfo.h"
#include "dCollisionCapsuleNodeInfo.h"
#include "dCollisionCylinderNodeInfo.h"
#include "dCollisionCompoundNodeInfo.h"
#include "dCollisionConvexHullNodeInfo.h"
#include "dGeometryNodeSkinModifierInfo.h"
#include "dCollisionChamferCylinderNodeInfo.h"
#include <tinyxml.h>


// revision 101:  change SceneNodeMatrix Matrix from global to local
static void MakeSceneNodeMatricesLocalToNodeParent (dScene* const scene)
{
	dList<dMatrix> matrixPool;
	dList<dScene::dTreeNode*> stackPool;
	dScene::dTreeNode* const rootNode = scene->GetRootNode();
	for (void* link = scene->GetFirstChildLink(rootNode); link; link = scene->GetNextChildLink(rootNode, link)) {
		dScene::dTreeNode* const sceneNode = scene->GetNodeFromLink(link);
		dNodeInfo* const sceneNodeInfo = scene->GetInfoFromNode(sceneNode);
		if (sceneNodeInfo->IsType(dSceneNodeInfo::GetRttiType())) {
			stackPool.Append(sceneNode);
			matrixPool.Append(dGetIdentityMatrix());
		}
	}

	while (stackPool.GetCount()) {
		dScene::dTreeNode* const root = stackPool.GetLast()->GetInfo();
		dMatrix parentMatrix (matrixPool.GetLast()->GetInfo());
		stackPool.Remove(stackPool.GetLast());
		matrixPool.Remove(matrixPool.GetLast());

		dSceneNodeInfo* const rootNodeInfo = (dSceneNodeInfo*)scene->GetInfoFromNode(root);
		dAssert (rootNodeInfo->IsType(dSceneNodeInfo::GetRttiType()));
		dMatrix matrix (rootNodeInfo->GetTransform());
		dMatrix localMatrix = matrix * parentMatrix;
		rootNodeInfo->SetTransform(localMatrix);

		matrix = matrix.Inverse4x4();
		for (void* link = scene->GetFirstChildLink(root); link; link = scene->GetNextChildLink(root, link)) {
			dScene::dTreeNode* const node = scene->GetNodeFromLink(link);
			dNodeInfo* const nodeInfo = scene->GetInfoFromNode(node);
			if (nodeInfo->IsType(dSceneNodeInfo::GetRttiType())) {
				stackPool.Append(node);
				matrixPool.Append(matrix);
			}
		}
	}


	// also make sure the textures ids match 
	dScene::Iterator iter (*scene);
	for (iter.Begin(); iter; iter ++) {
		dScene::dTreeNode* const materialNode = iter.GetNode();
		dMaterialNodeInfo* const materialInfo = (dMaterialNodeInfo*)scene->GetInfoFromNode(materialNode);
		if (materialInfo->IsType(dMaterialNodeInfo::GetRttiType())) {
			if (materialInfo->GetDiffuseTextId() != -1) {
				// return any texture because the ids where change from 32 to 64 bit and the do no match anymore
				for (void* link = scene->GetFirstChildLink(materialNode); link; link = scene->GetNextChildLink(materialNode, link)) {
					dScene::dTreeNode* const node = scene->GetNodeFromLink(link);
					dTextureNodeInfo* const texture = (dTextureNodeInfo*) scene->GetInfoFromNode(node);
					if (texture->IsType(dTextureNodeInfo::GetRttiType())) {
						if (dString(texture->GetName()) == dString("texture")){
							texture->SetName (texture->GetPathName());
						}
						materialInfo->SetDiffuseTextId(texture->GetId());
						break;
					}
				}
			}
		}
	}
}

// revision 102: add a texture cache node as child of root node
static void PopupateTextureCacheNode (dScene* const scene)
{
	dScene::dTreeNode* const cacheNode = scene->GetTextureCacheNode ();
	dAssert (cacheNode);
	for (dScene::dTreeNode* node = scene->GetFirstNode(); node; node = scene->GetNextNode(node)) {
		dTextureNodeInfo* const nodeInfo = (dTextureNodeInfo*)scene->GetInfoFromNode(node);
		if (nodeInfo->IsType(dTextureNodeInfo::GetRttiType())) {
			scene->AddReference(cacheNode, node);
		}
	}


	for (void* link = scene->GetFirstChildLink(cacheNode); link; link = scene->GetNextChildLink(cacheNode, link)) {
		dScene::dTreeNode* const textNode = scene->GetNodeFromLink(link);
		dAssert (scene->GetInfoFromNode(textNode)->IsType(dTextureNodeInfo::GetRttiType()));

		for (void* link1 = scene->GetFirstParentLink(textNode); link1; link1 = scene->GetNextParentLink(textNode, link1)) {
			dScene::dTreeNode* const node = scene->GetNodeFromLink(link1);
			dNodeInfo* const info = scene->GetInfoFromNode(node);
			if (!info->IsType(dSceneCacheInfo::GetRttiType())) {
				scene->RemoveReference(node, textNode);
			}
		}
	}
}


// revision 102: add a material cache node as child of root node
static void PopupateMaterialCacheNode (dScene* const scene)
{
	dScene::dTreeNode* const cacheNode = scene->GetMaterialCacheNode ();
	dScene::Iterator iter (*scene);
	for (iter.Begin(); iter; iter ++) {
		dScene::dTreeNode* const materialNode = iter.GetNode();
		dMaterialNodeInfo* const materialInfo = (dMaterialNodeInfo*)scene->GetInfoFromNode(materialNode);
		if (materialInfo->IsType(dMaterialNodeInfo::GetRttiType())) {
			scene->AddReference(cacheNode, materialNode);
		}
	}
}


// revision 102: add a material cache node as child of root node
static void PopupateGeomteryCacheNode (dScene* const scene)
{
	dScene::dTreeNode* const cacheNode = scene->GetGeometryCacheNode();
	dScene::Iterator iter (*scene);
	for (iter.Begin(); iter; iter ++) {
		dScene::dTreeNode* const geometryNode = iter.GetNode();
		dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*)scene->GetInfoFromNode(geometryNode);
		if (geometryInfo->IsType(dGeometryNodeInfo::GetRttiType())) {
			scene->AddReference(cacheNode, geometryNode);
		}
	}
}

// revision 102: add a texture cache, material cache, and mesh cache as children of root node.
static void AddTextureCacheMaterianCacheMeshCache (dScene* const scene)
{
	PopupateTextureCacheNode (scene);
	PopupateMaterialCacheNode (scene);
	PopupateGeomteryCacheNode (scene);
	scene->DeleteDuplicateGeometries();
}

static void RemoveLocalTransformFromGeometries (dScene* const scene)
{
	dScene::dTreeNode* const cacheNode = scene->FindGetGeometryCacheNode();
	for (void* link = scene->GetFirstChildLink(cacheNode); link; link = scene->GetNextChildLink(cacheNode, link)) {
		dScene::dTreeNode* const geometryNode = scene->GetNodeFromLink(link);
		dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*)scene->GetInfoFromNode(geometryNode);
		dMatrix matrix (geometryInfo->GetPivotMatrix());
		for (void* link1 = scene->GetFirstParentLink(geometryNode); link1; link1 = scene->GetNextParentLink(geometryNode, link1)) {
			dScene::dTreeNode* const sceneNode = scene->GetNodeFromLink(link1);
			if (sceneNode != cacheNode) {
				dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*)scene->GetInfoFromNode(sceneNode);
				sceneInfo->SetGeometryTransform(matrix * sceneInfo->GetGeometryTransform());
			}
		}
		geometryInfo->SetPivotMatrix(dGetIdentityMatrix());
	}
	scene->FreezeScale();
}


// this constructor is for the editor only
dScene::dScene(NewtonWorld* const newton)
	:dSceneGraph(new dRootNodeInfo())
	,dRefCounter()
	,m_revision(D_SCENE_REVISION_NUMBER)
	,m_newton (newton)
{
}

dScene::dScene(const dScene& me)
	:dSceneGraph(me)
	,dRefCounter()
	,m_revision(me.m_revision)
	,m_newton (me.m_newton)
{
	dBoneNodeInfo::GetRttiType();
	dMeshNodeInfo::GetRttiType();
	dLineNodeInfo::GetRttiType();
	dRootNodeInfo::GetRttiType();
	dSceneNodeInfo::GetRttiType();
	dSceneCacheInfo::GetRttiType();
	dSceneModelInfo::GetRttiType();
	dTextureNodeInfo::GetRttiType();
	dMaterialNodeInfo::GetRttiType();
	dGeometryNodeInfo::GetRttiType();
	dCollisionNodeInfo::GetRttiType();
	dRigidbodyNodeInfo::GetRttiType();
	dCollisionBoxNodeInfo::GetRttiType();
	dCollisionConeNodeInfo::GetRttiType();
	dCollisionTreeNodeInfo::GetRttiType();
	dCollisionSphereNodeInfo::GetRttiType();
	dGeometryNodeModifierInfo::GetRttiType();
	dCollisionCapsuleNodeInfo::GetRttiType();
	dCollisionCompoundNodeInfo::GetRttiType();
	dCollisionCylinderNodeInfo::GetRttiType();
	dCollisionConvexHullNodeInfo::GetRttiType();
	dGeometryNodeSkinModifierInfo::GetRttiType();
	dCollisionChamferCylinderNodeInfo::GetRttiType();
}

dScene::~dScene(void)
{
}

int dScene::GetRevision() const
{
	return m_revision;
}

void dScene::CleanUp()
{
	dSceneGraph::Cleanup();
}

dScene::dTreeNode* dScene::GetRootNode() const
{
	return dSceneGraph::GetRootNode();
}

void dScene::AddReference(dTreeNode* const parent, dTreeNode* const child)
{
	 AddEdge (parent, child);
}

void dScene::RemoveReference(dTreeNode* const node1, dTreeNode* const node2)
{
	DeleteEdge (node1, node2);
}

dScene::dTreeNode* dScene::CreateNode (const char* const className, dTreeNode* const parent)
{
	dTreeNode* node = NULL;
	dNodeInfo* const info = dNodeInfo::CreateFromClassName (className, this);
	if (info) {
		node = AddNode(info, parent);
		info->Release();
	}
	return node;
}


dScene::dTreeNode* dScene::CreateCollisionFromNewtonCollision(dTreeNode* const parent, NewtonCollision* const collision)
{
	NewtonCollisionInfoRecord record;
	NewtonCollisionGetInfo(collision, &record);

	dNodeInfo* info = NULL;
	dNodeInfo* const tmp = new dNodeInfo();
	dTreeNode* node = AddNode(tmp, parent);
	tmp->Release();

	switch (record.m_collisionType)
	{
		case SERIALIZE_ID_SPHERE:
		{
			info = new dCollisionSphereNodeInfo(collision);
			break;
		}
		case SERIALIZE_ID_BOX:
		{
			info = new dCollisionBoxNodeInfo(collision);
			break;
		}

		case SERIALIZE_ID_CONE:
		{
			info = new dCollisionConeNodeInfo(collision);
			break;
		}

		case SERIALIZE_ID_CAPSULE:
		{
			info = new dCollisionCapsuleNodeInfo(collision);
			break;
		}
		case SERIALIZE_ID_CYLINDER:
		{
			info = new dCollisionCylinderNodeInfo(collision);
			break;
		}
		case SERIALIZE_ID_CHAMFERCYLINDER:
		{
			info = new dCollisionChamferCylinderNodeInfo(collision);
			break;
		}

		case SERIALIZE_ID_CONVEXHULL:
		{
			info = new dCollisionConvexHullNodeInfo(collision);
			break;
		}

		case SERIALIZE_ID_COMPOUND:
		{
			info = new dCollisionCompoundNodeInfo(collision);
			for (void* collisionNode = NewtonCompoundCollisionGetFirstNode (collision); collisionNode; collisionNode = NewtonCompoundCollisionGetNextNode(collision, collisionNode)) {
				NewtonCollision* const convexCollision = NewtonCompoundCollisionGetCollisionFromNode (collision, collisionNode);
				CreateCollisionFromNewtonCollision(node, convexCollision);
			}

			break;
		}

		case SERIALIZE_ID_TREE:
		{
			info = new dCollisionTreeNodeInfo(collision);
			break;
		}


		case SERIALIZE_ID_NULL:
		case SERIALIZE_ID_HEIGHTFIELD:
		case SERIALIZE_ID_USERMESH:
		case SERIALIZE_ID_SCENE:
		case SERIALIZE_ID_FRACTURED_COMPOUND:
		default:
		{
			dAssert(0);
			break;
		}
	}

//	dTreeNode* const node = AddNode(info, parent);
	if (info) {
		SetNodeInfo(info, node);
		info->Release();
	} else {
		DeleteNode(node);
		node = NULL;
	}
	
	return node;
}


dScene::dTreeNode* dScene::AddNode(dNodeInfo* const sceneInfo, dTreeNode* const parent)
{
	dTreeNode* const node = dSceneGraph::AddNode (sceneInfo, parent);
	return node;
}

dScene::dTreeNode* dScene::CreateModelNode(dTreeNode* const parent)
{
	return CreateNode ("dSceneModelInfo", parent);
}

dScene::dTreeNode* dScene::CreateSceneNode(dTreeNode* const parent)
{
	return CreateNode ("dSceneNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateGeometryTransformNode(dTreeNode* const parent)
{
	return CreateNode ("dGeometryTransformdNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateBoneNode(dTreeNode* const parent)
{
	return CreateNode ("dBoneNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateRigidbodyNode(dTreeNode* const parent)
{
	return CreateNode ("dRigidbodyNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionBoxNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionBoxNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionConeNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionConeNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionSphereNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionSphereNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionCapsuleNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionCapsuleNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionCylinderNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionCylinderNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionChamferCylinderNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionChamferCylinderNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionConvexHullNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionConvexHullNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisionCompoundNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionCompoundNodeInfo", parent);
}

dScene::dTreeNode* dScene::CreateCollisioTreeNode(dTreeNode* const parent)
{
	return CreateNode ("dCollisionTreeNodeInfo", parent);
}


dScene::dTreeNode* dScene::CreateMeshNode(dTreeNode* const parent)
{
	dTreeNode* const cacheNode = GetGeometryCacheNode();
	dTreeNode* const node = CreateNode ("dMeshNodeInfo", parent);
	AddReference (cacheNode, node);
	return node;
}

dScene::dTreeNode* dScene::CreateLineNode(dTreeNode* const parent)
{
	dTreeNode* const cacheNode = GetGeometryCacheNode();
	dTreeNode* const node = CreateNode ("dLineNodeInfo", parent);
	AddReference (cacheNode, node);
	return node;
}

dScene::dTreeNode* dScene::CreateSkinModifierNode(dTreeNode* const parent)
{
	return CreateNode ("dGeometryNodeSkinModifierInfo", parent);
}


dScene::dTreeNode* dScene::CreateTextureNode (const char* const pathName)
{
	dTreeNode* const root = GetTextureCacheNode();

	// see if this texture is already exist
	dCRCTYPE crc = dCRC64 (dGetNameFromPath(pathName));
	for (void* ptr = GetFirstChildLink(root); ptr; ptr = GetNextChildLink(root, ptr)) {
		dNodeInfo* const info = GetInfoFromNode(GetNodeFromLink (ptr));
		if (info->IsType(dTextureNodeInfo::GetRttiType())) {
			dTextureNodeInfo* const texture = (dTextureNodeInfo*) info;
			if (crc == texture->GetId()) {
				// we found a texture, return the node
				return GetNodeFromLink (ptr);
			}
		}
	}

	dTreeNode* const node = CreateNode ("dTextureNodeInfo", root);
	dTextureNodeInfo* const info = (dTextureNodeInfo*) GetInfoFromNode(node);
	info->SetPathName (pathName);

	return node;
}


dScene::dTreeNode* dScene::CreateMaterialNode (int id)
{
	dTreeNode* const root = GetMaterialCacheNode();
	dScene::dTreeNode* const node = CreateNode ("dMaterialNodeInfo", root);
	dMaterialNodeInfo* const info = (dMaterialNodeInfo*) GetInfoFromNode(node);
	info->m_id = id;
	return node;
}

dScene::dTreeNode* dScene::GetCacheNode (const char* const cacheName)
{
	dTreeNode* const root = GetRootNode();

	dTreeNode* node = FindChildByName (root, cacheName);
	if (!node) {
		dCRCTYPE id = dCRC64 (cacheName);
		node = CreateNode ("dSceneCacheInfo", root);
		dSceneCacheInfo* const info = (dSceneCacheInfo*) GetInfoFromNode(node);
		info->SetName(cacheName);
		info->SetID(id);
	}
/*
	
	for (void* ptr = GetFirstChildLink(root); ptr; ptr = GetNextChild(root, ptr)) {
		dTreeNode* const node = GetNodeFromLink (ptr);
		dSceneCacheInfo* const info = (dSceneCacheInfo*) GetInfoFromNode(node);
		if (info->IsType(dSceneCacheInfo::GetRttiType())) {
			if (info->GetID() == id) {
				return node;
			}
		}
	}
*/

	return node;

}

dScene::dTreeNode* dScene::GetTextureCacheNode ()
{
	return GetCacheNode (D_TEXTURE_CACHE_NODE_MAME);
}

dScene::dTreeNode* dScene::GetMaterialCacheNode ()
{
	return GetCacheNode (D_MATERIAL_CACHE_NODE_MAME);
}

dScene::dTreeNode* dScene::GetGeometryCacheNode ()
{
	return GetCacheNode (D_GEOMETRY_CACHE_NODE_MAME);
}

dScene::dTreeNode* dScene::FindTextureCacheNode () const
{
	return FindChildByName(GetRootNode(), D_TEXTURE_CACHE_NODE_MAME);
}

dScene::dTreeNode* dScene::FindGetMaterialCacheNode () const
{
	return FindChildByName(GetRootNode(), D_MATERIAL_CACHE_NODE_MAME);
}

dScene::dTreeNode* dScene::FindGetGeometryCacheNode () const
{
	return FindChildByName(GetRootNode(), D_GEOMETRY_CACHE_NODE_MAME);
}



dScene::dTreeNode* dScene::GetFirstNode () const
{
	Iterator iter (*this);
	iter.Begin();
	return iter.GetNode();
}

dScene::dTreeNode* dScene::FindNode (dNodeInfo* const info) const
{
	return Find (info->GetUniqueID());
}

dScene::dTreeNode* dScene::GetNextNode (dTreeNode* const node) const
{
	Iterator iter (*this);
	iter.Set (node);
	iter++;
	return iter.GetNode();
}


void* dScene::GetFirstChildLink(dTreeNode* const parentNode) const
{
	dGraphNode& root = parentNode->GetInfo();
	return root.m_children.GetFirst();
}

void* dScene::GetNextChildLink(dTreeNode* const parentNode, void* const link) const
{
	dGraphNode::dLink::dListNode* const node = (dGraphNode::dLink::dListNode*) link;
	return node->GetNext();
}

void* dScene::GetFirstParentLink(dTreeNode* const childNode) const
{
	dGraphNode& root = childNode->GetInfo();
	return root.m_parents.GetFirst();
}

void* dScene::GetNextParentLink(dTreeNode* const childNode, void* const link) const
{
	dGraphNode::dLink::dListNode* const node = (dGraphNode::dLink::dListNode*) link;
	return node->GetNext();
}


dScene::dTreeNode* dScene::GetNodeFromLink (void* const child) const
{
	dGraphNode::dLink::dListNode* const node = (dGraphNode::dLink::dListNode*) child;
	return node->GetInfo();
}

dNodeInfo* dScene::GetInfoFromNode(dTreeNode* const node) const
{	
	return node->GetInfo().GetNode();
}


dNodeInfo* dScene::CloneNodeInfo(dTreeNode* const node) const
{
	dNodeInfo* const info = node->GetInfo().GetNode();
	dNodeInfo* const clone = info->MakeCopy();
	dAssert (clone->GetUniqueID() != info->GetUniqueID());
	dAssert (clone->GetTypeId() == info->GetTypeId()) ;
	return clone;
}


dScene::dTreeNode* dScene::FindTextureByTextId(dTreeNode* const parentNode, dCRCTYPE textId) const
{
	for (void* ptr = GetFirstChildLink(parentNode); ptr; ptr = GetNextChildLink(parentNode, ptr)) {
		dScene::dTreeNode* const node = GetNodeFromLink(ptr);
		const dTextureNodeInfo* const texture = (dTextureNodeInfo*) GetInfoFromNode(node);
		if (texture->IsType(dTextureNodeInfo::GetRttiType())) {
			if (texture->GetId() == textId) {
				return node;
			}
		}
	}

	return NULL;
}

dScene::dTreeNode* dScene::FindTextureByTextId(dCRCTYPE textId)
{
	return FindTextureByTextId (GetTextureCacheNode(), textId);
}

dScene::dTreeNode* dScene::FindMaterialById(dTreeNode* const parentNode, int materialId) const
{
	for (void* ptr = GetFirstChildLink(parentNode); ptr; ptr = GetNextChildLink(parentNode, ptr)) {
		dScene::dTreeNode* const node = GetNodeFromLink(ptr);
		dNodeInfo* const info = GetInfoFromNode(node);
		if (info->IsType(dMaterialNodeInfo::GetRttiType())) {
			const dMaterialNodeInfo* const material = (dMaterialNodeInfo*) info;
			if (material->GetId() == materialId) {
				return node;
			}
		}
	}
	return NULL;
}

dScene::dTreeNode* dScene::FindMaterialBySignature(dCRCTYPE signature)
{
	dTreeNode* const parentNode = GetMaterialCacheNode(); 
	for (void* ptr = GetFirstChildLink(parentNode); ptr; ptr = GetNextChildLink(parentNode, ptr)) {
		dScene::dTreeNode* const node = GetNodeFromLink(ptr);
		dNodeInfo* const info = GetInfoFromNode(node);
		if (info->IsType(dMaterialNodeInfo::GetRttiType())) {
			const dMaterialNodeInfo* const material = (dMaterialNodeInfo*) info;
			if (material->CalculateSignature() == signature) {
				return node;
			}
		}
	}
	return NULL;

}

dScene::dTreeNode* dScene::FindMaterialById(int materialId)
{
	return FindMaterialById (GetMaterialCacheNode(), materialId);
}

dScene::dTreeNode* dScene::FindChildByName(dTreeNode* const root, const char* const name) const
{
	dCRCTYPE id = dCRC64 (name);
	for (void* ptr = GetFirstChildLink(root); ptr; ptr = GetNextChildLink(root, ptr)) {
		dTreeNode* const node = GetNodeFromLink (ptr);
		dSceneCacheInfo* const info = (dSceneCacheInfo*) GetInfoFromNode(node);
		if (info->IsType(dSceneCacheInfo::GetRttiType())) {
			if (info->GetID() == id) {
				return node;
			}
		}
	}
	return NULL;
}

dScene::dTreeNode* dScene::FindChildByType(dTreeNode* const parentNode, dCRCTYPE type) const
{
	for (void* child = GetFirstChildLink (parentNode); child; child = GetNextChildLink(parentNode, child)) {
		dTreeNode* const tmpNode = GetNodeFromLink (child);
		dNodeInfo* const info = GetInfoFromNode(tmpNode);
		if (info->IsType(type)) {
			return tmpNode;
		}
	}
	return NULL;
}

dScene::dTreeNode* dScene::FindParentByType(dTreeNode* const childNode, dCRCTYPE type) const
{
	for (void* parent = GetFirstParentLink(childNode); parent; parent = GetNextChildLink(childNode, parent)) {
		dTreeNode* const tmpNode = GetNodeFromLink (parent);
		dNodeInfo* const info = GetInfoFromNode(tmpNode);
		if (info->IsType(type)) {
			return tmpNode;
		}
	}
	return NULL;
}


void dScene::FreezeScale ()
{
	dList<dTreeNode*> nodeStack;
	dList<dMatrix> parentMatrixStack;

	dTreeNode* const rootNode0 = GetRootNode();
	for (void* link0 = GetFirstChildLink(rootNode0); link0; link0 = GetNextChildLink(rootNode0, link0)) {
		dTreeNode* const node0 = GetNodeFromLink(link0);
		dNodeInfo* const nodeInfo0 = GetInfoFromNode(node0);
		if (nodeInfo0->IsType(dSceneNodeInfo::GetRttiType())) {
			nodeStack.Append(node0);
			parentMatrixStack.Append(dGetIdentityMatrix());
		}
	}

	dTree<dGeometryNodeInfo*, dGeometryNodeInfo*> geoFilter;	
	while (nodeStack.GetCount()) {
		dTreeNode* const rootNode = nodeStack.GetLast()->GetInfo();
		dMatrix parentMatrix (parentMatrixStack.GetLast()->GetInfo());
		
		nodeStack.Remove(nodeStack.GetLast());
		parentMatrixStack.Remove(parentMatrixStack.GetLast());

		dSceneNodeInfo* const sceneNodeInfo = (dSceneNodeInfo*)GetInfoFromNode(rootNode);
		dAssert (sceneNodeInfo->IsType(dSceneNodeInfo::GetRttiType()));
		dMatrix transform (sceneNodeInfo->GetTransform() * parentMatrix);

		dMatrix matrix;
		dMatrix stretchAxis;
		dVector scale(0.0f);
		transform.PolarDecomposition (matrix, scale, stretchAxis);
		sceneNodeInfo->SetTransform (matrix);

		dMatrix scaleMatrix (dGetIdentityMatrix(), scale, stretchAxis);
		sceneNodeInfo->SetGeometryTransform (sceneNodeInfo->GetGeometryTransform() * scaleMatrix);

		for (void* link = GetFirstChildLink(rootNode); link; link = GetNextChildLink(rootNode, link)) {
			dTreeNode* const node = GetNodeFromLink(link);
			dNodeInfo* const nodeInfo = GetInfoFromNode(node);
			if (nodeInfo->IsType(dSceneNodeInfo::GetRttiType())) {
				nodeStack.Append(node);
				parentMatrixStack.Append(scaleMatrix);
			}
		}
	}
}

void dScene::FreezeRootRotation ()
{
	dList<dTreeNode*> nodeStack;
	dList<dMatrix> parentMatrixStack;

	dTreeNode* const rootNode = GetRootNode();
	for (void* link0 = GetFirstChildLink(rootNode); link0; link0 = GetNextChildLink(rootNode, link0)) {
		dTreeNode* const node0 = GetNodeFromLink(link0);
		dNodeInfo* const nodeInfo0 = GetInfoFromNode(node0);
		if (nodeInfo0->IsType(dSceneNodeInfo::GetRttiType())) {
			
			dSceneNodeInfo* const sceneNodeInfo = (dSceneNodeInfo*) nodeInfo0;
			dMatrix matrix (sceneNodeInfo->GetTransform());
			dMatrix unitRotation (dGetIdentityMatrix());
			unitRotation.m_posit = matrix.m_posit;
			matrix.m_posit = dVector (0.0f, 0.0f, 0.0f, 1.0f);
			sceneNodeInfo->SetTransform(unitRotation);
			sceneNodeInfo->SetGeometryTransform (sceneNodeInfo->GetGeometryTransform() * matrix);

			for (void* link1 = GetFirstChildLink(node0); link1; link1 = GetNextChildLink(node0, link1)) {
				dTreeNode* const node1 = GetNodeFromLink(link1);
				dNodeInfo* const nodeInfo = GetInfoFromNode(node1);
				if (nodeInfo->IsType(dSceneNodeInfo::GetRttiType())) {
					nodeStack.Append(node1);
					parentMatrixStack.Append(matrix);
				}
			}
		}
	}

	dTree<dGeometryNodeInfo*, dGeometryNodeInfo*> geoFilter;	
	while (nodeStack.GetCount()) {
		//dTreeNode* const rootNode = nodeStack.GetLast()->GetInfo();
		dMatrix parentMatrix (parentMatrixStack.GetLast()->GetInfo());

		nodeStack.Remove(nodeStack.GetLast());
		parentMatrixStack.Remove(parentMatrixStack.GetLast());

		dSceneNodeInfo* const sceneNodeInfo = (dSceneNodeInfo*)GetInfoFromNode(rootNode);
		dAssert (sceneNodeInfo->IsType(dSceneNodeInfo::GetRttiType()));
		dMatrix transform (sceneNodeInfo->GetTransform() * parentMatrix);

		dVector scale;
		dMatrix matrix;
		dMatrix stretchAxis;
		transform.PolarDecomposition (matrix, scale, stretchAxis);
		sceneNodeInfo->SetTransform (matrix);

		dMatrix scaleMatrix (dGetIdentityMatrix(), scale, stretchAxis);
		sceneNodeInfo->SetGeometryTransform (sceneNodeInfo->GetGeometryTransform() * scaleMatrix);

		for (void* link = GetFirstChildLink(rootNode); link; link = GetNextChildLink(rootNode, link)) {
			dTreeNode* const node = GetNodeFromLink(link);
			dNodeInfo* const nodeInfo = GetInfoFromNode(node);
			if (nodeInfo->IsType(dSceneNodeInfo::GetRttiType())) {
				nodeStack.Append(node);
				parentMatrixStack.Append(scaleMatrix);
			}
		}
	}
}


void dScene::FreezeGeometryPivot ()
{
	dScene::dTreeNode* const geometryCache = FindGetGeometryCacheNode ();

	dList<dScene::dTreeNode*> nodeList;
	for (void* link = GetFirstChildLink(geometryCache); link; link = GetNextChildLink(geometryCache, link)) {
		dScene::dTreeNode* const meshNode = GetNodeFromLink(link);
		nodeList.Addtop(meshNode);
	}

	dList<dScene::dTreeNode*>::dListNode* nextLink; 
	for (dList<dScene::dTreeNode*>::dListNode* link = nodeList.GetFirst(); link; link = nextLink) {
		nextLink = link->GetNext();

		dScene::dTreeNode* const meshNode = GetNodeFromLink(link);
		dMeshNodeInfo* meshInfo = (dMeshNodeInfo*)GetInfoFromNode(meshNode);
		if (meshInfo->IsType(dMeshNodeInfo::GetRttiType())) {
			int parentCount = 0;
			dScene::dTreeNode* sceneNodeParent = NULL;
			for (void* meshParentlink = GetFirstParentLink(meshNode); meshParentlink; meshParentlink = GetNextParentLink(meshNode, meshParentlink)) {
				dScene::dTreeNode* const parentNode = GetNodeFromLink(meshParentlink);
				dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*)GetInfoFromNode(parentNode);
				if (sceneInfo->IsType(dSceneNodeInfo::GetRttiType())) {
					parentCount ++;
					if (!sceneNodeParent) {
						sceneNodeParent = parentNode;
					}
				}
			}

			if (parentCount > 1) {

				dScene::dTreeNode* const meshNodeCopy = CreateMeshNode (sceneNodeParent);
				for (void* matLinks = GetFirstChildLink(meshNode); matLinks; matLinks = GetNextChildLink(meshNode, matLinks)) {
					dScene::dTreeNode* const matNode = GetNodeFromLink(matLinks);
					AddReference(meshNodeCopy, matNode);
				}

				dMeshNodeInfo* const meshInfoCopy = (dMeshNodeInfo*) CloneNodeInfo (meshNode);
				dString name (dString (meshInfo->GetName()) + dString ("_clone"));
				meshInfoCopy->SetName(name.GetStr());
				SetNodeInfo (meshInfoCopy, meshNodeCopy);
				meshInfoCopy->Release();
				RemoveReference (meshNode, sceneNodeParent);

				meshInfo = meshInfoCopy;
				nextLink = link;
			}

			dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*)GetInfoFromNode(sceneNodeParent);

			dMatrix geoScaleMatrix (sceneInfo->GetGeometryTransform());
			sceneInfo->SetGeometryTransform(dGetIdentityMatrix());

			dMatrix meshPivotMatrix (meshInfo->GetPivotMatrix());
			meshInfo->SetPivotMatrix (dGetIdentityMatrix());

			dMatrix marix (meshPivotMatrix * geoScaleMatrix);
			meshInfo->BakeTransform (marix);
		}
	}
}



void dScene::BakeTransform (const dMatrix& matrix)
{
	Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dTreeNode* const node = iter.GetNode();
		dNodeInfo* const nodeInfo = GetInfoFromNode(node);
		nodeInfo->BakeTransform (matrix);
	}
}


void dScene::Serialize (const char* const fileName)
{
	// save the file, using standard localization
	char* const oldloc = setlocale( LC_ALL, 0 );
	setlocale( LC_ALL, "C" );

	TiXmlDocument asciifile;
	TiXmlDeclaration* decl = new TiXmlDeclaration( "1.0", "", "" );
	asciifile.LinkEndChild (decl);

	TiXmlElement* parentNode = new TiXmlElement (D_SCENE_ROOT_NODE_NAME);
	asciifile.LinkEndChild(parentNode);

	// save the file description and version
	TiXmlElement* header = new TiXmlElement ("header");
	parentNode->LinkEndChild(header);

	// save configuration for the main frame window
	header->SetAttribute ("description", D_SCENE_FILE_DESCRITION);

	// write the revision number
	header->SetAttribute ("revision", m_revision);

	// need to remove unused vertices's before saving, otherwise Deserialize will not work,
	RemoveUnusedVertex();

	// save file content
	dSceneGraph::Serialize (parentNode);
	asciifile.SaveFile (fileName);

	// restore locale settings
	setlocale (LC_ALL, oldloc);
}


void dScene::MergeScene (dScene* const scene)
{
	dTree<dTreeNode*,dTreeNode*> map;
	Iterator iter (*scene);

	dTreeNode* const rootNode = GetRootNode();

	map.Insert(rootNode, scene->GetRootNode());
	for (iter.Begin(); iter; iter ++) {
		dTreeNode* const node = iter.GetNode();
		dNodeInfo* const info = scene->GetInfoFromNode(node);
		if (!(info->IsType(dRootNodeInfo::GetRttiType()))) {
			dAssert (!Find (info->GetUniqueID()));

			if (info->IsType(dSceneCacheInfo::GetRttiType())) {
				dTreeNode* newNode = FindChildByName(rootNode, info->GetName());
				if (!newNode) {
					newNode = AddNode (info, NULL);
				}
				map.Insert(newNode, node);
			} else {
				dTreeNode* const newNode = AddNode (info, NULL);
				map.Insert(newNode, node);
			}
		} 
	}

	//now connect all edges
	dTree<dTreeNode*,dTreeNode*>::Iterator mapIter (map);
	for (mapIter.Begin(); mapIter; mapIter ++) {
		dTreeNode* const keyNode = mapIter.GetKey();
		dTreeNode* const curNode = mapIter.GetNode()->GetInfo();
		//dGraphNode& srcInfoHeader = mapIter.GetNode()->GetInfo()->GetInfo();
		for (void* ptr = scene->GetFirstChildLink (keyNode); ptr; ptr = scene->GetNextChildLink(keyNode, ptr)) {
			dTreeNode* const srcLinkNode = scene->GetNodeFromLink(ptr);

			dTree<dTreeNode*,dTreeNode*>::dTreeNode* const mapSaved = map.Find(srcLinkNode);
			if (mapSaved) {
				dTreeNode* const node = mapSaved->GetInfo();
				//srcInfoHeader.m_children.Append(node);
				AddReference (curNode, node);
			}
		}

		for (void* ptr = scene->GetFirstParentLink (keyNode); ptr; ptr = scene->GetNextParentLink(keyNode, ptr)) {
			dTreeNode* const srcLinkNode = scene->GetNodeFromLink(ptr);

			dTree<dTreeNode*,dTreeNode*>::dTreeNode* const mapSaved = map.Find(srcLinkNode);
			if (mapSaved) {
				dTreeNode* const node = mapSaved->GetInfo();
				//srcInfoHeader.m_parents.Append(node);
				AddReference (node, curNode);
			}
		}
	}
}

void dScene::UnmergeScene (dScene* const scene)
{
	dTree<dTreeNode*, const dNodeInfo*> map;

	dTreeNode* const sceneRootName = scene->GetRootNode();
	map.Insert(GetRootNode(), scene->GetInfoFromNode (sceneRootName));

	if (scene->FindChildByName (sceneRootName, D_TEXTURE_CACHE_NODE_MAME)) {
		map.Insert(GetTextureCacheNode(), scene->GetInfoFromNode (scene->GetTextureCacheNode()));
	}
	if (scene->FindChildByName (sceneRootName, D_MATERIAL_CACHE_NODE_MAME)) {
		map.Insert(GetMaterialCacheNode(), scene->GetInfoFromNode (scene->GetMaterialCacheNode()));
	}
	if (scene->FindChildByName (sceneRootName, D_GEOMETRY_CACHE_NODE_MAME)) {
		map.Insert(GetGeometryCacheNode(), scene->GetInfoFromNode (scene->GetGeometryCacheNode()));
	}

	for (dTreeNode* node = GetFirstNode(); node; node = GetNextNode(node)) {
		map.Insert(node, GetInfoFromNode(node));
	}

	for (dTreeNode* parentNode = scene->GetFirstNode(); parentNode; parentNode = scene->GetNextNode(parentNode)) {
		dNodeInfo* const parentInfo = GetInfoFromNode(parentNode);
		dTree<dTreeNode*, const dNodeInfo*>::dTreeNode* const parentMapNode = map.Find(parentInfo);
		if (parentMapNode) {
			dTreeNode* const myParentNode = parentMapNode->GetInfo();
			for (void* link = scene->GetFirstChildLink(parentNode); link; link = scene->GetNextChildLink(parentNode, link)) {
				dTreeNode* const childNode = scene->GetNodeFromLink(link);
				if (childNode != scene->GetRootNode()) {
					dNodeInfo* const childInfo = GetInfoFromNode(childNode);
					if (!childInfo->IsType(dSceneCacheInfo::GetRttiType())) {
						dTree<dTreeNode*, const dNodeInfo*>::dTreeNode* const mapNode = map.Find(childInfo);
						if (mapNode) {
							dTreeNode* const myChildNode = mapNode->GetInfo();
							RemoveReference(myParentNode, myChildNode);
						}
					}
				}
			}
		}
	}

	dTreeNode* const rootNode = GetRootNode();
	dTreeNode* cacheNode = FindChildByName (rootNode, D_TEXTURE_CACHE_NODE_MAME);
	if (cacheNode) { 
		if (!GetFirstChildLink(cacheNode)) {
			DeleteNode (cacheNode);
		}
	}

	cacheNode = FindChildByName (rootNode, D_MATERIAL_CACHE_NODE_MAME);
	if (cacheNode) { 
		if (!GetFirstChildLink(cacheNode)) {
			DeleteNode (cacheNode);
		}
	}

	cacheNode = FindChildByName (rootNode, D_GEOMETRY_CACHE_NODE_MAME);
	if (cacheNode) { 
		if (!GetFirstChildLink(cacheNode)) {
			DeleteNode (cacheNode);
		}
	}
}

void dScene::DeleteNode (dTreeNode* const node)
{
	dSceneGraph::DeleteNode (node);
}

void dScene::DeleteDuplicateTextures()
{
	dTree<dScene::dTreeNode*, dCRCTYPE> dictionary;
	dTree<dScene::dTreeNode*, dScene::dTreeNode*> textureNodeMap;

	dScene::dTreeNode* const textureCacheNode = GetTextureCacheNode();
	dAssert (textureCacheNode);

	for (void* link = GetFirstChildLink(textureCacheNode); link; link = GetNextChildLink(textureCacheNode, link)) {
		dScene::dTreeNode* const textureNode = GetNodeFromLink(link);
		dTextureNodeInfo* const textureInfo = (dTextureNodeInfo*)GetInfoFromNode(textureNode);

		dCRCTYPE signature = textureInfo->GetId();
		dTree<dScene::dTreeNode*, dCRCTYPE>::dTreeNode* dictionaryNode = dictionary.Find(signature);
		if (!dictionaryNode) {
			dictionaryNode = dictionary.Insert(textureNode, signature);
		}
		textureNodeMap.Insert(dictionaryNode->GetInfo(), textureNode);
	}

	dScene::dTreeNode* const materialCacheNode = GetMaterialCacheNode();
	for (void* link0 = GetFirstChildLink(materialCacheNode); link0; link0 = GetNextChildLink(materialCacheNode, link0)) {
		dScene::dTreeNode* const materialNode0 = GetNodeFromLink(link0);
		dMaterialNodeInfo* const materialInfo0 = (dMaterialNodeInfo*)GetInfoFromNode(materialNode0);

		if (materialInfo0->IsType(dMaterialNodeInfo::GetRttiType())) {

			dList<dScene::dTreeNode*> textureNodes;
			for (void* link1 = GetFirstChildLink(materialNode0); link1; link1 = GetNextChildLink(materialNode0, link1)) {
				dScene::dTreeNode* const textureNode1 = GetNodeFromLink(link1);
				dTextureNodeInfo* const materialInfo1 = (dTextureNodeInfo*)GetInfoFromNode(textureNode1);
				if (materialInfo1->IsType(dTextureNodeInfo::GetRttiType())) {
					textureNodes.Append(textureNode1);
				}
			}

			while (textureNodes.GetCount()) {

				dScene::dTreeNode* const textureNode = textureNodes.GetFirst()->GetInfo();
				textureNodes.Remove(textureNodes.GetFirst());

				dScene::dTreeNode* const reUsedTextureNode = textureNodeMap.Find(textureNode)->GetInfo();

				RemoveReference(materialNode0, textureNode);
				AddReference(materialNode0, reUsedTextureNode);
			}
		}
	}

	void* nextLink;
	for (void* link0 = GetFirstChildLink(textureCacheNode); link0; link0 = nextLink) {
		nextLink = GetNextChildLink(textureCacheNode, link0);
		dScene::dTreeNode* const node0 = GetNodeFromLink(link0);
		int parents = 0;
		for (void* link1 = GetFirstParentLink(node0); link1; link1 = GetNextParentLink(node0, link1)) {
			parents ++;
		}
		if (parents == 1) {
			RemoveReference(textureCacheNode, node0);
		}
	}
}

void dScene::DeleteDuplicateMaterials()
{
	DeleteDuplicateTextures();

	int id = 0;
	dTree<int, dScene::dTreeNode*> materialIDMap;
	dTree<dScene::dTreeNode*, dCRCTYPE> dictionary;
	dTree<dScene::dTreeNode*, dScene::dTreeNode*> materialNodeMap;

	dScene::dTreeNode* const materialCacheNode = GetMaterialCacheNode();
	dAssert (materialCacheNode);

	for (void* link = GetFirstChildLink(materialCacheNode); link; link = GetNextChildLink(materialCacheNode, link)) {
		dScene::dTreeNode* const materialNode = GetNodeFromLink(link);
		dMaterialNodeInfo* const materialInfo = (dMaterialNodeInfo*)GetInfoFromNode(materialNode);

		dCRCTYPE signature = materialInfo->CalculateSignature();
		dTree<dScene::dTreeNode*, dCRCTYPE>::dTreeNode* dictionaryNode = dictionary.Find(signature);

		int oldId = materialInfo->GetId(); 
		if (!dictionaryNode) {
			dictionaryNode = dictionary.Insert(materialNode, signature);
			materialInfo->SetId(id);
			id ++;
		}

		materialIDMap.Insert(oldId, materialNode);
		materialNodeMap.Insert(dictionaryNode->GetInfo(), materialNode);
	}


	dScene::dTreeNode* const geometryCacheNode = GetGeometryCacheNode();
	for (void* link0 = GetFirstChildLink(geometryCacheNode); link0; link0 = GetNextChildLink(geometryCacheNode, link0)) {
		dScene::dTreeNode* const geometryNode0 = GetNodeFromLink(link0);
		dGeometryNodeInfo* const geometryInfo0 = (dGeometryNodeInfo*)GetInfoFromNode(geometryNode0);

		if (geometryInfo0->IsType(dGeometryNodeInfo::GetRttiType())) {
			dList<dScene::dTreeNode*> materialNodes;
			for (void* link1 = GetFirstChildLink(geometryNode0); link1; link1 = GetNextChildLink(geometryNode0, link1)) {
				dScene::dTreeNode* const materialNode1 = GetNodeFromLink(link1);
				dMaterialNodeInfo* const materialInfo1 = (dMaterialNodeInfo*)GetInfoFromNode(materialNode1);
				if (materialInfo1->IsType(dMaterialNodeInfo::GetRttiType())) {
					materialNodes.Append(materialNode1);
				}
			}

			while (materialNodes.GetCount()) {
				dScene::dTreeNode* const materialNode = materialNodes.GetFirst()->GetInfo();
				materialNodes.Remove(materialNodes.GetFirst());

				dScene::dTreeNode* const reUsedMaterialNode = materialNodeMap.Find(materialNode)->GetInfo();

				//dMaterialNodeInfo* const materialInfo = (dMaterialNodeInfo*)GetInfoFromNode(materialNode);
				dMaterialNodeInfo* const reUsedMaterialInfo = (dMaterialNodeInfo*)GetInfoFromNode(reUsedMaterialNode);

				int newID = reUsedMaterialInfo->GetId();
				int oldID = materialIDMap.Find(materialNode)->GetInfo();

				if (newID != oldID) {
					dMeshNodeInfo* const meshInfo = (dMeshNodeInfo*)geometryInfo0;
					if (meshInfo->IsType(dMeshNodeInfo::GetRttiType())) {
						NewtonMesh* const mesh = meshInfo->GetMesh();
						for (void* face = NewtonMeshGetFirstFace(mesh); face; face = NewtonMeshGetNextFace(mesh, face)) {
							if (!NewtonMeshIsFaceOpen(mesh, face)) {
								if (NewtonMeshGetFaceMaterial(mesh, face) == oldID) {
									NewtonMeshSetFaceMaterial(mesh, face, newID);
								}
							}
						}
					} else {
						dAssert (0);
					}
				}

				// important to remove the reference first, because there cannot be duplicate edge in the graph
				RemoveReference(geometryNode0, materialNode);
				AddReference(geometryNode0, reUsedMaterialNode);
				
			}
		}
	}

	void* nextLink;
	for (void* link0 = GetFirstChildLink(materialCacheNode); link0; link0 = nextLink) {
		nextLink = GetNextChildLink(materialCacheNode, link0);
		dScene::dTreeNode* const node0 = GetNodeFromLink(link0);
		int parents = 0;
		for (void* link1 = GetFirstParentLink(node0); link1; link1 = GetNextParentLink(node0, link1)) {
			parents ++;
		}
		if (parents == 1) {
			RemoveReference(materialCacheNode, node0);
		}
	}
}

void dScene::DeleteDuplicateGeometries()
{
	DeleteDuplicateMaterials();

	dTree<dScene::dTreeNode*, dCRCTYPE> dictionary;
	dTree<dScene::dTreeNode*, dScene::dTreeNode*> geometryNodeMap;

	dScene::dTreeNode* const geometryCacheNode = GetGeometryCacheNode();
	dAssert (geometryCacheNode);

	for (void* link = GetFirstChildLink(geometryCacheNode); link; link = GetNextChildLink(geometryCacheNode, link)) {
		dScene::dTreeNode* const geometryNode = GetNodeFromLink(link);
		dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*)GetInfoFromNode(geometryNode);

		dCRCTYPE signature = geometryInfo->CalculateSignature();
		dTree<dScene::dTreeNode*, dCRCTYPE>::dTreeNode* dictionaryNode = dictionary.Find(signature);
		if (!dictionaryNode) {
			dictionaryNode = dictionary.Insert(geometryNode, signature);
		}
		geometryNodeMap.Insert(dictionaryNode->GetInfo(), geometryNode);
	}


	dScene::Iterator iter (*this);
	for (iter.Begin(); iter; iter ++) {
		dScene::dTreeNode* const sceneNode = iter.GetNode();
		dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*)GetInfoFromNode(sceneNode);

		if (sceneInfo->IsType(dSceneNodeInfo::GetRttiType())) {

			dList<dScene::dTreeNode*> geometryNodes;
			for (void* link = GetFirstChildLink(sceneNode); link; link = GetNextChildLink(sceneNode, link)) {
				dScene::dTreeNode* const geometryNode = GetNodeFromLink(link);
				dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*)GetInfoFromNode(geometryNode);
				if (geometryInfo->IsType(dGeometryNodeInfo::GetRttiType())) {
					geometryNodes.Append(geometryNode);
				}
			}

			while (geometryNodes.GetCount()) {

				dScene::dTreeNode* const geometryNode = geometryNodes.GetFirst()->GetInfo();
				geometryNodes.Remove(geometryNodes.GetFirst());

				dScene::dTreeNode* const reUsedGeometryNode = geometryNodeMap.Find(geometryNode)->GetInfo();

			   // important to remove the reference first, because there cannot be duplicate edge in the graph
				RemoveReference(sceneNode, geometryNode);
				AddReference(sceneNode, reUsedGeometryNode);
			}
		}
	}


	void* nextLink;
	for (void* link0 = GetFirstChildLink(geometryCacheNode); link0; link0 = nextLink) {
		nextLink = GetNextChildLink(geometryCacheNode, link0);
		dScene::dTreeNode* const node0 = GetNodeFromLink(link0);
		int parents = 0;
		for (void* link1 = GetFirstParentLink(node0); link1; link1 = GetNextParentLink(node0, link1)) {
			parents ++;
		}
		if (parents == 1) {
			RemoveReference(geometryCacheNode, node0);
		}
	}
}

void dScene::RemoveUnusedMaterials()
{
	dScene::dTreeNode* const materialCacheNode = FindGetMaterialCacheNode();
	if (materialCacheNode) {
		void* nextMaterialLink;
		for (void* materialLink = GetFirstChildLink(materialCacheNode); materialLink; materialLink = nextMaterialLink) {
			nextMaterialLink = GetNextChildLink(materialCacheNode, materialLink);
			dTreeNode* const materialNode = GetNodeFromLink(materialLink);
			if (!GetNextParentLink(materialCacheNode, GetFirstParentLink(materialNode))) {
				RemoveReference (materialNode, materialCacheNode);
			}
		}
	}
}

void dScene::RemoveUnusedVertex()
{
	dScene::dTreeNode* const geometryCacheNode = FindGetGeometryCacheNode();
	if (geometryCacheNode) {
		for (void* link = GetFirstChildLink(geometryCacheNode); link; link = GetNextChildLink(geometryCacheNode, link)) {
			dTreeNode* const node = GetNodeFromLink(link);
			dNodeInfo* const info = node->GetInfo().GetNode();
			if (info->IsType(dMeshNodeInfo::GetRttiType())) {
				dMeshNodeInfo* const mesh = (dMeshNodeInfo*) info;
				mesh->RemoveUnusedVertices(this, node);
			}
		}
	}
}


void dScene::SetNodeLRU (dTreeNode* const node, int lru) 
{
	node->GetInfo().SetLRU(lru);
}

int dScene::GetNodeLRU (dTreeNode* const node) const 
{ 
	return node->GetInfo().GetLRU();
}



//dScene::dTreeNode* dScene::RayCast (const dVector& p0, const dVector& p1) const
dFloat dScene::RayCast (const dVector& globalP0, const dVector& globalP1, dList<dTreeNode*>& traceToRoot) const
{
	dFloat t = 1.2f;
	dVector p0 (globalP0);
	dVector p2 (globalP1);
	p0.m_w = 1.0f;
	p2.m_w = 1.0f;

	dList<int> parentIndex;
	dList<dMatrix> rootMatrix;
	dList<dTreeNode*> rootNodes;
	
	for (void* link0 = GetFirstChildLink(GetRootNode()); link0; link0 = GetNextChildLink(GetRootNode(), link0)) {
		dTreeNode* const node0 = GetNodeFromLink(link0);
		dSceneNodeInfo* const sceneInfo0 = (dSceneNodeInfo*) GetInfoFromNode(node0);
		if (sceneInfo0->IsType(dSceneNodeInfo::GetRttiType())){
			rootMatrix.Append(dGetIdentityMatrix());
			rootNodes.Append(node0);
			parentIndex.Append(0);
		}
	}
	
	dTreeNode* trace[128];
	traceToRoot.RemoveAll();
	dVector globalP1p0 (globalP1 - globalP0);
	dFloat	den = 1.0f / globalP1p0.DotProduct3(globalP1p0);
	while (rootNodes.GetCount()) {
		dTreeNode* const node0 = rootNodes.GetLast()->GetInfo();
		dMatrix parentMatrix (rootMatrix.GetLast()->GetInfo());
		int index = parentIndex.GetLast()->GetInfo();
		trace[index] = node0;
		index ++;

		parentIndex.Remove(parentIndex.GetLast());
		rootNodes.Remove(rootNodes.GetLast());
		rootMatrix.Remove(rootMatrix.GetLast());

		const dSceneNodeInfo* const sceneInfo0 = (dSceneNodeInfo*) GetInfoFromNode(node0);
		dAssert (sceneInfo0->IsType(dSceneNodeInfo::GetRttiType()));

 		dMatrix matrix (sceneInfo0->GetTransform() * parentMatrix);
		dMatrix invMatrix (matrix.Inverse4x4());
		dVector q0 (invMatrix.RotateVector4x4(p0));
		dVector q2 (invMatrix.RotateVector4x4(p2));
		dFloat t1 = sceneInfo0->RayCast(q0, q2);
		if (t1 < 1.0f) {
			dScene::dTreeNode* const geomNode = FindChildByType(node0, dGeometryNodeInfo::GetRttiType());
			if (geomNode) {
				dGeometryNodeInfo* const geometryInfo = (dGeometryNodeInfo*) GetInfoFromNode(geomNode);
				t1 = geometryInfo->RayCast(q0, q2);
				if (t1 < 1.0f) {
					dVector p1p0 (p2 - p0);
					p2 = p0 + p1p0.Scale (t1);
					t = den * p1p0.DotProduct3(globalP1p0);
					p2.m_w = 1.0f;
					traceToRoot.RemoveAll();
					for (int i = 0; i < index; i ++) {
						traceToRoot.Append(trace[i]);
					}
				}
			}
		}

		for (void* link1 = GetFirstChildLink(node0); link1; link1 = GetNextChildLink(node0, link1)) {
			dTreeNode* const sceneNode1 = GetNodeFromLink(link1);
			dSceneNodeInfo* const sceneInfo1 = (dSceneNodeInfo*) GetInfoFromNode(sceneNode1);
			if (sceneInfo1->IsType(dSceneNodeInfo::GetRttiType())){
				rootMatrix.Append(matrix);
				rootNodes.Append(sceneNode1);
				parentIndex.Append(index);
			}
		}
	}

	return t;
}


void dScene::SceneToNewtonWorld (NewtonWorld* world, dList<NewtonBody*>& loadedBodies)
{
	// Load the Physics scene
	for (dTreeNode* node = GetFirstNode (); node; node = GetNextNode (node)) {
		dNodeInfo* const info = GetInfoFromNode(node);

		if (info->GetTypeId() == dRigidbodyNodeInfo::GetRttiType()) {
			dRigidbodyNodeInfo* const bodyData = (dRigidbodyNodeInfo*) info;
			NewtonBody* const rigidBody = bodyData->CreateNewtonBody(world, this, node);
			loadedBodies.Append(rigidBody);
		}
	}
}


struct dSceneNodeCollisionPair
{
	dScene::dTreeNode* m_mesh;
	dScene::dTreeNode* m_collision;
};

void dScene::NewtonWorldToScene (const NewtonWorld* const world, dSceneExportCallback* const visualContext)
{
	// search for all collision mesh and create make a dictionary
	dTree<dSceneNodeCollisionPair, NewtonCollision*> dictionary;
	
	int count = 0;
	dScene::dTreeNode* const materialNode = CreateMaterialNode (0);
	for (NewtonBody* body = NewtonWorldGetFirstBody(world); body; body = NewtonWorldGetNextBody(world, body)) {
		NewtonCollision* const collision = NewtonBodyGetCollision(body);
		dTree<dSceneNodeCollisionPair, NewtonCollision*>::dTreeNode* node = dictionary.Find(collision);
		if (!node) {
			char meshName[256];
			sprintf (meshName, "mesh_%d", count);
			count ++;
			NewtonMesh* const mesh = visualContext->CreateVisualMesh(body, meshName, sizeof (meshName));

			dScene::dTreeNode* const meshNode = CreateMeshNode(GetRootNode());
			AddReference(meshNode, materialNode);

			dMeshNodeInfo* const info = (dMeshNodeInfo*)GetInfoFromNode(meshNode);
			info->ReplaceMesh (mesh);
			info->SetName(meshName);

			NewtonCollisionInfoRecord collsionRecord;
			NewtonCollisionGetInfo(collision, &collsionRecord);

			// extract the offset matrix form the collision
			dMatrix& offsetMatrix = *((dMatrix*)&collsionRecord.m_offsetMatrix[0][0]);
			info->BakeTransform (offsetMatrix.Inverse());
			info->SetPivotMatrix(offsetMatrix * info->GetPivotMatrix());

			dScene::dTreeNode* const collisionNode = CreateCollisionFromNewtonCollision(GetRootNode(), collision);

			dSceneNodeCollisionPair pair;
			pair.m_mesh = meshNode;
			pair.m_collision = collisionNode;

			node = dictionary.Insert(pair, collision);
		} 
		
		// add a visual mesh
		dSceneNodeCollisionPair& info = node->GetInfo();
		dScene::dTreeNode* const sceneNode = CreateSceneNode(GetRootNode());
		dSceneNodeInfo* const sceneInfo = (dSceneNodeInfo*) GetInfoFromNode(sceneNode);
		dMatrix matrix;
		NewtonBodyGetMatrix(body, &matrix[0][0]);
		sceneInfo->SetTransform(matrix);
		AddReference(sceneNode, info.m_mesh);


		// add a rigid body
		dScene::dTreeNode* const sceneBody = CreateRigidbodyNode(sceneNode);
		AddReference(sceneBody, info.m_collision);

		dRigidbodyNodeInfo* const bodyInfo = (dRigidbodyNodeInfo*) GetInfoFromNode(sceneBody);

		dVector com;
		NewtonBodyGetCentreOfMass(body, &com[0]);
		bodyInfo->SetCenterOfMass(com);

		dVector massMatrix;
		NewtonBodyGetMass(body, &massMatrix.m_w, &massMatrix.m_x, &massMatrix.m_y, &massMatrix.m_z);
		bodyInfo->SetMassMatrix(massMatrix);

		dVector veloc;
		NewtonBodyGetVelocity(body, &veloc[0]);
		bodyInfo->SetVelocity(veloc);

		dVector omega;
		NewtonBodyGetOmega(body, &omega[0]);
		bodyInfo->SetOmega(omega);

		dVariable* var = bodyInfo->CreateVariable ("rigidBodyType");
		var->SetValue("default gravity");
	}

	void* nextPtr = NULL;
	for (void* ptr = GetFirstChildLink (GetRootNode()); ptr; ptr = nextPtr) {
		nextPtr = GetNextChildLink(GetRootNode(), ptr);
		dScene::dTreeNode* const node = GetNodeFromLink(ptr);
		dNodeInfo* const info = GetInfoFromNode(node);
		if ((info->IsType(dMeshNodeInfo::GetRttiType())) || (info->IsType(dCollisionNodeInfo::GetRttiType()))) {
			RemoveReference(node, GetRootNode());	
		}
	}
}


bool dScene::Deserialize (const char* const fileName)
{
	// apply last Configuration, using standard localization
	//static char* oldloc = setlocale( LC_ALL, 0 );
	setlocale( LC_ALL, "C" );

	TiXmlDocument doc (fileName);
	doc.LoadFile();
	dAssert (!doc.Error());


	bool state = true;
	const TiXmlElement* const rootNode = doc.RootElement();
	if (rootNode && (doc.FirstChild (D_SCENE_ROOT_NODE_NAME) || doc.FirstChild ("alchemedia"))){

		TiXmlElement* root = (TiXmlElement*) doc.FirstChild (D_SCENE_ROOT_NODE_NAME);
		if (!root) {
			// this is a legacy file description
			root = (TiXmlElement*) doc.FirstChild ("alchemedia");
		}
		dAssert (root);

		TiXmlElement* const header = (TiXmlElement*) root->FirstChild ("header");
		dAssert (header);

		header->Attribute("revision", &m_revision);

		TiXmlElement* const nodes = (TiXmlElement*) root->FirstChild ("nodes");
		dAssert (nodes);

		state = dSceneGraph::Deserialize (nodes);

		// promote older revision to current revisions 
		if (GetRevision() < 101) {
			// change SceneNodeMatrix Matrix from global to local
			m_revision = 101;
			MakeSceneNodeMatricesLocalToNodeParent (this);
		}

		if (GetRevision() < 102) {
			// added a texture cache, material cache, and mesh cache as children of root node.
			m_revision = 102;
			AddTextureCacheMaterianCacheMeshCache (this);
		}

		// update the revision to latest 
		if (GetRevision() < 104) {
			m_revision = 104;
			RemoveLocalTransformFromGeometries (this);
		}
		
	}

	// restore locale settings
	//setlocale( LC_ALL, ""oldloc );
	setlocale( LC_ALL, "");
	return state;
}


