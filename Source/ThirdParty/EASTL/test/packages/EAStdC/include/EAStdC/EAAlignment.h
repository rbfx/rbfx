///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This header provides:
//    EAAlignOf
//    AlignOf<T>
//
//    AlignUp<T>            -- Provides basic address and object alignment.
//    AlignDown<T>
//    GetAlignment
//    IsAligned<T>
//    
//    AlignedArray           -- Allows for specifying alignment of objects at runtime and in a compiler-independent way.
//    AlignedObject
//    
//    ReadMisalignedUint16   -- Allows for reading from misaligned memory.
//    ReadMisalignedUint32
//    ReadMisalignedUint64
//    WriteMisalignedUint16  -- Allows for writing to misaligned memory.
//    WriteMisalignedUint32
//    WriteMisalignedUint64
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAALIGNMENT_H
#define EASTDC_EAALIGNMENT_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>

EA_DISABLE_ALL_VC_WARNINGS()
EA_DISABLE_VC_WARNING(4548 4836 4574)
#include <stddef.h>
#include <new>
EA_RESTORE_VC_WARNING()
EA_RESTORE_ALL_VC_WARNINGS()



// The default HasTrivialDestructor template implementation doesn't always work on all 
// compilers for vector intrinsic types. It is more reliable for us to use the built-in 
// compiler traits. It's possible we may want to do this for other compilers as well. 
// Currently this is only supported on newer versions of GCC and Clang.
#if defined EA_COMPILER_GNUC || defined EA_COMPILER_CLANG
	#if defined EA_COMPILER_GNUC && (EA_COMPILER_VERSION <= 4004)
		#define EASTDC_TRIVIAL_DTOR_VIA_TRAITS 0
	#elif defined EA_PLATFORM_APPLE
		#define EASTDC_TRIVIAL_DTOR_VIA_TRAITS 0
	#else
		#define EASTDC_TRIVIAL_DTOR_VIA_TRAITS 1
	#endif
#else
	#define EASTDC_TRIVIAL_DTOR_VIA_TRAITS 0
#endif



/// EAAlignOf
/// 
/// Note: This has been superceded by EABase's EA_ALIGN_OF.
///
/// Gives you the alignment of a given data type. If you are using 
/// a compiler that doesn't support a built-in alignof then you 
/// are stuck with being able to use EAAlignOf only on "POD" types
/// (i.e. basic C structures).
///
/// Example usage:
///    printf("Alignment of type 'int' is %u.", (unsigned)EAAlignOf(int)); 
/// 
#ifndef EAAlignOf
	#define EAAlignOf(x) EA_ALIGN_OF(x)
#endif



namespace EA
{
namespace StdC
{
	///////////////////////////////////////////////////////////////////////////
	/// Type Traits
	template <typename T, T v>
	struct IntegralConstant
	{
		static const T value = v;
		typedef T value_type;
		typedef IntegralConstant<T, v> type;
	};

	typedef IntegralConstant<bool, true>  TrueType;
	typedef IntegralConstant<bool, false> FalseType;

	#if EASTDC_TRIVIAL_DTOR_VIA_TRAITS
		template <typename T> struct HasTrivialDestructor : public IntegralConstant<bool, __has_trivial_destructor(T) || __is_pod(T)> {};
	#else
		template <typename T> struct HasTrivialDestructor : public FalseType{};
	#endif

