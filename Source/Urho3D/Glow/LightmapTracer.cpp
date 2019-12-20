//
// Copyright (c) 2008-2019 the Urho3D project.
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

// Embree includes must be first
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>
#define _SSIZE_T_DEFINED

#include "../Glow/EmbreeScene.h"
#include "../Glow/LightmapTracer.h"

#include <future>

namespace Urho3D
{

namespace
{

/// Parallel for loop.
template <class T>
void ParallelFor(unsigned count, unsigned numThreads, const T& callback)
{
    // Post async tasks
    ea::vector<std::future<void>> tasks;
    const unsigned chunkSize = (count + numThreads - 1) / numThreads;
    for (unsigned i = 0; i < numThreads; ++i)
    {
        tasks.push_back(std::async([&, i]()
        {
            const unsigned fromIndex = i * chunkSize;
            const unsigned toIndex = ea::min(fromIndex + chunkSize, count);
            callback(fromIndex, toIndex);
        }));
    }

    // Wait for async tasks
    for (auto& task : tasks)
        task.wait();
}

/// Array of pre-generated random directions.
const Vector3 randomDirections[16] =
{
    { 0.30543888085776905f, 0.16673583915989262f, -0.9375f },
    { -0.17489098099486633f, 0.8820469912463017f, 0.4374999999999999f },
    { -0.522789582031234f, 0.2579434103046781f, 0.8125f },
    { 0.7656372099622851f, -0.5622663183235928f, 0.31249999999999994f },
    { -0.5659813812606815f, 0.13967399924914875f, -0.8124999999999999f },
    { 0.6972420064972874f, 0.20297126490130715f, 0.6875f },
    { -0.9765201651685348f, -0.10607599643282929f, 0.18749999999999997f },
    { 0.1261011249359089f, -0.3243335571443361f, 0.9375f },
    { 0.9477945559022632f, -0.0634762144588949f, -0.3125f },
    { 0.6588190400569273f, 0.7497007552740418f, 0.06249999999999993f },
    { -0.4292555116564302f, -0.7066353060190074f, 0.5625f },
    { 0.4023411805801347f, -0.6045372812402751f, -0.6875f },
    { 0.020622927364748606f, -0.9978318720440374f, -0.06250000000000003f },
    { 0.1271591951810903f, 0.8169603962744446f, -0.5624999999999999f },
    { -0.702162735202489f, -0.5617483807657655f, -0.4375f },
    { -0.6783348074811467f, 0.7104263782824479f, -0.18750000000000006f },
};

/// Generate random quaternion.
Quaternion RandomQuaternion()
{
    const float u1 = Random(1.0f);
    const float u2 = Random(360.0f);
    const float u3 = Random(360.0f);

    const float su2 = Sin(u2);
    const float cu2 = Cos(u2);
    const float su3 = Sin(u3);
    const float cu3 = Cos(u3);
    return { Sqrt(1 - u1) * su2, Sqrt(1 - u1) * cu2, Sqrt(u1) * su3, Sqrt(u1) * cu3 };
}

/// Generate pseudo-random vector in hemisphere.
Vector3 PseudoRandomDirectionInHemisphere(const IntVector2& seed, const Quaternion& baseRotation, const Vector3& normal)
{
    const auto sx = static_cast<unsigned>(seed.x_) % 4;
    const auto sy = static_cast<unsigned>(seed.y_) % 4;
    const Vector3 direction = randomDirections[sx + sy * 4];
    const Vector3 rotatedDirection = baseRotation * direction;
    return rotatedDirection.DotProduct(normal) >= 0.0f ? rotatedDirection : -rotatedDirection;
}

/// Generate random direction.
void RandomDirection(Vector3& result)
{
    float len;

    do
    {
        result.x_ = Random(-1.0f, 1.0f);
        result.y_ = Random(-1.0f, 1.0f);
        result.z_ = Random(-1.0f, 1.0f);
        len = result.Length();

    } while (len > 1.0f);

    result /= len;
}

/// Generate random hemisphere direction.
Vector3 RandomHemisphereDirection(const Vector3& normal)
{
    Vector3 result;
    RandomDirection(result);

    if (result.DotProduct(normal) < 0)
        result = -result;

    return result;
}

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

/// Get luminance of given color value.
float GetLuminance(const Vector4& color)
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

}

ea::vector<LightmapChartBakedDirect> InitializeLightmapChartsBakedDirect(const LightmapChartVector& charts)
{
    ea::vector<LightmapChartBakedDirect> chartsBakedDirect;
    for (const LightmapChart& chart : charts)
        chartsBakedDirect.emplace_back(chart.width_, chart.height_);
    return chartsBakedDirect;
}

ea::vector<LightmapChartBakedIndirect> InitializeLightmapChartsBakedIndirect(const LightmapChartVector& charts)
{
    ea::vector<LightmapChartBakedIndirect> chartsBakedIndirect;
    for (const LightmapChart& chart : charts)
        chartsBakedIndirect.emplace_back(chart.width_, chart.height_);
    return chartsBakedIndirect;
}

void BakeDirectionalLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const DirectionalLightParameters& light, const LightmapTracingSettings& settings)
{
    const Vector3 rayDirection = -light.direction_.Normalized();
    const float maxDistance = embreeScene.GetMaxDistance();
    const Vector3 lightColor = light.color_.ToVector3();
    RTCScene scene = embreeScene.GetEmbreeScene();

    ParallelFor(bakedDirect.light_.size(), settings.numThreads_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        rayHit.ray.dir_x = rayDirection.x_;
        rayHit.ray.dir_y = rayDirection.y_;
        rayHit.ray.dir_z = rayDirection.z_;
        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const Vector3 position = geometryBuffer.geometryPositions_[i];
            const Vector3 smoothNormal = geometryBuffer.smoothNormals_[i];
            const unsigned geometryId = geometryBuffer.geometryIds_[i];

            if (!geometryId)
                continue;

            // Cast direct ray
            rayHit.ray.org_x = position.x_ + rayDirection.x_ * settings.rayPositionOffset_;
            rayHit.ray.org_y = position.y_ + rayDirection.y_ * settings.rayPositionOffset_;
            rayHit.ray.org_z = position.z_ + rayDirection.z_ * settings.rayPositionOffset_;
            rayHit.ray.tfar = maxDistance;
            rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rtcIntersect1(scene, &rayContext, &rayHit);

            const float shadowFactor = rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID ? 1.0f : 0.0f;
            const float directLight = ea::max(0.0f, smoothNormal.DotProduct(rayDirection));

            bakedDirect.light_[i] += lightColor * shadowFactor * directLight;
        }
    });
}

