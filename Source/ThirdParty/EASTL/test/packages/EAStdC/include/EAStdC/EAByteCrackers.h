///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements safe "byte crackers" and builders for all primary 
// built-in integral data types. These are particularly useful because working 
// with signed values creates opportunities for mistakes, and the macros present 
// in EAByteCrackers remove such possibilities for error.
///////////////////////////////////////////////////////////////////////////////


#if !defined(EASTDC_EABYTECRACKERS_H) && !defined(FOUNDATION_EABYTECRACKERS_H)
#define EASTDC_EABYTECRACKERS_H
#define FOUNDATION_EABYTECRACKERS_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>


///////////////////////////////////////////////////////////////////////////////
// Conventions:
//    0 (zero) refers to the lowest byte, 1 refers to the second lowest byte, etc. 
//    UINT8_1_FROM_UINT32(0x12345678) for example, returns 0x56.
//
//    b means  8 bit byte
//    w means 16 bit word
//    d means 32 bit dword
//    q means 64 bit quadword
//
// Example usage:
//     uint8_t  u8  = UINT8_0_FROM_UINT16(0x1100);                // Returns 0x00
//     uint8_t  u8  = UINT8_2_FROM_UINT64(0x7766554433221100);    // Returns 0x22
//     uint16_t u16 = UINT16_3_FROM_UINT64(0x7766554433221100);   // Returns 0x7766
//
//     uint16_t u16 = UINT16_FROM_UINT8(0x11, 0x00);              // Returns 0x1100
//     uint32_t u32 = UINT32_FROM_UINT8(0x33, 0x22, 0x11, 0x00);  // Returns 0x33221100
//
///////////////////////////////////////////////////////////////////////////////


#ifndef UINT8_0_FROM_UINT16

	// uint8_t byte manipulators
	#define UINT8_0_FROM_UINT16(w)                             ((uint8_t)(w))                         // Get the right-most byte from a uint16_t. (e.g. 0x1234 returns 0x34)
	#define UINT8_1_FROM_UINT16(w)                             ((uint8_t)((uint16_t)(w) >>  8))       // Get the left-most byte from a uint16_t. (e.g. 0x1234 returns 0x12)

	#define UINT8_0_FROM_UINT32(d)                             ((uint8_t)(d))                         // Get byte 0 from a uint32_t. (e.g. 0x12345678 returns 0x78)
	#define UINT8_1_FROM_UINT32(d)                             ((uint8_t)((uint32_t)(d) >>  8))       // Get byte 1 from a uint32_t. (e.g. 0x12345678 returns 0x56)
	#define UINT8_2_FROM_UINT32(d)                             ((uint8_t)((uint32_t)(d) >> 16))       // Get byte 2 from a uint32_t. (e.g. 0x12345678 returns 0x34)
	#define UINT8_3_FROM_UINT32(d)                             ((uint8_t)((uint32_t)(d) >> 24))       // Get byte 3 from a uint32_t. (e.g. 0x12345678 returns 0x12)

	#define UINT8_0_FROM_UINT64(q)                             ((uint8_t)(q))
	#define UINT8_1_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >>  8))
	#define UINT8_2_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 16))
	#define UINT8_3_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 24))
	#define UINT8_4_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 32))
	#define UINT8_5_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 40))
	#define UINT8_6_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 48))
	#define UINT8_7_FROM_UINT64(q)                             ((uint8_t)((uint64_t)(q) >> 56))


	// uint16_t byte manipulators
	#define UINT16_0_FROM_UINT32(d)                            ((uint16_t)(d))                        // Get the right-most word from a uint32_t. (e.g. 0x12345678 returns 0x5678)
	#define UINT16_1_FROM_UINT32(d)                            ((uint16_t)((uint32_t)(d) >> 16))      // Get the left-most  word from a uint32_t. (e.g. 0x12345678 returns 0x1234)

	#define UINT16_0_FROM_UINT64(q)                            ((uint16_t)(q))
	#define UINT16_1_FROM_UINT64(q)                            ((uint16_t)((uint64_t)(q) >> 16))
	#define UINT16_2_FROM_UINT64(q)                            ((uint16_t)((uint64_t)(q) >> 32))
	#define UINT16_3_FROM_UINT64(q)                            ((uint16_t)((uint64_t)(q) >> 48))

	#define UINT16_FROM_UINT8(b1, b0)                          ((uint16_t)(((uint8_t)(b1) << 8) | (uint8_t)(b0)))


	// uint32_t byte manipulators
	#define UINT32_0_FROM_UINT64(q)                            ((uint32_t)(q))                        // Get the right-most dword from a uint64_t. (e.g. 0x0000000012345678 returns 0x12345678)
	#define UINT32_1_FROM_UINT64(q)                            ((uint32_t)((uint64_t)(q) >> 32))      // Get the left-most  dword from a uint64_t. (e.g. 0x1234567800000000 returns 0x12345678)

	#define UINT32_FROM_UINT8(b3, b2, b1, b0)                  ((uint32_t)(((uint8_t)(b3) << 24) | ((uint8_t)(b2) << 16) | ((uint8_t)(b1) << 8) | (uint8_t)(b0))) //Build a uint32_t from four Uint8s
	#define UINT32_FROM_UINT16(w1, w0)                         ((uint32_t)(((uint16_t)(w1) << 16) | (uint16_t)(w0))) // Build a uint32_t from two Uint16s


	// uint64_t byte manipulators
	#define UINT64_FROM_UINT8(b7, b6, b5, b4, b3, b2, b1, b0)  ((uint64_t)(((uint64_t)((uint8_t)(b7)) << 56) | ((uint64_t)((uint8_t)(b6)) << 48) | ((uint64_t)((uint8_t)(b5)) << 40) | ((uint64_t)((uint8_t)(b4)) << 32) | ((uint64_t)((uint8_t)(b3)) << 24) | ((uint64_t)((uint8_t)(b2)) << 16) | ((uint64_t)((uint8_t)(b1)) << 8) | (uint64_t)((uint8_t)(b0))))
	#define UINT64_FROM_UINT16(w3, w2, w1, w0)                 ((uint64_t)(((uint64_t)((uint16_t)(w3)) << 48) | ((uint64_t)((uint16_t)(w2)) << 32) | ((uint64_t)((uint16_t)(w1)) << 16) | (uint64_t)((uint16_t)(w0))))
	#define UINT64_FROM_UINT32(d1, d0)                         ((uint64_t)(((uint64_t)((uint32_t)(d1)) << 32) | (uint64_t)((uint32_t)(d0))))

#endif


#endif // Header include guard







