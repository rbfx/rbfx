///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAENDIAN_H
#define EASTDC_EAENDIAN_H


#include <EABase/eabase.h>
#include <EAStdC/internal/Config.h>
#include <EAStdC/Int128_t.h>

#if defined(_MSC_VER) && (_MSC_VER >= 1300) // VC++ >= v7.1
	#include <stdlib.h> // for _byteswap_ushort, _byteswap_ulong
	#pragma intrinsic(_byteswap_ushort, _byteswap_ulong) // Unless you do this, you will get linker errors with the DLL CRT.
	#if defined(EA_PROCESSOR_X86_64)
		#pragma intrinsic(_byteswap_uint64)
	#endif
#endif


//   ---------------------------------------------------------------------------------------------------------------------------------------------------------
//   | Big-endian version      | Little-endian version      | Purpose                               | Example Usage
//   |========================================================================================================================================================
//   | Swizzle                 | Swizzle                    | Unilaterally converts a value to the  | uint16_t x = Swizzle(0x1234);
//   |                         |                            | opposite endian.                      | (x becomes 0x3412)
//   |                         |                            |                                       |
//   | ---------------------------------------------------------------------------------------------|---------------------------------------------------------
//   | ReadFromBigEndian       | ReadFromLittleEndian       | Reads a value from a buffer with      | void* pBuffer;
//   |                         |                            | a source endian-ness to a variable    | uint16_t x = ReadFromLittleEndianUint16(pBuffer);
//   |                         |                            | of local endian.                      |
//   | ---------------------------------------------------------------------------------------------|---------------------------------------------------------
//   | WriteToBigEndian        | WriteToLittleEndian        | Writes a value from local endian      | void* pBuffer;
//   |                         |                            | to a buffer of destination endian.    | uint32_t x;
//   |                         |                            |                                       | WriteToLittleEndian(x, pBuffer);
//   |--------------------------------------------------------------------------------------------------------------------------------------------------------
//   | ToBigEndian             | ToLittleEndian             | Converts a variable from local        | uint64_t x;
//   |                         |                            | endian to destination endian.         | x = ToLittleEndian(x);
//   |                         |                            |                                       |
//   | ---------------------------------------------------------------------------------------------|---------------------------------------------------------
//   | FromBigEndian           | FromLittleEndian           | Converts a variable from a given      | uint32 x;
//   |                         |                            | endian-ness to local endian-ness.     | x = FromBigEndian(x);
//   |                         |                            |                                       |
//   | ---------------------------------------------------------------------------------------------|---------------------------------------------------------
//   | ToBigEndianConst        | ToLittleEndianConst        | Converts a constant from local        | uint32_t x = ToBigEndianConst(0x12345678);
//   |                         |                            | endian to destination endian. Faster  |
//   |                         |                            | than non-const and good for globals.  |
//   | -------------------------------------------------------------------------------------------------------------------------------------------------------
//   | FromBigEndianConst      | FromLittleEndianConst      | Converts a constant from a given      | uint16_t x = FromBigEndianConst(0x1234);
//   |                         |                            | endian-ness to local endian-ness.     |
//   |                         |                            | Faster than non-const and for globals.|
//   |----------------------------------------------------------------------------------------------|---------------------------------------------------------


namespace EA
{
	namespace StdC
	{

		/// Enum Endian
		///
		/// Allows a system to specify endian-ness. We recognize only big endian and 
		/// little endian, as these are by far the most common kind of endian-ness.
		///
		enum Endian
		{
			kEndianBig       = 0,            /// Big endian.
			kEndianLittle    = 1,            /// Little endian.
			#ifdef EA_SYSTEM_BIG_ENDIAN
				kEndianLocal = kEndianBig    /// Whatever endian is native to the machine.
			#else 
				kEndianLocal = kEndianLittle /// Whatever endian is native to the machine.
			#endif
		};


		namespace detail
		{
			#if defined(EA_PROCESSOR_X86_64) && defined(_MSC_VER) 
				EA_FORCE_INLINE uint16_t bswap16(uint16_t x) { return static_cast<uint16_t>(_byteswap_ushort(x)); }
				EA_FORCE_INLINE uint32_t bswap32(uint32_t x) { return static_cast<uint32_t>(_byteswap_ulong(x)); }
				EA_FORCE_INLINE uint64_t bswap64(uint64_t x) { return static_cast<uint64_t>(_byteswap_uint64(x)); }
				#define EASTDC_LITTLE_ENDIAN_WITH_BSWAP 1
			#elif defined(EA_PROCESSOR_X86_64) && (defined(EA_COMPILER_CLANG) || defined(EA_COMPILER_GNUC)) 
				EA_FORCE_INLINE uint16_t bswap16(uint16_t x) { return static_cast<uint16_t>(__builtin_bswap16(x)); }
				EA_FORCE_INLINE uint32_t bswap32(uint32_t x) { return static_cast<uint32_t>(__builtin_bswap32(x)); }
				EA_FORCE_INLINE uint64_t bswap64(uint64_t x) { return static_cast<uint64_t>(__builtin_bswap64(x)); }
				#define EASTDC_LITTLE_ENDIAN_WITH_BSWAP 1
			#endif

			#if defined(EASTDC_LITTLE_ENDIAN_WITH_BSWAP)
				EA_FORCE_INLINE int16_t bswap16(int16_t x) { return static_cast<int16_t>(bswap16(static_cast<uint16_t>(x))); }
				EA_FORCE_INLINE int32_t bswap32(int32_t x) { return static_cast<int32_t>(bswap32(static_cast<uint32_t>(x))); }
				EA_FORCE_INLINE int64_t bswap64(int64_t x) { return static_cast<int64_t>(bswap64(static_cast<uint64_t>(x))); }
			#endif 


			inline float FloatFromBitRepr(uint32_t bits)
			{
				union { float f; uint32_t u; } pun;
				pun.u = bits;
				return pun.f;
			}
			inline double DoubleFromBitRepr(uint64_t bits)
			{
				union { double d; uint64_t u; } pun;
				pun.u = bits;
				return pun.d;
			}
			inline uint32_t BitReprOfFloat(float f)
			{
				union { float f; uint32_t u; } pun;
				pun.f = f;
				return pun.u;
			}
			inline uint64_t BitReprOfDouble(double d)
			{
				union { double d; uint64_t u; } pun;
				pun.d = d;
				return pun.u;
			}
		}

