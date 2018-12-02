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


#ifndef __Hierarchy__
#define __Hierarchy__

#include "dCRC.h"
#include "dString.h"
#include "dContainersAlloc.h"



class dBaseHierarchy: public dContainersAlloc
{
	public:
	DCONTAINERS_API dBaseHierarchy* GetChild () const;
	DCONTAINERS_API dBaseHierarchy* GetParent () const;
	DCONTAINERS_API dBaseHierarchy* GetSibling () const;

	DCONTAINERS_API void Detach ();
	DCONTAINERS_API void Attach (dBaseHierarchy* const parent, bool addFirst = false);
	
	DCONTAINERS_API dBaseHierarchy* GetRoot () const;
	DCONTAINERS_API dBaseHierarchy* GetFirst() const;
	DCONTAINERS_API dBaseHierarchy* GetLast() const;
	DCONTAINERS_API dBaseHierarchy* GetNext() const;
	DCONTAINERS_API dBaseHierarchy* GetPrev() const;

	DCONTAINERS_API dBaseHierarchy* Find (dCRCTYPE nameCRC) const; 
	DCONTAINERS_API dBaseHierarchy* Find (const char* const name) const;

	DCONTAINERS_API long long GetNameID() const;
	DCONTAINERS_API const dString& GetName() const;
	DCONTAINERS_API void SetNameID(const char* const name);
	
	protected:
	DCONTAINERS_API dBaseHierarchy ();
	DCONTAINERS_API dBaseHierarchy (const char* const name);
	DCONTAINERS_API dBaseHierarchy (const dBaseHierarchy &clone);
	virtual DCONTAINERS_API ~dBaseHierarchy ();

	virtual DCONTAINERS_API dBaseHierarchy* CreateClone () const = 0;

	private:
	inline void Clear();

	dString m_name;
	long long m_nameID;
	dBaseHierarchy* m_parent;
	dBaseHierarchy* m_child;
	dBaseHierarchy* m_sibling;
};

template<class T>
class dHierarchy: public dBaseHierarchy
{
	public:
	dHierarchy ();
	dHierarchy (const char* const name);
	void Attach (T* const parent, bool addFirst = false);
	void Detach ();
	T* GetChild () const;
	T* GetParent () const;
	T* GetSibling () const;
	T* GetRoot () const;
	T* GetFirst() const;
	T* GetLast() const;
	T* GetNext() const;
	T* GetPrev() const;
	T* Find (long long nameCRC) const;
	T* Find (const char* const name) const;

	protected:
	dHierarchy (const T &clone);
	virtual ~dHierarchy ();
};


inline dBaseHierarchy::dBaseHierarchy ()
{
	Clear ();
}

inline dBaseHierarchy::dBaseHierarchy (const char* const name)
{
	Clear ();
	SetNameID (name);
}


inline void dBaseHierarchy::Clear()
{
	m_child = NULL;
	m_parent = NULL;
	m_sibling = NULL;
	m_nameID = 0;
	m_name = (char*)NULL;
}


inline dBaseHierarchy* dBaseHierarchy::GetChild () const
{
	return m_child;
}

inline dBaseHierarchy* dBaseHierarchy::GetSibling () const
{
	return m_sibling;
}

inline dBaseHierarchy* dBaseHierarchy::GetParent () const
{
	return m_parent;
}


inline dBaseHierarchy* dBaseHierarchy::Find (const char* const name) const
{
	return Find (dCRC64 (name)); 
} 

inline void dBaseHierarchy::SetNameID(const char* const name)
{
	m_nameID = dCRC64 (name);
	m_name = name;
}

inline long long  dBaseHierarchy::GetNameID() const
{
	return m_nameID;
}

inline const dString& dBaseHierarchy::GetName() const
{
	return m_name;
}


template<class T>
dHierarchy<T>::dHierarchy ()
	:dBaseHierarchy ()
{
}

template<class T>
dHierarchy<T>::dHierarchy (const T &clone)
	:dBaseHierarchy (clone)
{
}

template<class T>
dHierarchy<T>::dHierarchy (const char* const name)
	:dBaseHierarchy (name)
{
}

template<class T>
dHierarchy<T>::~dHierarchy () 
{
}


//template<class T>
//dBaseHierarchy* dHierarchy<T>::CreateClone () const
//{
//	return new T (*(T*)this);
//}

template<class T>
void dHierarchy<T>::Attach (T* const parent, bool addFirst)
{
	dBaseHierarchy::Attach(parent, addFirst);
}

template<class T>
void dHierarchy<T>::Detach ()
{
	dBaseHierarchy::Detach ();
}

template<class T>
T* dHierarchy<T>::GetChild () const
{
	return (T*) dBaseHierarchy::GetChild();
}

template<class T>
T* dHierarchy<T>::GetSibling () const
{
	return (T*) dBaseHierarchy::GetSibling ();
}

template<class T>
T* dHierarchy<T>::GetParent () const
{
	return (T*) dBaseHierarchy::GetParent ();
}


template<class T>
T* dHierarchy<T>::GetRoot () const
{
	return (T*) dBaseHierarchy::GetRoot ();
}


template<class T>
T* dHierarchy<T>::GetFirst() const
{
	return (T*) dBaseHierarchy::GetFirst ();
}

template<class T>
T* dHierarchy<T>::GetLast() const
{
	return (T*) dBaseHierarchy::GetLast ();
}


template<class T>
T* dHierarchy<T>::GetNext() const
{
	return (T*) dBaseHierarchy::GetNext ();
}

template<class T>
T* dHierarchy<T>::GetPrev() const
{
	return (T*) dBaseHierarchy::GetPrev ();
}


template<class T>
T* dHierarchy<T>::Find (dCRCTYPE nameCRC) const 
{
	return (T*) dBaseHierarchy::Find (nameCRC);
}

template<class T>
T* dHierarchy<T>::Find (const char* const name) const
{
	return (T*) dBaseHierarchy::Find (name);
} 


#endif

