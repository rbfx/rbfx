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

#ifndef __dVector__
#define __dVector__


#include "dMathDefines.h"


// small but very effective 4 dimensional template vector class 

template<class T>
class TemplateVector
{
	public:
	TemplateVector ();
	TemplateVector (const T val);
	TemplateVector (const T* const ptr);
	TemplateVector (T m_x, T m_y, T m_z, T m_w = T(1.0f)); 
	TemplateVector Scale (T s) const;

	T& operator[] (int i);
	const T& operator[] (int i) const;

	TemplateVector operator+ (const TemplateVector &A) const; 
	TemplateVector operator- (const TemplateVector &A) const; 
	TemplateVector operator* (const TemplateVector &A) const; 
	TemplateVector& operator+= (const TemplateVector &A);
	TemplateVector& operator-= (const TemplateVector &A); 
	TemplateVector& operator*= (const TemplateVector &A); 

	TemplateVector Abs() const;
	TemplateVector Min(const TemplateVector &A) const;
	TemplateVector Max(const TemplateVector &A) const;

	T DotProduct3 (const TemplateVector &A) const; 
	TemplateVector Normalize () const; 
	TemplateVector CrossProduct (const TemplateVector &A) const; 

	T m_x;
	T m_y;
	T m_z;
	T m_w;
};

class dVector: public TemplateVector<dFloat>
{
	public:
	dVector()
		:TemplateVector<dFloat>()
	{
	}

	dVector(dFloat val)
		:TemplateVector<dFloat>(val)
	{
	}

	dVector (const TemplateVector<dFloat>& v)
		:TemplateVector<dFloat>(v)
	{
	}

	dVector (const dFloat* const ptr)
		:TemplateVector<dFloat>(ptr)
	{
	}

	dVector (dFloat x, dFloat y, dFloat z, dFloat w = 1.0f) 
		:TemplateVector<dFloat>(x, y, z, w)
	{
	}

#ifndef _NEWTON_USE_DOUBLE
	dVector (const TemplateVector<dFloat64>& v)
		:TemplateVector<dFloat>(dFloat(v.m_x), dFloat(v.m_y), dFloat(v.m_z), dFloat(v.m_w))
	{
	}
#endif
};


class dBigVector: public TemplateVector<dFloat64>
{
	public: 
	dBigVector(){};
	dBigVector(dFloat64 val)
		:TemplateVector<dFloat64> (val)
	{
	}

	dBigVector (const dFloat64* const ptr)
		:TemplateVector<dFloat64> (ptr)
	{
	}

	dBigVector (const TemplateVector<dFloat64>& v)
		:TemplateVector<dFloat64> (v.m_x, v.m_y, v.m_z, v.m_w)
	{
	}

	dBigVector(const dVector& v)
		:TemplateVector<dFloat64>(v.m_x, v.m_y, v.m_z, v.m_w)
	{
	}

	dBigVector (dFloat64 x, dFloat64 y, dFloat64 z, dFloat64 w = dFloat(1.0f))
		:TemplateVector<dFloat64> (x, y, z, w)
	{
	}
};


template<class T>
TemplateVector<T>::TemplateVector() 
{
}

template<class T>
TemplateVector<T>::TemplateVector(const T val)
	:m_x (val), m_y(val), m_z(val), m_w(val)
{
}

template<class T>
TemplateVector<T>::TemplateVector(const T *ptr)
	:m_x (ptr[0]), m_y(ptr[1]), m_z(ptr[2]), m_w(0.0f)
{
}

template<class T>
TemplateVector<T>::TemplateVector(T x, T y, T z, T w) 
	:m_x (x), m_y(y), m_z(z), m_w(w)
{
}

template<class T>
T& TemplateVector<T>::operator[] (int i)
{
	return (&m_x)[i];
}	

template<class T>
const T& TemplateVector<T>::operator[] (int i) const
{
	return (&m_x)[i];
}

template<class T>
TemplateVector<T> TemplateVector<T>::Scale (T scale) const
{
	return TemplateVector<T> (m_x * scale, m_y * scale, m_z * scale, m_w);
}

template<class T>
TemplateVector<T> TemplateVector<T>::operator+ (const TemplateVector<T>& B) const
{
	return TemplateVector<T> (m_x + B.m_x, m_y + B.m_y, m_z + B.m_z, m_w);
}

