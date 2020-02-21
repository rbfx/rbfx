///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a fairly complete implementation of a fixed point
// C++ numerical data type. You can use the data type freely in mathematical
// statements just like any other data type. The following default types are
// defined:
//    SFixed16 -- Signed   Fixed Point, 16:16 precision
//    UFixed16 -- Unsigned Fixed Point, 16:16 precision
// You can easily define any other precision type by simply uncommenting 
// the appropriate typedefs at the bottom of the file, such as SFixed20 
// (Signed Fixed Point, 20:12 precision). You must also implement a custom
// version of each of FixedMul, FixedDiv, etc. See the cpp file for versions
// of these functions. 
//
// The following code is perfectly legal and tested. Note that arbitrary 
// mixing of data types:
//
//    SFixed16      a(1), b(2), c(3);
//    float         f = 4.5;
//    double        d = 3.2;
//    int           i =   6;
//    a = b * f;
//    a = (c / d) + b + f;
//    a = c / d * (b % i) + f / c;
//    a = i * -c / (b++);
//    a = sin(a) + pow(b, d) * sqrt(a);
//    a = log(a)/log(f);
//
// You will likely want to write special versions of the trigonometry functions
// to provide fast speeds. Simple versions below are implemented that simply 
// use the cpu's built-in floating point math hardware.
//
// Fixed point
//      - Limited range
//        Using fixed 16:16 which is common on PC's numbers are between -32767
//        and +32767. The fractional part is accurate to 1/65536. 
//      + Can be very fast
//        A lot of fixed point procedures are achieved with adds and bitwise shifts which
//        can be paired on the Pentium. So we have potentially two additions per clock
//        cycle. Multiplies take longer. Although less accurate, square roots and trig
//        functions can be really fast. 
//      + Executes concurrently
//        PCs with an FPU can execute integer instructions at the same time as
//        floating-point instructions. 
//      - Harder to code in high-level languages
//        Fixed point routines sometimes aren't high-level friendly. It's much easier to see
//        what's going on in a program if you use the standard floating point types. If you
//        are writing in C or Pascal, it's best to do the fixed point bits in embedded
//        assembler. 
//
//  Floating point
//      + Large range
//        The range for floating point numbers is typically in
//        excess of 1E-100 to 1E+100 and have an accuracy
//        of about 13 decimal places. 
//      - Sometimes slow
//        Generally fast for multiplies and addition but may be
//        slow for trig and square roots, depending on the processor. 
//        You should use the FPU for these if you require a high degree
//        of accuracy. Conversions from float to int are slow too.
//      + Executes concurrently
//        PCs with an FPU can execute integer instructions at
//        the same time as floating-point instructions. 
//
// Note that if you are using the Microsoft C++ compiler, then you can add this
// to your autoexp.dat file to have it auto-expand fixed point values to their
// floating point equivalents while holding the cursor over them or viewing then
// under the watch window:
//    SFixed16      = Value=<value/65536.F,f>
//    UFixed16      = Value=<value/65536.F,f>
//    FPTemplate<*> = Value=<value/65536.F,f>
//
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAFIXEDPOINT_H
#define EASTDC_EAFIXEDPOINT_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
#include <math.h>