		#ifdef EASTDC_LITTLE_ENDIAN_WITH_BSWAP
			// Runtime reading data from an address.
			// The address need not be aligned to the data size -- but unaligned accesses are acceptable on x64 processors.
			EA_FORCE_INLINE uint16_t ReadFromBigEndianUint16(const void* pData) { return detail::bswap16(static_cast<const uint16_t*>(pData)[0]); }
			EA_FORCE_INLINE uint32_t ReadFromBigEndianUint32(const void* pData) { return detail::bswap32(static_cast<const uint32_t*>(pData)[0]); }
			EA_FORCE_INLINE uint64_t ReadFromBigEndianUint64(const void* pData) { return detail::bswap64(static_cast<const uint64_t*>(pData)[0]); }
			EA_FORCE_INLINE int16_t  ReadFromBigEndianInt16(const void* pData)  { return detail::bswap16(static_cast<const int16_t*>( pData)[0]); }
			EA_FORCE_INLINE int32_t  ReadFromBigEndianInt32(const void* pData)  { return detail::bswap32(static_cast<const int32_t*>( pData)[0]); }
			EA_FORCE_INLINE int64_t  ReadFromBigEndianInt64(const void* pData)  { return detail::bswap64(static_cast<const int64_t*>( pData)[0]); }

		#else
			// Runtime reading data from an address.
			// The address need not be aligned to the data size.
			inline uint16_t ReadFromBigEndianUint16(const void* pData)
			{
				return (uint16_t)((uint32_t)(((const uint8_t*)pData)[0] << 8) | 
								  (uint32_t)(((const uint8_t*)pData)[1] << 0));
			}

			inline int16_t ReadFromBigEndianInt16(const void* pData)
				{ return (int16_t)ReadFromBigEndianUint16(pData); }

			inline uint32_t ReadFromBigEndianUint32(const void* pData)
			{
				return (uint32_t)((uint32_t)(((const uint8_t*)pData)[0] << 24) | 
								  (uint32_t)(((const uint8_t*)pData)[1] << 16) | 
								  (uint32_t)(((const uint8_t*)pData)[2] <<  8) | 
								  (uint32_t)(((const uint8_t*)pData)[3] <<  0));
			}

			inline int32_t ReadFromBigEndianInt32(const void* pData)
				{ return (int32_t)ReadFromBigEndianUint32(pData); }

			inline uint64_t ReadFromBigEndianUint64(const void* pData)
			{
				uint64_t nReturnValue(0);
				const uint8_t* const pData8 = (const uint8_t*)pData;
				for(size_t i = 0; i < sizeof(uint64_t); ++i)
					nReturnValue |= ((uint64_t)pData8[sizeof(uint64_t) - 1 - i] << (i * 8));
				return nReturnValue;
			}

			inline int64_t ReadFromBigEndianInt64(const void* pData)
				{ return (int64_t)ReadFromBigEndianUint64(pData); }
		#endif


		inline float ReadFromBigEndianFloat(const void* pData)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint32_t nResult = ReadFromBigEndianUint32(pData);
			return detail::FloatFromBitRepr(nResult);
		}