void BakeIndirectLight(LightmapChartBakedIndirect& bakedIndirect,
    const ea::vector<LightmapChartBakedDirect>& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    assert(settings.numBounces_ <= LightmapTracingSettings::MaxBounces);

    const Quaternion baseRotation = RandomQuaternion();
    ParallelFor(bakedIndirect.light_.size(), settings.numThreads_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCScene scene = embreeScene.GetEmbreeScene();
        const float maxDistance = embreeScene.GetMaxDistance();
        const auto& geometryIndex = embreeScene.GetEmbreeGeometryIndex();

        Vector3 incomingSamples[LightmapTracingSettings::MaxBounces];
        float incomingFactors[LightmapTracingSettings::MaxBounces];

        RTCRayHit rayHit;
        RTCIntersectContext rayContext;
        rtcInitIntersectContext(&rayContext);

        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = 0xffffffff;
        rayHit.ray.flags = 0xffffffff;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const IntVector2 location = geometryBuffer.IndexToLocation(i);
            const Vector3 position = geometryBuffer.geometryPositions_[i];
            const Vector3 smoothNormal = geometryBuffer.smoothNormals_[i];
            const unsigned geometryId = geometryBuffer.geometryIds_[i];

            if (!geometryId)
                continue;

            int numSamples = 0;
            Vector3 currentPosition = position;
            Vector3 currentNormal = smoothNormal;
            IntVector2 currentSeed = location;

            for (unsigned j = 0; j < settings.numBounces_; ++j)
            {
                // Get new ray direction
                const Vector3 rayDirection = settings.pseudoRandomSampling_
                    ? PseudoRandomDirectionInHemisphere(currentSeed, baseRotation, currentNormal)
                    : RandomHemisphereDirection(currentNormal);

                rayHit.ray.org_x = currentPosition.x_ + currentNormal.x_ * settings.rayPositionOffset_;
                rayHit.ray.org_y = currentPosition.y_ + currentNormal.y_ * settings.rayPositionOffset_;
                rayHit.ray.org_z = currentPosition.z_ + currentNormal.z_ * settings.rayPositionOffset_;
                rayHit.ray.dir_x = rayDirection.x_;
                rayHit.ray.dir_y = rayDirection.y_;
                rayHit.ray.dir_z = rayDirection.z_;
                rayHit.ray.tfar = maxDistance;
                rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIntersect1(scene, &rayContext, &rayHit);

                if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    break;

                // Check normal orientation
                if (rayDirection.DotProduct({ rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z }) > 0.0f)
                    break;

                // Sample lightmap UV
                const EmbreeGeometry& geometry = geometryIndex[rayHit.hit.geomID];
                Vector2 lightmapUV;
                rtcInterpolate0(geometry.embreeGeometry_, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                    RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &lightmapUV.x_, 2);

                // Modify incoming flux
                const float probability = 1 / (2 * M_PI);
                const float cosTheta = rayDirection.DotProduct(currentNormal);
                const float reflectance = 1 / M_PI;
                const float brdf = reflectance / M_PI;

                // TODO: Use real index here
                const unsigned lightmapIndex = geometry.lightmapIndex_;
                const IntVector2 sampleLocation = bakedDirect[lightmapIndex].GetNearestLocation(lightmapUV);
                incomingSamples[j] = bakedDirect[lightmapIndex].GetLight(sampleLocation);
                incomingFactors[j] = brdf * cosTheta / probability;
                ++numSamples;

                // Go to next hemisphere
                if (numSamples < settings.numBounces_)
                {
                    currentPosition.x_ = rayHit.ray.org_x + rayHit.ray.dir_x * rayHit.ray.tfar;
                    currentPosition.y_ = rayHit.ray.org_y + rayHit.ray.dir_y * rayHit.ray.tfar;
                    currentPosition.z_ = rayHit.ray.org_z + rayHit.ray.dir_z * rayHit.ray.tfar;
                    currentNormal = Vector3(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z).Normalized();
                    currentSeed = sampleLocation;
                }
            }

            // Accumulate samples back-to-front
            Vector3 indirectLighting;
            for (int j = numSamples - 1; j >= 0; --j)
            {
                indirectLighting += incomingSamples[j];
                indirectLighting *= incomingFactors[j];
            }

            bakedIndirect.light_[i] += Vector4(indirectLighting, 1);
        }
    });
}

