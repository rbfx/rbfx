///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Defines the following:
//     Integer data types int128_t and uint128_t
//     INT128_MIN, INT128_MAX, UINT128_MIN, UINT128_MAX
//     INT128_C, UINT128_C
//
// Issues:
//     * Automatic float and double conversion to int128_t is only partially supported.
//     * Some problems exist with respect to different compiler's view of what types 
//       like 'long' and 'long long' are. Some see these as 32 and 64 bits respectively
//       and some don't.
//
// Note that GCC defines the __int128_t and __uint128_t data types when building
// for 64 bit platforms. These types are binary-compatible with the int128_t and
// uint128_t types defined here.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_INT128_H
#define EASTDC_INT128_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>

EA_DISABLE_GCC_WARNING(-Wtype-limits)
#include <wchar.h>
EA_RESTORE_GCC_WARNING()

///////////////////////////////////////////////////////////////////////////////
// EA_INT128_USE_INT64
//
#ifndef EA_INT128_USE_INT64
	#if (EA_PLATFORM_WORD_SIZE >= 8)
		#define EA_INT128_USE_INT64 1
	#else
		#define EA_INT128_USE_INT64 0
	#endif
#endif


///////////////////////////////////////////////////////////////////////////////
// INT128_INT_TYPE / INT128_UINT_TYPE
//
// int32_t may be equivalent to int or it may be equivalent to long, depending 
// on the compiler / platform. In order for integer expressions involving 
// int128_t to work for all cases, we need to provide support for whichever 
// type is not handled by int32_t.

#if   defined(EA_PLATFORM_MICROSOFT)                // Microsoft always defines long as 32 bits, and int32_t as int.
	#define INT128_INT_TYPE  long
	#define INT128_UINT_TYPE unsigned long
#elif(EA_PLATFORM_PTR_SIZE == 4)                    // Other compilers define long as 32 or 64 bits, and int32_t as int and int64_t as long or long long.
	#define INT128_INT_TYPE  long
	#define INT128_UINT_TYPE unsigned long
#elif defined(__have_long64)                        // Some Standard C libraries use this to indicate the int64_t type.
	#define INT128_INT_TYPE  long long
	#define INT128_UINT_TYPE unsigned long long
#elif defined(__have_longlong64)                    // Some Standard C libraries use this to indicate the int64_t type.
	#define INT128_INT_TYPE  long
	#define INT128_UINT_TYPE unsigned long
#elif defined(EA_PLATFORM_APPLE)                    // Apple always sets int64_t as long long on 64 bit platforms.
	#define INT128_INT_TYPE  long
	#define INT128_UINT_TYPE unsigned long
#elif defined(EA_PLATFORM_UNIX)                     // Unix always sets int64_t as long on 64 bit platforms.
	#define INT128_INT_TYPE  long long
	#define INT128_UINT_TYPE unsigned long long
#elif defined(__WORDSIZE) && (__WORDSIZE == 64)     // When __WORDSIZE is 64 bit, the compiler sets int64_t to be long. Except on Apple platforms.
	#define INT128_INT_TYPE  long long
	#define INT128_UINT_TYPE unsigned long long
#else
	// To do: we need to detect whether the compiler/stdlib is defining int64_t as long or long long.
	// In the case that it's using long, we use long long here. In the case that it's using long long, we use long here.
#endif




namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// Forward declarations
//
class int128_t_base;
class int128_t;
class uint128_t;


/// \class int128_t_base
/// \brief Base class upon which int128_t and uint128_t build.
///
class EASTDC_API int128_t_base
{
public:
	// Constructors / destructors
	int128_t_base();
	int128_t_base(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3);
	int128_t_base(uint64_t nPart0, uint64_t nPart1);
	int128_t_base(uint8_t value);
	int128_t_base(uint16_t value);
	int128_t_base(uint32_t value);
  #if defined(INT128_UINT_TYPE)
	int128_t_base(INT128_UINT_TYPE value);
  #endif
	int128_t_base(uint64_t value);
	int128_t_base(const int128_t_base& value);

	// To do: Add an int128_t_base(unsigned int) constructor just for Metrowerks,
	// as Metrowerks defines uint32_t to be unsigned long and not unsigned int.

	// Assignment operator
	int128_t_base& operator=(const int128_t_base& value);

	// Math operators
	static void operatorPlus (const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);
	static void operatorMinus(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);
	static void operatorMul  (const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);

	// Shift operators
	static void operatorShiftRight(const int128_t_base& value, int nShift, int128_t_base& result);
	static void operatorShiftLeft (const int128_t_base& value, int nShift, int128_t_base& result);

	// Unary arithmetic/logic operators
	bool operator!() const;

