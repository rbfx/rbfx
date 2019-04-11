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


#include "dSceneStdafx.h"
#include "dMaterialNodeInfo.h"
#include <tinyxml.h>

D_IMPLEMENT_CLASS_NODE(dMaterialNodeInfo);

dMaterialNodeInfo::dMaterialNodeInfo()
	:dNodeInfo (), 
	m_ambientColor (0.8f, 0.8f, 0.8f, 1.0f), 
	m_diffuseColor(0.8f, 0.8f, 0.8f, 1.0f),
	m_specularColor (0.0f, 0.0f, 0.0f, 1.0f), 
	m_emissiveColor (0.0f, 0.0f, 0.0f, 1.0f),
	m_shininess (0.0f), 
	m_opacity (1.0f), 
	m_ambientTexId (-1), 
	m_diffuseTexId (-1), 
	m_specularTexId (-1), 
	m_emissiveTexId(-1),
	m_id(0)
{
	SetName ("material");
}

dMaterialNodeInfo::dMaterialNodeInfo(dScene* const world)
	:dNodeInfo (), 
	m_ambientColor (0.8f, 0.8f, 0.8f, 1.0f), 
	m_diffuseColor(0.8f, 0.8f, 0.8f, 1.0f),
	m_specularColor (0.0f, 0.0f, 0.0f, 1.0f), 
	m_emissiveColor (0.0f, 0.0f, 0.0f, 1.0f),
	m_shininess (0.0f), 
	m_opacity (1.0f), 
	m_ambientTexId (-1), 
	m_diffuseTexId (-1), 
	m_specularTexId (-1), 
	m_emissiveTexId(-1),
	m_id(0) 
{
	SetName ("material");
}

dMaterialNodeInfo::dMaterialNodeInfo(int id)
	:dNodeInfo (), 
	m_ambientColor (0.8f, 0.8f, 0.8f, 1.0f), 
	m_diffuseColor(0.8f, 0.8f, 0.8f, 1.0f),
	m_specularColor (0.0f, 0.0f, 0.0f, 1.0f), 
	m_emissiveColor (0.0f, 0.0f, 0.0f, 1.0f), 
	m_shininess (0.0f), 
	m_opacity (1.0f), 
	m_ambientTexId (-1), 
	m_diffuseTexId (-1), 
	m_specularTexId (-1), 
	m_emissiveTexId(-1),
	m_id(id) 
{
	SetName ("material");
}

dMaterialNodeInfo::~dMaterialNodeInfo(void)
{
}
/*
void dMaterialNodeInfo::InitFrom (const dMaterialNodeInfo& src, )
{
	m_ambientColor = src.m_ambientColor;
	m_diffuseColor = src.m_diffuseColor;
	m_specularColor = src.m_specularColor;
	m_emissiveColor = src.m_emissiveColor;
	m_shininess = src.m_shininess;
	m_opacity = src.m_opacity;
	m_ambientTexId = src.m_ambientTexId;
	m_diffuseTexId = src.m_diffuseTexId;
	m_specularTexId = src.m_specularTexId;
	m_emissiveTexId = src.m_emissiveTexId;
}
*/

dCRCTYPE dMaterialNodeInfo::CalculateSignature() const
{
	dCRCTYPE signature = 0;
	signature = dCRC64 (&m_ambientColor, sizeof (m_ambientColor), signature);
	signature = dCRC64 (&m_diffuseColor, sizeof (m_diffuseColor), signature);
	signature = dCRC64 (&m_specularColor, sizeof (m_specularColor), signature);
	signature = dCRC64 (&m_emissiveColor, sizeof (m_emissiveColor), signature);
	signature = dCRC64 (&m_shininess, sizeof (m_shininess), signature);
	
	signature = dCRC64 (&m_opacity, sizeof (m_opacity), signature);
	signature = dCRC64 (&m_ambientTexId, sizeof (m_ambientTexId), signature);
	signature = dCRC64 (&m_diffuseTexId, sizeof (m_diffuseTexId), signature);
	signature = dCRC64 (&m_specularTexId, sizeof (m_specularTexId), signature);
	signature = dCRC64 (&m_emissiveTexId, sizeof (m_emissiveTexId), signature);
	return signature;
}