void FilterIndirectLight(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const IndirectFilterParameters& params, unsigned numThreads)
{
    const ea::span<const float> kernelWeights = GetKernel(params.kernelRadius_);
    ParallelFor(bakedIndirect.light_.size(), numThreads,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        for (unsigned index = fromIndex; index < toIndex; ++index)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[index];
            if (!geometryId)
                continue;

            const IntVector2 centerLocation = geometryBuffer.IndexToLocation(index);

            const Vector4 centerColor = bakedIndirect.light_[index];
            const float centerLuminance = GetLuminance(centerColor);
            const Vector3 centerPosition = geometryBuffer.geometryPositions_[index];
            const Vector3 centerNormal = geometryBuffer.smoothNormals_[index];

            float colorWeight = kernelWeights[0] * kernelWeights[0];
            Vector4 colorSum = centerColor * colorWeight;
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

                    const Vector4 otherColor = bakedIndirect.light_[otherIndex];
                    const float weight = CalculateEdgeWeight(centerLuminance, GetLuminance(otherColor), params.luminanceSigma_,
                        centerPosition, geometryBuffer.geometryPositions_[otherIndex], dxdy * params.positionSigma_,
                        centerNormal, geometryBuffer.smoothNormals_[otherIndex], params.normalPower_);

                    colorSum += otherColor * weight * kernel;
                    colorWeight += weight * kernel;
                }
            }

            bakedIndirect.lightSwap_[index] = colorSum / ea::max(M_EPSILON, colorWeight);
        }
    });

    // Swap buffers
    ea::swap(bakedIndirect.light_, bakedIndirect.lightSwap_);
}

}
