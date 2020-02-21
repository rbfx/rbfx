///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This module implements the following functions which provide 
// implementations of basic memory manipulation functions.
//
// Note the presence of "C" functions below such as MemcpyC. These refer to 
// memory functions that operate only on cacheable memory, but are faster 
// than otherwise. Cacheable memory is standard system RAM used by applications
// and is the memory you work with 98% of the time. However, on some hardware
// systems (e.g. gaming console machines) there is uncacheable memory, such as
// memory that is mapped to video addresses. This memory is typically called
// write-through or write-combined memory and is useful for writing data in 
// one direction from system RAM to video memory. The "C" functions do not
// work on this kind of memory and you can instead use the regular functions
// such as Memcpy for uncacheable memory.
//
//      char*    Memcpy     (void* pDestination, const void* pSource, size_t n);
//      char*    MemcpyC    (void* pDestination, const void* pSource, size_t n);     // Faster version for cacheable memory (and not video memory).
//      char*    MemcpyS    (void* pDestination, const void* pSource, size_t n);     // Streaming memory copy, doesn't invalidate the cache.
//      char*    Memcpy128  (void* pDestination, const void* pSource, size_t n);
//      char*    Memcpy128C (void* pDestination, const void* pSource, size_t n);     // Faster version for cacheable memory (and not video memory).
//      char*    Memmove    (void* pDestination, const void* pSource, size_t n);
//      char*    MemmoveC   (void* pDestination, const void* pSource, size_t n);     // Faster version for cacheable memory (and not video memory).
//
//      const void* Memchr(const void* p, char c, size_t n);
//      int         Memcmp(const void* p1, const void* p2, size_t n);
//      void*       Memmem(const void* pMemory, size_t memorySize, const void* pFind, size_t findSize);
//
//      uint8_t*    Memset8      (void* pDestination, uint8_t  c, size_t uint8Count);
//      uint8_t*    Memset8C     (void* pDestination, uint8_t  c, size_t uint8Count);   // Faster version for cacheable memory (and not video memory).
//      uint8_t*    Memset8_128  (void* pDestination, uint8_t  c, size_t uint8Count);
//      uint8_t*    Memset8_128C (void* pDestination, uint8_t  c, size_t uint8Count);   // Faster version for cacheable memory (and not video memory).
//      uint16_t*   Memset16     (void* pDestination, uint16_t c, size_t uint16Count);
//      uint32_t*   Memset32     (void* pDestination, uint32_t c, size_t uint32Count);
//      uint64_t*   Memset64     (void* pDestination, uint64_t c, size_t uint64Count);
//      void*       MemsetPointer(void* pDestination, const void* const pValue, size_t ptrCount)
//      void*       MemsetN      (void* pDestination, const void* pSource, size_t sourceBytes, size_t nCount);
//      void        Memclear     (void* pDestination, size_t n);
//      void        MemclearC    (void* pDestination, size_t n);                        // Faster version for cacheable memory (and not video memory).
//
//      void        Memfill8       (void* pDestination, uint8_t c,  size_t byteCount);  // Same as Memset8
//      void        Memfill16      (void* pDestination, uint16_t c, size_t byteCount);
//      void        Memfill24      (void* pDestination, uint32_t c, size_t byteCount);
//      void        Memfill32      (void* pDestination, uint32_t c, size_t byteCount);
//      void        Memfill64      (void* pDestination, uint64_t c, size_t byteCount);
//      void        MemfillSpecific(void* pDestination, const void* pSource, size_t destByteCount, size_t sourceByteCount);
//
//      uint8_t*    Memcheck8      (void* pDestination, uint8_t  c, size_t byteCount);
//      uint16_t*   Memcheck16     (void* pDestination, uint16_t c, size_t byteCount);
//      uint32_t*   Memcheck32     (void* pDestination, uint32_t c, size_t byteCount);
//      uint64_t*   Memcheck64     (void* pDestination, uint64_t c, size_t byteCount);
//
//      bool        TimingSafeMemEqual(const void* p1, const void* p2, size_t n);
//      int         TimingSafeMemcmp(const void* p1, const void* p2, size_t n);
//      int         TimingSafeMemIsClear(const void* p, size_t n);
//
//      void*       EAAlloca(size_t n);
//      void*       EAMalloca(size_t n);
//      void        EAFreea();
//
//      template<size_t> StaticMemory;
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAMEMORY_H
#define EASTDC_EAMEMORY_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>


