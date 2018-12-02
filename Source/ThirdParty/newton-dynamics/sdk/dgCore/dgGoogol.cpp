/* Copyright (c) <2003-2016> <Julio Jerez, Newton Game Dynamics>
* 
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
* 
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely, subject to the following restrictions:
* 
* 1. The origin of this software must not be misrepresented; you must not
* claim that you wrote the original software. If you use this software
* in a product, an acknowledgment in the product documentation would be
* appreciated but is not required.
* 
* 2. Altered source versions must be plainly marked as such, and must not be
* misrepresented as being the original software.
* 
* 3. This notice may not be removed or altered from any source distribution.
*/


#include "dgStdafx.h"
#include "dgGoogol.h"

dgGoogol dgGoogol::m_zero(0.0); 
dgGoogol dgGoogol::m_one(1.0); 
dgGoogol dgGoogol::m_two(2.0);  
dgGoogol dgGoogol::m_three(3.0);   
dgGoogol dgGoogol::m_half(0.5);   

#ifdef _IMPLEMENT_USING_INTEGER_ARITHMETIC_

	dgGoogol::dgGoogol(void)
		:m_sign(0)
		,m_exponent(0)
	{
		memset (m_mantissa, 0, sizeof (m_mantissa));
	}

	dgGoogol::dgGoogol(dgFloat64 value)
		:m_sign(0)
		,m_exponent(0)
	{
		dgInt32 exp;
		dgFloat64 mantissa = fabs (frexp(value, &exp));

		m_exponent = dgInt16 (exp);
		m_sign = (value >= 0) ? 0 : 1;

		memset (m_mantissa, 0, sizeof (m_mantissa));
		m_mantissa[0] = dgUnsigned64 (dgFloat64 (dgUnsigned64(1)<<62) * mantissa);

		// it looks like GCC have problems with this
		//dgAssert (m_mantissa[0] >= 0);
		dgAssert ((m_mantissa[0] & dgUnsigned64(1)<<63) == 0);
	}

	void dgGoogol::CopySignedMantissa (dgUnsigned64* const mantissa) const
	{
		memcpy (mantissa, m_mantissa, sizeof (m_mantissa));
		if (m_sign) {
			NegateMantissa (mantissa);
		}
	}

	dgGoogol::operator double() const
	{
		dgFloat64 mantissa = (dgFloat64(1.0f) / dgFloat64 (dgUnsigned64(1)<<62)) * dgFloat64 (m_mantissa[0]);
		mantissa = ldexp(mantissa, m_exponent) * (m_sign ?  dgFloat64 (-1.0f) : dgFloat64 (1.0f));
		return mantissa;
	}

	dgGoogol dgGoogol::operator+ (const dgGoogol &A) const
	{
		dgGoogol tmp;
		dgAssert (dgInt64 (m_mantissa[0]) >= 0);
		dgAssert (dgInt64 (A.m_mantissa[0]) >= 0);

		if (m_mantissa[0] && A.m_mantissa[0]) {
			dgUnsigned64 mantissa0[DG_GOOGOL_SIZE];
			dgUnsigned64 mantissa1[DG_GOOGOL_SIZE];
			dgUnsigned64 mantissa[DG_GOOGOL_SIZE];

			CopySignedMantissa (mantissa0);
			A.CopySignedMantissa (mantissa1);

			dgInt32 exponetDiff = m_exponent - A.m_exponent;
			dgInt32 exponent = m_exponent;
			if (exponetDiff > 0) {
				ShiftRightMantissa (mantissa1, exponetDiff);
			} else if (exponetDiff < 0) {
				exponent = A.m_exponent;
				ShiftRightMantissa (mantissa0, -exponetDiff);
			} 

			dgUnsigned64 carrier = 0;
			for (dgInt32 i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
				dgUnsigned64 m0 = mantissa0[i];
				dgUnsigned64 m1 = mantissa1[i];
				mantissa[i] = m0 + m1 + carrier;
				carrier = CheckCarrier (m0, m1) | CheckCarrier (m0 + m1, carrier);
			}

			dgInt8 sign = 0;
			if (dgInt64 (mantissa[0]) < 0) {
				sign = 1;
				NegateMantissa (mantissa);
			}

			dgInt32 bits = NormalizeMantissa (mantissa);
			if (bits <= (-64 * DG_GOOGOL_SIZE)) {
				tmp.m_sign = 0;
				tmp.m_exponent = 0;
			} else {
				tmp.m_sign = sign;
				tmp.m_exponent =  dgInt16 (exponent + bits);
			}

			memcpy (tmp.m_mantissa, mantissa, sizeof (m_mantissa));


		} else if (A.m_mantissa[0]) {
			tmp = A;
		} else {
			tmp = *this;
		}

		dgAssert (dgInt64 (tmp.m_mantissa[0]) >= 0);
		return tmp;
	}

	dgGoogol dgGoogol::operator- (const dgGoogol &A) const
	{
		dgGoogol tmp (A);
		tmp.m_sign = !tmp.m_sign;
		return *this + tmp;
	}

	void dgGoogol::ScaleMantissa (dgUnsigned64* const dst, dgUnsigned64 scale) const
	{
		dgUnsigned64 carrier = 0;
		for (dgInt32 i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
			if (m_mantissa[i]) {
				dgUnsigned64 low;
				dgUnsigned64 high;
				ExtendeMultiply (scale, m_mantissa[i], high, low);
				dgUnsigned64 acc = low + carrier;
				carrier = CheckCarrier (low, carrier);	
				dgAssert (CheckCarrier (carrier, high) == 0);
				carrier += high;
				dst[i + 1] = acc;
			} else {
				dst[i + 1] = carrier;
				carrier = 0;
			}

		}
		dst[0] = carrier;
	}

	dgGoogol dgGoogol::operator* (const dgGoogol &A) const
	{
		dgAssert (dgInt64 (m_mantissa[0]) >= 0);
		dgAssert (dgInt64 (A.m_mantissa[0]) >= 0);

		if (m_mantissa[0] && A.m_mantissa[0]) {
			dgUnsigned64 mantissaAcc[DG_GOOGOL_SIZE * 2];
			memset (mantissaAcc, 0, sizeof (mantissaAcc));
			for (dgInt32 i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
				dgUnsigned64 a = m_mantissa[i];
				if (a) {
					dgUnsigned64 mantissaScale[2 * DG_GOOGOL_SIZE];
					memset (mantissaScale, 0, sizeof (mantissaScale));
					A.ScaleMantissa (&mantissaScale[i], a);

					dgUnsigned64 carrier = 0;
					for (dgInt32 j = 0; j < 2 * DG_GOOGOL_SIZE; j ++) {
						const dgInt32 k = 2 * DG_GOOGOL_SIZE - 1 - j;
						dgUnsigned64 m0 = mantissaAcc[k];
						dgUnsigned64 m1 = mantissaScale[k];
						mantissaAcc[k] = m0 + m1 + carrier;
						carrier = CheckCarrier (m0, m1) | CheckCarrier (m0 + m1, carrier);
					}
				}
			}

			dgUnsigned64 carrier = 0;
			//dgInt32 bits = dgUnsigned64(LeadingZeros (mantissaAcc[0]) - 2);
			dgInt32 bits = LeadingZeros (mantissaAcc[0]) - 2;
			for (dgInt32 i = 0; i < 2 * DG_GOOGOL_SIZE; i ++) {
				const dgInt32 k = 2 * DG_GOOGOL_SIZE - 1 - i;
				dgUnsigned64 a = mantissaAcc[k];
				mantissaAcc[k] = (a << dgUnsigned64(bits)) | carrier;
				carrier = a >> dgUnsigned64(64 - bits);
			}

			dgInt32 exp = m_exponent + A.m_exponent - (bits - 2);

			dgGoogol tmp;
			tmp.m_sign = m_sign ^ A.m_sign;
			tmp.m_exponent = dgInt16 (exp);
			memcpy (tmp.m_mantissa, mantissaAcc, sizeof (m_mantissa));

			return tmp;
		} 
		return dgGoogol(0.0);
	}

	dgGoogol dgGoogol::operator/ (const dgGoogol &A) const
	{
		dgGoogol tmp (1.0 / A);
		tmp = tmp * (m_two - A * tmp);
		tmp = tmp * (m_two - A * tmp);
		int test = 0;
		dgInt32 passes = 0;
		do {
			passes ++;
			dgGoogol tmp0 (tmp);
			tmp = tmp * (m_two - A * tmp);
			test = memcmp (&tmp0, &tmp, sizeof (dgGoogol));
		} while (test && (passes < (2 * DG_GOOGOL_SIZE)));	
		dgAssert (passes <= (2 * DG_GOOGOL_SIZE));
		return (*this) * tmp;
	}


	dgGoogol dgGoogol::Abs () const
	{
		dgGoogol tmp (*this);
		tmp.m_sign = 0;
		return tmp;
	}

	dgGoogol dgGoogol::Floor () const
	{
		if (m_exponent < 1) {
			return dgGoogol (0.0);
		} 
		dgInt32 bits = m_exponent + 2;
		dgInt32 start = 0;
		while (bits >= 64) {
			bits -= 64;
			start ++;
		}

		dgGoogol tmp (*this);
		for (dgInt32 i = DG_GOOGOL_SIZE - 1; i > start; i --) {
			tmp.m_mantissa[i] = 0;
		}
		// some compilers do no like this and I do not know why is that
		//dgUnsigned64 mask = (-1LL) << (64 - bits);
		dgUnsigned64 mask (~0ULL);
		mask <<= (64 - bits);
		tmp.m_mantissa[start] &= mask;
		if (m_sign) {
			dgAssert (0);
		}

		return tmp;
	}

	dgGoogol dgGoogol::InvSqrt () const
	{
		const dgGoogol& me = *this;
		dgGoogol x (1.0f / sqrt (me));

		dgInt32 test = 0;
		dgInt32 passes = 0;
		do {
			passes ++;
			dgGoogol tmp (x);
			x = m_half * x * (m_three - me * x * x);
			test = memcmp (&x, &tmp, sizeof (dgGoogol));
		} while (test && (passes < (2 * DG_GOOGOL_SIZE)));	
		dgAssert (passes <= (2 * DG_GOOGOL_SIZE));
		return x;
	}

	dgGoogol dgGoogol::Sqrt () const
	{
		return *this * InvSqrt();
	}

	void dgGoogol::ToString (char* const string) const
	{
		dgGoogol tmp (*this);
		dgGoogol base (10.0);
		while (dgFloat64 (tmp) > 1.0) {
			tmp = tmp/base;
		}

		dgInt32 index = 0;
		while (tmp.m_mantissa[0]) {
			tmp = tmp * base;
			dgGoogol digit (tmp.Floor());
			tmp -= digit;
			dgFloat64 val = digit;
			string[index] = char (val) + '0';
			index ++;
		}
		string[index] = 0;
	}


