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

#ifndef __D_ARRAY__
#define __D_ARRAY__

template<class T>
class dArray
{
	public:
	dArray();
	dArray(int size);
	~dArray();

	T& operator[] (int i);
	const T& operator[] (int i) const;

	int GetSize() const;
	void Resize(int size) const;

	protected:
	mutable int m_capacity;
	mutable T* m_data;
};

template<class T>
dArray<T>::dArray()
	:m_capacity(0)
	,m_data(NULL)
{
}

template<class T>
dArray<T>::dArray(int size)
	:m_capacity(0)
	,m_data(NULL)
{
	Resize(size);
}


template<class T>
dArray<T>::~dArray()
{
	if (m_data) {
		delete[] m_data;
	}
}


template<class T>
T& dArray<T>::operator[] (int i)
{
	dAssert(i >= 0);
	while (i >= m_capacity) {
		Resize(i * 2);
	}
	return m_data[i];
}

template<class T>
const T& dArray<T>::operator[] (int i) const
{
	dAssert(i >= 0);
	while (i >= m_capacity) {
		Resize(i * 2);
	}
	return m_data[i];
}

template<class T>
int dArray <T>::GetSize() const
{
	return m_capacity;
}

template<class T>
void dArray <T>::Resize(int size) const
{
	if (size >= m_capacity) {
		T* const newArray = new T[size];
		if (m_data) {
			for (int i = 0; i < m_capacity; i++) {
				newArray[i] = m_data[i];
			}
			delete[] m_data;
		}
		m_data = newArray;
		m_capacity = size;
	} else if (size < m_capacity) {
		T* const newArray = new T[size];
		if (m_data) {
			for (int i = 0; i < size; i++) {
				newArray[i] = m_data[i];
			}
			delete[] m_data;
		}
		m_data = newArray;
		m_capacity = size;
	}
}

#endif