template<class T>
TemplateVector<T> TemplateVector<T>::operator* (const TemplateVector<T>& B) const
{
	return TemplateVector<T>(m_x * B.m_x, m_y * B.m_y, m_z * B.m_z, m_w);
}


template<class T>
TemplateVector<T>& TemplateVector<T>::operator+= (const TemplateVector<T>& A) 
{
	m_x += A.m_x;
	m_y += A.m_y;
	m_z += A.m_z;
	return *this;
}

template<class T>
TemplateVector<T> TemplateVector<T>::operator- (const TemplateVector<T>& A) const
{
	return TemplateVector<T> (m_x - A.m_x, m_y - A.m_y, m_z - A.m_z, m_w);
}

template<class T>
TemplateVector<T>& TemplateVector<T>::operator-= (const TemplateVector<T>& A) 
{
	m_x -= A.m_x;
	m_y -= A.m_y;
	m_z -= A.m_z;
	return *this;
}

template<class T>
TemplateVector<T>& TemplateVector<T>::operator*= (const TemplateVector<T>& A)
{
	m_x *= A.m_x;
	m_y *= A.m_y;
	m_z *= A.m_z;
	//m_w *= A.m_w;
	return *this;
}

template<class T>
TemplateVector<T> TemplateVector<T>::Abs() const
{
	return dVector(dAbs(m_x), dAbs(m_y), dAbs(m_z), dAbs(m_w));
}

template<class T>
TemplateVector<T> TemplateVector<T>::Min(const TemplateVector &A) const
{
	return dVector (dMin (m_x, A.m_x), dMin (m_y, A.m_y), dMin (m_z, A.m_z), dMin (m_w, A.m_w));
}

template<class T>
TemplateVector<T> TemplateVector<T>::Max(const TemplateVector &A) const
{
	return dVector (dMax (m_x, A.m_x), dMax (m_y, A.m_y), dMax (m_z, A.m_z), dMax (m_w, A.m_w));
}


template<class T>
T TemplateVector<T>::DotProduct3 (const TemplateVector<T>& A) const
{
	return m_x * A.m_x + m_y * A.m_y + m_z * A.m_z;
}

template<class T>
TemplateVector<T> TemplateVector<T>::Normalize () const
{
	T mag (DotProduct3(*this));
	return Scale (1.0f / T(sqrt (mag)));
}

template<class T>
TemplateVector<T> TemplateVector<T>::CrossProduct (const TemplateVector<T>& A) const
{
	return TemplateVector<T> (m_y * A.m_z - m_z * A.m_y, m_z * A.m_x - m_x * A.m_z, m_x * A.m_y - m_y * A.m_x, m_w);
}


class dSpatialVector
{
	public:
	inline dSpatialVector()
	{
	}

	inline dSpatialVector(const dFloat a)
	{
		for (int i = 0; i < 6; i++) {
			m_d[i] = a;
		}
	}

	inline dSpatialVector(const dVector& low, const dVector& high)
	{
		m_d[0] = low[0];
		m_d[1] = low[1];
		m_d[2] = low[2];
		m_d[3] = high[0];
		m_d[4] = high[1];
		m_d[5] = high[2];
	}

	inline dFloat& operator[] (int i)
	{
		dAssert(i < 6);
		dAssert(i >= 0);
		return m_d[i];
	}

	inline const dFloat& operator[] (int i) const
	{
		dAssert(i < 6);
		dAssert(i >= 0);
		return m_d[i];
	}

	inline dSpatialVector operator+ (const dSpatialVector& A) const
	{
		dSpatialVector tmp;
		for (int i = 0; i < 6; i++) {
			tmp[i] = m_d[i] + A.m_d[i];
		}
		return tmp;
	}

	inline dSpatialVector operator* (const dSpatialVector& A) const
	{
		dSpatialVector tmp;
		for (int i = 0; i < 6; i++) {
			tmp[i] = m_d[i] * A.m_d[i];
		}
		return tmp;
	}

	inline dFloat DotProduct(const dSpatialVector& v) const
	{
		dFloat acc = dFloat (0.0f);
		for (int i = 0; i < 6; i++) {
			acc += m_d[i] * v.m_d[i];
		}
		return acc;
	}

	inline dSpatialVector Scale(dFloat s) const
	{
		dSpatialVector tmp;
		for (int i = 0; i < 6; i++) {
			tmp[i] = m_d[i] * s;
		}
		return tmp;
	}

	dFloat m_d[6];
};

#endif

