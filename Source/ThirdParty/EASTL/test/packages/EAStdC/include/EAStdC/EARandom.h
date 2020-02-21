///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// This file implements a basic set of random number generators suitable for game
// development usage.
/////////////////////////////////////////////////////////////////////////////////////


#ifndef EASTDC_EARANDOM_H
#define EASTDC_EARANDOM_H


#include <EAStdC/internal/Config.h>
#include <EABase/eabase.h>
EA_DISABLE_ALL_VC_WARNINGS()
#include <stddef.h>
#include <string.h>
EA_RESTORE_ALL_VC_WARNINGS()


namespace EA
{
namespace StdC
{

	/////////////////////////////////////////////////////////////////////////////////////
	/// Introduction
	///
	/// This code includes basic random number generator functionality. It is good for
	/// most game development uses with the exception of cryptographic uses and legally 
	/// controlled gambling mechanisms. For example, cryptographic random number generator 
	/// requirements have additional considerations regarding protection against 
	/// cryptographic attacks.
	///
	/// Purpose
	/// There are many free C/C++ random number packages available today. Many of them 
	/// are rather comprehensive and some are fairly flexible. The design of this package
	/// is based on the needs of game programmers and intentionally avoids some of the 
	/// complications of other packages. This package is designed first and foremost to 
	/// be fast and to offer at least the option of low footprint. Secondly this package
	/// is designed to be very easily understood with a minimum of effort; experience has
	/// shown that game programmers will not use a library unless they can figure it out 
	/// on their own in a matter of a few minutes. This last consideration rules out the 
	/// possibility of using a heavily templated library such as that found in Boost.
	///
	/// Distributions
	/// The generation of random number generation distributions other than linear
	/// distributions is part of an optional layer that sits on top of this core 
	/// generation layer. One implementation can be found in earandom_distribution.h.
	///
	/// Random Generator Misuses
	/// It is worth mentioning that it is surprisingly common for users of random number
	/// generator classes to come to the belief that the generator is broken when in fact
	/// they are misusing the generator. Common misuses of generators include:
	///     - Seeding a generator with the same seed every time it's used.
	///     - Seeding two generators at the same time via the system clock and 
	///        finding that they produce idential values.
	///     - Using 'RandomUint32Uniform() % 5000' instead of 'RandomUint32Uniform(5000)'.
	///     - Inventing flawed distribution generators.
	///     - Misusing the results of a generator but assuming the generator is
	///        yielding bad values.
	///     - Creating a random number generator for a single use right when it 
	///        is needed. This is usually bad because the first generated value is 
	///        no more random than the seed used to generate the number.
	///
	/// Weaknesses of the C Library rand Function
	/// The C library rand function does an OK job as a basic random number generator.
	/// for the purposes of writing applets of various types. However, it is not an 
	/// optimal generic solution. Some reasons include:
	///     - Generates only numbers between 0 and RAND_MAX, which is usually 32767.
	///     - Doesn't generate floating point numbers.
	///     - Doesn't generate random numbers within a prescribed range. Using 
	///        the % operator to rectify this is slow (requires integer division) and 
	///        produces a lopsided distribution unless N is evenly divisble into RAND_MAX.
	///     - Generates rather poor random numbers. In particular, they tend to have 
	///        obvious patterns in the low bits.
	///     - A single instance of a generator is shared with the entire application
	///        and there is no way to make another instance.
	///     - Is slow. Generating a random number via rand() was measured as 70% slower
	///        than the fast generator provided here (Intel Pentium P4 / MSVC++).
	///
	/// How to fill a container or sequence with random uint32_t values:
	///     #include <eastl/algorithm.h>   // or #include <algorithm> to use std STL.
	///     
	///     EA::StdC::Random rand(someSeedValue);                       // We can just use EA::StdC::Random directly because 
	///     eastl::generate(myVector.begin(), myVector.end(), rand);    // it has an operator() that returns uint32_t.
	///
	/// How to randomize (shuffle) an existing container or sequence of uint32_t values:
	///     EA::StdC::Random rand(someSeedValue);
	///     eastl::random_shuffle(myVector.begin(), myVector.end(), rand);
	/// 
	/// How to fill a container or sequence with random double:
	///     struct GenerateDouble { 
	///         EA::StdC::Random mRand;                                 // We need to make a tiny struct that simply 
	///         GenerateDouble(uint32_t seed) : mRand(seed){}           // has an operator() that returns double.
	///         double operator(){ mRand.RandomDoubleUniform(); }
	///     };
	///     GenerateDouble rand(someSeedValue);
	///     eastl::generate(myVector.begin(), myVector.end(), rand);
	/////////////////////////////////////////////////////////////////////////////////////