	// Example usage: 
	//     EASTDC_DECLARE_TRIVIAL_DESTRUCTOR(__m128)
	#define EASTDC_DECLARE_TRIVIAL_DESTRUCTOR(T) namespace EA {namespace StdC { \
		template <> struct HasTrivialDestructor<T> : public TrueType{}; \
		template <> struct HasTrivialDestructor<const T> : public TrueType{}; \
		template <> struct HasTrivialDestructor<volatile T> : public TrueType{}; \
		template <> struct HasTrivialDestructor<const volatile T> : public TrueType{}; } }



	/// AlignOf
	/// 
	/// Gives you the alignment of a given data type. If you are using 
	/// a compiler that doesn't support a built-in alignof then you 
	/// are stuck with being able to use EAAlignOf only on "POD" types
	/// (i.e. basic C structures).
	///
	/// Example usage:
	///    int x;
	///    printf("Alignment of type int is %u.", (unsigned)AlignOf<int>()); 
	/// 
	template <typename T>
	inline size_t AlignOf()
	{
		return EAAlignOf(T);
	}


	/// AlignOf
	/// 
	/// This is an instance-based version of AlignOf. 
	///
	/// Example usage:
	///    int x;
	///    printf("Alignment of x is %u.", (unsigned)AlignOf(x)); 
	/// 
	template <typename T>
	inline size_t AlignOf(const T&)
	{
		return EAAlignOf(T);
	}


	/// AlignUp
	/// Rounds a scalar value (integer or pointer) up to the nearest multiple of 
	/// Rounds values towards positive infinity.
	/// Returns 0 for an input of 0.
	/// The alignment 'a' must be an even power of 2.
	/// This version of AlignUp is at least as efficient as the version which takes alignment as an argument.
	/// Example:
	///    AlignUp<int, 4>(3)  ->  4
	///    AlignUp<int, 4>(8)  ->  8
	///    AlignUp<int, 4>(0)  ->  0
	///    AlignUp<int, 4>(-7) -> -4
	template <typename T, size_t a>
	inline T AlignUp(T x){
		return (T)((x + (a - 1)) & ~(a - 1));
	}

	template <typename T, size_t a>
	inline T* AlignUp(T* p){
		return (T*)(((uintptr_t)p + (a - 1)) & ~(a - 1));
	}



	/// AlignUp
	/// Rounds values towards positive infinity.
	/// Returns 0 for an input of 0.  
	/// The alignment 'a' must be an even power of 2.
	/// Example:
	///    AlignUp(3, 4)  ->  4
	///    AlignUp(8, 4)  ->  8
	///    AlignUp(0, 4)  ->  0
	///    AlignUp(-7, 4) -> -4
	template <typename T>
	inline T AlignUp(T x, size_t a){
		return (T)((x + (a - 1)) & ~(a - 1));
	}

	template <typename T>
	inline T* AlignUp(T* p, size_t a){
		return (T*)(((uintptr_t)p + (a - 1)) & ~(a - 1));
	}



	/// AlignDown
	/// Rounds a scalar value (integer or pointer) down to nearest multiple of template parameter <a>.
	/// Returns 0 for an input of 0.
	/// Rounds values towards negative infinity.
	/// The alignment 'a' must be an even power of 2.
	/// This version of AlignDown is at least as efficient as the version which takes alignment as an argument.
	/// Example:
	///    AlignDown<int, 4>(5)  ->  4
	///    AlignDown<int, 4>(4)  ->  4
	///    AlignDown<int, 4>(0)  ->  0
	///    AlignDown<int, 4>(-7) -> -8
	template <typename T, size_t a>
	inline T AlignDown(T x){
		return (T)(x & ~(a - 1));
	}

	template <typename T, size_t a>
	inline T* AlignDown(T* p){
		return (T*)((uintptr_t)p & ~(a - 1));
	}



	/// AlignDown
	/// Rounds a scalar value (integer or pointer) down to nearest multiple of of template parameter <a>.
	/// x must be a scalar value (integer or pointer), else the results are undefined.
	/// Returns 0 for an input of 0.
	/// Rounds values towards negative infinity.
	/// The alignment 'a' must be an even power of 2.
	/// Example:
	///    AlignDown(5, 4)  ->  4
	///    AlignDown(4, 4)  ->  4
	///    AlignDown(0, 4)  ->  0
	///    AlignDown(-7, 4) -> -8
	template <typename T>
	inline T AlignDown(T x, size_t a){
		return (T)(x & ~(a - 1));
	}

	template <typename T>
	inline T* AlignDown(T* p, size_t a){
		return (T*)((uintptr_t)p & ~(a - 1));
	}


	/// GetAlignment
	/// 
	/// Returns the highest power-of-two alignment of the given value x.
	/// x must be a scalar value (integer or pointer), else the results are undefined.
	/// Returns 0 for an input a value of 0.
	/// Beware that GetAlignment returns the highest power-of-two alignment, which 
	/// may result in a return value that is higher than you expect. Consider using
	/// the IsAligned functions to test for a specific alignment.
	/// Example:
	///    GetAlignment(0)  ->  0
	///    GetAlignment(1)  ->  1
	///    GetAlignment(2)  ->  2
	///    GetAlignment(3)  ->  1
	///    GetAlignment(4)  ->  4
	///    GetAlignment(5)  ->  1
	///    GetAlignment(6)  ->  2
	///    GetAlignment(7)  ->  1
	///    GetAlignment(8)  ->  8
	///    GetAlignment(9)  ->  1
	template <typename T>
	inline size_t GetAlignment(T x)
	{
		return (size_t)((x ^ (x - 1)) >> 1) + 1;
	}

	template <typename T>
	inline size_t GetAlignment(T* p)
	{
		return (size_t)(((uintptr_t)p ^ ((uintptr_t)p - 1)) >> 1) + 1;
	}


	/// IsAligned
	/// 
	/// Tells if a given integer is aligned to a given power-of-2 boundary.
	/// Returns true for an input x value of 0, regardless of the value of a.
	/// The template <a> value must be >= 1.
	/// Example:
	///    IsAligned<int, 8>(64)  ->  true
	///    IsAligned<int, 8>(67)  ->  false
	///
	/// To consider: wouldn't it be better if the template arguments were reversed?
	///
	template <typename T, int a>
	inline bool IsAligned(T x)
	{
		return (x & (a - 1)) == 0;
	}

	template <typename T, int a>
	inline bool IsAligned(T* p)
	{
		return ((uintptr_t)p & (a - 1)) == 0;
	}


	/// IsAligned
	/// 
	/// Tells if a given integer is aligned to a given power-of-2 boundary.
	/// Returns true for an input x value of 0, regardless of the value of a.
	/// The alignment value a must be >= 1.
	/// Example:
	///    IsAligned(64, 8)  ->  true
	///    IsAligned(67, 8)  ->  false
	///
	template <typename T>
	inline bool IsAligned(T x, size_t a)
	{
		return (x & (a - 1)) == 0;
	}

	template <typename T>
	inline bool IsAligned(T* p, size_t a)
	{
		return ((uintptr_t)p & (a - 1)) == 0;
	}


	/// AlignedType
	///
	/// This class makes an aligned typedef for a given class based on a user-supplied 
	/// class and alignment. This class exists because of a weakness in VC++ whereby 
	/// it can only accept __declspec(align) and thus EA_PREFIX_ALIGN usage via an
	/// integer literal (e.g. "16") and not an otherwise equivalent constant integral
	/// expression (e.g. sizeof Foo). 
	///
	/// Example usage:
	///     const size_t kAlignment = 32;
	///     AlignedType<int, kAlignment>::Type intAlignedAt128;
	///     intAlignedAt128 = 12345;
	///
	#if !defined(__GNUC__) || (__GNUC__ < 4) // GCC4 lost the ability to do this
		template <typename T, size_t alignment> struct AlignedType { };

		#if defined(EA_COMPILER_MSVC) && EA_COMPILER_VERSION >= 1900	// VS2015+
			EA_DISABLE_VC_WARNING(5029);  // nonstandard extension used: alignment attributes in C++ apply to variables, data members and tag types only
		#endif
		template<typename T> struct AlignedType<T,    2> { typedef EA_PREFIX_ALIGN(   2)    T    EA_POSTFIX_ALIGN(   2)    Type; };
		template<typename T> struct AlignedType<T,    4> { typedef EA_PREFIX_ALIGN(   4)    T    EA_POSTFIX_ALIGN(   4)    Type; };
		template<typename T> struct AlignedType<T,    8> { typedef EA_PREFIX_ALIGN(   8)    T    EA_POSTFIX_ALIGN(   8)    Type; };
		template<typename T> struct AlignedType<T,   16> { typedef EA_PREFIX_ALIGN(  16)    T    EA_POSTFIX_ALIGN(  16)    Type; };
		template<typename T> struct AlignedType<T,   32> { typedef EA_PREFIX_ALIGN(  32)    T    EA_POSTFIX_ALIGN(  32)    Type; };
		template<typename T> struct AlignedType<T,   64> { typedef EA_PREFIX_ALIGN(  64)    T    EA_POSTFIX_ALIGN(  64)    Type; };
		template<typename T> struct AlignedType<T,  128> { typedef EA_PREFIX_ALIGN( 128)    T    EA_POSTFIX_ALIGN( 128)    Type; };
		template<typename T> struct AlignedType<T,  256> { typedef EA_PREFIX_ALIGN( 256)    T    EA_POSTFIX_ALIGN( 256)    Type; };
		template<typename T> struct AlignedType<T,  512> { typedef EA_PREFIX_ALIGN( 512)    T    EA_POSTFIX_ALIGN( 512)    Type; };
		template<typename T> struct AlignedType<T, 1024> { typedef EA_PREFIX_ALIGN(1024)    T    EA_POSTFIX_ALIGN(1024)    Type; };
		template<typename T> struct AlignedType<T, 2048> { typedef EA_PREFIX_ALIGN(2048)    T    EA_POSTFIX_ALIGN(2048)    Type; };
		template<typename T> struct AlignedType<T, 4096> { typedef EA_PREFIX_ALIGN(4096)    T    EA_POSTFIX_ALIGN(4096)    Type; };
		#if defined(EA_COMPILER_MSVC) && EA_COMPILER_VERSION >= 1900	// VS2015+
			EA_RESTORE_VC_WARNING();
		#endif
	#else
		// Need to deal with this under GCC 4, now that they took this functionality away.
	#endif


	/// AlignedArray
	///
	/// Allows you to align an array of objects, regardless of when or where they are declared.
	/// Alignment must be a power-of-two value, such as 2, 4, 8, 16, 32, etc.
	/// A common use of this would be to have an object on the stack be aligned to a specific
	/// value. It also allows for a member variable of a struct to be aligned to a given size
	/// without having any special requirements about the alignment of the struct itself.
	/// This is advantageous relative to compiler-specific alignment mechanisms.
	/// The downside of this mechanism relative to compiler-specific alignment mechanisms is
	/// that this mechanism may use more memory.
	///
	/// Question: Should each element in the array be aligned or should just the beginning of
	/// the array be aligned? Normally the answer should be just the beginning. The reason for
	/// this is that the C/C++ standard expects array elements to be sizeof(T) apart from each
	/// other when in an array.
	///
	/// Example usage:
	///   AlignedArray<Vector, 10, 64> mVectorArray;
	///   mVectorArray[0] = mVectorArray[1];
	///   NormalizeVector(mVectorArray[3]);
	///   ProcessVectorArray(mVectorArray);
	///
	/// To do: Make this class act like AlignedType and retain the smart pointer functionality.
	///
	template <class T, int count, int alignment>
	class AlignedArray
	{
	public:
		typedef T                                 value_type;
		typedef AlignedArray<T, count, alignment> this_type;

		AlignedArray()
		{
			mpT = (T*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1));

			for(T *pT = mpT, *pTEnd = mpT + count; pT < pTEnd; ++pT)
				new(pT) T; // Note that we are not allocating memory here; we are constructing existing memory.
		}

		AlignedArray(const this_type& x)
		{
			mpT = (T*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1));

			for(T *pT = mpT, *pTSrc = x.mpT, *pTEnd = mpT + count; pT < pTEnd; ++pT, ++pTSrc)
				new(pT) T(*pTSrc); // Note that we are not allocating memory here; we are constructing existing memory.
		}

		void DeleteMembers(TrueType) // true_type means yes it's true that T has a trivial destructor.
		{
		}

		void DeleteMembers(FalseType) // false_type means the destructor is non-trivial.
		{
			for(T *pT = mpT + count - 1, *pTEnd = mpT; pT >= pTEnd; --pT)
				pT->~T();
		}

		~AlignedArray()
		{
			DeleteMembers(HasTrivialDestructor<value_type>());
		}

		// The following set of supported operators is currently experimental. A little more
		// thinking and testing will be needed to determine the proper best set of operators.

		this_type& operator=(const this_type& x)
		{
			for(int i = 0; i < count; i++)  // To consider: We could possibly improve this a little bit by
				mpT[i] = x.mpT[i];          // taking advantage of type traits or the STL/EASTL copy algorithm.
			return *this;
		}

		operator T*()
		{ return mpT; }

		operator const T*() const
		{ return mpT; }

	protected:
		T*   mpT;                                      // Points to a place within mBuffer.
		char mBuffer[(sizeof(T) * count) + alignment]; // This max possible space usage required to align an array of type T.
	};
		
	 
	/// AlignedObject
	///
	/// Allows you to align an  object, regardless of when or where it is declared.
	/// Alignment must be a power-of-two value, such as 2, 4, 8, 16, 32, etc.
	/// A common use of this would be to have an object on the stack be aligned to a specific 
	/// value. It also allows for a member variable of a struct to be aligned to a given size
	/// without having any special requirements about the alignment of the struct itself. 
	/// This is advantageous relative to compiler-specific alignment mechansims. 
	/// The downside of this mechanism relative to compiler-specific alignment mechanisms is 
	/// that this mechanism may use more memory.
	///
	/// This class lets you have most of the functionality you would have if the 
	/// alignment functionality was native. For the most part, the only limitations
	/// are that you sometimes need reference the object in a slightly different
	/// way than usual. For example, you need to use operator->() to reference a 
	/// class member instead of operator.() even though the object is not a pointer.
	///
	/// Example usage:
	///    AlignedObject<Matrix, 64> matrix;
	///    matrix->Normalize();
	///    NormalizeMatrix(&matrix);
	///
	/// To do: Make this class act like AlignedType and retain the smart pointer functionality.
	///
	template <class T, int alignment>
	class AlignedObject
	{
	public:
		typedef T                           value_type;
		typedef AlignedObject<T, alignment> this_type;

		AlignedObject()
			: mT(*(T*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1)))
			{ new((void*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1))) T; } // Note that we are not allocating memory here; we are constructing existing memory.

		AlignedObject(const this_type& x)
			: mT(*(T*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1)))
			{ mT = x.mT; }

		AlignedObject(const T& t)
			: mT(*(T*)(((intptr_t)(mBuffer + (alignment - 1))) & ~(alignment - 1)))
			{ mT = t; }

	   ~AlignedObject()
			{ mT.~T(); }

		// The following set of supported operators is currently experimental. A little more
		// thinking and testing will be needed to determine the proper best set of operators.

		this_type& operator=(const this_type& x)
			{ mT = x.mT; return *this; }

		this_type& operator=(const T& t)
			{ mT = t; return *this; }

		T* operator &()
			{ return &mT; }

		T* Get()
			{ return &mT; }

		const T* Get() const
			{ return &mT; }

		operator T&()
			{ return mT; }

		operator const T&() const
			{ return mT; }

		// C++ doesn't allow overloading the . operator. 
		// Use operator -> or the T& cast operator instead.
		//T& operator.() const
		//    { return mT; }

		T* operator->()
			{ return &mT; }

		const T* operator->() const
			{ return &mT; }
		
		operator T*()
			{ return &mT; }

		operator const T*() const
			{ return &mT; }

	protected:
		T&   mT;                             // Points to a place within mBuffer.
		char mBuffer[sizeof(T) + alignment]; // This max possible space usage required to align an object of type T.
	};




	/// ReadMisalignedUint16
	///
	/// Get an unsigned integer from a possibly non-aligned address.
	/// The MIPS processor on the PS2 cannot read a 32-bit value
	/// from an unaligned address, so this function can be used
	/// to make reading misaligned data portable.
	///
	inline uint16_t ReadMisalignedUint16(const void* p)
	{
		return *((const uint16_t*)p);
	}


	/// ReadMisalignedUint32
	///
	/// Get an unsigned integer from a possibly non-aligned address.
	/// The MIPS processor on the PS2 cannot read a 32-bit value
	/// from an unaligned address, so this function can be used
	/// to make reading misaligned data portable.
	///
	inline uint32_t ReadMisalignedUint32(const void* p)
	{
		return *((const uint32_t*)p);
	}

	/// ReadMisalignedUint64
	///
	/// Get an unsigned integer from a possibly non-aligned address.
	/// The MIPS processor on the PS2 cannot read a 32-bit value
	/// from an unaligned address, so this function can be used
	/// to make reading misaligned data portable.
	///
	inline uint64_t ReadMisalignedUint64(const void* p)
	{
		return *((const uint64_t*)p);
	}



	/// WriteMisalignedUint16
	inline void WriteMisalignedUint16(uint16_t n, void* p)
	{
		*(uint16_t*)p = n;
	}

	/// WriteMisalignedUint32
	inline void WriteMisalignedUint32(uint32_t n, void* p)
	{
		*(uint32_t*)p = n;
	}

	/// WriteMisalignedUint64
	inline void WriteMisalignedUint64(uint64_t n, void* p)
	{
		*(uint64_t*)p = n;
	}


} // namespace StdC
} // namespace EA