///////////////////////////////////////////////////////////////////////////////
// Classic C-style 16:16 fixed point functions/macros 
//
#ifndef EA_C_STYLE_FIXED_POINT_MATH
#define EA_C_STYLE_FIXED_POINT_MATH
	typedef int32_t EAFixed16;

	#define           EAMAX_FIXED16        0x7fffffff
	#define           EAMIN_FIXED16        0x80000000
	#define           EAFixed16ToInt(a)    ((int32_t)(a) >> 16)
	#define           EAIntToFixed16(a)    ((EAFixed16)((a) << 16))
	#define           EAFixed16ToDouble(a) (((double)a) / 65536.0)
	#define           EADoubleToFixed16(a) ((EAFixed16)((a) * 65536.0))
	#define           EAFixed16ToFloat(a)  (((float)a) / 65536.f)
	#define           EAFloatToFixed16(a)  ((EAFixed16)((a) * 65536.f))
	#define           EAFixed16Negate(a)   (-a)
	EAFixed16         EAFixed16Mul         (EAFixed16 a, EAFixed16 b);                  // Returns (a * b).
	EAFixed16         EAFixed16Div         (EAFixed16 a, EAFixed16 b);                  // Returns (a / y).
	EAFixed16         EAFixed16DivSafe     (EAFixed16 a, EAFixed16 b);                  // Returns (a / y). Returns max value possible if b is zero.
	EAFixed16         EAFixed16MulDiv      (EAFixed16 a, EAFixed16 b, EAFixed16 c);     // Calculates (a * b / c) faster than separate mul and div.
	EAFixed16         EAFixed16MulDivSafe  (EAFixed16 a, EAFixed16 b, EAFixed16 c);     // Calculates (a * b / c) faster than separate mul and div. Returns max value possible if z is zero.
	EAFixed16         EAFixed16Mod         (EAFixed16 a, EAFixed16 b);                  // Retuns a modulo b, in fixed point format. For example, 3 % 2 = 1.
	EAFixed16         EAFixed16ModSafe     (EAFixed16 a, EAFixed16 b);                  // Retuns a modulo b, in fixed point format. For example, 3 % 2 = 1. Returns max value possible if z is zero.
	inline  EAFixed16 EAFixed16Abs         (EAFixed16 a) { return (a >= 0) ? a : -a; } 
#endif
///////////////////////////////////////////////////////////////////////////////


namespace EA
{
namespace StdC
{


///////////////////////////////////////////////////////////////////////////////
// FP_USE_INTEL_ASM (Fixed point type use Intel asm)
//
#if defined(EA_PROCESSOR_X86) && defined(EA_ASM_STYLE_INTEL)
	#define FP_USE_INTEL_ASM
	#define FP_PASCAL EA_PASCAL
#else
	#define FP_PASCAL
#endif



///////////////////////////////////////////////////////////////////////////////
// class FPTemplate 
//
#define FPTemplateDeclaration template<class T, int upShiftInt, int downShiftInt, int upMulInt, int downDivInt>
#define FPTemplateType FPTemplate<T, upShiftInt, downShiftInt, upMulInt, downDivInt>

template <class  T,                            //'T' must be a signed or unsigned integer type (e.g. long, unsigned long, etc.)
		  int  upShiftInt, int downShiftInt, 
		  int  upMulInt,   int downDivInt>

struct FPTemplate
{
	// Data
	T value;

	// Types
	typedef T type;
	   
	//Functions
	FPTemplate() {}
	FPTemplate(const FPTemplate&    newValue) { value = newValue.value; }
	FPTemplate(const int&           newValue) { value = newValue << upShiftInt; }
	FPTemplate(const unsigned int&  newValue) { value = newValue << upShiftInt; }
	FPTemplate(const long&          newValue) { value = newValue << upShiftInt; }
	FPTemplate(const unsigned long& newValue) { value = newValue << upShiftInt; }
	FPTemplate(const float&         newValue) { value = (int)(newValue * (float)upMulInt);  }
	FPTemplate(const double&        newValue) { value = (int)(newValue * (double)upMulInt); }
	void FromFixed(const int&       newValue) { value = newValue; }    // Accepts an int that is in fixed point format (i.e. shifted) already. 
	T    AsFixed  ()                          { return value;     }    // Allows you to get the fixed point value itself and mess with it as you want. 

	// We don't define the conversion operators because that would cause a lot of compiler
	// errors complaining about not knowing what function to call. Thus, we have functions
	// below such as "AsInt()" to do explicit conversions. 
	// operator int()           const { return (int)          (value>>upShiftInt);          }
	// operator unsigned int()  const { return (unsigned int) (value>>upShiftInt);          }
	// operator long()          const { return (long)         (value>>upShiftInt);          }
	// operator unsigned long() const { return (unsigned long)(value>>upShiftInt);          }
	// operator float()         const { return (float)        (value /(float)upMulInt);     }
	// operator double()        const { return (double)       (value /(double)upMulInt);    }