		inline double ReadFromBigEndianDouble(const void* pData)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint64_t nResult = ReadFromBigEndianUint64(pData);
			return detail::DoubleFromBitRepr(nResult);
		}

		inline uint128_t ReadFromBigEndianUint128(const void* pData)
		{
			uint128_t nReturnValue((uint32_t)0);
			const uint8_t* const pData8 = (const uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(uint128_t); ++i)
			{
				uint128_t temp(pData8[sizeof(uint128_t) - 1 - i]);
				temp <<= (i * 8);
				nReturnValue |= temp;
			}
			return nReturnValue;
		}

		inline int128_t ReadFromBigEndianInt128(const void* pData)
		{
			int128_t nReturnValue((uint32_t)0);
			const uint8_t* const pData8 = (const uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(int128_t); ++i)
			{
				int128_t temp(pData8[sizeof(int128_t) - 1 - i]);
				temp <<= (i * 8);
				nReturnValue |= temp;
			}
			return nReturnValue;
		}


		// This function provided for backwards compatibility with older EA code.
		// Reads nSourceBytes stored in big-endian ordering into a uint32_t.
		// nSourceBytes can be a number in the range of 1..4. nSourceBytes counts
		// below 4 refer to the lower significant bytes of the uint32_t. The primary
		// purpose of this function is to decode packed integer streams, as a uint32_t
		// normally uses 4 bytes but if it's just small numbers then it's not useful
		// to store the higher bytes.
		// This function currently requires pSource to be 32 bit aligned or requires the 
		// given platform to be able to read uint32_t values that aren't 32 bit aligned.
		inline uint32_t ReadFromBigEndian(const void* pSource, int32_t nSourceBytes)
		{
			#if defined(EA_SYSTEM_BIG_ENDIAN)
				switch (nSourceBytes)
				{
					case 1:
						return (uint32_t)*(const uint8_t*)pSource;
					case 2: 
						return (uint32_t)*(const uint16_t*)pSource;
					case 3:
						return (uint32_t)*(const uint32_t*)pSource >> 8;
					case 4:
						return (uint32_t)*(const uint32_t*)pSource;
					default: 
						return 0;
				}
			#else
				switch (nSourceBytes)
				{
					case 1:
						return 
							  (uint32_t)*((const uint8_t*)pSource + 0);
					case 2: 
						return 
							(((uint32_t)*((const uint8_t*)pSource + 0)) << 8) |
							(((uint32_t)*((const uint8_t*)pSource + 1)));
					case 3:
						return 
							(((uint32_t)*((const uint8_t*)pSource + 0)) << 16) |
							(((uint32_t)*((const uint8_t*)pSource + 1)) << 8)  |
							(((uint32_t)*((const uint8_t*)pSource + 2)));
					case 4:
						return 
							(((uint32_t)*((const uint8_t*)pSource + 0)) << 24) |
							(((uint32_t)*((const uint8_t*)pSource + 1)) << 16) |
							(((uint32_t)*((const uint8_t*)pSource + 2)) << 8)  |
							(((uint32_t)*((const uint8_t*)pSource + 3)));
					default: 
						return 0;
				}
			#endif
		}

		inline void ReadFromBigEndian(const void* pSource, void* pDest, size_t sizeOfData)
		{
			// Currently we require the destination address to be aligned with the data type size.
			switch(sizeOfData)
			{
				case 16:
					*((uint128_t*)pDest) = ReadFromBigEndianUint128(pSource);
					break;
				case 8:
					*((uint64_t*)pDest) = ReadFromBigEndianUint64(pSource);
					break;
				case 4:
					*((uint32_t*)pDest) = ReadFromBigEndianUint32(pSource);
					break;
				case 2:
					*((uint16_t*)pDest) = ReadFromBigEndianUint16(pSource);
					break;
				case 1:
					*((uint8_t*)pDest) = *((uint8_t*)pSource);
					break;
			}
		}

		#ifdef EASTDC_LITTLE_ENDIAN_WITH_BSWAP
			// Runtime reading data from an address.
			// The address need not be aligned to the data size -- but unaligned accesses are acceptable on x64 processors.
			EA_FORCE_INLINE uint16_t ReadFromLittleEndianUint16(const void* pData) { return (static_cast<const uint16_t*>(pData)[0]); }
			EA_FORCE_INLINE uint32_t ReadFromLittleEndianUint32(const void* pData) { return (static_cast<const uint32_t*>(pData)[0]); }
			EA_FORCE_INLINE uint64_t ReadFromLittleEndianUint64(const void* pData) { return (static_cast<const uint64_t*>(pData)[0]); }
			EA_FORCE_INLINE int16_t ReadFromLittleEndianInt16(const void* pData)   { return (static_cast<const int16_t*>( pData)[0]); }
			EA_FORCE_INLINE int32_t ReadFromLittleEndianInt32(const void* pData)   { return (static_cast<const int32_t*>( pData)[0]); }
			EA_FORCE_INLINE int64_t ReadFromLittleEndianInt64(const void* pData)   { return (static_cast<const int64_t*>( pData)[0]); }

		#else
			// Runtime reading data from an address.
			// Address need not be aligned to the data size.
			// Thus we cannot safely simply cast the address to the desired data type.
			inline uint16_t ReadFromLittleEndianUint16(const void* pData)
			{
				return (uint16_t)((uint32_t)(((const uint8_t*)pData)[0] << 0) | 
								  (uint32_t)(((const uint8_t*)pData)[1] << 8));
			}

			inline int16_t ReadFromLittleEndianInt16(const void* pData)
				{ return (int16_t)ReadFromLittleEndianUint16(pData); }

			inline uint32_t ReadFromLittleEndianUint32(const void* pData)
			{
				return (uint32_t)((uint32_t)(((const uint8_t*)pData)[0] <<  0) | 
								  (uint32_t)(((const uint8_t*)pData)[1] <<  8) | 
								  (uint32_t)(((const uint8_t*)pData)[2] << 16) | 
								  (uint32_t)(((const uint8_t*)pData)[3] << 24));
			}

			inline int32_t ReadFromLittleEndianInt32(const void* pData)
				{ return (int32_t)ReadFromLittleEndianUint32(pData); }

			inline uint64_t ReadFromLittleEndianUint64(const void* pData)
			{
				uint64_t nReturnValue(0);
				const uint8_t* const pData8 = (const uint8_t*)pData;
				for(size_t i = 0; i < sizeof(uint64_t); ++i)
					nReturnValue |= ((uint64_t)pData8[i] << (i * 8));
				return nReturnValue;
			}

			inline int64_t ReadFromLittleEndianInt64(const void* pData)
				{ return (int64_t)ReadFromLittleEndianUint64(pData); }
		#endif

		inline float ReadFromLittleEndianFloat(const void* pData)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint32_t nResult = ReadFromLittleEndianUint32(pData);
			return detail::FloatFromBitRepr(nResult);
		}

		inline double ReadFromLittleEndianDouble(const void* pData)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint64_t nResult = ReadFromLittleEndianUint64(pData);
			return detail::DoubleFromBitRepr(nResult);
		}

		inline uint128_t ReadFromLittleEndianUint128(const void* pData)
		{
			uint128_t nReturnValue((uint32_t)0);
			const uint8_t* const pData8 = (const uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(uint128_t); ++i)
				nReturnValue |= ((uint128_t)pData8[i] << (i * 8));
			return nReturnValue;
		}

		inline int128_t ReadFromLittleEndianInt128(const void* pData)
		{
			int128_t nReturnValue((uint32_t)0);
			const uint8_t* const pData8 = (const uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(int128_t); ++i)
				nReturnValue |= ((int128_t)pData8[i] << (i * 8));
			return nReturnValue;
		}

		inline void ReadFromLittleEndian(const void* pSource, void* pDest, size_t sizeOfData)
		{
			// Currently we require the destination address to be aligned with the data type size.
			switch(sizeOfData)
			{
				case 16:
					*((uint128_t*)pDest) = ReadFromLittleEndianUint128(pSource);
					break;
				case 8:
					*((uint64_t*)pDest) = ReadFromLittleEndianUint64(pSource);
					break;
				case 4:
					*((uint32_t*)pDest) = ReadFromLittleEndianUint32(pSource);
					break;
				case 2:
					*((uint16_t*)pDest) = ReadFromLittleEndianUint16(pSource);
					break;
				case 1:
					*((uint8_t*)pDest) = *((uint8_t*)pSource);
					break;
			}
		}

		#ifdef EASTDC_LITTLE_ENDIAN_WITH_BSWAP
			inline void WriteToBigEndian(void* pData, uint16_t x) { static_cast<uint16_t*>(pData)[0] = detail::bswap16(x); }
			inline void WriteToBigEndian(void* pData, uint32_t x) { static_cast<uint32_t*>(pData)[0] = detail::bswap32(x); }
			inline void WriteToBigEndian(void* pData, uint64_t x) { static_cast<uint64_t*>(pData)[0] = detail::bswap64(x); }
			inline void WriteToBigEndian(void* pData, int16_t x)  { static_cast<int16_t*>( pData)[0] = detail::bswap16(x); }
			inline void WriteToBigEndian(void* pData, int32_t x)  { static_cast<int32_t*>( pData)[0] = detail::bswap32(x); }
			inline void WriteToBigEndian(void* pData, int64_t x)  { static_cast<int64_t*>( pData)[0] = detail::bswap64(x); }
		#else
			// Runtime swizzling of write data to an address.
			// Address need not be aligned to the data size.
			inline void WriteToBigEndian(void* pData, uint16_t x)
			{
				((uint8_t*)pData)[0] = (uint8_t)(x >> 8);
				((uint8_t*)pData)[1] = (uint8_t)(x);
			}

			inline void WriteToBigEndian(void* pData, int16_t x)
				{ WriteToBigEndian(pData, (uint16_t)x); }

			inline void WriteToBigEndian(void* pData, uint32_t x)
			{
				((uint8_t*)pData)[0] = (uint8_t)(x >> 24);
				((uint8_t*)pData)[1] = (uint8_t)(x >> 16);
				((uint8_t*)pData)[2] = (uint8_t)(x >> 8);
				((uint8_t*)pData)[3] = (uint8_t)(x);
			}

			inline void WriteToBigEndian(void* pData, int32_t x)
				{ WriteToBigEndian(pData, (uint32_t)x); }

			inline void WriteToBigEndian(void* pData, uint64_t x)
			{
				uint8_t* const pData8 = (uint8_t*)pData;
				for(size_t i = 0; i < sizeof(uint64_t); ++i)
					pData8[sizeof(uint64_t) - 1 - i] = (uint8_t)(x >> (i * 8));
			}

			inline void WriteToBigEndian(void* pData, int64_t x)
				{ WriteToBigEndian(pData, (uint64_t)x); }
		#endif

		inline void WriteToBigEndian(void* pData, float x)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint32_t nValue = detail::BitReprOfFloat(x);
			WriteToBigEndian(pData, nValue);
		}

		inline void WriteToBigEndian(void* pData, double x)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint64_t nValue = detail::BitReprOfDouble(x);
			WriteToBigEndian(pData, nValue);
		}

		inline void WriteToBigEndian(void* pData, uint128_t x)
		{
			uint8_t* const pData8 = (uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(uint128_t); ++i)
				pData8[sizeof(uint128_t) - 1 - i] = (x >> (i * 8)).AsUint8();
		}

		inline void WriteToBigEndian(void* pData, int128_t x)
		{
			uint8_t* const pData8 = (uint8_t*)pData;
			for(int i = 0; i < (int)sizeof(int128_t); ++i)
				pData8[sizeof(int128_t) - 1 - i] = (x >> (i * 8)).AsUint8();
		}

		// This function provided for backwards compatibility with older EA code.
		// Reads nSourceBytes stored in big-endian ordering into a uint32_t.
		// nSourceBytes can be a number in the range of 1..4. nSourceBytes counts
		// below 4 refer to the lower significant bytes of the uint32_t. The primary
		// purpose of this function is to decode packed integer streams, as a uint32_t
		// normally uses 4 bytes but if it's just small numbers then it's not useful
		// to store the higher bytes.
		// This function currently requires pSource to be 32 bit aligned or requires the 
		// given platform to be able to read uint32_t values that aren't 32 bit aligned.
		inline void WriteToBigEndian(const void* pDest, uint32_t data, int32_t nSourceBytes)
		{
			#if defined(EA_SYSTEM_BIG_ENDIAN)
				switch (nSourceBytes)
				{
					case 1:
						((uint8_t*)pDest)[0] = (uint8_t)data;
						break;

					case 2: 
						((uint16_t*)pDest)[0] = (uint16_t)data;
						break;

					case 3:
						((uint32_t*)pDest)[0] = (uint32_t)data;
						break;

					case 4:
						((uint8_t*)pDest)[0] = (uint8_t)(data >> 16);
						((uint8_t*)pDest)[1] = (uint8_t)(data >>  8);
						((uint8_t*)pDest)[2] = (uint8_t)(data >>  0);
						break;
				}
			#else
				switch (nSourceBytes)
				{
					case 1:
						((uint8_t*)pDest)[0] = (uint8_t)(data >> 0);
						break;

					case 2: 
						((uint8_t*)pDest)[0] = (uint8_t)(data >> 8);
						((uint8_t*)pDest)[1] = (uint8_t)(data >> 0);
						break;

					case 3:
						((uint8_t*)pDest)[0] = (uint8_t)(data >> 16);
						((uint8_t*)pDest)[1] = (uint8_t)(data >>  8);
						((uint8_t*)pDest)[2] = (uint8_t)(data >>  0);
						break;

					case 4:
						((uint8_t*)pDest)[0] = (uint8_t)(data >> 24);
						((uint8_t*)pDest)[1] = (uint8_t)(data >> 16);
						((uint8_t*)pDest)[2] = (uint8_t)(data >>  8);
						((uint8_t*)pDest)[3] = (uint8_t)(data >>  0);
						break;
				}
			#endif
		}

		inline void WriteToBigEndian(const void* pSource, void* pDest, size_t sizeOfData)
		{
			// Currently we require the source address to be aligned with the data type size.
			switch(sizeOfData)
			{
				case 16:
					WriteToBigEndian(pDest, *((uint128_t*)pSource));
					break;
				case 8:
					WriteToBigEndian(pDest, *((uint64_t*)pSource));
					break;
				case 4:
					WriteToBigEndian(pDest, *((uint32_t*)pSource));
					break;
				case 2:
					WriteToBigEndian(pDest, *((uint16_t*)pSource));
					break;
				case 1:
					*((uint8_t*)pDest) = *((uint8_t*)pSource);
					break;
			}
		}

		#ifdef EASTDC_LITTLE_ENDIAN_WITH_BSWAP
			// Runtime swizzling of write data to an address.
			// Address need not be aligned to the data size -- but unaligned accesses are acceptable on x64 processors.
			inline void WriteToLittleEndian(void* pData, uint16_t x) { (static_cast<uint16_t*>(pData))[0] = x; }
			inline void WriteToLittleEndian(void* pData, uint32_t x) { (static_cast<uint32_t*>(pData))[0] = x; }
			inline void WriteToLittleEndian(void* pData, uint64_t x) { (static_cast<uint64_t*>(pData))[0] = x; }
			inline void WriteToLittleEndian(void* pData, int16_t x)  { (static_cast<int16_t*>( pData))[0] = x; }
			inline void WriteToLittleEndian(void* pData, int32_t x)  { (static_cast<int32_t*>( pData))[0] = x; }
			inline void WriteToLittleEndian(void* pData, int64_t x)  { (static_cast<int64_t*>( pData))[0] = x; }
		#else
			// Runtime swizzling of write data to an address.
			// Address need not be aligned to the data size.
			inline void WriteToLittleEndian(void* pData, uint16_t x)
			{
				((uint8_t*)pData)[0] = (uint8_t)(x);
				((uint8_t*)pData)[1] = (uint8_t)(x >> 8);
			}

			inline void WriteToLittleEndian(void* pData, int16_t x)
				{ WriteToLittleEndian(pData, (uint16_t)x); }

			inline void WriteToLittleEndian(void* pData, uint32_t x)
			{
				((uint8_t*)pData)[0] = (uint8_t)(x);
				((uint8_t*)pData)[1] = (uint8_t)(x >> 8);
				((uint8_t*)pData)[2] = (uint8_t)(x >> 16);
				((uint8_t*)pData)[3] = (uint8_t)(x >> 24);
			}

			inline void WriteToLittleEndian(void* pData, int32_t x)
				{  WriteToLittleEndian(pData, (uint32_t)x); }

			inline void WriteToLittleEndian(void* pData, uint64_t x)
			{
				uint8_t* const pData8 = (uint8_t*)pData;
				for(int i = sizeof(uint64_t) - 1; i >= 0; --i)
					pData8[i] = (uint8_t)(x >> (i * 8));
			}

			inline void WriteToLittleEndian(void* pData, int64_t x)
				{ WriteToLittleEndian(pData, (uint64_t)x); }
		#endif

		inline void WriteToLittleEndian(void* pData, float x)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint32_t nValue = detail::BitReprOfFloat(x);
			WriteToLittleEndian(pData, nValue);
		}

		inline void WriteToLittleEndian(void* pData, double x)
		{
			// Working with floating point values is safe here because 
			// at no time does the FPU see an invalid swizzled float.
			uint64_t nValue = detail::BitReprOfDouble(x);
			WriteToLittleEndian(pData, nValue);
		}

		inline void WriteToLittleEndian(void* pData, uint128_t x)
		{
			uint8_t* const pData8 = (uint8_t*)pData;
			for(int i = sizeof(uint128_t) - 1; i >= 0; --i)
				pData8[i] = (x >> (i * 8)).AsUint8();
		}

		inline void WriteToLittleEndian(void* pData, int128_t x)
		{
			uint8_t* const pData8 = (uint8_t*)pData;
			for(int i = sizeof(int128_t) - 1; i >= 0; --i)
				pData8[i] = (x >> (i * 8)).AsUint8();
		}

		inline void WriteToLittleEndian(const void* pSource, void* pDest, size_t sizeOfData)
		{
			// Currently we require the source address to be aligned with the data type size.
			switch(sizeOfData)
			{
				case 16:
					WriteToLittleEndian(pDest, *((uint128_t*)pSource));
					break;
				case 8:
					WriteToLittleEndian(pDest, *((uint64_t*)pSource));
					break;
				case 4:
					WriteToLittleEndian(pDest, *((uint32_t*)pSource));
					break;
				case 2:
					WriteToLittleEndian(pDest, *((uint16_t*)pSource));
					break;
				case 1:
					*((uint8_t*)pDest) = *((uint8_t*)pSource);
					break;
			}
		}




		// Compiler/platform-specific implementations
		// Implement this for as many of the following platforms as possible,
		// defining EA_SWIZZLE_X_DEFINED for each one implemented:
		//       SNSystems/PlayStation
		//       Microsoft/XBox
		//       Microsoft/PC
		//       GCC/PC
		//       GCC/Linux
		//       GCC/Macintosh

		#ifdef EASTDC_LITTLE_ENDIAN_WITH_BSWAP
			EA_FORCE_INLINE uint16_t Swizzle(uint16_t x) { return detail::bswap16(x); }
			EA_FORCE_INLINE int16_t  Swizzle(int16_t x)  { return detail::bswap16(x); }
			#define EA_SWIZZLE_16_DEFINED
			
			EA_FORCE_INLINE uint32_t Swizzle(uint32_t x) { return detail::bswap32(x); }
			EA_FORCE_INLINE int32_t  Swizzle(int32_t x)  { return detail::bswap32(x); }
			#define EA_SWIZZLE_32_DEFINED
			
			EA_FORCE_INLINE uint64_t Swizzle(uint64_t x) { return detail::bswap64(x); }
			EA_FORCE_INLINE int64_t  Swizzle(int64_t x)  { return detail::bswap64(x); }
			#define EA_SWIZZLE_64_DEFINED

		// MSVC7 / any platform
		#elif defined(EA_PLATFORM_MICROSOFT) && (_MSC_VER >= 1300)
			inline uint16_t Swizzle(uint16_t x)
			{
				return _byteswap_ushort(x); // The Microsoft compiler will fail to link _byteswap_ushort unless you 
			}                               // enable intrinsic functions or link in an appropriate library.
			inline int16_t Swizzle(int16_t x)
			{
				return (int16_t)_byteswap_ushort((uint16_t)x);
			}

			#define EA_SWIZZLE_16_DEFINED          

			inline uint32_t Swizzle(uint32_t x)
			{
				return _byteswap_ulong(x);  // The Microsoft compiler will fail to link _byteswap_ulong unless you 
			}                               // enable intrinsic functions or link in an appropriate library.
			inline int32_t Swizzle(int32_t x)
			{
				return (int32_t)_byteswap_ulong((uint32_t)x);
			}
			#define EA_SWIZZLE_32_DEFINED          

		#elif defined(EA_PROCESSOR_X86) && (defined(EA_COMPILER_MSVC))
			inline uint16_t Swizzle(uint16_t x)
			{
				__asm xor eax, eax
				__asm mov ax, x
				__asm xchg ah, al
			}
			inline int16_t Swizzle(int16_t x)
				{ return (int16_t)Swizzle((uint16_t)x); }
			#define EA_SWIZZLE_16_DEFINED

			inline uint32_t Swizzle(uint32_t x)
			{
				// VC7 and later have a _bswap_ulong() function which converts over to a bswap 
				// instruction if you use use /Ox ("full optimization") compiler option. 
				// However, with other optimization options it yields rather poor code. So we 
				// take the reliable route here and use asm and don't use _bswap_ulong().
				__asm mov eax, x
				__asm bswap eax
			}
			inline int32_t Swizzle(int32_t x)
				{ return (int32_t)Swizzle((uint32_t)x); }
			#define EA_SWIZZLE_32_DEFINED


		// GNUC/IntelX86
		#elif defined(EA_PROCESSOR_X86) && defined(EA_COMPILER_GNUC)
			inline uint16_t Swizzle(uint16_t x)
			{
				__asm__ __volatile__("xchgb %b0,%h0" : "=q" (x) : "0" (x));
				return x;
			}
			inline int16_t Swizzle(int16_t x)
				{ return (int16_t)Swizzle((uint16_t)x); }
			#define EA_SWIZZLE_16_DEFINED

			inline uint32_t Swizzle(uint32_t x)
			{
				__asm__ __volatile__("bswap %0" : "=r" (x) : "0" (x));
				return x;
			}
			inline int32_t Swizzle(int32_t x)
				{ return (int32_t)Swizzle((uint32_t)x); }
			#define EA_SWIZZLE_32_DEFINED



		// GNUC/PowerPC
		#elif defined(EA_PROCESSOR_POWERPC) && defined(EA_COMPILER_GNUC)
			inline uint16_t Swizzle(uint16_t x)
			{
				uint16_t temp;
				__asm__ __volatile__("lhbrx %0,0,%1" : "=r" (temp) : "r" (&x): "memory"); 
				return temp;
			}
			inline int16_t Swizzle(int16_t x)
				{ return (int16_t)Swizzle((uint16_t)x); }
			#define EA_SWIZZLE_16_DEFINED

			inline uint32_t Swizzle(uint32_t x)
			{
				uint32_t temp;
				__asm__ __volatile__("lwbrx %0,0,%1" : "=r" (temp) : "r" (&x): "memory"); 
				return temp;
			}
			inline int32_t Swizzle(int32_t x)
				{ return (int32_t)Swizzle((uint32_t)x); }
			#define EA_SWIZZLE_32_DEFINED

		// GNUC/ARM (ARM is a handheld device processor)
		// GCC for ARM generates bad swizzle code which doesn't use the rev instruction. So we don't use them here.
		// http://hardwarebug.org/2010/01/14/beware-the-builtins/
		#elif defined(EA_PROCESSOR_ARM) && defined(EA_COMPILER_GNUC) && !defined(__THUMB_INTERWORK__) // The Thumb instruction set doesn't have endian reversing instructions.
			inline uint16_t Swizzle(uint16_t x)
			{
				int temp;
				__asm__ __volatile__(
					"and %1, %2, #0xff\n"
					"mov %0, %2, lsr #8\n"
					"orr %0, %0, %1, lsl #8"
					: "=r" (x), "=r" (temp)
					: "r" (x));
				return (x);
			}
			inline int16_t Swizzle(int16_t x)
				{ return (int16_t)Swizzle((uint16_t)x); }
			#define EA_SWIZZLE_16_DEFINED

			inline uint32_t Swizzle(uint32_t x)
			{
				int temp;
				__asm__ __volatile__(
					"eor   %1, %2, %2, ror #16\n"
					"bic   %1, %1, #0x00ff0000\n"
					"mov   %0, %2, ror #8\n"
					"eor   %0, %0, %1, lsr #8"
					: "=r" (x), "=r" (temp)
					: "r" (x));
				return (x);
			}
			inline int32_t Swizzle(int32_t x)
				{ return (int32_t)Swizzle((uint32_t)x); }
			#define EA_SWIZZLE_32_DEFINED

		#endif


		// Compiler/platform-independent implementations.
		#ifndef EA_SWIZZLE_16_DEFINED
			inline uint16_t Swizzle(uint16_t x)
			{
				return (uint16_t) ((x >> 8) | (x << 8));
			}
			inline int16_t Swizzle(int16_t x)
				{ return (int16_t)Swizzle((uint16_t)x); }
		#endif

		#ifndef EA_SWIZZLE_32_DEFINED
			inline uint32_t Swizzle(uint32_t x)
			{
				// An alternative to the mechanism of using shifts and ors below
				// is to use byte addressing. However, tests have shown that  
				// while the shifts and ORs look heavier than byte addressing, 
				// they are easier for the compiler to optimize and make it much
				// more likely that the compiler will optimize away the code
				// below and simply precalculate the result.
				return (uint32_t)
					((x >> 24)               |
					((x << 24) & 0xff000000) |
					((x <<  8) & 0x00ff0000) |
					((x >>  8) & 0x0000ff00)); 
			}
			inline int32_t Swizzle(int32_t x)
				{ return (int32_t)Swizzle((uint32_t)x); }
		#endif

		#ifndef EA_SWIZZLE_64_DEFINED
			inline uint64_t Swizzle(uint64_t x)
			{
				const uint32_t high32Bits = Swizzle((uint32_t)(x));
				const uint32_t low32Bits  = Swizzle((uint32_t)(x >> 32));

				return ((uint64_t)high32Bits << 32) | low32Bits;
			}
			inline int64_t Swizzle(int64_t x)
				{ return (int64_t)Swizzle((uint64_t)x); }
		#endif

		#ifndef EA_SWIZZLE_128_DEFINED
			inline uint128_t Swizzle(uint128_t x)
			{
				const uint64_t high64Bits = Swizzle(x.AsUint64());
				const uint64_t low64Bits  = Swizzle((x >> 64).AsUint64());

				return ((uint128_t)high64Bits << 64) | low64Bits;
			}

			inline int128_t Swizzle(int128_t x)
			{
				const uint64_t high64Bits = Swizzle(x.AsUint64());
				const uint64_t low64Bits  = Swizzle((x >> 64).AsUint64());

				return ((int128_t)high64Bits << 64) | low64Bits;
			}
		#endif

		// This is done by pointer instead of value because floating point values
		// touch the FPU, and you can't swizzle values that touch the FPU.
		inline void Swizzle(float* pFloat)
		{
			union FloatConv
			{
				float    f;
				uint32_t i;
			} fc = { *pFloat };

			fc.i = Swizzle(fc.i);
			*pFloat = fc.f;
		}

		// This is done by pointer instead of value because floating point values
		// touch the FPU, and you can't swizzle values that touch the FPU.
		inline void Swizzle(double* pDouble)
		{
			union DoubleConv
			{
				double   d;
				uint64_t i;
			} fc = { *pDouble };

			fc.i = Swizzle(fc.i);
			*pDouble = fc.d;
		}


		#if defined(EA_SYSTEM_BIG_ENDIAN)
			inline uint16_t  ToBigEndian(uint16_t x)      { return x; }
			inline  int16_t  ToBigEndian( int16_t x)      { return x; }
			inline uint32_t  ToBigEndian(uint32_t x)      { return x; } 
			inline  int32_t  ToBigEndian( int32_t x)      { return x; } 
			inline uint64_t  ToBigEndian(uint64_t x)      { return x; }
			inline  int64_t  ToBigEndian( int64_t x)      { return x; }
			inline uint128_t ToBigEndian(uint128_t x)     { return x; }
			inline  int128_t ToBigEndian( int128_t x)     { return x; }
			inline void      ToBigEndian(float*)          { }
			inline void      ToBigEndian(double*)         { }

			inline uint16_t  FromBigEndian(uint16_t x)    { return x; }
			inline  int16_t  FromBigEndian( int16_t x)    { return x; }
			inline uint32_t  FromBigEndian(uint32_t x)    { return x; }
			inline  int32_t  FromBigEndian( int32_t x)    { return x; }
			inline uint64_t  FromBigEndian(uint64_t x)    { return x; }
			inline  int64_t  FromBigEndian( int64_t x)    { return x; }
			inline uint128_t FromBigEndian(uint128_t x)   { return x; }
			inline  int128_t FromBigEndian( int128_t x)   { return x; }
			inline void      FromBigEndian(float*)        { }
			inline void      FromBigEndian(double*)       { }

			inline uint16_t  ToLittleEndian(uint16_t x)    { return Swizzle(x); }
			inline  int16_t  ToLittleEndian( int16_t x)    { return Swizzle(x); }
			inline uint32_t  ToLittleEndian(uint32_t x)    { return Swizzle(x); }
			inline  int32_t  ToLittleEndian( int32_t x)    { return Swizzle(x); }
			inline uint64_t  ToLittleEndian(uint64_t x)    { return Swizzle(x); }
			inline  int64_t  ToLittleEndian( int64_t x)    { return Swizzle(x); }
			inline uint128_t ToLittleEndian(uint128_t x)   { return Swizzle(x); }
			inline  int128_t ToLittleEndian( int128_t x)   { return Swizzle(x); }
			inline void      ToLittleEndian(float* x)      { Swizzle(x); }
			inline void      ToLittleEndian(double* x)     { Swizzle(x); }

			inline uint16_t  FromLittleEndian(uint16_t x)  { return Swizzle(x); }
			inline  int16_t  FromLittleEndian( int16_t x)  { return Swizzle(x); }
			inline uint32_t  FromLittleEndian(uint32_t x)  { return Swizzle(x); }
			inline  int32_t  FromLittleEndian( int32_t x)  { return Swizzle(x); }
			inline uint64_t  FromLittleEndian(uint64_t x)  { return Swizzle(x); }
			inline  int64_t  FromLittleEndian (int64_t x)  { return Swizzle(x); }
			inline uint128_t FromLittleEndian(uint128_t x) { return Swizzle(x); }
			inline  int128_t FromLittleEndian( int128_t x) { return Swizzle(x); }
			inline void      FromLittleEndian(float* x)    { Swizzle(x); }
			inline void      FromLittleEndian(double* x)   { Swizzle(x); }

		#elif defined(EA_SYSTEM_LITTLE_ENDIAN)

			inline uint16_t  ToBigEndian(uint16_t x)       { return Swizzle(x); }
			inline  int16_t  ToBigEndian( int16_t x)       { return Swizzle(x); }
			inline uint32_t  ToBigEndian(uint32_t x)       { return Swizzle(x); }
			inline  int32_t  ToBigEndian( int32_t x)       { return Swizzle(x); }
			inline uint64_t  ToBigEndian(uint64_t x)       { return Swizzle(x); }
			inline  int64_t  ToBigEndian( int64_t x)       { return Swizzle(x); }
			inline uint128_t ToBigEndian(uint128_t x)      { return Swizzle(x); }
			inline  int128_t ToBigEndian( int128_t x)      { return Swizzle(x); }
			inline void      ToBigEndian(float* x)         { Swizzle(x); }
			inline void      ToBigEndian(double* x)        { Swizzle(x); }

			inline uint16_t  FromBigEndian(uint16_t x)     { return Swizzle(x); }
			inline  int16_t  FromBigEndian( int16_t x)     { return Swizzle(x); }
			inline uint32_t  FromBigEndian(uint32_t x)     { return Swizzle(x); }
			inline  int32_t  FromBigEndian( int32_t x)     { return Swizzle(x); }
			inline uint64_t  FromBigEndian(uint64_t x)     { return Swizzle(x); }
			inline  int64_t  FromBigEndian( int64_t x)     { return Swizzle(x); }
			inline uint128_t FromBigEndian(uint128_t x)    { return Swizzle(x); }
			inline  int128_t FromBigEndian( int128_t x)    { return Swizzle(x); }
			inline void      FromBigEndian(float* x)       { Swizzle(x); }
			inline void      FromBigEndian(double* x)      { Swizzle(x); }

			inline uint16_t  ToLittleEndian(uint16_t x)    { return x; }
			inline  int16_t  ToLittleEndian( int16_t x)    { return x; }
			inline uint32_t  ToLittleEndian(uint32_t x)    { return x; }
			inline  int32_t  ToLittleEndian( int32_t x)    { return x; }
			inline uint64_t  ToLittleEndian(uint64_t x)    { return x; }
			inline  int64_t  ToLittleEndian( int64_t x)    { return x; }
			inline uint128_t ToLittleEndian(uint128_t x)   { return x; }
			inline  int128_t ToLittleEndian( int128_t x)   { return x; }
			inline void      ToLittleEndian(float*)        { }
			inline void      ToLittleEndian(double*)       { }

			inline uint16_t  FromLittleEndian(uint16_t x)  { return x; }
			inline  int16_t  FromLittleEndian( int16_t x)  { return x; }
			inline uint32_t  FromLittleEndian(uint32_t x)  { return x; }
			inline  int32_t  FromLittleEndian( int32_t x)  { return x; }
			inline uint64_t  FromLittleEndian(uint64_t x)  { return x; }
			inline  int64_t  FromLittleEndian( int64_t x)  { return x; }
			inline uint128_t FromLittleEndian(uint128_t x) { return x; }
			inline  int128_t FromLittleEndian( int128_t x) { return x; }
			inline void      FromLittleEndian(float*)      { }
			inline void      FromLittleEndian(double*)     { }

		#endif


		// SwizzleConst
		// Used for swizzling compile-time constants.
		// The implementations are defined in a simplistic way so that 
		// the compiler can optimize away the logic in each function.
		// These functions are intentionally not optimized with tricks such 
		// as assembly language, compiler intrinsics, or memory tricks, as
		// such things would interfere with the compiler's ability to optimize
		// away these operations. All of this functions should compile away
		// when used with compile-time constants.
		EA_FORCE_INLINE uint16_t SwizzleConst(uint16_t x)
		{
			return (uint16_t) ((x >> 8) | (x << 8));
		}
		EA_FORCE_INLINE int16_t SwizzleConst(int16_t x)
			{ return (int16_t)SwizzleConst((uint16_t)x); }

		EA_FORCE_INLINE uint32_t SwizzleConst(uint32_t x)
		{
			return (uint32_t)
				((x >> 24)               |
				((x << 24) & 0xff000000) |
				((x <<  8) & 0x00ff0000) |
				((x >>  8) & 0x0000ff00)); 
		}
		EA_FORCE_INLINE int32_t SwizzleConst(int32_t x)
			{ return (int32_t)SwizzleConst((uint32_t)x); }

		EA_FORCE_INLINE uint64_t SwizzleConst(uint64_t x)
		{
			const uint32_t high32Bits = Swizzle((uint32_t)(x));
			const uint32_t low32Bits  = Swizzle((uint32_t)(x >> 32));

			return ((uint64_t)high32Bits << 32) | low32Bits;
		}
		EA_FORCE_INLINE int64_t SwizzleConst(int64_t x)
			{ return (int64_t)SwizzleConst((uint64_t)x); }

		inline uint128_t SwizzleConst(uint128_t x)
		{
			const uint64_t high64Bits = SwizzleConst(x.AsUint64());
			const uint64_t low64Bits  = SwizzleConst((x >> 64).AsUint64());

			return ((uint128_t)high64Bits << 64) | low64Bits;
		}
		inline int128_t SwizzleConst(int128_t x)
		{
			const uint64_t high64Bits = SwizzleConst(x.AsUint64());
			const uint64_t low64Bits  = SwizzleConst((x >> 64).AsUint64());

			return ((int128_t)high64Bits << 64) | low64Bits;
		}


		#if defined(EA_SYSTEM_BIG_ENDIAN)
			inline uint16_t  ToBigEndianConst(uint16_t x)         { return x; }
			inline  int16_t  ToBigEndianConst( int16_t x)         { return x; }
			inline uint32_t  ToBigEndianConst(uint32_t x)         { return x; }
			inline  int32_t  ToBigEndianConst( int32_t x)         { return x; }
			inline uint64_t  ToBigEndianConst(uint64_t x)         { return x; }
			inline  int64_t  ToBigEndianConst( int64_t x)         { return x; }
			inline uint128_t ToBigEndianConst(uint128_t x)        { return x; }
			inline  int128_t ToBigEndianConst( int128_t x)        { return x; }

			inline uint16_t  FromBigEndianConst(uint16_t x)       { return x; }
			inline uint32_t  FromBigEndianConst(uint32_t x)       { return x; }
			inline uint64_t  FromBigEndianConst(uint64_t x)       { return x; }
			inline uint128_t FromBigEndianConst(uint128_t x)      { return x; }
			inline  int16_t  FromBigEndianConst( int16_t x)       { return x; }
			inline  int32_t  FromBigEndianConst( int32_t x)       { return x; }
			inline  int64_t  FromBigEndianConst( int64_t x)       { return x; }
			inline  int128_t FromBigEndianConst( int128_t x)      { return x; }

			inline uint16_t  ToLittleEndianConst(uint16_t x)      { return SwizzleConst(x); }
			inline  int16_t  ToLittleEndianConst( int16_t x)      { return SwizzleConst(x); }
			inline uint32_t  ToLittleEndianConst(uint32_t x)      { return SwizzleConst(x); }
			inline  int32_t  ToLittleEndianConst( int32_t x)      { return SwizzleConst(x); }
			inline uint64_t  ToLittleEndianConst(uint64_t x)      { return SwizzleConst(x); }
			inline  int64_t  ToLittleEndianConst( int64_t x)      { return SwizzleConst(x); }
			inline uint128_t ToLittleEndianConst(uint128_t x)     { return SwizzleConst(x); }
			inline  int128_t ToLittleEndianConst( int128_t x)     { return SwizzleConst(x); }

			inline uint16_t  FromLittleEndianConst(uint16_t x)    { return SwizzleConst(x); }
			inline  int16_t  FromLittleEndianConst( int16_t x)    { return SwizzleConst(x); }
			inline uint32_t  FromLittleEndianConst(uint32_t x)    { return SwizzleConst(x); }
			inline  int32_t  FromLittleEndianConst( int32_t x)    { return SwizzleConst(x); }
			inline uint64_t  FromLittleEndianConst(uint64_t x)    { return SwizzleConst(x); }
			inline  int64_t  FromLittleEndianConst( int64_t x)    { return SwizzleConst(x); }
			inline uint128_t FromLittleEndianConst(uint128_t x)   { return SwizzleConst(x); }
			inline  int128_t FromLittleEndianConst( int128_t x)   { return SwizzleConst(x); }

		#elif defined(EA_SYSTEM_LITTLE_ENDIAN)

			inline uint16_t  ToBigEndianConst(uint16_t x)         { return SwizzleConst(x); }
			inline  int16_t  ToBigEndianConst( int16_t x)         { return SwizzleConst(x); }
			inline uint32_t  ToBigEndianConst(uint32_t x)         { return SwizzleConst(x); }
			inline  int32_t  ToBigEndianConst( int32_t x)         { return SwizzleConst(x); }
			inline uint64_t  ToBigEndianConst(uint64_t x)         { return SwizzleConst(x); }
			inline  int64_t  ToBigEndianConst( int64_t x)         { return SwizzleConst(x); }
			inline uint128_t ToBigEndianConst(uint128_t x)        { return SwizzleConst(x); }
			inline  int128_t ToBigEndianConst( int128_t x)        { return SwizzleConst(x); }

			inline uint16_t  FromBigEndianConst(uint16_t x)       { return SwizzleConst(x); }
			inline  int16_t  FromBigEndianConst( int16_t x)       { return SwizzleConst(x); }
			inline uint32_t  FromBigEndianConst(uint32_t x)       { return SwizzleConst(x); }
			inline  int32_t  FromBigEndianConst( int32_t x)       { return SwizzleConst(x); }
			inline uint64_t  FromBigEndianConst(uint64_t x)       { return SwizzleConst(x); }
			inline  int64_t  FromBigEndianConst( int64_t x)       { return SwizzleConst(x); }
			inline uint128_t FromBigEndianConst(uint128_t x)      { return SwizzleConst(x); }
			inline  int128_t FromBigEndianConst( int128_t x)      { return SwizzleConst(x); }

			inline uint16_t  ToLittleEndianConst(uint16_t x)      { return x; }
			inline  int16_t  ToLittleEndianConst( int16_t x)      { return x; }
			inline uint32_t  ToLittleEndianConst(uint32_t x)      { return x; }
			inline  int32_t  ToLittleEndianConst( int32_t x)      { return x; }
			inline uint64_t  ToLittleEndianConst(uint64_t x)      { return x; }
			inline  int64_t  ToLittleEndianConst( int64_t x)      { return x; }
			inline uint128_t ToLittleEndianConst(uint128_t x)     { return x; }
			inline  int128_t ToLittleEndianConst( int128_t x)     { return x; }

			inline uint16_t  FromLittleEndianConst(uint16_t x)    { return x; }
			inline  int16_t  FromLittleEndianConst( int16_t x)    { return x; }
			inline uint32_t  FromLittleEndianConst(uint32_t x)    { return x; }
			inline  int32_t  FromLittleEndianConst( int32_t x)    { return x; }
			inline uint64_t  FromLittleEndianConst(uint64_t x)    { return x; }
			inline  int64_t  FromLittleEndianConst( int64_t x)    { return x; }
			inline uint128_t FromLittleEndianConst(uint128_t x)   { return x; }
			inline  int128_t FromLittleEndianConst( int128_t x)   { return x; }

		#endif


		#undef EASTDC_LITTLE_ENDIAN_WITH_BSWAP

	} // namespace StdC

} // namespace EA


#endif // Header include guard




