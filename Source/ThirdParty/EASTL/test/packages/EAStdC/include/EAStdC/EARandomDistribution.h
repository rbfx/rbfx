///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a basic set of random number generators suitable for game
// development usage.
///////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////////////
// This code includes basic random number generator functionality. It is good for
// most game development uses with the exception of cryptographic uses and legally 
// controlled gambling mechanisms. For example, cryptographic random number generator 
// requirements have additional considerations regarding protection against 
// cryptographic attacks.
//
// There are many free C/C++ random number packages available today. Many of them 
// are rather comprehensive and some are fairly flexible. The design of this package
// is based on the needs of game programmers and intentionally avoids some of the 
// complications of other packages. This package is designed first and foremost to 
// be fast and to offer at least the option of low footprint. Secondly this package
// is designed to be very easily understood with a minimum of effort; experience has
// shown that game programmers will not use a library unless they can figure it out 
// on their own in a matter of a few minutes. This last consideration rules out the 
// possibility of using a heavily templated library such as that found in Boost.
//
// Functions:
//     bool     RandomBool(Random& r);
//     int32_t  Random2(Random& r);
//     int32_t  Random4(Random& r);
//     int32_t  Random8(Random& r);
//     int32_t  Random16(Random& r);
//     int32_t  Random32(Random& r);
//     int32_t  Random64(Random& r);
//     int32_t  Random128(Random& r);
//     int32_t  Random256(Random& r);
//     uint32_t RandomLimit(Random& r, uint32_t nLimit);
//     int32_t  RandomPowerOfTwo(Random& r, unsigned nPowerOfTwo);
//     int32_t  RandomInt32UniformRange(Random& r, int32_t nBegin, int32_t nEnd);
//     double   RandomDoubleUniformRange(Random& r, double begin, double end);
//     uint32_t RandomUint32WeightedChoice(Random& r, uint32_t nLimit, float weights[]);
//     int32_t  RandomInt32GaussianRange(Random& r, int32_t nBegin, int32_t nEnd);
//     Float    RandomFloatGaussianRange(Random& r, Float fBegin, Float fEnd);
//     int32_t  RandomInt32TriangleRange(Random& r, int32_t nBegin, int32_t nEnd);
//     Float    RandomFloatTriangleRange(Random& r, Float fBegin, Float fEnd);
//
/////////////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EARANDOMDISTRIBUTION_H
#define EASTDC_EARANDOMDISTRIBUTION_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
#include <EAAssert/eaassert.h>
#include <EAStdC/EARandom.h>
#include <math.h>


namespace EA
{
namespace StdC
{

	/// RandomBool
	/// Returns true or false.
	template <typename Random>
	bool RandomBool(Random& r)
	{
		return (r.RandomUint32Uniform() & 0x80000000) != 0;
	} 


	/// Random2
	/// Returns a value between 0 and 1, inclusively.
	template <typename Random>
	int32_t Random2(Random& r)
	{  // Don't trust the low bits, as some generators don't have good low bits.
		return (int32_t)(r.RandomUint32Uniform() >> 31);
	} 


	/// Random4
	/// Returns a value between 0 and 3, inclusively.
	template <typename Random>
	int32_t Random4(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 30);
	} 