	int           AsInt()         const { return (int)          (value>>upShiftInt);       }
	unsigned int  AsUnsignedInt() const { return (unsigned int) (value>>upShiftInt);       }
	long          AsLong()        const { return (long)         (value>>upShiftInt);       }
	unsigned long AsUnsignedLong()const { return (unsigned long)(value>>upShiftInt);       }
	float         AsFloat()       const { return (float)        (value /(float)upMulInt);  }
	double        AsDouble()      const { return (double)       (value /(double)upMulInt); }

	FPTemplate& operator=(const FPTemplate&    newValue) { value = newValue.value;                     return *this; }
	FPTemplate& operator=(const int&           newValue) { value = newValue << upShiftInt;             return *this; }
	FPTemplate& operator=(const unsigned int&  newValue) { value = newValue << upShiftInt;             return *this; }
	FPTemplate& operator=(const long&          newValue) { value = newValue << upShiftInt;             return *this; }
	FPTemplate& operator=(const unsigned long& newValue) { value = newValue << upShiftInt;             return *this; }
	FPTemplate& operator=(const float&         newValue) { value = (T)(newValue * (float)upMulInt);  return *this; }
	FPTemplate& operator=(const double&        newValue) { value = (T)(newValue * (double)upMulInt); return *this; }

	bool operator< (const FPTemplate& compareValue) const { return value <  compareValue.value; }
	bool operator> (const FPTemplate& compareValue) const { return value >  compareValue.value; }
	bool operator>=(const FPTemplate& compareValue) const { return value >= compareValue.value; }
	bool operator<=(const FPTemplate& compareValue) const { return value <= compareValue.value; }
	bool operator==(const FPTemplate& compareValue) const { return value == compareValue.value; }
	bool operator!=(const FPTemplate& compareValue) const { return value != compareValue.value; }
	   
	bool operator< (const int& compareValue) const { return value <  (T)(compareValue<<upShiftInt); }  
	bool operator> (const int& compareValue) const { return value >  (T)(compareValue<<upShiftInt); } 
	bool operator>=(const int& compareValue) const { return value >= (T)(compareValue<<upShiftInt); }
	bool operator<=(const int& compareValue) const { return value <= (T)(compareValue<<upShiftInt); }
	bool operator==(const int& compareValue) const { return value == (T)(compareValue<<upShiftInt); }
	bool operator!=(const int& compareValue) const { return value != (T)(compareValue<<upShiftInt); }
	   
	bool operator< (const unsigned int& compareValue) const { return value <  (T)(compareValue<<upShiftInt); } 
	bool operator> (const unsigned int& compareValue) const { return value >  (T)(compareValue<<upShiftInt); }  
	bool operator>=(const unsigned int& compareValue) const { return value >= (T)(compareValue<<upShiftInt); }
	bool operator<=(const unsigned int& compareValue) const { return value <= (T)(compareValue<<upShiftInt); }
	bool operator==(const unsigned int& compareValue) const { return value == (T)(compareValue<<upShiftInt); }
	bool operator!=(const unsigned int& compareValue) const { return value != (T)(compareValue<<upShiftInt); }
	   
	bool operator< (const long& compareValue) const { return value <  (T)(compareValue<<upShiftInt); }  
	bool operator> (const long& compareValue) const { return value >  (T)(compareValue<<upShiftInt); } 
	bool operator>=(const long& compareValue) const { return value >= (T)(compareValue<<upShiftInt); }
	bool operator<=(const long& compareValue) const { return value <= (T)(compareValue<<upShiftInt); }
	bool operator==(const long& compareValue) const { return value == (T)(compareValue<<upShiftInt); }
	bool operator!=(const long& compareValue) const { return value != (T)(compareValue<<upShiftInt); }
	   