#else 

	dgGoogol::dgGoogol(void)
		:m_value(0.0)
	{
	}

	dgGoogol::dgGoogol(dgFloat64 value)
		:m_value (value)
	{
	}

	void dgGoogol::CopySignedMantissa (dgUnsigned64* const mantissa) const
	{
	}

	dgGoogol::operator double() const
	{
		return m_value;
	}

	dgGoogol dgGoogol::operator+ (const dgGoogol &A) const
	{
		return m_value + A.m_value;
	}


	dgGoogol dgGoogol::operator- (const dgGoogol &A) const
	{
		return m_value - A.m_value;
	}

	dgGoogol dgGoogol::operator* (const dgGoogol &A) const
	{
		return m_value * A.m_value;
	}

	dgGoogol dgGoogol::operator/ (const dgGoogol &A) const
	{
		return m_value / A.m_value;
	}


	dgGoogol dgGoogol::Abs () const
	{
		return fabs (m_value);
	}

	dgGoogol dgGoogol::InvSqrt () const
	{
		return 1.0 / sqrt (m_value);
	}

	dgGoogol dgGoogol::Sqrt () const
	{
		return sqrt(m_value);
	}


	dgGoogol dgGoogol::Floor () const
	{
		return floor (m_value);
	}

	void dgGoogol::ToString (char* const string) const
	{
		sprintf (string, "%f", m_value);
	}

	void dgGoogol::ScaleMantissa (dgUnsigned64* const dst, dgUnsigned64 scale) const
	{
	}

