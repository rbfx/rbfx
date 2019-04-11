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

#ifndef _D_SCENE_H_
#define _D_SCENE_H_

#include "dSceneGraph.h"

class NewtonWorld;
class dCameraNodeInfo;
class dRootNodeInfo;
class dMaterialNodeInfo;
class dTextureNodeInfo;

#define D_SCENE_ROOT_NODE_NAME		"NewtonGameDynamnics"
#define D_SCENE_FILE_DESCRITION		"Newton Dynamnics 3.00, universal file format"

// adding first revision number to the file
//#define D_SCENE_REVISION_NUMBER 100

// change SceneNodeMatrix Matrix from global to local
//#define D_SCENE_REVISION_NUMBER 101

// added a texture cache, material cache, and mesh cache as children of root node.
//#define D_SCENE_REVISION_NUMBER 102

// skip this revision number
//#define D_SCENE_REVISION_NUMBER 103

// adding the geometry transform node info 
//#define D_SCENE_REVISION_NUMBER 104

// adding node ID as part of the file
#define D_SCENE_REVISION_NUMBER 105


#define D_TEXTURE_CACHE_NODE_MAME	"dTextureCache"
#define D_MATERIAL_CACHE_NODE_MAME	"dMaterialCache"
#define D_GEOMETRY_CACHE_NODE_MAME	"dGeometryCache"

class dScene: public dSceneGraph, public dRefCounter
{
	public:

	class dSceneExportCallback
	{
		public:
		// this call back should here the use should take the user data from the body create newtonMesh from it and return that back
		virtual NewtonMesh* CreateVisualMesh (NewtonBody* const body, char* const name, int maxNameSize) const = 0;

		// Get the material information for this visual Mesh material ID
		//... 
		void SetMaterialInfo (dMaterialNodeInfo& materialInfo, const NewtonMesh* const mesh, const NewtonBody* const body, int MaterialID) {}
		void SetTextureInfo (dTextureNodeInfo& textureInfo, const NewtonMesh* const mesh, const NewtonBody* const body, int MaterialID) {}
	};

	DSCENE_API dScene(NewtonWorld* const newton);
	DSCENE_API dScene(const dScene& me);
	DSCENE_API virtual ~dScene(void);
	DSCENE_API virtual void CleanUp();
	DSCENE_API virtual dTreeNode* GetRootNode() const;

