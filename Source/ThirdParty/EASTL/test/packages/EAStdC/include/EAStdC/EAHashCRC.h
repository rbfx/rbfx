///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Defines the following:
//    uint16_t CRC16(const void* pData, size_t nLength, uint16_t nInitialValue = kCRC16InitialValue, bool bFinalize = true);
//    uint32_t CRC24(const void* pData, size_t nLength, uint32_t nInitialValue = kCRC24InitialValue, bool bFinalize = true);
//    uint32_t CRC32(const void* pData, size_t nLength, uint32_t nInitialValue = kCRC32InitialValue, bool bFinalize = true);
//    uint64_t CRC64(const void* pData, size_t nLength, uint64_t nInitialValue = kCRC64InitialValue, bool bFinalize = true);
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EAHASHCRC_H
#define EASTDC_EAHASHCRC_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
EA_RESTORE_ALL_VC_WARNINGS()


namespace EA
{
namespace StdC
{

	/// CRC16 (Cyclical Redundancy Code)
	///
	/// Classification:
	///    Binary hash
	///    CRC16 is not a cryptographic-level hash.
	///
	/// Description:
	///    This is a well-known standardized hash which does a decent
	///    job of generating non-colliding hash values.
	///
	/// You can use this function as a one-shot CRC or iteratively 
	/// when your data is not contiguous. In the case of iterative 
	/// calculation, you must set bFinalize to false for all but the 
	/// final iteration. If you don't know that an iteration is the 
	/// final iteration until after you have called this function, 
	/// then you merely need to do this to 'finalize' the crc value:
	///    crc = ~crc;
	///
	/// Example usage (single call):
	///     uint16_t crc = CRC16(pSomeData, nSomeDataLength);
	///
	/// Example usage (multiple successive calls):
	///     char     pDataArray[32][1024];
	///     uint16_t crc = kCRC16InitialValue;
	///
	///     for(int i = 0; i < 32; i++) // Calculate the cumulative CRC for the data array.
	///         crc = CRC16(pDataArray[i], 1024, crc, i == 31);
	///
	const uint16_t kCRC16InitialValue = 0xffff;

	EASTDC_API uint16_t CRC16(const void* pData, size_t nLength, uint16_t nInitialValue = kCRC16InitialValue, bool bFinalize = true);



	/// CRC24 (Cyclical Redundancy Code)
	///
	/// Classification:
	///    Binary hash
	///    CRC24 is not a cryptographic-level hash.
	/// Description:
	///    This is a well-known standardized hash which does a decent
	///    job of generating non-colliding hash values.
	///
	/// You can use this function as a one-shot CRC or iteratively 
	/// when your data is not contiguous. In the case of iterative 
	/// calculation, you must set bFinalize to false for all but the 
	/// final iteration. If you don't know that an iteration is the 
	/// final iteration until after you have called this function, 
	/// then you merely need to do this to 'finalize' the crc value:
	///    crc = ~crc;
	///
	/// Example usage (single call):
	///     uint32_t crc = CRC24(pSomeData, nSomeDataLength);
	///
	/// Example usage (multiple successive calls):
	///     char     pDataArray[32][1024];
	///     uint32_t crc = kCRC24InitialValue;
	///
	///     for(int i = 0; i < 32; i++) // Calculate the cumulative CRC for the data array.
	///         crc = CRC24(pDataArray[i], 1024, crc, i == 31);
	///
	const uint32_t kCRC24InitialValue = 0x00b704ce; // This is the conventionally used initial value.

	EASTDC_API uint32_t CRC24(const void* pData, size_t nLength, uint32_t nInitialValue = kCRC24InitialValue, bool bFinalize = true);



	/// CRC32 (Cyclical Redundancy Code)
	///
	/// Classification:
	///    Binary hash
	///    CRC32 is not a cryptographic-level hash.
	///
	/// Description:
	///    This is a well-known standardized hash which does a decent
	///    job of generating non-colliding hash values.
	///
	/// You can use this function as a one-shot CRC or iteratively 
	/// when your data is not contiguous. In the case of iterative 
	/// calculation, you must set bFinalize to false for all but the 
	/// final iteration. If you don't know that an iteration is the 
	/// final iteration until after you have called this function, 
	/// then you merely need to do this to 'finalize' the crc value:
	///    crc = ~crc;
	///
	/// Example usage (single call):
	///     uint32_t crc = CRC32(pSomeData, nSomeDataLength);
	///
	/// Example usage (multiple successive calls):
	///     char     pDataArray[32][1024];
	///     uint32_t crc = kCRC32InitialValue;
	///
	///     for(int i = 0; i < 32; i++) // Calculate the cumulative CRC for the data array.
	///         crc = CRC32(pDataArray[i], 1024, crc, i == 31);
	///
	const uint32_t kCRC32InitialValue = 0xffffffff;

	EASTDC_API uint32_t CRC32(const void* pData, size_t nLength, uint32_t nInitialValue = kCRC32InitialValue, bool bFinalize = true);


	/// CRC32Reverse
	///
	/// This implements reverse CRC32 as used by some software.
	/// See http://en.wikipedia.org/wiki/Cyclic_redundancy_check, for example.
	///
	EASTDC_API uint32_t CRC32Reverse(const void* pData, size_t nLength, uint32_t nInitialValue = kCRC32InitialValue, bool bFinalize = true);


	/// CRC32Rwstdc
	/// Provided for compatibility with rwstdc. This version acts identically to the CRC32 
	/// from the rwstdc package. Users are advised to switch to the regular CRC32 function,
	/// as it follows the CRC standard and thus is portable. 
	EASTDC_API uint32_t CRC32Rwstdc(const void* pData, size_t nLength);



	/// CRC64 (Cyclical Redundancy Code)
	///
	/// Classification:
	///    Binary hash
	///    CRC64 is not a cryptographic-level hash.
	///
	/// Description:
	///    This is a well-known standardized hash which does a decent
	///    job of generating non-colliding hash values.
	///
	/// You can use this function as a one-shot CRC or iteratively 
	/// when your data is not contiguous. In the case of iterative 
	/// calculation, you must set bFinalize to false for all but the 
	/// final iteration. If you don't know that an iteration is the 
	/// final iteration until after you have called this function, 
	/// then you merely need to do this to 'finalize' the crc value:
	///    crc ^= UINT64_C(0xffffffffffffffff);
	///
	/// Example usage (single call):
	///     uint64_t crc = CRC64(pSomeData, nSomeDataLength);
	///
	/// Example usage (multiple successive calls):
	///     char     pDataArray[32][1024];
	///     uint64_t crc = kCRC64InitialValue;
	///
	///     for(int i = 0; i < 32; i++) // Calculate the cumulative CRC for the data array.
	///         crc = CRC64(pDataArray[i], 1024, crc, i == 31);
	///
	const uint64_t kCRC64InitialValue = UINT64_C(0xffffffffffffffff);

	EASTDC_API uint64_t CRC64(const void* pData, size_t nLength, uint64_t nInitialValue = kCRC64InitialValue, bool bFinalize = true);


} // namespace StdC
} // namespace EA


#endif // Header include guard