#endif



void dgGoogol::NegateMantissa (dgUnsigned64* const mantissa) const
{
	dgUnsigned64 carrier = 1;
	for (dgInt32 i = DG_GOOGOL_SIZE - 1; i >= 0; i --) {
		dgUnsigned64 a = ~mantissa[i] + carrier;
		if (a) {
			carrier = 0;
		}
		mantissa[i] = a;
	}
}


void dgGoogol::ShiftRightMantissa (dgUnsigned64* const mantissa, dgInt32 bits) const
{
	dgUnsigned64 carrier = 0;
	if (dgInt64 (mantissa[0]) < dgInt64 (0)) {
		carrier = dgUnsigned64 (-1);
	}
	
	while (bits >= 64) {
		for (dgInt32 i = DG_GOOGOL_SIZE - 2; i >= 0; i --) {
			mantissa[i + 1] = mantissa[i];
		}
		mantissa[0] = carrier;
		bits -= 64;
	}

	if (bits > 0) {
		carrier <<= (64 - bits);
		for (dgInt32 i = 0; i < DG_GOOGOL_SIZE; i ++) {
			dgUnsigned64 a = mantissa[i];
			mantissa[i] = (a >> bits) | carrier;
			carrier = a << (64 - bits);
		}
	}
}

dgInt32 dgGoogol::LeadingZeros (dgUnsigned64 a) const
{
	#define dgCOUNTBIT(mask,add)		\
	{									\
		dgUnsigned64 test = a & mask;	\
		n += test ? 0 : add;			\
		a = test ? test : (a & ~mask);	\
	}

	dgInt32 n = 0;
    dgAssert (a);
	dgCOUNTBIT (0xffffffff00000000LL, 32);
	dgCOUNTBIT (0xffff0000ffff0000LL, 16);
	dgCOUNTBIT (0xff00ff00ff00ff00LL,  8);
	dgCOUNTBIT (0xf0f0f0f0f0f0f0f0LL,  4);
	dgCOUNTBIT (0xccccccccccccccccLL,  2);
	dgCOUNTBIT (0xaaaaaaaaaaaaaaaaLL,  1);

	return n;
}

