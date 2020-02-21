///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a basic set of random number generators suitable for game
// development usage.
/////////////////////////////////////////////////////////////////////////////////////


#include <EAStdC/internal/Config.h>
#include <EAStdC/EARandom.h>
#include <EAStdC/EARandomDistribution.h>
#include <EAStdC/EAStopwatch.h>
#include <string.h>
#include <EAAssert/eaassert.h>


#if defined(EASTDC_EASTOPWATCH_H)   
	#define EARandomGetCPUCycle EA::StdC::Stopwatch::GetCPUCycle
#elif EASTDC_TIME_H_AVAILABLE
	#include <time.h>

	static inline uint64_t EARandomGetCPUCycle()
	{
		return (uint64_t)clock();
	}
#else
	#error Must define a way to get some pseudorandom bits with EARandomGetCPUCycle.
#endif



namespace EA
{
namespace StdC
{

namespace Internal
{
	// this constant is designed to produce a maximum double value that when cast to a float does not fall outside the
	// valid range of [0..1).
	static const float_t RAND_FLOAT_MAX = 1.0f - 1.0f / 1048576.0f; // 1 - 2^-20
}

EASTDC_API void GetRandomSeed(void* pSeed, size_t nLength)
{
	// We get a 64 bit value to work with and copy it repeatedly into 
	// the bytes of the seed.
	const uint64_t nSeed64 = EARandomGetCPUCycle();

	for(size_t i = 0; i < nLength; i++)
		((unsigned char*)pSeed)[i] = (unsigned char)(nSeed64 >> ((i % sizeof(uint64_t)) * sizeof(uint64_t)));
}




///////////////////////////////////////////////////////////////////////////////
// RandomLinearCongruential
///////////////////////////////////////////////////////////////////////////////

void RandomLinearCongruential::SetSeed(uint32_t nSeed)
{
	if(nSeed == 0xffffffff)
		nSeed = (uint32_t)(EARandomGetCPUCycle() & 0xffffffff);
	else if(nSeed == 0)     // Test for seed == 0 because that's an illegal value for us.
		nSeed = 0xaaaaaaaa; // Convert it to some other constant. The actual value of the constant doesn't matter much.

	mnSeed = nSeed;
}


uint32_t RandomLinearCongruential::RandomUint32Uniform(uint32_t nLimit)
{
	return EA::StdC::RandomLimit(*this, nLimit);
}


double RandomLinearCongruential::RandomDoubleUniform()
{
	// All powers of two (such as this) are exact in floating point
	static const double kDoubleUniformScaleFactor = 2.32830643653870e-10f; // = (1 / 4294967296)


	// Unsigned conversions to float are often slow in due to store-to-load 
	// mismatch stalls (well, at least on some architectures), so we do a 
	// signed conversion.
	int32_t randInt = int32_t(RandomUint32Uniform());
	double dResult = (kDoubleUniformScaleFactor * randInt) + 0.5;

	if(dResult > Internal::RAND_FLOAT_MAX)  // Due to precision issues, we need to clamp this.
		dResult = Internal::RAND_FLOAT_MAX;

	return dResult;
}



///////////////////////////////////////////////////////////////////////////////
// RandomTaus
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// P. L'Ecuyer, "Maximally Equidistributed Combined Tausworthe Generators",
// Mathematics of Computation, 65, 213 (1996), 203-213.
// 
// This generator has a period of approximately 2^88. This should be
// preferred over simple linear congruential generators which fail to
// produce uniformly distributed k-tuplets of numbers.
//
// Approved for EA use by EA Legal: 
// http://easites.ea.com/legal/Lists/OpenSourceDealSheet/DispForm.aspx?ID=589
///////////////////////////////////////////////////////////////////////////////

const uint32_t kTausSeed0 = UINT32_C(3719485138);
const uint32_t kTausSeed1 = UINT32_C(840184915);
const uint32_t kTausSeed2 = UINT32_C(2586639250);

uint32_t RandomTaus::GetSeed() const
{
	return (mState[0] ^ kTausSeed0);
}

void RandomTaus::SetSeed(uint32_t nSeed)
{
	if(nSeed == 0xffffffff)
		nSeed = (uint32_t)(EARandomGetCPUCycle() & 0xffffffff);

	const uint32_t newState[3] = { kTausSeed0 ^ nSeed, kTausSeed1 ^ nSeed, kTausSeed2 ^ nSeed };
	SetSeed(newState);
}

void RandomTaus::SetSeed(const uint32_t* pSeedArray)
{
	if(pSeedArray)
	{
		mState[0] = pSeedArray[0];
		mState[1] = pSeedArray[1];
		mState[2] = pSeedArray[2];

		if (mState[0] < 2)
			mState[0] += kTausSeed0; // bad seed -- fix it

		if (mState[1] < 8)
			mState[1] += kTausSeed1; // bad seed -- fix it

		if (mState[2] < 16)
			mState[2] += kTausSeed2; // bad seed -- fix it
	}
	else
		SetSeed(0xffffffff); // Set seed automatically.
}

uint32_t RandomTaus::RandomUint32Uniform()
{
	mState[0] = ((mState[0] & 0xfffffffe) << 12) ^ (((mState[0] << 13) ^ mState[0]) >> 19);
	mState[1] = ((mState[1] & 0xfffffff8) <<  4) ^ (((mState[1] <<  2) ^ mState[1]) >> 25);
	mState[2] = ((mState[2] & 0xfffffff0) << 17) ^ (((mState[2] <<  3) ^ mState[2]) >> 11);

	return (mState[0] ^ mState[1] ^ mState[2]);
}

uint32_t RandomTaus::RandomUint32Uniform(uint32_t nLimit)
{
	return EA::StdC::RandomLimit(*this, nLimit);
}

double RandomTaus::RandomDoubleUniform()
{
	const uint32_t nRandNoLimit = RandomUint32Uniform();

	// All powers of two (such as this) are exact in floating point
	static const float kDoubleUniformScaleFactor = 2.32830643653870e-10f;

	// Unsigned conversions to float are often slow in due to store-to-load 
	// mismatch stalls (well, at least on some architectures), so we do a 
	// signed conversion.
	double dResult = (kDoubleUniformScaleFactor * (int32_t)nRandNoLimit) + 0.5;

	if(dResult > Internal::RAND_FLOAT_MAX)  // Due to precision issues, we need to clamp this.
		dResult = Internal::RAND_FLOAT_MAX;

	return dResult;
}


double RandomTaus::RandomDoubleUniform(double limit)
{
	EA_ASSERT(limit > 0);

	const uint32_t nRandNoLimit = RandomUint32Uniform();

	// All powers of two (such as this) are exact in floating point
	static const float kDoubleUniformScaleFactor = 2.32830643653870e-10f;

	// Unsigned conversions to float are often slow in due to store-to-load 
	// mismatch stalls (well, at least on some architectures), so we do a 
	// signed conversion.
	double dResult = (kDoubleUniformScaleFactor * limit * (int32_t)nRandNoLimit) + 0.5;

	if(dResult > Internal::RAND_FLOAT_MAX)  // Due to precision issues, we need to clamp this.
		dResult = Internal::RAND_FLOAT_MAX;

	return dResult;
}




///////////////////////////////////////////////////////////////////////////////
// RandomMersenneTwister
///////////////////////////////////////////////////////////////////////////////

RandomMersenneTwister::RandomMersenneTwister(uint32_t nSeed)
{
	mpNextState = NULL;
	mnCountRemaining = kStateCount;
	SetSeed(nSeed);
}


RandomMersenneTwister::RandomMersenneTwister(const uint32_t seedArray[], unsigned nSeedArraySize)
{
	mpNextState = NULL;
	mnCountRemaining = kStateCount;
	SetSeed(seedArray, nSeedArraySize);
}


RandomMersenneTwister& RandomMersenneTwister::operator=(const RandomMersenneTwister& randomMT)
{
	::memcpy(mState, randomMT.mState, sizeof(mState));
	mpNextState      = &mState[0] + (randomMT.mpNextState - randomMT.mState);
	mnCountRemaining = randomMT.mnCountRemaining;
	return *this;
}


#define LOCAL_MIN(x, y) (x) < (y) ? (x) : (y)


unsigned RandomMersenneTwister::GetSeed(uint32_t seedArray[], unsigned nSeedArraySize) const
{
	if(nSeedArraySize >= 1)
	{
		// Get mnCountRemaining
		seedArray[0] = (uint32_t)mnCountRemaining;

		// Get mState
		unsigned i, copyCount = LOCAL_MIN((unsigned)kStateCount, nSeedArraySize - 1);

		for(i = 0; i < copyCount; i++)
			seedArray[i + 1] = mState[i];

		for(i = copyCount; i < (nSeedArraySize - 1); i++)
			seedArray[i + 1] = 0;

		return copyCount + 1;
	}

	return 0;
}


void RandomMersenneTwister::SetSeed(const uint32_t seedArray[], unsigned nSeedArraySize)
{
	if(nSeedArraySize >= 1)
	{
		// Set mnCountRemaining
		mnCountRemaining = (int32_t)seedArray[0];
		if(mnCountRemaining > kStateCount)
			mnCountRemaining = kStateCount;

		// Set mpNextState
		mpNextState = mState + (kStateCount - mnCountRemaining);

		// Set mState
		const uint32_t* pStateInput     = seedArray + 1;  // +1 because seedArray[0] stores mnCountRemaining.
		uint32_t*       pStateOutput    = &mState[0];
		uint32_t*       pStateOutputEnd = pStateOutput + kStateCount;

		while(pStateOutput < pStateOutputEnd)
		{
			if(pStateInput >= (seedArray + 1 + nSeedArraySize))
				pStateInput = (seedArray + 1); // Go back to the beginning.

			*pStateOutput++ = *pStateInput++;
		}
	}
}


void RandomMersenneTwister::SetSeed(uint32_t nSeed)
{
	uint32_t* pState = &mState[0];
	int i = kStateCount;

	if(nSeed == 0xffffffff)
		nSeed = (uint32_t)(EARandomGetCPUCycle() & 0xffffffff);

	// Even seeds for the Mersenne Twister are known to be bad,
	// where bad means a non-maximal period and striping.
	nSeed |= 1; 
					
	while(i--)
	{
		*pState  = nSeed & 0xffff0000;
		*pState |= ((nSeed *= 69069)++ & 0xffff0000) >> 16;
		 pState++;
		(nSeed *= 69069)++;
	}
	Reload();
}


uint32_t RandomMersenneTwister::RandomUint32Uniform()
{
	uint32_t nValue;

	if(--mnCountRemaining < 0)
	{
		Reload();
		--mnCountRemaining;
	}

	nValue  = *mpNextState++;
	nValue ^= (nValue >> 11);
	nValue ^= (nValue <<  7) & 0x9d2c5680;
	nValue ^= (nValue << 15) & 0xefc60000;
	return nValue ^ (nValue >> 18);
}


uint32_t RandomMersenneTwister::RandomUint32Uniform(uint32_t nLimit)
{
	return EA::StdC::RandomLimit(*this, nLimit);
}


double RandomMersenneTwister::RandomDoubleUniform()
{
	double dResult = (int32_t)RandomUint32Uniform() * 2.3283064365386963e-10 + 0.5;

	if(dResult > Internal::RAND_FLOAT_MAX)  // Due to precision issues, we need to clamp this.
		dResult = Internal::RAND_FLOAT_MAX;

	return dResult;
}


static inline uint32_t LoBit(uint32_t n)
{
	return (n & 0x00000001);
}


static inline uint32_t MixBits(uint32_t n, uint32_t m)
{
	return ((n & 0x80000000) | (m & 0x7FFFFFFF));
}


void RandomMersenneTwister::Reload()
{
	const     uint32_t kMagicNumber = 0x9908b0df;  // Needs to be used as unsigned.
	const     int kPeriodValue = 397;
	uint32_t *p0 = &mState[0], *p2 = &mState[1], *pM = &mState[kPeriodValue];
	uint32_t  s0 =  mState[0],  s1 =  mState[1];
	int i;

	for(i = kStateCount - kPeriodValue; i--; s0 = s1, s1 = *++p2)
		*p0++ = *pM++ ^ (MixBits(s0, s1) >> 1) ^ (LoBit(s1) ? kMagicNumber : 0);

	for(pM = &mState[0], i = kPeriodValue; --i; s0 = s1, s1 = *++p2 )
		*p0++ = *pM++ ^ (MixBits(s0, s1) >> 1) ^ (LoBit(s1) ? kMagicNumber : 0);

	s1 = mState[0];
	*p0 = *pM ^ (MixBits(s0, s1) >> 1) ^ (LoBit(s1) ? kMagicNumber : 0);
	
	mnCountRemaining = kStateCount;
	mpNextState = &mState[0];
}


uint32_t RandomMersenneTwister::Hash(int t, int c)
{
	static uint32_t nIncrementor = 0;

	uint32_t       h1 = 0;
	uint32_t       h2 = 0;
	unsigned char* p = (unsigned char*)&t;
	unsigned       i;

	for(i=0; i < sizeof(t); ++i)
	{
		h1 *= UINT8_MAX + 2;
		h1 += p[i];
	}

	p = (unsigned char*)&c;

	for(i=0; i < sizeof(c); ++i)
	{
		h2 *= UINT8_MAX + 2;
		h2 += p[i];
	}

	return (h1 + nIncrementor++) ^ h2;
}


// For unity build friendliness, undef all local #defines.
#undef EARandomGetCPUCycle
#undef LOCAL_MIN


} // namespace StdC
} // namespace EA