	/// GetRandomSeed
	///
	/// This type of function is also known as an "entropy collector".
	/// This function generates a pseudorandom number generator seed. As such, this function is 
	/// a random number generator itself. However, this random number generator is likely to be
	/// much slower than a standard pseudorandom number generator on most systems. Some systems
	/// such as Linux support seeding natively via reading from the /dev/random file. Other
	/// systems that don't support that natively implement it by reading sources of randomness
	/// on the system such as the system clock, input device state, processor state, etc.
	EASTDC_API void GetRandomSeed(void* pSeed, size_t nLength);


	// Random number generator prototype
	// For ease of readability, we display a condensed version of a random number generator.
	// Each function is present for a reason. For example, the functions that take a limit
	// argument are provide an efficient and reliable implementation of generator with a
	// limited return value. It is both inefficient and unreliable to generate a random 
	// integer within a range by using the % operator, as is commonly done. 
	//
	// class Random
	// {
	// public:
	//     Random(uint32_t nSeed = 0xffffffff);
	//     Random(const Random& random);
	//     Random& operator=(const Random& random);
	// 
	//     uint32_t GetSeed() const;
	//     void     SetSeed(uint32_t nSeed = 0xffffffff);
	// 
	//     uint32_t operator()(uint32_t nLimit = 0);
	//     uint32_t RandomUint32Uniform();                                                        
	//     uint32_t RandomUint32Uniform(uint32_t nLimit);
	//     double   RandomDoubleUniform();
	//     double   RandomDoubleUniform(double limit);
	// };



	/// RandomLinearCongruential
	///
	/// Implements a random number generator via the linear congruential algorithm.
	/// This algorithm generates good enough pseudorandom numbers for most simulation
	/// uses. Its biggest weakness is that there are some patterns that occur in the 
	/// lower bits. 
	///
	/// This generator optimizes speed and size at the cost of randomness.
	///
	class EASTDC_API RandomLinearCongruential
	{
	public:
		typedef uint32_t result_type;

		/// RandomLinearCongruential
		/// Constructs the random number generator with a given initial state (seed).
		/// If the seed is 0xffffffff (MAX_UINT32), then the seed is generated automatically
		/// by a semi-random internal mechanism such as reading the system clock. Note that 
		/// seeding by this mechanism can yield unexpected poor results if you create multiple
		/// instances of this class within a short period of time, as they will all get the 
		/// same seed due to the system clock having not advanced.
		RandomLinearCongruential(uint32_t nSeed = 0xffffffff);

		/// RandomLinearCongruential
		/// Copy constructor
		RandomLinearCongruential(const RandomLinearCongruential& randomLC);

		/// operator =
		RandomLinearCongruential& operator=(const RandomLinearCongruential& randomLC);

		/// GetSeed
		/// Gets the state of the random number generator, which can be entirely 
		/// defined by a single uint32_t. 
		uint32_t GetSeed() const;

		/// SetSeed
		/// Sets the current state of the random number generator, which can be 
		/// entirely defined by a single uint32_t. If you want random number generation
		/// to appear random, the seeds that you supply must themselves be randomly 
		/// selected. 
		void SetSeed(uint32_t nSeed = 0xffffffff);

		/// operator ()
		/// Generates a random uint32 with an optional limit. Acts the same as the 
		/// RandomUint32Uniform(uint32_t nLimit) function. This function is useful for
		/// some templated uses whereby you want the class to act as a function object.
		/// If the input nLimit is 0, then the return value is from 0 to MAX_UINT32 inclusively.
		uint32_t operator()(uint32_t nLimit = 0);