	// Logical operators
	static void operatorXOR(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);
	static void operatorOR (const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);
	static void operatorAND(const int128_t_base& value1, const int128_t_base& value2, int128_t_base& result);

	// Operators to convert back to basic types
	// We do not provide casting operators (e.g. operator float()) because doing so would
	// cause the compiler to complian about multiple conversion choices while doing freeform
	// math. That's standard C++ behaviour and the conventional solution is to not provide
	// implicit casting operators but to provide functions such as AsFloat() to allow the 
	// user to do explicit casts.
	bool     AsBool()   const;
	uint8_t  AsUint8()  const;
	uint16_t AsUint16() const;
	uint32_t AsUint32() const;
	uint64_t AsUint64() const;

	// Misc. Functions
	// The index values below start with zero, and zero means the lowest part.
	// For example, calling SetPartUint32(3, 0x00000000) zeros the high 32 bits of the int128.
	int      GetBit(int nIndex) const;
	void     SetBit(int nIndex, int value);
	uint8_t  GetPartUint8 (int nIndex) const;
	uint16_t GetPartUint16(int nIndex) const;
	uint32_t GetPartUint32(int nIndex) const;
	uint64_t GetPartUint64(int nIndex) const;
	void     SetPartUint8 (int nIndex, uint8_t  value);
	void     SetPartUint16(int nIndex, uint16_t value);
	void     SetPartUint32(int nIndex, uint32_t value);
	void     SetPartUint64(int nIndex, uint64_t value);

	bool     IsZero() const;
	void     SetZero();
	void     TwosComplement();
	void     InverseTwosComplement();

	enum LeadingZeroes
	{
		kLZDefault,     // Do the default for the base. By default there are leading zeroes only with base 16.
		kLZEnable,
		kLZDisable
	};

	enum Prefix
	{
		kPrefixDefault, // Do the default for the base. By default there is a prefix only with base 16.
		kPrefixEnable,
		kPrefixDisable
	};

protected:
	void DoubleToUint128(double value);

protected:
	#if EA_INT128_USE_INT64
		#ifdef EA_SYSTEM_BIG_ENDIAN
			uint64_t mPart1;  // Most significant byte.
			uint64_t mPart0;  // Least significant byte.
		#else
			uint64_t mPart0;  // Most significant byte.
			uint64_t mPart1;  // Least significant byte.
		#endif
	#else
		#ifdef EA_SYSTEM_BIG_ENDIAN
			uint32_t mPart3;  // Most significant byte.
			uint32_t mPart2;
			uint32_t mPart1;
			uint32_t mPart0;  // Least significant byte.
		#else
			uint32_t mPart0;  // Most significant byte.
			uint32_t mPart1;
			uint32_t mPart2;
			uint32_t mPart3;  // Least significant byte.
		#endif
	#endif
};


/// \class int128_t
/// \brief Implements signed 128 bit integer.
///
class EASTDC_API int128_t : public int128_t_base
{
public:
	// Constructors / destructors
	int128_t();
	int128_t(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3);
	int128_t(uint64_t nPart0, uint64_t nPart1);

	int128_t(int8_t value);
	int128_t(uint8_t value);

	int128_t(int16_t value);
	int128_t(uint16_t value);

	int128_t(int32_t value);
	int128_t(uint32_t value);

  #if defined(INT128_INT_TYPE)
	int128_t(INT128_INT_TYPE value);
	int128_t(INT128_UINT_TYPE value);
  #endif

	int128_t(int64_t value);
	int128_t(uint64_t value);

	int128_t(const int128_t& value);
	//int128_t(const uint128_t& value); // Not defined because doing so would make the compiler unable to decide how to choose binary functions involving int128/uint128.

	int128_t(const float value);
	int128_t(const double value);

	int128_t(const char* pValue, int nBase = 10);
	int128_t(const wchar_t* pValue, int nBase = 10);

	// To do: Add an int128_t(int) constructor just for Metrowerks,
	// as Metrowerks defines uint32_t to be unsigned long and not unsigned int.

	// Assignment operator
	int128_t& operator=(const int128_t_base& value);

	// Unary arithmetic/logic operators
	int128_t  operator-() const;
	int128_t& operator++();
	int128_t& operator--();
	int128_t  operator++(int);
	int128_t  operator--(int);
	int128_t  operator~() const;
	int128_t  operator+() const;