EA_DISABLE_ALL_VC_WARNINGS()

#include <string.h>
#include <stdlib.h>

#if defined(_MSC_VER) || defined(__MINGW32__)
	EA_DISABLE_ALL_VC_WARNINGS()
	// void* __cdecl _alloca(size_t n);  To consider: directly forward declare _alloca in order to avoid #include of <malloc.h>, as it's slow to compile.
	#include <malloc.h>         // For alloca.
	EA_RESTORE_ALL_VC_WARNINGS()
#elif defined (EA_PLATFORM_BSD)
	// do nothing since alloca is in stdlib.h
#elif (defined(__GNUC__) || defined(__clang__)) && !defined(EAAlloca)
	#define EAAlloca __builtin_alloca
#endif

#if defined(EA_PLATFORM_MICROSOFT)
	#include <math.h>       // VS2008 has an acknowledged bug that requires math.h (and possibly also string.h) to be #included before intrin.h.
	#include <intrin.h>
#endif

#if EA_SSE
	#include <xmmintrin.h>
#endif

EA_RESTORE_ALL_VC_WARNINGS()




namespace EA
{
namespace StdC
{

	#if   defined(EA_PROCESSOR_X86)
		typedef uint32_t machine_word_t;
		const size_t     kMachineWordSize     = sizeof(machine_word_t);
		const size_t     kMachineWordSizeMask = sizeof(machine_word_t) - 1;
		#if EA_SSE
			#define EA_CACHE_PREFETCH_128(addr)  _mm_prefetch((const char*)(uintptr_t)(addr), _MM_HINT_NTA)
			#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)
		#else
			#define EA_CACHE_PREFETCH_128(addr)  // Need to support _mm_prefetch.
			#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)
		#endif
	#elif defined(EA_PROCESSOR_X86_64)
		typedef uint64_t machine_word_t;
		const size_t     kMachineWordSize     = sizeof(machine_word_t);
		const size_t     kMachineWordSizeMask = sizeof(machine_word_t) - 1;
		#define EA_CACHE_PREFETCH_128(addr)  _mm_prefetch((const char*)(uintptr_t)(addr), _MM_HINT_NTA)
		#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)

	#elif defined(EA_PROCESSOR_ARM32)
		typedef uint32_t machine_word_t;
		const size_t     kMachineWordSize     = sizeof(machine_word_t);
		const size_t     kMachineWordSizeMask = sizeof(machine_word_t) - 1;
		// Modern ARM CPUs have auto-prefetch functionality, though it's 
		// not necessarily smart and has to be enabled by the OS.
		#if defined(__GNUC__) && (__GNUC__ >= 4)
			#define EA_CACHE_PREFETCH_128(addr) __builtin_prefetch(addr)
		#else
			#define EA_CACHE_PREFETCH_128(addr)
		#endif
		#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)
	#elif (EA_PLATFORM_WORD_SIZE == 4)
		typedef uint32_t machine_word_t;
		const size_t     kMachineWordSize     = sizeof(machine_word_t);
		const size_t     kMachineWordSizeMask = sizeof(machine_word_t) - 1;
		#define EA_CACHE_PREFETCH_128(addr)
		#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)
	#else
		typedef uint64_t machine_word_t;
		const size_t     kMachineWordSize     = sizeof(machine_word_t);
		const size_t     kMachineWordSizeMask = sizeof(machine_word_t) - 1;
		#define EA_CACHE_PREFETCH_128(addr)
		#define EA_CACHE_ZERO_128(addr)      memset(addr, 0, 128)
	#endif