		/// RandomUint32Uniform
		/// Return value is from 0 to MAX_UINT32 inclusively, with uniform probability.
		/// This is the most basic random integer generator for this class; it has no 
		/// extra options but is also the fastest. Note that if you want a random
		/// integer between 0 and some value, you should use RandomUint32Uniform(nLimit)
		/// and not use RandomUint32Uniform() % nLimit. The former is both faster and 
		/// more random; using % to achieve a truly random distribution fails unless
		/// nLimit is evenly divisible into MAX_UINT32.
		uint32_t RandomUint32Uniform();

		/// RandomUint32Uniform (with limit)
		/// Return value is from 0 to nLimit-1 inclusively, with uniform probability.
		uint32_t RandomUint32Uniform(uint32_t nLimit);

		/// RandomDoubleUniform
		/// Output is in range of [0, 1) with uniform numeric (not bit) distribution.
		double RandomDoubleUniform();

		/// RandomDoubleUniform (with limit)
		/// Output is in range of [0, limit) with uniform numeric (not bit) distribution.
		/// limit is a value > 0.
		double RandomDoubleUniform(double limit);

	protected:
		uint32_t mnSeed;
	};



	/// RandomTaus
	///
	/// P. L'Ecuyer, "Maximally Equidistributed Combined Tausworthe Generators",
	/// Mathematics of Computation, 65, 213 (1996), 203-213.
	///
	/// RandomTaus is slower than the other EARandom generators but has only 12 bytes of 
	/// state data. RandomLinearCongruental has only 4 bytes of data but is not as 
	/// random as RandomTaus. RandomMersenneTwister is more random than RandomTaus but 
	/// has about 2500 bytes of state data. Thus RandomTaus is a tradeoff.
	///
	/// This generator optimizes randomness and and to some degree size at the cost of speed.
	///
	class EASTDC_API RandomTaus
	{
	public:
		typedef uint32_t result_type;

		RandomTaus(uint32_t nSeed = 0xffffffff);
		RandomTaus(const uint32_t* pSeedArray); // Array of 3 uint32_t values.

		RandomTaus(const RandomTaus& randomT);
		RandomTaus& operator=(const RandomTaus& randomT);

		// Single uint32_t version, for compatibility.
		// Use the seed array version for best behavior.
		// Not guaranteed to return the uint32_t set by SetSeed(uint32_t).
		uint32_t GetSeed() const;
		void     SetSeed(uint32_t nSeed = 0xffffffff);

		void GetSeed(uint32_t* pSeedArray) const; // Array of 3 uint32_t values.
		void SetSeed(const uint32_t* pSeedArray); // Array of 3 uint32_t values.

		/// Output is in range of [0, nLimit) with uniform distribution.
		uint32_t operator()(uint32_t nLimit = 0);

		/// Output is in range of [0, UINT32_MAX] with uniform distribution.
		uint32_t RandomUint32Uniform();

		/// Output is in range of [0, nLimit) with uniform distribution.
		uint32_t RandomUint32Uniform(uint32_t nLimit);

		/// Output is in range of [0, 1) with uniform numeric (not bit) distribution.
		double RandomDoubleUniform();

		/// Output is in range of [0, limit) with uniform numeric (not bit) distribution.
		/// limit is a value > 0.
		double RandomDoubleUniform(double limit);

	protected:
		uint32_t mState[3];
	};



	/// \class RandomMersenneTwister
	/// \brief Implements a random number generator via the Mersenne Twister algorithm. 
	///
	/// This algorithm is popular for its very high degree of randomness (period of 2^19937-1
	/// with 623-dimensional equidistribution) while achieving good speed. 
	///
	/// This generator optimizes randomness and to some degree speed at the cost of size.
	///
	/// Algorithm Reference:
	/// M. Matsumoto and T. Nishimura, "Mersenne Twister: A 623-Dimensionally
	/// Equidistributed Uniform Pseudo-Random Number Generator", ACM Transactions 
	/// on Modeling and Computer Simulation, Vol. 8, No. 1, January 1998, pp 3-30.
	/// See http://www.math.keio.ac.jp/~matumoto/emt.html
	///
	/// The Mersenne Twister is an algorithm for generating random numbers. 
	/// It was designed with consideration of the flaws in various other 
	/// generators. It has a period of 2^19937-1 and the order of equidistribution
	/// of 623 dimensions. It is also quite fast; it avoids multiplication and
	/// division.
	/// 
	/// License:
	///     Permission of Commercial Use of Mersenne Twister
	///     We Makoto Matsumoto and Takuji Nishimura decided to 
	///     let MT be used in commercial products freely.
	///
	class EASTDC_API RandomMersenneTwister
	{
	public:
		/// enum kSeedArrayCount
		/// This enum is public because it allows the user to know how much 
		/// data or space to provide for the GetSeed and SetSeed functions.
		enum { kSeedArrayCount = 625 };  // 624 + 1.