	DSCENE_API virtual dTreeNode* CreateAnimationTake();
	DSCENE_API virtual dTreeNode* CreateAnimationTrack(dTreeNode* const take);
	DSCENE_API virtual dTreeNode* CreateModelNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateSceneNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateGeometryTransformNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateBoneNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateRigidbodyNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionBoxNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionConeNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionSphereNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionCapsuleNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionCylinderNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionCompoundNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionConvexHullNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionChamferCylinderNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisioTreeNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateMeshNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateLineNode(dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateSkinModifierNode(dTreeNode* const parent);

	DSCENE_API virtual dTreeNode* CreateMaterialNode (int id);
	DSCENE_API virtual dTreeNode* CreateTextureNode (const char* const pathName);

	DSCENE_API virtual dTreeNode* GetTextureCacheNode ();
	DSCENE_API virtual dTreeNode* GetMaterialCacheNode ();
	DSCENE_API virtual dTreeNode* GetGeometryCacheNode ();
	
	DSCENE_API virtual dTreeNode* FindTextureCacheNode () const;
	DSCENE_API virtual dTreeNode* FindGetMaterialCacheNode () const;
	DSCENE_API virtual dTreeNode* FindGetGeometryCacheNode () const;

	DSCENE_API virtual dTreeNode* AddNode(dNodeInfo* const sceneInfo, dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateNode (const char* const className, dTreeNode* const parent);
	DSCENE_API virtual dTreeNode* CreateCollisionFromNewtonCollision(dTreeNode* const parent, NewtonCollision* const collision);
	
	DSCENE_API virtual void AddReference(dTreeNode* const parent, dTreeNode* const child);
	DSCENE_API virtual void RemoveReference(dTreeNode* const node1, dTreeNode* const node2);

	DSCENE_API virtual dTreeNode* GetFirstNode () const;
	DSCENE_API virtual dTreeNode* GetNextNode (dTreeNode* const node) const;
	DSCENE_API virtual dTreeNode* FindNode (dNodeInfo* const info) const;
	 
	DSCENE_API virtual void* GetFirstChildLink(dTreeNode* const parentNode) const;
	DSCENE_API virtual void* GetNextChildLink(dTreeNode* const parentNode, void* const link) const;

	DSCENE_API virtual void* GetFirstParentLink(dTreeNode* const childNode) const;
	DSCENE_API virtual void* GetNextParentLink(dTreeNode* const childNode, void* const link) const;

	DSCENE_API virtual dTreeNode* GetNodeFromLink (void* const child) const;
	DSCENE_API virtual dNodeInfo* CloneNodeInfo(dTreeNode* const node) const;
	DSCENE_API virtual dNodeInfo* GetInfoFromNode(dTreeNode* const node) const;
	
	DSCENE_API virtual dTreeNode* FindMaterialById(int materialId);
	DSCENE_API virtual dTreeNode* FindMaterialBySignature(dCRCTYPE signature);

	DSCENE_API virtual dTreeNode* FindTextureByTextId(dCRCTYPE textId);
	DSCENE_API virtual dTreeNode* FindMaterialById(dTreeNode* const parentNode, int materialId) const;
	DSCENE_API virtual dTreeNode* FindTextureByTextId(dTreeNode* const parentNode, dCRCTYPE textId) const;

	DSCENE_API virtual dTreeNode* FindChildByName(dTreeNode* const parentNode, const char* const name) const;
	DSCENE_API virtual dTreeNode* FindChildByType(dTreeNode* const parentNode, dCRCTYPE type) const;
	DSCENE_API virtual dTreeNode* FindParentByType(dTreeNode* const child, dCRCTYPE type) const;

	DSCENE_API virtual NewtonWorld* GetNewtonWorld() const {return m_newton;}

	DSCENE_API virtual void DeleteNode (dTreeNode* const node);
	DSCENE_API virtual void MergeScene (dScene* const scene);
	DSCENE_API virtual void UnmergeScene (dScene* const scene);

	DSCENE_API virtual void Serialize (const char* const fileName);
	DSCENE_API virtual bool Deserialize (const char* const fileName);

	DSCENE_API virtual dFloat RayCast (const dVector& p0, const dVector& p1, dList<dTreeNode*>& traceRoot) const;

	DSCENE_API virtual void FreezeScale ();
	DSCENE_API virtual void FreezeGeometryPivot ();
	DSCENE_API virtual void FreezeRootRotation ();
	DSCENE_API virtual void BakeTransform (const dMatrix& matrix);
	
	DSCENE_API virtual int GetRevision() const;

	DSCENE_API virtual void RemoveUnusedVertex();
	DSCENE_API virtual void RemoveUnusedMaterials();

	DSCENE_API virtual void SetNodeLRU (dTreeNode* const node, int lru);
	DSCENE_API virtual int GetNodeLRU (dTreeNode* const node) const;

	DSCENE_API virtual void NewtonWorldToScene (const NewtonWorld* const world, dSceneExportCallback* const visualContext);
	DSCENE_API virtual void SceneToNewtonWorld (NewtonWorld* const world, dList<NewtonBody*>& loadedBodies);

	DSCENE_API virtual void DeleteDuplicateTextures();
	DSCENE_API virtual void DeleteDuplicateMaterials();
	DSCENE_API virtual void DeleteDuplicateGeometries();
	
	protected:
	DSCENE_API virtual void Serialize(TiXmlElement* const parentNode);
	DSCENE_API virtual bool Deserialize(TiXmlElement* const parentNode);

	DSCENE_API void RegisterClasses();
	DSCENE_API virtual dTreeNode* GetCacheNode (const char* const cacheName);

	int m_revision;

	NewtonWorld* m_newton;
};


#endif