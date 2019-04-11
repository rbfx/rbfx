/////////////////////////////////////////////////////////////////////////////
// Name:        dVariable.h
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

#ifndef _D_VARIABLE_H_
#define _D_VARIABLE_H_

class dScene;

class dVariable
{	
	public:
	enum dType
	{
		m_int,
		m_float,
		m_string,
	};

	dVariable ();
	dVariable (const dVariable& me);
	virtual ~dVariable ();

	virtual dType GetType() const;

	virtual void SetName (const char* const name);
	virtual const char* GetName() const;
	virtual void SetValue (int value);
	virtual void SetValue (float value);
	virtual void SetValue (const char* const value);

	virtual int GetInt () const;
	virtual dFloat GetFloat () const;
	virtual const char* GetString () const;

	

	private:
	dType m_type;
	dString m_name;
	union
	{
		char*	m_data;
		int		m_integer;
		dFloat	m_real;
	};	

	friend class dVariableList;
};

class dVariableList: public dTree<dVariable, dCRCTYPE>
{
	public:
	dVariableList();
	dVariableList(const dVariableList& me);
	virtual ~dVariableList ();

	virtual dVariable* CreateVariable (const char* const name);

	virtual dVariable* FindVariable(dCRCTYPE crc) const;
	virtual dVariable* FindVariable(const char* const name) const;

	virtual void Serialize(TiXmlElement* const rootNode);
	virtual bool Deserialize(const dScene* const scene, TiXmlElement* const rootNode);
};

#endif