		RandomMersenneTwister(uint32_t nSeed = 0xffffffff);
		RandomMersenneTwister(const uint32_t seedArray[], unsigned nSeedArraySize);
		RandomMersenneTwister(const RandomMersenneTwister& randomMT);

		RandomMersenneTwister& operator=(const RandomMersenneTwister& randomMT);

		// GetSeed retrieves the current seed. The nSeedArraySize parameter should 
		// how many values the seedArray holds. Normally it should be kSeedArrayCount values.
		// The return value is the number of items written to the seedArray, which will
		// be min(nSeedArraySize, kSeedArrayCount).
		unsigned GetSeed(uint32_t seedArray[], unsigned nSeedArraySize = kSeedArrayCount) const;

		// Sets the seed to be used for random number generation. Using GetSeed and SetSeed
		// allows for saving the state of the random number generator between application runs.
		// nSeedArraySize must be at least two.
		void SetSeed(const uint32_t seedArray[], unsigned nSeedArraySize = kSeedArrayCount);

		// This is a simple seed specification function. It will work OK for many cases but 
		// doesn't provide the direct mapping of state that the other SetSeed function does.
		// A seed of 0xffffffff results in the generation of a random seed from system information.
		void SetSeed(uint32_t nSeed = 0xffffffff);

		/// Output is in range of [0, nLimit) with uniform distribution.
		uint32_t operator()(uint32_t nLimit = 0);

		/// Output is in range of [0, UINT32_MAX] with uniform distribution.
		uint32_t RandomUint32Uniform();

		/// Output is in range of [0, nLimit) with uniform distribution.                                                  
		uint32_t RandomUint32Uniform(uint32_t nLimit);

		/// Output is in range of [0, 1) with uniform numeric (not bit) distribution.
		double   RandomDoubleUniform();

		/// Output is in range of [0, limit) with uniform numeric (not bit) distribution.
		/// limit is a value > 0.
		double   RandomDoubleUniform(double limit);

	protected:
		void     Reload();
		uint32_t Hash(int t, int c);

	protected:
		enum { kStateCount = 624 };

		uint32_t  mState[kStateCount]; // State data.
		uint32_t* mpNextState;         // Next state data.
		int32_t   mnCountRemaining;    // Count of remaining entries before reloading.
	};



	// Typedefs

	/// class Random
	/// Default random number generator. We use the RandomLinearCongruential 
	/// generator as random because for most uses it is random enough and it 
	/// uses up very little space and is fairly fast.
	typedef RandomLinearCongruential Random;

	/// class RandomSmall
	/// Implements a random number generator with a small footprint. 
	/// The tradeoff is that it is not as highly random as other generators 
	/// and perhaps not as fast as other generators.
	typedef RandomLinearCongruential RandomSmall;

	/// class RandomFast
	/// Implements a random number generator optimized for speed. 
	/// The tradeoff is that it is not as highly random as other generators 
	/// and perhaps not as fast as other generators.
	typedef RandomLinearCongruential RandomFast;

	/// class RandomQuality
	/// Implements a random number generator optimized for high randomness. 
	/// The tradeoff for being highly random is that the space used by the 
	/// class is not small.
	typedef RandomMersenneTwister RandomQuality;


} // namespace StdC
} // namespace EA







///////////////////////////////////////////////////////////////////////////////
// Inlines
///////////////////////////////////////////////////////////////////////////////

