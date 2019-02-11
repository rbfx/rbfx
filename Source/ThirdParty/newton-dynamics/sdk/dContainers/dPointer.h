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


#ifndef __D_POINTER_H__
#define __D_POINTER_H__


template <typename T>
class dPointer
{
	public:
	dPointer(T* const pValue);
	~dPointer();
	T& operator* ();
	T* operator-> ();

	private:
	T*	m_data; 
};

template<class T>
dPointer<T>::dPointer(T* pValue)
	:m_data(pValue)
{
}

template<class T>
dPointer<T>::~dPointer()
{
	delete m_data;
}

template<class T>
T& dPointer<T>::operator* ()
{
	return *m_data;
}

template<class T>
T* dPointer<T>::operator-> ()
{
	return m_data;
}

#endif