	bool operator< (const unsigned long& compareValue) const { return value <  (T)(compareValue<<upShiftInt); } 
	bool operator> (const unsigned long& compareValue) const { return value >  (T)(compareValue<<upShiftInt); } 
	bool operator>=(const unsigned long& compareValue) const { return value >= (T)(compareValue<<upShiftInt); }
	bool operator<=(const unsigned long& compareValue) const { return value <= (T)(compareValue<<upShiftInt); }
	bool operator==(const unsigned long& compareValue) const { return value == (T)(compareValue<<upShiftInt); }
	bool operator!=(const unsigned long& compareValue) const { return value != (T)(compareValue<<upShiftInt); }

	bool operator< (const float& compareValue) const { return value <  compareValue*(float)upMulInt; } //Note that we convert the float down rather 
	bool operator> (const float& compareValue) const { return value >  compareValue*(float)upMulInt; } //that convert the fixed up.
	bool operator>=(const float& compareValue) const { return value >= compareValue*(float)upMulInt; }
	bool operator<=(const float& compareValue) const { return value <= compareValue*(float)upMulInt; }
	bool operator==(const float& compareValue) const { return value == compareValue*(float)upMulInt; }
	bool operator!=(const float& compareValue) const { return value != compareValue*(float)upMulInt; }
	   
	bool operator< (const double& compareValue) const { return value <  compareValue*(double)upMulInt; } //Note that we convert the float down rather 
	bool operator> (const double& compareValue) const { return value >  compareValue*(double)upMulInt; } //that convert the fixed up.
	bool operator>=(const double& compareValue) const { return value >= compareValue*(double)upMulInt; }
	bool operator<=(const double& compareValue) const { return value <= compareValue*(double)upMulInt; }
	bool operator==(const double& compareValue) const { return value == compareValue*(double)upMulInt; }
	bool operator!=(const double& compareValue) const { return value != compareValue*(double)upMulInt; }
	bool operator! ()                           const { return value == 0; }
	   
	FPTemplate  operator~ ()                    const { FPTemplate temp; temp.value = ~value; return temp; } 
	FPTemplate  operator- ()                    const { FPTemplate temp; temp.value = -value; return temp; }
	FPTemplate  operator+ ()                    const { return *this; }