namespace EA
{
namespace StdC
{
	///////////////////////////////////////////////////////////////////////////
	// Deprecated functions
	///////////////////////////////////////////////////////////////////////////

	/// AlignAddressUp
	/// 
	/// Aligns a given addess up to a specified power-of-two.
	/// Example usage:
	///    void* p;
	///    p = AlignAddressUp(p, 64); 
	/// 
	inline void* AlignAddressUp(const void* p, size_t a)
	{
		return (void*)(((uintptr_t)p + (a - 1)) & ~(a - 1));
	}


	/// AlignObjectUp
	/// 
	/// Aligns a given object up to an address of a specified power-of-two.
	/// Example usage:
	///    Matrix* pM;
	///    pM = AlignObjectUp(pM, 16); 
	/// 
	template <typename T>
	inline T* AlignObjectUp(const T* p, size_t a)
	{
		return (T*)(((uintptr_t)p + (a - 1)) & ~(a - 1));
	}

	/// AlignAddressDown
	/// 
	/// Aligns a given addess down to a specified power-of-two.
	/// Example usage:
	///    void* p;
	///    p = AlignAddressDown(p, 8); 
	/// 
	inline void* AlignAddressDown(const void* p, size_t a)
	{
		return (void*)((uintptr_t)p & ~(a - 1));
	}