	const size_t     kCacheLineSize       = EA_CACHE_LINE_SIZE;  
	const size_t     kCacheLineSizeMask   = (EA_CACHE_LINE_SIZE-1);


	///////////////////////////////////////////////////////////////////////////////
	/// EAAlloca / EAAllocaAligned
	///
	/// Platform-independent version of alloca. This is a #define instead of a
	/// function, and so it isn't used within a namespace. This #define is located
	/// within a namespace merely for file organization purposes.
	/// EAAlloca is prefixed with EA because it is (necessarily) a #define and 
	/// the EA makes its usage less likely to collide with other defines.
	///
	/// As with alloca, emory allocated by EAAalloca must be used and freed within
	/// the same scope that it is allocated.
	///
	/// Return value:
	/// EAAlloca may return NULL with some implementations, may throw an 
	/// exception in some implementations, and my return a non-NULL value
	/// which is usuable beyond an arbitrary number of bytes. Thus EAlloca/alloca is
	/// dangerous to use unless you are sure it will work for you, and that is very
	/// platform/compiler/ABI-dependent. Consider using EAMalloca/EAFreea instead.
	///
	/// If EAAlloca is not #defined for some platform then it is not supported by 
	/// that platform. However, as of this writing EAAlloca/alloca is supported by all 
	/// compilers and platforms that have been significant to EA, including console, 
	/// mobile, desktop, and hand-held platforms.
	///
	/// Declaration:
	///     void* EAAlloca(size_t n);
	///
	/// Example usage:
	///     void Function() {
	///         char* buffer = (char*)EAAlloca(24);
	///         // Beware the return value behavior of EAAlloca (see above).
	///         sprintf(buffer, "hello world");
	///         // No need to try to free it.
	///     }
	///
	#if !defined(EAAlloca)
		#define EAAlloca alloca
	#endif

	#if !defined(EAAllocaAligned)
		#define EAAllocaAligned(size, alignment) ((void*)(((uintptr_t)EAAlloca((size)+(alignment)) + ((alignment)-1)) & ~((alignment)-1)))
	#endif



	///////////////////////////////////////////////////////////////////////////////
	/// EAMalloca / EAFreea
	///
	/// Allocates memory from the stack if possible, otherwise allocates memory
	/// via malloc. It's not possible to determine ahead of time which of the two
	/// it will allocate from. EAMalloca must be eventually matched with an EAFreea.
	/// Memory allocated by EAMalloca must be used and freed within the same scope
	/// that it is allocated.
	///
	/// Return value:
	/// Returns NULL if the memory could not be allocated. 
	///
	/// Declaration:
	///     void* EAMalloca(size_t n);
	///     void  EAFreea(void *p);
	///
	/// Example usage:
	///     void Function() {
	///         char* buffer = (char*)EAMalloca(24);
	///         if(buffer) { // This will fail only if malloc fails for the given size.
	///             sprintf(buffer, "hello world");
	///             EAFreea(buffer);
	///         }
	///     }
	///
	/// To consider: Implement EAMallocaAligned
	/// 
	#if defined(EA_PLATFORM_MICROSOFT) // Microsoft platforms support larger stacks than many other platforms, as well as auto-growable stacks.
		#define EAMALLOCA_THRESHOLD 8192
	#else
		#define EAMALLOCA_THRESHOLD 1024
	#endif
	#define EAMALLOCA_TYPE_ALLOCA   0xaa
	#define EAMALLOCA_TYPE_MALLOC   0xbb
	#define EAMALLOCA_TYPE_SIZE     EA_PLATFORM_MIN_MALLOC_ALIGNMENT