void dMaterialNodeInfo::Serialize (TiXmlElement* const rootNode) 
{
	SerialiseBase(dNodeInfo, rootNode);

	char buffer[4096];

//	itoa64____ (m_id, id, 10);
//	rootNode->SetAttribute("id", id);
	rootNode->SetAttribute("id", m_id);

//	char id[256];
	TiXmlElement* const ambient = new TiXmlElement ("ambient");
	rootNode->LinkEndChild(ambient);
	dFloatArrayToString(&m_ambientColor[0], 4, buffer, sizeof (buffer));

	//itoa64____ (m_ambientTexId, id, 10);
	dString id (m_ambientTexId);
	ambient->SetAttribute("textureId", id.GetStr());
	ambient->SetAttribute("color", buffer);

	TiXmlElement* const diffuse = new TiXmlElement ("diffuse");
	rootNode->LinkEndChild(diffuse);
	dFloatArrayToString(&m_diffuseColor[0], 4, buffer, sizeof (buffer));

	//itoa64____ (m_diffuseTexId, id, 10);
	id = dString (m_diffuseTexId);
	diffuse->SetAttribute("textureId", id.GetStr());
	diffuse->SetAttribute("color", buffer);

	TiXmlElement* const specular = new TiXmlElement ("specular");
	rootNode->LinkEndChild(specular);
	dFloatArrayToString(&m_specularColor[0], 4, buffer, sizeof (buffer));

	//itoa64____ (m_specularTexId, id, 10);
	id =  dString (m_specularTexId);
	specular->SetAttribute("textureId", id.GetStr());
	specular->SetAttribute("color", buffer);

	TiXmlElement* const emissive = new TiXmlElement ("emissive");
	rootNode->LinkEndChild(emissive);
	dFloatArrayToString(&m_emissiveColor[0], 4, buffer, sizeof (buffer));

	//itoa64____ (m_emissiveTexId, id, 10);
	id =  dString (m_emissiveTexId);
	emissive->SetAttribute("textureId", id.GetStr());
	emissive->SetAttribute("color", buffer);

	TiXmlElement* const shininess = new TiXmlElement ("shininess");
	rootNode->LinkEndChild(shininess);
	shininess->SetDoubleAttribute ("float", m_shininess);

	TiXmlElement* const opacity = new TiXmlElement ("opacity");
	rootNode->LinkEndChild(opacity);
	opacity->SetDoubleAttribute ("float", m_opacity);
}

bool dMaterialNodeInfo::Deserialize (const dScene* const scene, TiXmlElement* const rootNode) 
{
	DeserialiseBase(scene, dNodeInfo, rootNode);

	char text[1024];

	rootNode->Attribute("id", &m_id);

	TiXmlElement* const ambient = (TiXmlElement*) rootNode->FirstChild ("ambient");
	sprintf (text, "%s", ambient->Attribute("textureId"));
	m_ambientTexId = dString((const char*)text).ToInteger64();
	dStringToFloatArray (ambient->Attribute("color"), &m_ambientColor[0], 4);

	TiXmlElement* const diffuse = (TiXmlElement*) rootNode->FirstChild ("diffuse");
	sprintf (text, "%s", diffuse->Attribute("textureId"));
	m_diffuseTexId = dString((const char*)text).ToInteger64();
	dStringToFloatArray (diffuse->Attribute("color"), &m_diffuseColor[0], 4);

	TiXmlElement* const specular = (TiXmlElement*) rootNode->FirstChild ("specular");

	sprintf (text, "%s", specular->Attribute("textureId"));
	m_emissiveTexId = dString((const char*)text).ToInteger64();
	dStringToFloatArray (specular->Attribute("color"), &m_specularColor[0], 4);

	TiXmlElement* const emissive = (TiXmlElement*) rootNode->FirstChild ("emissive");
	sprintf (text, "%s", emissive->Attribute("textureId"));
	m_emissiveTexId = dString((const char*)text).ToInteger64();
	dStringToFloatArray (emissive->Attribute("color"), &m_emissiveColor[0], 4);

	TiXmlElement* const shininess = (TiXmlElement*) rootNode->FirstChild ("shininess");
	double value;
	shininess->Attribute("float", &value);
	m_shininess = dFloat (value);
	
	TiXmlElement* const opacity = (TiXmlElement*) rootNode->FirstChild ("opacity");
	opacity->Attribute("float", &value);
	m_opacity = dFloat (value);

	return true;
}