	// Math operators
	friend EASTDC_API int128_t operator+(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator-(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator*(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator/(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator%(const int128_t& value1, const int128_t& value2);

	int128_t& operator+=(const int128_t& value);
	int128_t& operator-=(const int128_t& value);
	int128_t& operator*=(const int128_t& value);
	int128_t& operator/=(const int128_t& value);
	int128_t& operator%=(const int128_t& value);

	// Shift operators
	int128_t  operator>> (int nShift) const;
	int128_t  operator<< (int nShift) const;
	int128_t& operator>>=(int nShift);
	int128_t& operator<<=(int nShift);

	// Logical operators
	friend EASTDC_API int128_t operator^(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator|(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API int128_t operator&(const int128_t& value1, const int128_t& value2);

	int128_t& operator^= (const int128_t& value);
	int128_t& operator|= (const int128_t& value);
	int128_t& operator&= (const int128_t& value);

	// Equality operators
	friend EASTDC_API int  compare   (const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator==(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator!=(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator> (const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator>=(const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator< (const int128_t& value1, const int128_t& value2);
	friend EASTDC_API bool operator<=(const int128_t& value1, const int128_t& value2);

	// Operators to convert back to basic types
	// We do not provide casting operators (e.g. operator float()) because doing so would
	// cause the compiler to complian about multiple conversion choices while doing freeform
	// math. That's standard C++ behaviour and the conventional solution is to not provide
	// implicit casting operators but to provide functions such as AsFloat() to allow the 
	// user to do explicit casts.
	int8_t  AsInt8()   const;
	int16_t AsInt16()  const;
	int32_t AsInt32()  const;
	int64_t AsInt64()  const;
	float   AsFloat()  const;
	double  AsDouble() const;

	// Misc. Functions
	void    Negate();
	bool    IsNegative() const;    // Returns true for value <  0
	bool    IsPositive() const;    // Returns true for value >= 0
	void    Modulus(const int128_t& divisor, int128_t& quotient, int128_t& remainder) const;

	// String conversion functions
	static int128_t StrToInt128(const char*    pValue, char**    ppEnd, int base);
	static int128_t StrToInt128(const wchar_t* pValue, wchar_t** ppEnd, int base);

	void     Int128ToStr(char*    pValue, char**    ppEnd, int base, LeadingZeroes lz = kLZDefault, Prefix prefix = kPrefixDefault) const;
	void     Int128ToStr(wchar_t* pValue, wchar_t** ppEnd, int base, LeadingZeroes lz = kLZDefault, Prefix pPrefix = kPrefixDefault) const;
};



/// \class uint128_t
/// \brief Implements unsigned 128 bit integer.
///
class EASTDC_API uint128_t : public int128_t_base
{
public:
	// Constructors / destructors
	uint128_t();
	uint128_t(uint32_t nPart0, uint32_t nPart1, uint32_t nPart2, uint32_t nPart3);
	uint128_t(uint64_t nPart0, uint64_t nPart1);

	uint128_t(int8_t value);
	uint128_t(uint8_t value);

	uint128_t(int16_t value);
	uint128_t(uint16_t value);

	uint128_t(int32_t value);
	uint128_t(uint32_t value);

  #if defined(INT128_INT_TYPE)
	uint128_t(INT128_INT_TYPE value);
	uint128_t(INT128_UINT_TYPE value);
  #endif

	uint128_t(int64_t value);
	uint128_t(uint64_t value);

	uint128_t(const int128_t& value);
	uint128_t(const uint128_t& value);

	uint128_t(const float value);
	uint128_t(const double value);

	uint128_t(const char* pValue, int nBase = 10);
	uint128_t(const wchar_t* pValue, int nBase = 10);

	// To do: Add a uint128_t(unsigned int) constructor just for Metrowerks,
	// as Metrowerks defines uint32_t to be unsigned long and not unsigned int.

	// Assignment operator
	uint128_t& operator=(const int128_t_base& value);

	// Unary arithmetic/logic operators
	uint128_t  operator-() const;
	uint128_t& operator++();
	uint128_t& operator--();
	uint128_t  operator++(int);
	uint128_t  operator--(int);
	uint128_t  operator~() const;
	uint128_t  operator+() const;

	// Math operators
	friend EASTDC_API uint128_t operator+(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator-(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator*(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator/(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator%(const uint128_t& value1, const uint128_t& value2);

	uint128_t& operator+=(const uint128_t& value);
	uint128_t& operator-=(const uint128_t& value);
	uint128_t& operator*=(const uint128_t& value);
	uint128_t& operator/=(const uint128_t& value);
	uint128_t& operator%=(const uint128_t& value);

	// Shift operators
	uint128_t  operator>> (int nShift) const;
	uint128_t  operator<< (int nShift) const;
	uint128_t& operator>>=(int nShift);
	uint128_t& operator<<=(int nShift);

	// Logical operators
	friend EASTDC_API uint128_t operator^(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator|(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API uint128_t operator&(const uint128_t& value1, const uint128_t& value2);

	uint128_t& operator^= (const uint128_t& value);
	uint128_t& operator|= (const uint128_t& value);
	uint128_t& operator&= (const uint128_t& value);

	// Equality operators
	friend EASTDC_API int  compare   (const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator==(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator!=(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator> (const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator>=(const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator< (const uint128_t& value1, const uint128_t& value2);
	friend EASTDC_API bool operator<=(const uint128_t& value1, const uint128_t& value2);

	// Operators to convert back to basic types
	// We do not provide casting operators (e.g. operator float()) because doing so would
	// cause the compiler to complian about multiple conversion choices while doing freeform
	// math. That's standard C++ behaviour and the conventional solution is to not provide
	// implicit casting operators but to provide functions such as AsFloat() to allow the 
	// user to do explicit casts.
	int8_t  AsInt8()   const;
	int16_t AsInt16()  const;
	int32_t AsInt32()  const;
	int64_t AsInt64()  const;
	float   AsFloat()  const;
	double  AsDouble() const;

	// Misc. Functions
	void    Negate();
	bool    IsNegative() const;    // Returns true for value <  0
	bool    IsPositive() const;    // Returns true for value >= 0
	void    Modulus(const uint128_t& divisor, uint128_t& quotient, uint128_t& remainder) const;

	// String conversion functions
	static uint128_t StrToInt128(const char*    pValue, char**    ppEnd, int base);
	static uint128_t StrToInt128(const wchar_t* pValue, wchar_t** ppEnd, int base);

	void      Int128ToStr(char*    pValue, char**    ppEnd, int base, LeadingZeroes lz = kLZDefault, Prefix prefix = kPrefixDefault) const;
	void      Int128ToStr(wchar_t* pValue, wchar_t** ppEnd, int base, LeadingZeroes lz = kLZDefault, Prefix pPrefix = kPrefixDefault) const;
};


///////////////////////////////////////////////////////////////////////////////
// binary operators
//
// Operators involving two operands are made as independent functions instead
// of member functions because if they were member functions then an expression
// involving both operands would always have to be written in a specific order.
//


///////////////////////////////////////////////////////////////////////////////
// int128_t
//
EASTDC_API int128_t operator+(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator-(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator*(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator/(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator%(const int128_t& value1, const int128_t& value2);

EASTDC_API int128_t operator^(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator|(const int128_t& value1, const int128_t& value2);
EASTDC_API int128_t operator&(const int128_t& value1, const int128_t& value2);

EASTDC_API int      compare   (const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator==(const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator!=(const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator> (const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator>=(const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator< (const int128_t& value1, const int128_t& value2);
EASTDC_API bool     operator<=(const int128_t& value1, const int128_t& value2);


///////////////////////////////////////////////////////////////////////////////
// uint128_t
//
EASTDC_API uint128_t operator+(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator-(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator*(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator/(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator%(const uint128_t& value1, const uint128_t& value2);

EASTDC_API uint128_t operator^(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator|(const uint128_t& value1, const uint128_t& value2);
EASTDC_API uint128_t operator&(const uint128_t& value1, const uint128_t& value2);

EASTDC_API int       compare   (const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator==(const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator!=(const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator> (const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator>=(const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator< (const uint128_t& value1, const uint128_t& value2);
EASTDC_API bool      operator<=(const uint128_t& value1, const uint128_t& value2);



///////////////////////////////////////////////////////////////////////////////
// Min / Max
//
// The C99 language standard defines macros for sized types, such as INT32_MAX.
// Usually such macros are defined like this:
//     #define INT32_MAX 2147483647
// We cannot do this with int128, so instead we define a const variable that
// can be linked to which has the desirable property.
//
extern EASTDC_API const int128_t  EASTDC_INT128_MIN;
extern EASTDC_API const int128_t  EASTDC_INT128_MAX;

extern EASTDC_API const uint128_t EASTDC_UINT128_MIN;
extern EASTDC_API const uint128_t EASTDC_UINT128_MAX;



///////////////////////////////////////////////////////////////////////////////
// INT128_C / UINT128_C
//
// The C99 language defines macros for portably defining constants of 
// sized numeric types. For example, there might be:
//     #define UINT64_C(x) x##ULL
// Since our int128 data type is not a built-in type, we can't define a
// UINT128_C macro as something that pastes ULLL at the end of the digits.
// Instead we define it to create a temporary that is constructed from a 
// string of the digits. This will work in most cases that suffix pasting
// would work.
//
#define EASTDC_INT128_C(x)  EA::StdC::int128_t(#x)
#define EASTDC_UINT128_C(x) EA::StdC::uint128_t(#x)


} // namespace StdC
} // namespace EA


#endif // Header include guard