	#if !defined(EAMalloca)
		#define EAMalloca(size) ((((size) + EAMALLOCA_TYPE_SIZE) < EAMALLOCA_THRESHOLD) ?                                       \
									EA::StdC::EAMallocaSetType(EAAlloca((size) + EAMALLOCA_TYPE_SIZE), EAMALLOCA_TYPE_ALLOCA) : \
									EA::StdC::EAMallocaSetType(malloc((size) + EAMALLOCA_TYPE_SIZE), EAMALLOCA_TYPE_MALLOC))
	#endif
	#if !defined(EAFreea)
		#define EAFreea(p) EA::StdC::EAFreeaImpl(p)
	#endif

	inline void* EAMallocaSetType(void* p, uint32_t type)
	{
		if(p) {
			*reinterpret_cast<uint32_t*>(p) = type;
			p = reinterpret_cast<char*>(p) + EAMALLOCA_TYPE_SIZE;
		}
		return p;
	}

	inline void EAFreeaImpl(void* p)
	{
		if (p) {
			p = reinterpret_cast<char*>(p) - EAMALLOCA_TYPE_SIZE;
			uint32_t type = *reinterpret_cast<uint32_t*>(p);
			if(type == EAMALLOCA_TYPE_MALLOC)
				free(p);
		}
	}



	///////////////////////////////////////////////////////////////////////////
	/// Memcpy
	///
	/// Copies nByteCount bytes from pSource to pDestination. 
	/// The source and destination memory ranges must not overlap.
	/// Returns pDestination.
	/// There are no restrictions in the way of size, alignment, or memory type,
	/// though the source memory must be readable and the destination memory 
	/// must be writable. 
	/// Works with uncacheable memory, such as video memory. 
	///
	EASTDC_API char* Memcpy(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount);

	// Cacheable memory copy
	// Works only with cacheable memory (i.e. conventional system memory).
	// The source and destination memory must not overlap.
	// Cannot be relied on to work with uncachable memory, such as video memory.
	// There are no alignment restrictions on either pDestination or pSource.
	EASTDC_API char* MemcpyC(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount);

	// Streaming memcpy
	// This function copies memory from source to destination without filling the cache with the memory.
	// This is useful for when you want to write memory that will not be read by the processor used
	//   to write it, such as when the CPU writes to memory used by the GPU. 
	// Works on both cacheable and uncacheable memory.
	// The source and destination memory must not overlap.
	// There are no alignment restrictions on either pDestination or pSource.
	EASTDC_API char* MemcpyS(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount);


	///////////////////////////////////////////////////////////////////////////
	/// Memcpy128
	///
	/// Copies nByteCount bytes from the source to the destination. nByteCount
	/// must be a multiple of 128, pDestination must be on at least a 128 byte 
	/// boundary, and pSource must be on at least a 16 byte boundary.
	///
	/// Additionally, pDestination and pSource must refer to cacheable memory.
	/// Cacheable memory is standard RAM application memory as opposed to being
	/// video memory or IO-mapped memory. On XBox 360, uncacheable memory is 
	/// that allocated with VirtualAlloc(..., PAGE_NOCACHE) or VirtualAlloc(..., PAGE_WRITECOMBINE).
	/// In fact, on XBox 360 this function is the same as XMemCpy128.
	///
	EASTDC_API char* Memcpy128(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount);

	// cacheable 128 byte memcpy
	// This function is useful for higher performance memory copies when the requirements can be met.
	// The address pointed to by pDestination must be aligned on a 128-byte boundary, and uint8Count must be a multiple of 128.
	// Works only with cacheable memory (i.e. conventional system memory).
	// The source and destination memory must not overlap.
	EASTDC_API char* Memcpy128C(void* EA_RESTRICT pDestination, const void* EA_RESTRICT pSource, size_t nByteCount);


	///////////////////////////////////////////////////////////////////////////
	/// Memmove
	///
	/// Copies nByteCount bytes from pSource to pDestination. 
	/// The source and destination memory ranges may overlap.
	/// Returns pDestination.
	/// There are no restrictions in the way of size, alignment, or memory type,
	/// though the source memory must be readable and the destination memory 
	/// must be writable. 
	/// Works with uncacheable memory, such as video memory.
	///
	EASTDC_API char* Memmove(void* pDestination, const void* pSource, size_t nByteCount);