	FPTemplate& operator+=(const FPTemplate&    argValue) { value += argValue.value;            return *this; }
	FPTemplate& operator+=(const int&           argValue) { value += (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator+=(const unsigned int&  argValue) { value += (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator+=(const long &         argValue) { value += (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator+=(const unsigned long& argValue) { value += (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator+=(const float&         argValue) { value += int(argValue*(float)upMulInt);  return *this; }
	FPTemplate& operator+=(const double&        argValue) { value += int(argValue*(double)upMulInt); return *this; }
	FPTemplate& operator-=(const FPTemplate&    argValue) { value -= argValue.value;            return *this; }
	FPTemplate& operator-=(const int&           argValue) { value -= (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator-=(const unsigned int&  argValue) { value -= (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator-=(const long&          argValue) { value -= (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator-=(const unsigned long& argValue) { value -= (T)(argValue<<upShiftInt); return *this; }
	FPTemplate& operator-=(const float&         argValue) { value -= int(argValue*(float)upMulInt);  return *this; }
	FPTemplate& operator-=(const double&        argValue) { value -= int(argValue*(double)upMulInt); return *this; }
	FPTemplate& operator*=(const FPTemplate&    argValue) { value = FixedMul(value, argValue.value);        return *this; }
	FPTemplate& operator*=(const int&           argValue) { value = FixedMul(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator*=(const unsigned int&  argValue) { value = FixedMul(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator*=(const long&          argValue) { value = FixedMul(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator*=(const unsigned long& argValue) { value = FixedMul(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator*=(const float&         argValue) { value = FixedMul(value, (type)(argValue*(float)upMulInt));   return *this; }
	FPTemplate& operator*=(const double&        argValue) { value = FixedMul(value, (type)(argValue*(double)upMulInt));  return *this; }
	FPTemplate& operator/=(const FPTemplate&    argValue) { value = FixedDiv(value, argValue.value);  return *this; }
	FPTemplate& operator/=(const int&           argValue) { value = FixedDiv(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator/=(const unsigned int&  argValue) { value = FixedDiv(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator/=(const long&          argValue) { value = FixedDiv(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator/=(const unsigned long& argValue) { value = FixedDiv(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator/=(const float&         argValue) { value = FixedDiv(value, (type)(argValue*(float)upMulInt));   return *this; }
	FPTemplate& operator/=(const double&        argValue) { value = FixedDiv(value, (type)(argValue*(double)upMulInt));  return *this; }
	FPTemplate& operator%=(const FPTemplate&    argValue) { value = FixedMod(value, argValue.value);  return *this; }
	FPTemplate& operator%=(const int&           argValue) { value = FixedMod(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator%=(const unsigned int&  argValue) { value = FixedMod(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator%=(const long&          argValue) { value = FixedMod(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator%=(const unsigned long& argValue) { value = FixedMod(value, argValue<<upShiftInt);  return *this; }
	FPTemplate& operator%=(const float&         argValue) { value = FixedMod(value, (type)(argValue*(float)upMulInt));   return *this; }
	FPTemplate& operator%=(const double&        argValue) { value = FixedMod(value, (type)(argValue*(double)upMulInt));  return *this; }

	FPTemplate& operator|=(const FPTemplate& argValue) { value |=  argValue.value;         return *this;}
	FPTemplate& operator|=(const int&        argValue) { value |= (argValue<<upShiftInt);  return *this;} //Convert Fixed to int, then do operation
	FPTemplate& operator&=(const FPTemplate& argValue) { value &= argValue.value;          return *this;}
	FPTemplate& operator&=(const int&        argValue) { value &= (argValue<<upShiftInt);  return *this;} //Convert Fixed to int, then do operation
	FPTemplate& operator^=(const FPTemplate& argValue) { value ^= argValue.value;          return *this;}
	FPTemplate& operator^=(const int&        argValue) { value ^= (argValue<<upShiftInt);  return *this;} //Convert Fixed to int, then do operation

	FPTemplate operator<<(int numBits) const { FPTemplate temp; temp.value = value << numBits; return temp; }
	FPTemplate operator>>(int numBits) const { FPTemplate temp; temp.value = value >> numBits; return temp; }

	FPTemplate& operator<<=(int numBits) { value <<= numBits; return *this;}
	FPTemplate& operator>>=(int numBits) { value >>= numBits; return *this;}

	FPTemplate& operator++()    { value += 1<<upShiftInt; return *this; } 
	FPTemplate& operator--()    { value -= 1<<upShiftInt; return *this; }
	FPTemplate  operator++(int) { FPTemplate temp(*this); value += 1<<upShiftInt; return temp; }
	FPTemplate  operator--(int) { FPTemplate temp(*this); value -= 1<<upShiftInt; return temp; }

	FPTemplate  Abs() { if(value<0) return -value; return value; }
	FPTemplate  DivSafe(const FPTemplate& denominator)       { FPTemplate temp; temp.FromFixed(FixedDivSafe(value, denominator.value)); return temp; }
	FPTemplate& DivSafeAssign(const FPTemplate& denominator) { value = FixedDivSafe(value, denominator.value); return *this; }

	// These are left public for practical utility purposes.
	static EASTDC_API T FP_PASCAL FixedMul       (const T t1, const T t2);              // If you are using one of these functions directly or
	static EASTDC_API T FP_PASCAL FixedDiv       (const T t1, const T t2);              // indirectly through one of the above operators, you 
	static EASTDC_API T FP_PASCAL FixedDivSafe   (const T t1, const T t2);              // need to define the function in a separate .cpp file.
	static EASTDC_API T FP_PASCAL FixedMulDiv    (const T t1, const T t2, const T t3);  // The actual implementation may be processor- or compiler-specific. 
	static EASTDC_API T FP_PASCAL FixedMulDivSafe(const T t1, const T t2, const T t3);  //
	static EASTDC_API T FP_PASCAL FixedMod       (const T t1, const T t2);              // The 'Safe' versions of functions return the maximum possible
	static EASTDC_API T FP_PASCAL FixedModSafe   (const T t1, const T t2);              // value when a overflow or division by zero would occur, instead of bombing.
};
///////////////////////////////////////////////////////////////////////////////







////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const FPTemplateType& t2){ 
   FPTemplateType temp;
   temp.value = t1.value+t2.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const int& t2){
   FPTemplateType temp;
   temp.value = t1.value+(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const int& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = t1.value+(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const unsigned int& t2){
   FPTemplateType temp;
   temp.value = t1.value+(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const unsigned int& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)+t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const long& t2){
   FPTemplateType temp;
   temp.value = t1.value+(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const long& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)+t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const unsigned long& t2){
   FPTemplateType temp;
   temp.value = t1.value+(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const unsigned long& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)+t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const float& t2){
   FPTemplateType temp;
   temp.value = t1.value+(typename FPTemplateType::type)(t2*(float)upMulInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const float& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (typename FPTemplateType::type)(t2*(float)upMulInt)+t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const FPTemplateType& t1, const double& t2){
   FPTemplateType temp;
   temp.value = t1.value+(typename FPTemplateType::type)(t2*(double)upMulInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator+(const double& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (typename FPTemplateType::type)(t2*(double)upMulInt)+t1.value;
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const FPTemplateType& t2){ 
   FPTemplateType temp;
   temp.value = t1.value-t2.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const int& t2){
   FPTemplateType temp;
   temp.value = t1.value-(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const int& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)-t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const unsigned int& t2){
   FPTemplateType temp;
   temp.value = t1.value-(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const unsigned int& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)-t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const long& t2){
   FPTemplateType temp;
   temp.value = t1.value-(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const long& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)-t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const unsigned long& t2){
   FPTemplateType temp;
   temp.value = t1.value-(t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const unsigned long& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (t2<<upShiftInt)-t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const float& t2){
   FPTemplateType temp;
   temp.value = t1.value-(typename FPTemplateType::type)(t2*(float)upMulInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const float& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (typename FPTemplateType::type)(t2*(float)upMulInt)-t1.value;
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const FPTemplateType& t1, const double& t2){
   FPTemplateType temp;
   temp.value = t1.value-(typename FPTemplateType::type)(t2*(double)upMulInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator-(const double& t2, const FPTemplateType& t1){
   FPTemplateType temp;
   temp.value = (typename FPTemplateType::type)(t2*(double)upMulInt)-t1.value;
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const FPTemplateType& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, t2.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const int& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const int& t2, const FPTemplateType& t1){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const unsigned int& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const unsigned int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const long& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const unsigned long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const unsigned long& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const float& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, (typename FPTemplateType::type)(t2*(float)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const float& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul((typename FPTemplateType::type)(t2*(float)upMulInt), t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const FPTemplateType& t1, const double& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul(t1.value, (typename FPTemplateType::type)(t2*(double)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator*(const double& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMul((typename FPTemplateType::type)(t2*(double)upMulInt), t1.value);
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const FPTemplateType& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, t2.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const int& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const int& t2, const FPTemplateType& t1){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const unsigned int& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const unsigned int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const long& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const unsigned long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const unsigned long& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const float& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, (typename FPTemplateType::type)(t2*(float)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const float& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv((typename FPTemplateType::type)(t2*(float)upMulInt), t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const FPTemplateType& t1, const double& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv(t1.value, (typename FPTemplateType::type)(t2*(double)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator/(const double& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedDiv((typename FPTemplateType::type)(t2*(double)upMulInt), t1.value);
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const FPTemplateType& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, t2.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const int& t2){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const int& t2, const FPTemplateType& t1){ 
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const unsigned int& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const unsigned int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const unsigned long& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, t2<<upShiftInt);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const unsigned long& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t2<<upShiftInt, t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const float& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, (typename FPTemplateType::type)(t2*(float)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const float& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod((typename FPTemplateType::type)(t2*(float)upMulInt), t1.value);
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const FPTemplateType& t1, const double& t2){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod(t1.value, (typename FPTemplateType::type)(t2*(double)upMulInt));
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator%(const double& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = FPTemplateType::FixedMod((typename FPTemplateType::type)(t2*(double)upMulInt), t1.value);
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator|(const FPTemplateType& t1, const FPTemplateType& t2){
   FPTemplateType temp; 
   temp.value = t1.value | t2.value; 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator|(const FPTemplateType& t1, const int& t2){
   FPTemplateType temp; 
   temp.value = t1.value | (t2<<upShiftInt); 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator|(const int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = (t2<<upShiftInt) | t1.value; 
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator&(const FPTemplateType& t1, const FPTemplateType& t2){
   FPTemplateType temp; 
   temp.value = t1.value & t2.value; 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator&(const FPTemplateType& t1, const int& t2){
   FPTemplateType temp; 
   temp.value = t1.value & (t2<<upShiftInt); 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator&(const int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = (t2<<upShiftInt) & t1.value; 
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////
FPTemplateDeclaration
inline FPTemplateType operator^(const FPTemplateType& t1, const FPTemplateType& t2){
   FPTemplateType temp; 
   temp.value = t1.value ^ t2.value; 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator^(const FPTemplateType& t1, const int& t2){
   FPTemplateType temp; 
   temp.value = t1.value ^ (t2<<upShiftInt); 
   return temp;
}

FPTemplateDeclaration
inline FPTemplateType operator^(const int& t2, const FPTemplateType& t1){
   FPTemplateType temp; 
   temp.value = (t2<<upShiftInt) ^ t1.value; 
   return temp;
}
////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////

FPTemplateDeclaration
inline FPTemplateType sin(const FPTemplateType& t){ 
   return FPTemplateType(::sin(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType asin(const FPTemplateType& t){ 
   return FPTemplateType(::asin(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType cos(const FPTemplateType& t){ 
   return FPTemplateType(::cos(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType acos(const FPTemplateType& t){ 
   return FPTemplateType(::acos(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType tan(const FPTemplateType& t){ 
   return FPTemplateType(::tan(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType atan(const FPTemplateType& t){ 
   return FPTemplateType(::atan(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType atan2(const FPTemplateType& t1, const FPTemplateType& t2){ 
   return FPTemplateType(::atan2(t1.AsDouble(), t2.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType atan2(const double& t1, const FPTemplateType& t2){ 
   return FPTemplateType(::atan2(t1, t2.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType atan2(const FPTemplateType& t1, const double& t2){ 
   return FPTemplateType(::atan2(t1.AsDouble(), t2));
}

FPTemplateDeclaration
inline FPTemplateType sqrt(const FPTemplateType& t){ 
   return FPTemplateType(::sqrt(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType pow(const FPTemplateType& t1, const FPTemplateType& t2){ 
   return FPTemplateType(::pow(t1.AsDouble(), t2.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType pow(const double& t1, const FPTemplateType& t2){ 
   return FPTemplateType(::pow(t1, t2.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType pow(const FPTemplateType& t1, const double& t2){ 
   return FPTemplateType(::pow(t1.AsDouble(), t2));
}

FPTemplateDeclaration
inline FPTemplateType exp(const FPTemplateType& t){ 
   return FPTemplateType(::exp(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType log(const FPTemplateType& t){ 
   return FPTemplateType(::log(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType log10(const FPTemplateType& t){ 
   return FPTemplateType(::log10(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType ceil(const FPTemplateType& t){ 
   return FPTemplateType(::ceil(t.AsDouble()));
}

FPTemplateDeclaration
inline FPTemplateType floor(const FPTemplateType& t){ 
   return FPTemplateType(::floor(t.AsDouble()));
}
////////////////////////////////////////////////////////////////////////////////////





///////////////////////////////////////////////////////////////////////////////
// Actual types
//
// SFixed16_16  --   signed 16:16 fixed point
// UFixed16_16  -- unsigned 16:16 fixed point
//
// SFixed32_32  --   signed 32:32 fixed point
// UFixed32_32  -- unsigned 32:32 fixed point
//
// You can actually define these types externally. However, you will likely 
// have to provde an implementation of FixedMul, FixedDiv, and FixedMod
// like we do for SFixed16 and UFixed16 in the .cpp file for this header.
//

typedef FPTemplate<int32_t,     8, 24,      256,  16777216>   SFixed24;  //  24:8 fixed point (8 bits of fraction)
typedef FPTemplate<uint32_t,    8, 24,      256,  16777216>   UFixed24;

typedef FPTemplate<int32_t,    10, 22,     1024,   4194304>   SFixed22;   // 22:10 fixed point (10 bits of fraction)
typedef FPTemplate<uint32_t,   10, 22,     1024,   4194304>   UFixed22;

typedef FPTemplate<int32_t,    12, 20,     4096,   1048576>   SFixed20;   // 20:12 fixed point (12 bits of fraction)
typedef FPTemplate<uint32_t,   12, 20,     4096,   1048576>   UFixed20;

typedef FPTemplate<int32_t,    14, 18,    16384,    262144>   SFixed18;   // 18:14 fixed point (14 bits of fraction)
typedef FPTemplate<uint32_t,   14, 18,    16384,    262144>   UFixed18;

typedef FPTemplate<int32_t,    16, 16,    65536,     65536>   SFixed16;   // 16:16 fixed point (16 bits of fraction)
typedef FPTemplate<uint32_t,   16, 16,    65536,     65536>   UFixed16;

typedef FPTemplate<int32_t,    18, 14,   262144,     16384>   SFixed14;   // 14:18 fixed point (18 bits of fraction)
typedef FPTemplate<uint32_t,   18, 14,   262144,     16384>   UFixed14;

typedef FPTemplate<int32_t,    20, 12,  1048576,      4096>   SFixed12;   // 12:20 fixed point (20 bits of fraction)
typedef FPTemplate<uint32_t,   20, 12,  1048576,      4096>   UFixed12;

typedef FPTemplate<int32_t,    22, 10,  4194304,      1024>   SFixed10;   // 10:22 fixed point (22 bits of fraction)
typedef FPTemplate<uint32_t,   10, 22,  4194304,      1024>   UFixed10;

typedef FPTemplate<int32_t,    24,  8, 16777216,       256>    SFixed8;   //  8:24 fixed point (24 bits of fraction)
typedef FPTemplate<uint32_t,   24,  8, 16777216,       256>    UFixed8;

///////////////////////////////////////////////////////////////////////////////

#define FPDeclareTemplateSpecializations(TypeDef) \
	template<> EASTDC_API int32_t FP_PASCAL SFixed16::FixedMul(const int32_t a, const int32_t b); \
	template<> EASTDC_API int32_t FP_PASCAL SFixed16::FixedDiv(const int32_t a, const int32_t b); \
	template<> EASTDC_API int32_t FP_PASCAL SFixed16::FixedMod(const int32_t a, const int32_t b)

FPDeclareTemplateSpecializations(SFixed16);
FPDeclareTemplateSpecializations(UFixed16);

} // namespace StdC
} // namespace EA


#endif // Header include guard