dgInt32 dgGoogol::NormalizeMantissa (dgUnsigned64* const mantissa) const
{
	dgAssert (dgInt64 (mantissa[0]) >= 0);

	dgInt32 bits = 0;
	if(dgInt64 (mantissa[0] * 2) < 0) {
		bits = 1;
		ShiftRightMantissa (mantissa, 1);
	} else {
		while (!mantissa[0] && bits > (-64 * DG_GOOGOL_SIZE)) {
			bits -= 64;
			for (dgInt32 i = 1; i < DG_GOOGOL_SIZE; i ++) {
				mantissa[i - 1] = mantissa[i];
			}
			mantissa[DG_GOOGOL_SIZE - 1] = 0;
		}

		if (bits > (-64 * DG_GOOGOL_SIZE)) {
			dgInt32 n = LeadingZeros (mantissa[0]) - 2;
			if (n > 0) {
				dgAssert (n > 0);
				dgUnsigned64 carrier = 0;
				for (dgInt32 i = DG_GOOGOL_SIZE-1; i >= 0; i --) {
					dgUnsigned64 a = mantissa[i];
					mantissa[i] = (a << n) | carrier;
					carrier = a >> (64 - n);
				}
				bits -= n;
			} else if (n < 0) {
				// this is very rare but it does happens, whee the leading zeros of the mantissa is an exact multiple of 64
				dgAssert (mantissa[0] & dgUnsigned64(3)<<62);
				dgUnsigned64 carrier = 0;
				dgInt32 shift = -n;
				for (dgInt32 i = 0; i < DG_GOOGOL_SIZE; i ++) {
					dgUnsigned64 a = mantissa[i];
					mantissa[i] = (a >> shift) | carrier;
					carrier = a << (64 - shift);
				}
				bits -= n;

			}
		}
	}
	return bits;
}

dgUnsigned64 dgGoogol::CheckCarrier (dgUnsigned64 a, dgUnsigned64 b) const
{
	return ((dgUnsigned64 (-1) - b) < a) ? dgUnsigned64(1) : 0;
}


void dgGoogol::ExtendeMultiply (dgUnsigned64 a, dgUnsigned64 b, dgUnsigned64& high, dgUnsigned64& low) const
{
	dgUnsigned64 bLow = b & 0xffffffff; 
	dgUnsigned64 bHigh = b >> 32; 
	dgUnsigned64 aLow = a & 0xffffffff; 
	dgUnsigned64 aHigh = a >> 32; 

	dgUnsigned64 l = bLow * aLow;

	dgUnsigned64 c1 = bHigh * aLow;
	dgUnsigned64 c2 = bLow * aHigh;
	dgUnsigned64 m = c1 + c2;
	dgUnsigned64 carrier = CheckCarrier (c1, c2) << 32;

	dgUnsigned64 h = bHigh * aHigh + carrier;

	dgUnsigned64 ml = m << 32;	
	dgUnsigned64 ll = l + ml;
	dgUnsigned64 mh = (m >> 32) + CheckCarrier (l, ml);	
	dgAssert ((mh & ~0xffffffff) == 0);

	dgUnsigned64 hh = h + mh;

	low = ll;
	high = hh;
}





dgGoogol dgGoogol::operator+= (const dgGoogol &A)
{
	*this = *this + A;
	return *this;
}

dgGoogol dgGoogol::operator-= (const dgGoogol &A)
{
	*this = *this - A;
	return *this;
}


bool dgGoogol::operator> (const dgGoogol &A) const
{
	dgGoogol tmp (*this - A);
	return dgFloat64(tmp) > 0.0;
}

bool dgGoogol::operator>= (const dgGoogol &A) const 
{
	dgGoogol tmp (*this - A);
	return dgFloat64 (tmp) >= 0.0;
}

bool dgGoogol::operator< (const dgGoogol &A) const 
{
	dgGoogol tmp (*this - A);
	return dgFloat64 (tmp) < 0.0;
}

bool dgGoogol::operator<= (const dgGoogol &A) const 
{
	dgGoogol tmp (*this - A);
	return dgFloat64 (tmp) <= 0.0;
}

bool dgGoogol::operator== (const dgGoogol &A) const 
{
	dgGoogol tmp (*this - A);
	return dgFloat64 (tmp) == 0.0;
}

bool dgGoogol::operator!= (const dgGoogol &A) const 
{
	dgGoogol tmp (*this - A);
	return dgFloat64 (tmp) != 0.0;
}

void dgGoogol::Trace () const
{
	dgTrace (("%f ", dgFloat64 (*this)));
}