	// Cacheable memory move
	// Works only with cacheable memory (i.e. conventional system memory).
	// The source and destination memory may overlap.
	// Cannot be relied on to work with uncachable memory, such as video memory.
	EASTDC_API char* MemmoveC(void* pDestination, const void* pSource, size_t nByteCount);


	///////////////////////////////////////////////////////////////////////////
	/// Memchr
	///
	/// Same as memchr and wmemchr.
	/// Searches the first n characters (not necessarily bytes) of the memory block 
	/// pointed to by pString for character c.
	/// Returns a pointer to the character or NULL if not found.
	/// There are no restrictions about the type of memory p refers
	/// to except that it be readable.
	///
	EASTDC_API const char*     Memchr  (const char*  p,    char c,     size_t n);
	EASTDC_API const char16_t* Memchr16(const char16_t* p, char16_t c, size_t n);
	EASTDC_API const char32_t* Memchr32(const char32_t* p, char32_t c, size_t n);
	#if EA_WCHAR_UNIQUE
		inline const wchar_t* MemchrW(const wchar_t* p, wchar_t c, size_t n)
		{
			#if (EA_WCHAR_SIZE == 2)
				return reinterpret_cast<const wchar_t*>(Memchr16(reinterpret_cast<const char16_t*>(p), (char16_t)c, n));
			#else
				return reinterpret_cast<const wchar_t*>(Memchr32(reinterpret_cast<const char32_t*>(p), (char32_t)c, n));
			#endif
		}
	#endif


	///////////////////////////////////////////////////////////////////////////
	/// Memcmp
	///
	/// Same as memcmp and wmemcmp.
	/// Compares the first n bytes of two memory blocks pointed by p1 and p2. 
	/// The comparison is a bytewise compare and thus for strings it is case-sensitive. 
	/// For a case-insensitive string comparison, use the Stricmp function.
	/// Bytes are treated as uint8_t for comparison purposes.
	/// Returns 0 if the memory is equal, < 0 if p1 < p2, and > 0 if p1 > p2.
	/// There are no restrictions about the type of memory p1 and p2 refer
	/// to except that they be readable.
	///
	EASTDC_API int Memcmp(const void* p1, const void* p2, size_t n);


	///////////////////////////////////////////////////////////////////////////
	/// Memmem
	///
	/// Search for pFind/findSize within pMemory/memorySize.
	/// Return NULL if not found, return the first found location within pMemory/memory size if found.
	/// If memorySize is 0, return value is NULL. Otherwise if the findSize is 0, then the return value is pMemory.
	/// If pMemory or pFind is NULL, their respective size must be 0.
	/// The return value is non-const because that's how C memmem (non-standard function) works.
	/// There are no restrictions about the type of memory pMemory and pFind refer
	/// to except that they be readable.
	/// There are no alignment restrictions on either pMemory or pFind.
	/// See  http://linux.die.net/man/3/memmem
	///
	EASTDC_API void* Memmem(const void* pMemory, size_t memorySize, const void* pFind, size_t findSize);


	///////////////////////////////////////////////////////////////////////////
	/// Memclear
	///
	/// Has the same effect as memset(pDestination, 0, n) but may be faster.
	///
	/// Sets n bytes at pDestination to zero.
	/// Works with uncacheable memory, such as video memory.
	/// There are no alignment restrictions on either pDestination or pSource.
	///
	EASTDC_API void Memclear(void* pDestination, size_t n);

	/// Sets n bytes at pDestination to zero.
	/// Works only with cacheable memory (i.e. conventional system memory).
	/// Cannot be relied on to clear uncachable memory, such as video memory.
	/// There are no alignment restrictions on either pDestination or pSource.
	EASTDC_API void MemclearC(void* pDestination, size_t n);