namespace EA
{
	namespace StdC
	{
		////////////////////////////////
		// RandomLinearCongruential   //
		////////////////////////////////

		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline RandomLinearCongruential::RandomLinearCongruential(uint32_t nSeed)
		{
			SetSeed(nSeed);
		}


		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline RandomLinearCongruential::RandomLinearCongruential(const RandomLinearCongruential& randomLC)
		{
			SetSeed(randomLC.GetSeed());
		}


		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline RandomLinearCongruential& RandomLinearCongruential::operator=(const RandomLinearCongruential& randomLC)

		{
			SetSeed(randomLC.GetSeed());
			return *this;
		}


		// Inlined because an inlined implementation would be smaller than 
		// a function call, as it is simply a reading of a variable.
		inline uint32_t RandomLinearCongruential::GetSeed() const
		{
			return mnSeed;
		}


		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline uint32_t RandomLinearCongruential::operator()(uint32_t nLimit)
		{
			return RandomUint32Uniform(nLimit);
		}


		inline uint32_t RandomLinearCongruential::RandomUint32Uniform()
		{
			// If mnSeed == 0, then we would have a problem. But in practice you 
			// will never get an mnSeed of zero from an mnSeed of non-zero.
			const uint64_t nResult64 = mnSeed * (uint64_t)1103515245 + 12345;
			mnSeed = (uint32_t)nResult64;
			return (uint32_t)(nResult64 >> 16);
		}


		// Inlined because an inlined implementation would be hardly larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call. Since the only operation beyond the function
		// call is a multiply, we can feel safe that inlining this is a win.
		inline double RandomLinearCongruential::RandomDoubleUniform(double limit)
		{
			// For the time being, we simply return rand * limit. This is 
			// however not an ideal solution because in going from a range
			// of [0,1) to a range of [0,2000) you are expanding the bit
			// information and thus not yielding all possible values 
			// between 0 and 2000. At least you will have a distribution
			// that is largely still uniform between 0 and 2000. Ideally
			// we can find an algorithm that generates more possible bit
			// patterns between 0 and 2000 (for example).
			return RandomDoubleUniform() * limit;
		}



		////////////////////////////////
		// RandomTaus                 //
		////////////////////////////////

		inline RandomTaus::RandomTaus(uint32_t nSeed)
		{
			SetSeed(nSeed);
		}

		inline RandomTaus::RandomTaus(const uint32_t* pSeedArray)
		{
			SetSeed(pSeedArray);
		}

		inline RandomTaus::RandomTaus(const RandomTaus& randomT)
		{
			memcpy(mState, randomT.mState, sizeof(mState));
		}

		inline RandomTaus& RandomTaus::operator=(const RandomTaus& randomT)
		{
			memcpy(mState, randomT.mState, sizeof(mState));
			return *this;
		}

		inline void RandomTaus::GetSeed(uint32_t* pSeedArray) const
		{
			memcpy(pSeedArray, mState, sizeof(mState));
		}

		inline uint32_t RandomTaus::operator()(uint32_t nLimit)
		{
			return RandomUint32Uniform(nLimit);
		}



		////////////////////////////////
		// RandomMersenneTwister      //
		////////////////////////////////

		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline RandomMersenneTwister::RandomMersenneTwister(const RandomMersenneTwister& randomMT)
		{
			operator=(randomMT); 
		}


		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline uint32_t RandomMersenneTwister::operator()(uint32_t nLimit)
		{
			return RandomUint32Uniform(nLimit);
		}


		// Inlined because an inlined implementation would be no larger than 
		// non-inline implementation due to the fact that the code here is but 
		// a single function call.
		inline double RandomMersenneTwister::RandomDoubleUniform(double limit)
		{
			// For the time being, we simply return rand * limit. This is 
			// however not an ideal solution because in going from a range
			// of [0,1) to a range of [0,2000) you are expanding the bit
			// information and thus not yielding all possible values 
			// between 0 and 2000. At least you will have a distribution
			// that is largely still uniform between 0 and 2000. Ideally
			// we can find an algorithm that generates more possible bit
			// patterns between 0 and 2000 (for example).
			return RandomDoubleUniform() * limit;
		}

	} // namespace StdC

} // namespace EA


#endif // Header include guard



















