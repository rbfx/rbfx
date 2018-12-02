// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include <stdio.h>
#include <tchar.h>
#include <string.h>

// TODO: reference additional headers your program requires here
#include <dMap.h>
#include <dList.h>
#include <dString.h>
#include <dMathDefines.h>

#include <imgui.h>
#include <glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <glfw3native.h>
#endif


template<class T>
class dArray
{
	public:
	dArray()
		:m_data(new T[1])
		,m_count(0)
		,m_maxCount(1)
	{
		memset (m_data, 0, sizeof (T));
	}

	T& operator[] (int i)
	{
		Resize(i);
		return m_data[i];
	}

	const T& operator[] (int i) const
	{
		return m_data[i];
	}

	int GetSize() const
	{
		return m_count;
	}

	void Push(T val)
	{
		Resize(m_count + 1);
		m_data[m_count] = val;
		m_count++;
	}

	private:
	void Resize(int index)
	{
		int currentMaxCount = m_maxCount;
		while (index >= m_maxCount) {
			T* const data = new T[m_maxCount * 2];
			memcpy(data, m_data, currentMaxCount * sizeof(T));
			delete[] m_data;
			m_data = data;
			m_maxCount = m_maxCount * 2;
		}
	}

	protected:
	T* m_data;
	int m_count;
	int m_maxCount;
};