	///////////////////////////////////////////////////////////////////////////
	/// Memset8 / Memset16 / Memset32 / Memset64 / MemsetN / MemsetPointer
	///
	/// The standard memset function replicates a given 8 bit value into a memory
	/// block. However, we might want to replicate a 16 bit, 32 bit, or 64 bit value
	/// into a block. That's what these functions do. The MemsetN function is a 
	/// generic version which can copy unusual sizes such as 24 bits (e.g. RGB fills).
	/// The count values for each of these is the count of uint16_t, count of uint32_t,
	/// count of pointers, etc.
	/// 
	/// Memset8 is the same as the C memset function. We provide additional variations
	/// of memset which set uint16_t values, uint32_t value, etc. instead of uint8_t 
	/// values like Memset8. MemsetN writes a generic type of any size. In each case 
	/// pDestination must point to enough memory to hold full values. Thus pDestination
	/// for Memset32 must have a capacity for at least (uint32Count * sizeof(uint32_t)) bytes.
	///
	/// The destination is required to be aligned to its type. Thus the destination
	/// of Memset32 must be 32 bit aligned.
	///
	/// There are no restrictions about the type of memory pDestination refers
	/// to except that it be writable.
	///
	/// Works with uncacheable memory, such as video memory, but also works on conventional 
	/// cacheable system memory. 
	EASTDC_API uint8_t*  Memset8      (void* pDestination, uint8_t  c, size_t uint8Count);
	EASTDC_API uint16_t* Memset16     (void* pDestination, uint16_t c, size_t uint16Count);
	EASTDC_API uint32_t* Memset32     (void* pDestination, uint32_t c, size_t uint32Count);
	EASTDC_API uint64_t* Memset64     (void* pDestination, uint64_t c, size_t uint64Count);
	EASTDC_API void*     MemsetN      (void* pDestination, const void* pSource, size_t sourceBytes, size_t nCount);
	EASTDC_API void*     MemsetPointer(void* pDestination, const void* const pValue, size_t ptrCount);

	// Works only with cacheable memory (i.e. conventional system memory).
	// Cannot be relied on to work with uncachable memory, such as video memory.
	// There are no alignment restrictions on either pDestination or pSource.
	EASTDC_API uint8_t* Memset8C(void* pDestination, uint8_t c, size_t uint8Count);

	// Specialized memset for multiple of 128 byte sized blocks.
	EASTDC_API uint8_t*  Memset8_128(void* pDestination, uint8_t c, size_t uint8Count);

	// Cacheable 128 byte memory set
	// Works only with cacheable memory (i.e. conventional system memory).
	// Cannot be relied on to work with uncachable memory, such as video memory.
	// The address pointed to by pDestination must be aligned on a 128-byte boundary, and uint8Count must be a multiple of 128.
	EASTDC_API uint8_t* Memset8_128C(void* pDestination, uint8_t c, size_t uint8Count);


	///////////////////////////////////////////////////////////////////////////
	/// Memfill16 / Memfill24 / Memfill32 / Memfill64 / MemfillSpecific
	/// 
	/// Memfill is the same as memset except that the count parameter is a count 
	/// of bytes and not (for example) a count of uint32_t values. Memfill supports 
	/// byte counts that aren't an even multiple the value size. Thus a call to 
	/// Memfill32(p, 0x00112233, 3) is valid and does what you would expect. 
	/// MemfillSpecific fills (and potentially repeats) any source pattern into any destination space.
	/// There are no restrictions about the type of memory pDestination refers
	/// to except that it be writable.
	///
	EASTDC_API void Memfill8       (void* pDestination, uint8_t c,  size_t byteCount);
	EASTDC_API void Memfill16      (void* pDestination, uint16_t c, size_t byteCount);
	EASTDC_API void Memfill24      (void* pDestination, uint32_t c, size_t byteCount);
	EASTDC_API void Memfill32      (void* pDestination, uint32_t c, size_t byteCount);
	EASTDC_API void Memfill64      (void* pDestination, uint64_t c, size_t byteCount);
	EASTDC_API void MemfillSpecific(void* pDestination, const void* pSource, size_t destByteCount, size_t sourceByteCount);


