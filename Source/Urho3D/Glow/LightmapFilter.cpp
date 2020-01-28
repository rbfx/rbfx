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

#include "../Glow/Helpers.h"
#include "../Glow/LightmapFilter.h"
#include "../IO/Log.h"

#include <EASTL/span.h>

namespace Urho3D
{

namespace
{

/// Get Gauss kernel of given radius.
ea::span<const float> GetKernel(int radius)
{
    static const float k0[] = { 1 };
    static const float k1[] = { 0.684538f, 0.157731f };
    static const float k2[] = { 0.38774f, 0.24477f, 0.06136f };
    static const float k3[] = { 0.266346f, 0.215007f, 0.113085f, 0.038735f };
    static const float k4[] = { 0.20236f, 0.179044f, 0.124009f, 0.067234f, 0.028532f };
    static const float k5[] = { 0.163053f, 0.150677f, 0.118904f, 0.080127f, 0.046108f, 0.022657f };

    switch (radius)
    {
    case 0: return k0;
    case 1: return k1;
    case 2: return k2;
    case 3: return k3;
    case 4: return k4;
    case 5: return k5;
    default:
        assert(0);
        return {};
    }
}

/// Get luminance of given color value (for 3D and 4D vectors).
template <class T>
float GetLuminance(const T& color)
{
    return Color{ color.x_, color.y_, color.z_ }.Luma();
}

/// Calculate edge-stopping weight.
float CalculateEdgeWeight(
    float luminance1, float luminance2, float luminanceSigma,
    const Vector3& position1, const Vector3& position2, float positionSigma,
    const Vector3& normal1, const Vector3& normal2, float normalPower)
{
    const float colorWeight = Abs(luminance1 - luminance2) / luminanceSigma;
    const float positionWeight = positionSigma > M_EPSILON ? (position1 - position2).LengthSquared() / positionSigma : 0.0f;
    const float normalWeight = Pow(ea::max(0.0f, normal1.DotProduct(normal2)), normalPower);

    return std::exp(0.0f - colorWeight - positionWeight) * normalWeight;
}

/// Apply Gauss filter edge stopping function to array.
template <class T>
void FilterArray(const ea::vector<T>& input, ea::vector<T>& output,
    const LightmapChartGeometryBuffer& geometryBuffer,
    const EdgeStoppingGaussFilterParameters& params, unsigned numTasks)
{
    const ea::span<const float> kernelWeights = GetKernel(params.kernelRadius_);
    ParallelFor(input.size(), numTasks,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        for (unsigned index = fromIndex; index < toIndex; ++index)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[index];
            if (!geometryId)
            {
                output[index] = {};
                continue;
            }

            const IntVector2 centerLocation = geometryBuffer.IndexToLocation(index);

            const T centerColor = input[index];
            const float centerLuminance = GetLuminance(centerColor);
            const Vector3 centerPosition = geometryBuffer.positions_[index];
            const Vector3 centerNormal = geometryBuffer.smoothNormals_[index];

            float colorWeight = kernelWeights[0] * kernelWeights[0];
            T colorSum = centerColor * colorWeight;
            for (int dy = -params.kernelRadius_; dy <= params.kernelRadius_; ++dy)
            {
                for (int dx = -params.kernelRadius_; dx <= params.kernelRadius_; ++dx)
                {
                    if (dx == 0 && dy == 0)
                        continue;

                    const IntVector2 offset = IntVector2{ dx, dy } * params.upscale_;
                    const IntVector2 otherLocation = centerLocation + offset;
                    if (!geometryBuffer.IsValidLocation(otherLocation))
                        continue;

                    const float dxdy = Vector2{ static_cast<float>(dx), static_cast<float>(dy) }.Length();
                    const float kernel = kernelWeights[Abs(dx)] * kernelWeights[Abs(dy)];

                    const unsigned otherIndex = geometryBuffer.LocationToIndex(otherLocation);
                    const unsigned otherGeometryId = geometryBuffer.geometryIds_[otherIndex];
                    if (!otherGeometryId)
                        continue;

                    const T otherColor = input[otherIndex];
                    const float weight = CalculateEdgeWeight(centerLuminance, GetLuminance(otherColor), params.luminanceSigma_,
                        centerPosition, geometryBuffer.positions_[otherIndex], dxdy * params.positionSigma_,
                        centerNormal, geometryBuffer.smoothNormals_[otherIndex], params.normalPower_);

                    colorSum += otherColor * weight * kernel;
                    colorWeight += weight * kernel;
                }
            }

            output[index] = colorSum / ea::max(M_EPSILON, colorWeight);
        }
    });
}

}

void FilterDirectLight(const LightmapChartBakedDirect& bakedDirect, ea::vector<Vector3>& outputBuffer,
    const LightmapChartGeometryBuffer& geometryBuffer, const EdgeStoppingGaussFilterParameters& params, unsigned numTasks)
{
    FilterArray(bakedDirect.directLight_, outputBuffer, geometryBuffer, params, numTasks);
}

void FilterIndirectLight(const LightmapChartBakedIndirect& bakedIndirect, ea::vector<Vector4>& outputBuffer,
    const LightmapChartGeometryBuffer& geometryBuffer, const EdgeStoppingGaussFilterParameters& params, unsigned numTasks)
{
    FilterArray(bakedIndirect.light_, outputBuffer, geometryBuffer, params, numTasks);
}

}