	/// AlignObjectDown
	/// 
	/// Aligns a given object down to an address of a specified power-of-two.
	/// Example usage:
	///    Matrix* pM;
	///    pM = AlignObjectDown(pM, 16); 
	/// 
	template <typename T>
	inline T* AlignObjectDown(const T* p, size_t a)
	{
		return (T*)((uintptr_t)p & ~(a - 1));
	}


	/// IsAddressAligned
	/// 
	/// Tells if a given address is aligned to a given power-of-2 boundary.
	/// Always returns true for a pointer 'p' value of NULL.
	/// The alignment 'a' must be >= 1.
	/// Example:
	///    IsAddressAligned((char*)0x00000000, 8)  ->  true
	///    IsAddressAligned((char*)0x00000001, 8)  ->  false
	///
	inline bool IsAddressAligned(const void* p, size_t a)
	{
		return ((uintptr_t)p & (a - 1)) == 0;
	}


	/// IsObjectAligned
	/// 
	/// Tells if a given object is aligned to a given power-of-2 boundary.
	/// This is redundant with respect to IsAddressAligned, but is provided
	/// for consistency.
	/// The alignment 'a' must be >= 1.
	///
	template <typename T>
	inline bool IsObjectAligned(const T* p, size_t a)
	{
		return ((uintptr_t)p & (a - 1)) == 0;
	}



} // namespace StdC
} // namespace EA


#endif // Header include guard