	///////////////////////////////////////////////////////////////////////////
	/// Memcheck8 / Memcheck16 / Memcheck32 / Memcheck64
	/// 
	/// This family of functions is like Memfill except it verifies that the 
	/// memory is filled as per the value and byte count. Returns a pointer 
	/// to the first mis-matching byte if there's a mismatch. Returns NULL if 
	/// there are no mismatches.
	/// There are no restrictions about the type of memory pDestination refers
	/// to except that it be readable.

	EASTDC_API const void* Memcheck8 (const void* p, uint8_t  c, size_t byteCount);
	EASTDC_API const void* Memcheck16(const void* p, uint16_t c, size_t byteCount);
	EASTDC_API const void* Memcheck32(const void* p, uint32_t c, size_t byteCount);
	EASTDC_API const void* Memcheck64(const void* p, uint64_t c, size_t byteCount);


	///////////////////////////////////////////////////////////////////////////
	/// rwstdc compatibility
	///
	/// These are identical in behaviour to the EAStdC Memfill functions.
	/// There are no restrictions about the type of memory pDestination refers
	/// to except that it be writable.
	///
	EASTDC_API void MemFill16(void* pDestination, uint16_t c, unsigned int byteCount);
	EASTDC_API void MemFill32(void* pDestination, unsigned int value, unsigned int byteCount);
	EASTDC_API void MemFillSpecific(void* pDestination, const void* pSource, unsigned int destByteCount, unsigned int sourceByteCount);


	/// StaticMemory
	///
	/// Allows you to declare memory that's sized and aligned appropriately and 
	/// is allocated outside of the dynamic heap. But also avoids declaring a
	/// class or struct, as that might be unfeasible in some circumstances.
	/// For example, if you have a class that you want to have a global variable
	/// for, but it's impossible for the class to be constructed before main or
	/// the compiler simply fails to do so, you can use StaticMemory.
	///
	/// Example usage:
	///     StaticMemory<sizeof(MyClass)> mStaticMemory;
	///     MyClass* pClass = new(mStaticMemory.Memory()) MyClass;
	///
	template <size_t n>
	struct StaticMemory
	{
		EA_PREFIX_ALIGN(8) uint64_t mMemory[(n + 7) / 8];
		inline void* Memory() { return mMemory; }
	};


	/// TimingSafeMemEqual
	///
	/// Similar to Memcmp but returns simply true or false. Performs slightly faster
	/// than TimingSafeMemcmp.
	/// Executes in a constant time for any given n value. The primary use case for this is 
	/// for security in the presence of possible timing attacks.
	///
	EASTDC_API bool TimingSafeMemEqual(const void* p1, const void* p2, size_t n);


	/// TimingSafeMemcmp
	///
	/// Behaves the same as Memcmp, but executes in a constant time for any given n value. 
	/// The primary use case for this is for security in the presence of possible timing attacks.
	///
	EASTDC_API int TimingSafeMemcmp(const void* p1, const void* p2, size_t n);


	/// TimingSafeMemIsClear
	///
	/// Returns true if the given n bytes of memory are all zero in value, as would be the case
	/// if the memory was cleared with Memclear.
	/// Executes in a constant time for any given n value and regardless of whether the return 
	/// value is true or valse. The primary use case for this is for security in the presence of 
	/// possible timing attacks.
	///
	EASTDC_API bool TimingSafeMemIsClear(const void* p, size_t n);


} // namespace StdC
} // namespace EA



///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

// In optimized non-debug builds, we inline various functions.
// We don't inline these functions in debug builds because in debug builds they
// contain diagnostic code that can't be exposed in headers because that would the 
// user of this header to #include all the debug functionality headers, which isn't 
// feasible.
#if EASTDC_MEMORY_INLINE_ENABLED
	#include <EAStdC/internal/EAMemory.inl>
#endif



#endif // Header include guard