	/// Random8
	/// Returns a value between 0 and 7, inclusively.
	template <typename Random>
	int32_t Random8(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 29);
	} 


	/// Random16
	/// Returns a value between 0 and 15, inclusively.
	template <typename Random>
	int32_t Random16(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 28);
	} 


	/// Random32
	/// Returns a value between 0 and 31, inclusively.
	template <typename Random>
	int32_t Random32(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 27);
	} 


	/// Random64
	/// Returns a value between 0 and 63, inclusively.
	template <typename Random>
	int32_t Random64(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 26);
	} 


	/// Random128
	/// Returns a value between 0 and 127, inclusively.
	template <typename Random>
	int32_t Random128(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 25);
	} 


	/// Random256
	/// Returns a value between 0 and 255, inclusively.
	template <typename Random>
	int32_t Random256(Random& r)
	{
		return (int32_t)(r.RandomUint32Uniform() >> 24);
	} 


	/// RandomLimit
	/// Return value is from 0 to nLimit-1 inclusively, with uniform probability.
	template <typename Random>
	uint32_t RandomLimit(Random& r, uint32_t nLimit)
	{
		if((nLimit & (nLimit - 1)) == 0)  // If nLimit is an unsigned power of 2...
			 return (uint32_t)((r.RandomUint32Uniform() * (uint64_t)nLimit) >> 32); // Scales the value from [0, MAX_UINT32] to a range of [0, nLimit).

		uint32_t bits, returnValue;
		do
		{
			bits = r.RandomUint32Uniform();
			returnValue = (bits % nLimit);
		}                                                      // Ignore the highest bits of the representation that are remainder bits. 
		while((bits + ((nLimit - 1) - returnValue) < bits));   // This is a faster way of testing that (bits < (0xFFFFFFFF - (0xFFFFFFFF % nLimit))). 
															   // This depends on unsigned integer wraparound occurring.
		return returnValue;
	} 


	/// RandomLimitFastWithBias
	/// Return value is from 0 to nLimit-1 inclusively, but with a small amount of bias towards
	/// some values. This function is not suitable for uses where you absolutely need perfectly
	/// uniform distribution, and probably not suitable for use with limits greater than ~2^20.
	/// However, this function is significantly faster than the RandomLimit function and is useful 
	/// for generating many values quickly where truly uniform results aren't critical.
	/// The bias is roughly one in a billion for smaller numbers, but larger for huge numbers (e.g. numbers > 2^20). 
	/// If you were to call RandomUint32Uniform(0xfffffff0) 2^32 times, you'd find that 16 of the numbers 
	/// would be returned twice, while the rest would be returned once.
	template <typename Random>
	uint32_t RandomLimitFastBiased(Random& r, uint32_t nLimit)
	{
		const uint32_t nRandNoLimit = r.RandomUint32Uniform();
		const uint32_t nReturnValue = (uint32_t)((nRandNoLimit * (uint64_t)nLimit) >> 32);
		return nReturnValue; 
	} 


	/// RandomPowerOfTwo
	/// Returns a value between 0 and 2 ^ nPowerOfTwo - 1, inclusively. 
	/// This is a generalized form of the RandomN set of functions.
	template <typename Random>
	int32_t RandomPowerOfTwo(Random& r, unsigned nPowerOfTwo)
	{
		//assert(nPowerOfTwo <= 32);
		return (int32_t)(r.RandomUint32Uniform() >> (32 - nPowerOfTwo));
	} 


	/// RandomInt32UniformRange
	/// Return value is from nBegin to nEnd-1 inclusively, with uniform probability.
	template <typename Random>
	int32_t RandomInt32UniformRange(Random& r, int32_t nBegin, int32_t nEnd)
	{
		return nBegin + (int32_t)r.RandomUint32Uniform((uint32_t)(nEnd - nBegin));
	}


	/// RandomDoubleUniformRange
	/// Return value is in range of [nBegin, nEnd) with uniform probability.
	template <typename Random>
	double RandomDoubleUniformRange(Random& r, double begin, double end)
	{
		const double result = begin + r.RandomDoubleUniform(end - begin);

		if(result >= end)   // FPU roundoff errors can cause the result to 
			return end;     // go slightly outside the range. We deal with 
		if(result < begin)  // the possibility of this.
			return begin;
		return result;
	}


	/// RandomUint32WeightedChoice
	///
	/// Return value is from 0 to nLimit-1 inclusively, with probabilities proportional to weights.
	/// The input array weights must be of length <nLimit>. These values are used to 
	/// determine the probability of each choice. That is, weight[i] is proportional 
	/// to the probability that this function will return i. Negative values are ignored. 
	/// This function is useful in generating a custom distribution.
	///
	/// Example usage:
	///    const float weights[7] = { 128, 64, 32, 16, 8, 4, 2 };  // Create a logarithmic distribution in the range of [0, 6).
	///
	///    uint32_t x = RandomUint32WeightedChoice(random, 7, weights);
	///
	template <typename Random>
	uint32_t RandomUint32WeightedChoice(Random& r, uint32_t nLimit, const float weights[])
	{
		if(nLimit >= 2)
		{
			float weightSum = 0;

			for(uint32_t i = 0; i < nLimit; ++i)
			{
				const float weight = weights[i];

				if(weight > 0)
					weightSum += weight;
			}

			if(weightSum > 0)
			{
				float value = (float)RandomDoubleUniformRange(r, 0, weightSum);

				// Do a linear search. A binary search would be faster for 
				// cases where the array size is > ~10.
				for(uint32_t j = 0; j < nLimit; ++j)
				{
					const float weight = weights[j];

					if(weight > 0)
					{
						if(value < weight)
							return j;
						value -= weight;
					}
				}
			}
			else
				return r.RandomUint32Uniform(nLimit);
		}

		// Normally we shouldn't get here, but we might due to rounding errors.
		return nLimit - 1;
	}




	/// RandomInt32GaussianRange
	///
	///  Creates an approximation to a normal distribution (a.k.a. "Gaussian", 
	///  "bell-curve") in the range of [nBegin, nEnd).
	///
	///         |                                       
	///         |                ****                   
	///         |              *      *                 
	///         |             *        *                
	///         |            *          *               
	///         |           *            *              
	///         |          *              *             
	///         |         *                *            
	///         |        *                  *           
	///         |    * *                      * *       
	///         ----------------------------------------
	///              |                          |
	///            begin                       end
	///
	template <typename Random>
	int32_t RandomInt32GaussianRange(Random& r, int32_t nBegin, int32_t nEnd)
	{
		const uint32_t t0     = r.RandomUint32Uniform();
		const uint32_t t1     = r.RandomUint32Uniform();
		const uint32_t t2     = r.RandomUint32Uniform();
		const uint32_t t3     = r.RandomUint32Uniform();
		const uint64_t tcubic = ((((uint64_t)t0 + t1) + ((uint64_t)t2 + t3) + 2) >> 2);

		return nBegin + (int32_t)((tcubic * (uint32_t)(nEnd - nBegin)) >> 32);
	}


	/// RandomFloatGaussianRange
	///
	/// Generates a floating point value with an approximated 
	/// Guassian distribution in the range of [fBegin, fEnd).
	///
	/// Example usage:
	///    float x = RandomFloatGaussianRange(random, 0.f, 100.f);
	///
	template <typename Random, typename Float>
	Float RandomFloatGaussianRange(Random& r, Float fBegin, Float fEnd)
	{
		const Float x0 = (Float)r.RandomDoubleUniform();
		const Float x1 = (Float)r.RandomDoubleUniform();
		const Float x2 = (Float)r.RandomDoubleUniform();

		return fBegin + ((fEnd - fBegin) * Float(0.33333333) * (x0 + x1 + x2));
	}



	/// RandomInt32TriangleRange
	///
	/// Creates a "triangle" distribution in the range of [nBegin, nEnd).
	///
	///         |                                     
	///         |                *                    
	///         |                                     
	///         |             *     *                 
	///         |                                     
	///         |          *           *              
	///         |                                     
	///         |       *                 *           
	///         |                                     
	///         |    *                       *        
	///         --------------------------------------
	///              |                       |
	///            begin                    end
	///
	template <typename Random>
	int32_t RandomInt32TriangleRange(Random& r, int32_t nBegin, int32_t nEnd)
	{
		const uint32_t t0   = r.RandomUint32Uniform(); 
		const uint32_t t1   = r.RandomUint32Uniform();
		const uint64_t ttri = (t0 >> 1) + (t1 >> 1) + (t0 & t1 & 1); // triangular from 0...2^31-1

		return nBegin + (int32_t)((ttri * (uint32_t)(nEnd - nBegin)) >> 32);
	}


	/// RandomFloatTriangleRange
	///
	/// Generates a floating point value with a triangular distribution 
	/// in the range of [fBegin, fEnd).
	///
	/// Example usage:
	///    double x = RandomFloatTriangleRange(random, 0.0, 100.0);
	///
	template <typename Random, typename Float>
	Float RandomFloatTriangleRange(Random& r, Float fBegin, Float fEnd)
	{
		const Float u0 = (Float)r.RandomDoubleUniform();
		const Float u1 = (Float)r.RandomDoubleUniform();

		return fBegin + (fEnd - fBegin) * Float(0.5) * (u0 + u1);
	}

	///	Devroye, Luc (1986). "Discrete Univariate Distributions" (PDF).
	///	Non-Uniform Random Variate Generation. New York: Springer-Verlag. p. 505.
	///
	///	Poisson generator based upon the inversion by sequential search.
	///
	///	This Poisson Random generator only works for a mean that is <= 100.
	///
	/// "x" is a uniform distributed float in the range of 0.0f and 1.0f.
	///	"mean" is the expected number of occurrences during a given interval.
	///
	inline int32_t RandomInt32Poisson(float x, float mean)
	{
		EA_ASSERT(x >= 0.0f && x <= 1.0f);
		EA_ASSERT_MSG(mean >= 0.f && mean <= 100.0f, "Poisson random generator only works for means that are <= 100.0f");

		// clamp x value
		if (x < 0.f) x = 0.f;
		else if (x > 1.f) x = 1.f;

		// clamp mean value
		if (mean < 0.f) mean = 0.f;
		else if (mean >= 100.f) mean = 100.f;

		int32_t k = 0;                    // Counter
		const int32_t max_k = 1000;       // k upper limit
		float P = (float)::exp(-mean);    // probability
		float sum = P;                    // cumulant
		if (sum >= x) return 0;           // done already
		for (k = 1; k < max_k; ++k)
		{                                 // Loop over all k:s
			P *= mean / (float)k;         // Calc next prob
			sum += P;                     // Increase cumulant
			if (sum >= x) break;          // Leave loop
		}

		return k;                         // return random number
	}


} // namespace StdC
} // namespace EA



#endif // Header include guard

























