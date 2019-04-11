/////////////////////////////////////////////////////////////////////////////
// Name:        dMaterialNodeInfo.h
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

#ifndef _D_MATERIAL_NODE_H_
#define _D_MATERIAL_NODE_H_

#include "dNodeInfo.h"


class dMaterialNodeInfo: public dNodeInfo
{
	public:
	D_DEFINE_CLASS_NODE(dMaterialNodeInfo,dNodeInfo,DSCENE_API)

	dMaterialNodeInfo();
	dMaterialNodeInfo(dScene* const world);
	dMaterialNodeInfo(int id);
	virtual ~dMaterialNodeInfo(void);

	virtual void SetId (int id){m_id = id;}
	virtual int GetId () const {return m_id;}

	virtual void SetAmbientTextId(dCRCTYPE id) {m_ambientTexId = id;}
	virtual void SetDiffuseTextId(dCRCTYPE id) {m_diffuseTexId = id;}
	virtual void SetSpecularTextId(dCRCTYPE id) {m_specularTexId = id;}
	virtual void SetEmissiveTextId(dCRCTYPE id) {m_emissiveTexId = id;}

	virtual dCRCTYPE GetAmbientTextId() const {return m_ambientTexId;}
	virtual dCRCTYPE GetDiffuseTextId() const {return m_diffuseTexId;}
	virtual dCRCTYPE GetSpecularTextId() const {return m_specularTexId;}
	virtual dCRCTYPE GetEmissiveTextId() const {return m_emissiveTexId;}

	virtual void SetAmbientColor(const dVector& color) {m_ambientColor = color;}
	virtual void SetDiffuseColor(const dVector& color) {m_diffuseColor = color;}
	virtual void SetSpecularColor(const dVector& color) {m_specularColor = color;}
	virtual void SetEmissiveColor(const dVector& color) {m_emissiveColor = color;}
	virtual void SetShininess(dFloat power) {m_shininess = power;}
	virtual void SetOpacity(dFloat value) {m_opacity = value;}

	virtual const dVector& GetAmbientColor() const {return m_ambientColor;}
	virtual const dVector& GetDiffuseColor() const {return m_diffuseColor;}
	virtual const dVector& GetSpecularColor() const {return m_specularColor;}
	virtual const dVector& GetEmissiveColor() const {return m_emissiveColor;}
	virtual dFloat GetShininess() const {return m_shininess;}
	virtual dFloat GetOpacity() const {return m_opacity;}
	
	virtual dCRCTYPE CalculateSignature() const; 

	protected:
	virtual void Serialize (TiXmlElement* const rootNode); 
	virtual bool Deserialize (const dScene* const scene, TiXmlElement* const rootNode);

	dVector m_ambientColor;
	dVector m_diffuseColor;
	dVector m_specularColor;
	dVector m_emissiveColor;
	dFloat m_shininess;
	dFloat m_opacity;
	dCRCTYPE m_ambientTexId;
	dCRCTYPE m_diffuseTexId;
	dCRCTYPE m_specularTexId;
	dCRCTYPE m_emissiveTexId;
	int m_id;
	friend class dScene;
};





#endif