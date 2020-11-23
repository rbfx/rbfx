//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#pragma once

#include "../Math/BoundingBox.h"
#include "../Math/Quaternion.h"
#include "../Math/Vector2.h"
#include "../Math/Vector3.h"

#include <EASTL/array.h>
#include <EASTL/span.h>
#include <EASTL/string.h>

#include <random>

namespace Urho3D
{

/// Random number generator. Stable across platforms and runs for any given seed.
class URHO3D_API RandomEngine
{
public:
    /// Underlying random engine type.
    using EngineType = std::minstd_rand;
    /// Max number of tries to produce "good" random values.
    static const unsigned MaxIterations = 32;

    /// Construct with random seed.
    RandomEngine();
    /// Construct with seed.
    explicit RandomEngine(unsigned seed);
    /// Construct from state.
    explicit RandomEngine(const ea::string& state);
    /// Return default thread-local random engine.
    static RandomEngine& GetDefaultEngine();

    /// Load state from string.
    void Load(const ea::string& state);
    /// Save state as string.
    ea::string Save() const;

    /// Return random generator range (2147483646).
    static constexpr unsigned MaxRange() { return EngineType::max() - EngineType::min() + 1; }

    /// Return random unsigned integer number in range [0, MaxRange) with uniform distribution.
    unsigned GetUInt() { return engine_() - EngineType::min(); }
    /// Return random unsigned integer number in range [0, range) with uniform distribution. Range should not exceed MaxRange.
    unsigned GetUInt(unsigned range);
    /// Return random unsigned int in range [min, max) with uniform distribution. Range should not exceed MaxRange.
    unsigned GetUInt(unsigned min, unsigned max);

    /// Return random int in range [min, max) with uniform distribution. Range should not exceed MaxRange.
    int GetInt(int min, int max);

    /// Shuffle range in random order.
    template <class RandomIter>
    void Shuffle(RandomIter first, RandomIter last)
    {
        const auto n = last - first;
        for (auto i = n - 1; i > 0; --i)
        {
            using std::swap;
            const unsigned j = GetUInt(static_cast<unsigned>(i + 1));
            swap(first[i], first[j]);
        }
    }

    /// Return random double in range [0, 1] with uniform distribution.
    double GetDouble() { return GetUInt() / static_cast<double>(MaxRange() - 1); }
    /// Return random double in range [min, max] with uniform distribution.
    double GetDouble(double min, double max) { return GetDouble() * (max - min) + min; }

    /// Return random boolean with given probability of returning true.
    bool GetBool(float probability) { return GetDouble() <= probability && probability != 0.0f; }

    /// Return random float in range [0, 1] with uniform distribution.
    float GetFloat() { return static_cast<float>(GetDouble()); }
    /// Return random float in range [min, max] with uniform distribution.
    float GetFloat(float min, float max) { return static_cast<float>(GetDouble(min, max)); }

    /// Return pair of random floats with standard normal distribution.
    ea::pair<float, float> GetStandardNormalFloatPair();
    /// Return random float with standard normal distribution.
    float GetStandardNormalFloat() { return GetStandardNormalFloatPair().first; }

    /// Return random 2D direction (normalized).
    Vector2 GetDirectionVector2();
    /// Return random 3D direction (normalized).
    Vector3 GetDirectionVector3();
    /// Return random quaternion (normalized).
    Quaternion GetQuaternion();
    /// Return random 2D vector in 2D volume.
    Vector2 GetVector2(const Vector2& min, const Vector2& max);
    /// Return random 3D vector in 3D volume.
    Vector3 GetVector3(const Vector3& min, const Vector3& max);
    /// Return random 3D vector in 3D volume.
    Vector3 GetVector3(const BoundingBox& boundingBox) { return GetVector3(boundingBox.min_, boundingBox.max_); }

private:
    /// Return random array of floats with standard normal distribution.
    void GetStandardNormalFloatArray(ea::span<float> array);
    /// Return random N-dimensional direction (normalized).
    void GetDirection(ea::span<float> direction);

    /// Underlying engine.
    EngineType engine_;
};

}
