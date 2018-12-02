/////////////////////////////////////////////////////////////////////////////
// Name:        dSceneNodeInfo.h
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

#ifndef _D_SCENE_RENDER_H_
#define _D_SCENE_RENDER_H_ 

class dScene;

class dSceneRender: public dRefCounter
{
	public:
	DSCENE_API dSceneRender(void);
	DSCENE_API virtual ~dSceneRender(void);

	virtual bool Init() = 0;
	virtual int GetViewPortWidth() const = 0;
	virtual int GetViewPortHeight() const = 0;
	virtual dMatrix GetProjectionMatrix () const = 0;
	virtual dMatrix GetModelViewMatrix() const = 0; 

	virtual void SetModelViewMatrix(const dMatrix& modelview) = 0; 
	virtual void SetProjectionMatrix (const dMatrix& modelview) = 0;

	virtual void SetOrtographicProjection (int width, int height, dFloat minPlane, dFloat maxPlane) = 0;
	virtual void SetPerspectiveProjection (int width, int height, dFloat fov, dFloat frontPlane, dFloat backPlane) = 0;

	virtual void Print (int displayListFont, dFloat x, dFloat y, const char* const fmt, ... ) = 0;
	
	virtual void LoadMatrix(const dMatrix& matrix) = 0;
	virtual void PushMatrix(const dMatrix& matrix) = 0;
	virtual void PopMatrix() = 0;

	virtual void BeginRender() = 0;
	virtual void EndRender() = 0;

	virtual int CreateDisplayList(int range = 1) = 0;
	virtual void DestroyDisplayList(int lists, int range = 1) = 0;

	virtual void BeginDisplayList(int displayList) = 0;
	virtual void EndDisplayList() = 0;
	virtual void DrawDisplayList(int displayList) = 0;

	// material interface
	virtual void EnableZbuffer() = 0;
	virtual void DisableZbuffer() = 0;
	virtual void EnableBackFace() = 0;
	virtual void DisableBackFace() = 0;
	virtual void EnableBlend() = 0;
	virtual void DisableBlend() = 0;
	virtual void EnableLighting() = 0;
	virtual void DisableLighting() = 0;
	virtual void EnableTexture() = 0;
	virtual void DisableTexture() = 0;
	virtual void EnableZBias(dFloat val) = 0;
	virtual void DisableZBias() = 0;
	virtual const dVector& GetColor() const = 0;
	virtual void SetColor(const dVector& color) = 0;

	virtual void SetMaterialDiffuse(const dVector& color) = 0;
	virtual void SetMaterialAmbient(const dVector& color) = 0;
	virtual void SetMaterialSpecular(const dVector& color) = 0;
	virtual void SetMaterialShininess(dFloat normalizedPower) = 0;
	

	virtual int GetCachedWireframeDisplayList(NewtonMesh* const mesh) = 0; 
	virtual int GetCachedFlatShadedDisplayList(NewtonMesh* const mesh) = 0; 
	virtual void InvalidateCachedDisplayList(const NewtonMesh* const mesh) = 0; 
		

	// primitive drawing functions
	virtual void BeginLine() = 0;
	virtual void BeginTriangle() = 0;
	virtual void SubmitNormal(const dVector& normal) = 0;
	virtual void SubmitVertex(const dVector& posit) = 0;
	virtual void End() = 0;

	DSCENE_API virtual dVector GlobalToScreen (const dVector& global) const;
	DSCENE_API virtual dVector ScreenToGlobal (const dVector& screen) const;

	DSCENE_API virtual void DrawLine(const dVector& p0, const dVector& p1);
	DSCENE_API virtual void DrawTriangle(const dVector& p0, const dVector& p1, const dVector& p2);

	DSCENE_API virtual void DrawCone(int segments, dFloat radius, dFloat heigh);
	DSCENE_API virtual void DrawCylinder(int segments, dFloat radius, dFloat heigh);
	DSCENE_API virtual void DrawArrow (int segments, dFloat radius, dFloat heigh, const dVector& stemColor, const dVector& tipColor);
};




#endif