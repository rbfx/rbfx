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

#include "../Glow/Helpers.h"
#include "../Glow/EmbreeScene.h"
#include "../Glow/LightmapTracer.h"
#include "../Graphics/LightProbeGroup.h"
#include "../IO/Log.h"
#include "../Math/TetrahedralMesh.h"

#include <future>

namespace Urho3D
{

namespace
{

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

/// Return true if first geometry is non-primary LOD of another geometry or different LOD of itself.
bool IsUnwantedLod(const EmbreeGeometry& currentGeometry, const EmbreeGeometry& hitGeometry)
{
    const bool hitLod = hitGeometry.lodIndex_ != 0;
    const bool sameGeometry = currentGeometry.objectIndex_ == hitGeometry.objectIndex_
        && currentGeometry.geometryIndex_ == hitGeometry.geometryIndex_;

    const bool hitLodOfAnotherGeometry = !sameGeometry && hitLod;
    const bool hitAnotherLodOfSameGeometry = sameGeometry && hitGeometry.lodIndex_ != currentGeometry.lodIndex_;
    return hitLodOfAnotherGeometry || hitAnotherLodOfSameGeometry;
}

/// Return true if transparent, update incoming light. Used for direct light calculations.
bool IsTransparedForDirect(const EmbreeGeometry& hitGeometry, Vector3& incomingLight)
{
    if (hitGeometry.opaque_)
        return false;

    incomingLight = Lerp(incomingLight, incomingLight * hitGeometry.diffuseColor_, hitGeometry.alpha_);
    return true;
}

/// Return true if transparent. Used for indirect light calculations.
bool IsTransparentForIndirect(const EmbreeGeometry& hitGeometry)
{
    if (hitGeometry.opaque_)
        return false;

    if (hitGeometry.alpha_ < 0.5f)
        return true;

    return false;
}

/// Ray tracing context for geometry buffer preprocessing.
struct GeometryBufferPreprocessContext : public RTCIntersectContext
{
    /// Current geometry.
    const EmbreeGeometry* currentGeometry_{};
    /// Geometry index.
    const ea::vector<EmbreeGeometry>* geometryIndex_{};
};

/// Filter function for geometry buffer preprocessing.
void GeometryBufferPreprocessFilter(const RTCFilterFunctionNArguments* args)
{
    const auto& ctx = *static_cast<const GeometryBufferPreprocessContext*>(args->context);
    const auto& hit = *reinterpret_cast<RTCHit*>(args->hit);
    assert(args->N == 1);

    // Ignore invalid
    if (args->valid[0] == 0)
        return;

    // Ignore all LODs
    const EmbreeGeometry& hitGeometry = (*ctx.geometryIndex_)[hit.geomID];
    if (IsUnwantedLod(*ctx.currentGeometry_, hitGeometry))
        args->valid[0] = 0;
}

/// Ray tracing context for direct light baking for charts.
struct DirectTracingContextForCharts : public RTCIntersectContext
{
    /// Current geometry.
    const EmbreeGeometry* currentGeometry_{};
    /// Geometry index.
    const ea::vector<EmbreeGeometry>* geometryIndex_{};
    /// Incoming light.
    Vector3* incomingLight_{};
};

/// Filter function for direct light baking for charts.
void TracingFilterForChartsDirect(const RTCFilterFunctionNArguments* args)
{
    const auto& ctx = *static_cast<const DirectTracingContextForCharts*>(args->context);
    auto& hit = *reinterpret_cast<RTCHit*>(args->hit);
    assert(args->N == 1);

    // Ignore invalid
    if (args->valid[0] == 0)
        return;

    // Ignore if unwanted LOD
    const EmbreeGeometry& hitGeometry = (*ctx.geometryIndex_)[hit.geomID];
    if (IsUnwantedLod(*ctx.currentGeometry_, hitGeometry))
        args->valid[0] = 0;

    // Accumulate and ignore if transparent
    if (IsTransparedForDirect(hitGeometry, *ctx.incomingLight_))
        args->valid[0] = 0;
}

/// Ray tracing context for indirect light baking.
struct IndirectTracingContext : public RTCIntersectContext
{
    /// Geometry index.
    const ea::vector<EmbreeGeometry>* geometryIndex_{};
};

/// Filter function for indirect light baking.
void TracingFilterIndirect(const RTCFilterFunctionNArguments* args)
{
    const auto& ctx = *static_cast<const IndirectTracingContext*>(args->context);
    auto& hit = *reinterpret_cast<RTCHit*>(args->hit);
    assert(args->N == 1);

    // Ignore invalid
    if (args->valid[0] == 0)
        return;

    // Ignore if transparent
    const EmbreeGeometry& hitGeometry = (*ctx.geometryIndex_)[hit.geomID];
    if (IsTransparentForIndirect(hitGeometry))
        args->valid[0] = 0;
}

/// Indirect light tracing for charts: tracing element.
struct ChartIndirectTracingElement
{
    /// Position.
    Vector3 position_;
    /// Normal of actual geometry face.
    Vector3 faceNormal_;
    /// Smooth interpolated normal.
    Vector3 smoothNormal_;
    /// Geometry ID.
    unsigned geometryId_;

    /// Indirect light value.
    Vector4 indirectLight_;

    /// Returns whether element is valid.
    explicit operator bool() const { return geometryId_ != 0; }
    /// Begin sample. Return position, normal and initial ray direction.
    void BeginSample(unsigned /*sampleIndex*/,
        Vector3& position, Vector3& faceNormal, Vector3& smoothNormal, Vector3& rayDirection, Vector3& albedo) const
    {
        position = position_;
        faceNormal = faceNormal_;
        smoothNormal = smoothNormal_;
        albedo = Vector3::ONE;
        rayDirection = RandomHemisphereDirection(faceNormal_);
    }
    /// End sample.
    void EndSample(const Vector3& light)
    {
        indirectLight_ += Vector4(light, 1.0f);
    }
};

/// Indirect light tracing for charts: tracing kernel.
struct ChartIndirectTracingKernel
{
    /// Indirect light chart.
    LightmapChartBakedIndirect* bakedIndirect_{};
    /// Geometry buffer.
    const LightmapChartGeometryBuffer* geometryBuffer_{};
    /// Light probes mesh for fallback.
    const TetrahedralMesh* lightProbesMesh_{};
    /// Light probes data for fallback.
    const LightProbeCollection* lightProbesData_{};
    /// Mapping from geometry buffer ID to embree geometry ID.
    const ea::vector<unsigned>* geometryBufferToEmbree_{};
    /// Embree geometry index.
    const ea::vector<EmbreeGeometry>* embreeGeometryIndex_{};
    /// Settings.
    const LightmapTracingSettings* settings_{};

    /// Last sampled tetrahedron.
    unsigned lightProbesMeshHint_{};

    /// Return number of elements to trace.
    unsigned GetNumElements() const { return bakedIndirect_->light_.size(); }
    /// Return number of samples.
    unsigned GetNumSamples() const { return settings_->numIndirectChartSamples_; }
    /// Begin tracing element.
    ChartIndirectTracingElement BeginElement(unsigned elementIndex)
    {
        const unsigned geometryId = geometryBuffer_->geometryIds_[elementIndex];
        if (!geometryId)
            return {};

        const Vector3& position = geometryBuffer_->positions_[elementIndex];
        const Vector3& smoothNormal = geometryBuffer_->smoothNormals_[elementIndex];
        const unsigned embreeGeometryId = (*geometryBufferToEmbree_)[geometryId];
        const EmbreeGeometry& embreeGeometry = (*embreeGeometryIndex_)[embreeGeometryId];

        if (embreeGeometry.numLods_ > 1)
        {
            const SphericalHarmonicsDot9 sh = lightProbesMesh_->Sample(
                lightProbesData_->bakedSphericalHarmonics_, position, lightProbesMeshHint_);
            bakedIndirect_->light_[elementIndex] += { sh.Evaluate(smoothNormal), 1.0f };
            return {};
        }

        const Vector3& faceNormal = geometryBuffer_->faceNormals_[elementIndex];
        return { position + faceNormal * settings_->rayPositionOffset_, faceNormal, smoothNormal, geometryId };
    };
    /// End tracing element.
    void EndElement(unsigned elementIndex, const ChartIndirectTracingElement& element)
    {
        bakedIndirect_->light_[elementIndex] += element.indirectLight_;
    }
};

/// Light probe indirect tracing: tracing element.
struct LightProbeIndirectTracingElement
{
    /// Position.
    Vector3 position_;

    /// Current direction.
    Vector3 currentDirection_;
    /// Indirect light SH.
    SphericalHarmonicsColor9 sh_;
    /// Indirect light average value.
    Vector3 average_;
    /// Weight.
    float weight_{};

    /// Returns whether element is valid.
    explicit operator bool() const { return true; }
    /// Begin sample. Return position, normal and initial ray direction.
    void BeginSample(unsigned /*sampleIndex*/,
        Vector3& position, Vector3& faceNormal, Vector3& smoothNormal, Vector3& rayDirection, Vector3& albedo)
    {
        position = position_;
        RandomDirection(currentDirection_);
        faceNormal = currentDirection_;
        smoothNormal = currentDirection_;
        rayDirection = currentDirection_;
        albedo = Vector3::ONE;
    }
    /// End sample.
    void EndSample(const Vector3& light)
    {
        sh_ += SphericalHarmonicsColor9(currentDirection_, light);
        average_ += light;
        weight_ += 1.0f;
    }
};

/// Light probe indirect tracing: tracing kernel.
struct LightProbeIndirectTracingKernel
{
    /// Light probes collection.
    LightProbeCollection* collection_{};
    /// Settings.
    const LightmapTracingSettings* settings_{};

    /// Return number of elements to trace.
    unsigned GetNumElements() const { return collection_->Size(); }
    /// Return number of samples.
    unsigned GetNumSamples() const { return settings_->numIndirectProbeSamples_; }
    /// Begin tracing element.
    LightProbeIndirectTracingElement BeginElement(unsigned elementIndex) const
    {
        const Vector3& position = collection_->worldPositions_[elementIndex];
        return { position };
    };
    /// End tracing element.
    void EndElement(unsigned elementIndex, const LightProbeIndirectTracingElement& element)
    {
        const SphericalHarmonicsDot9 sh{ element.sh_ * (M_PI / element.weight_) };
        collection_->bakedSphericalHarmonics_[elementIndex] += sh;
    }
};

/// Trace indirect lighting.
template <class T>
void TraceIndirectLight(T& sharedKernel, const ea::vector<const LightmapChartBakedDirect*>& bakedDirect,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    assert(settings.numBounces_ <= LightmapTracingSettings::MaxBounces);

    ParallelFor(sharedKernel.GetNumElements(), settings.numTasks_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        T kernel = sharedKernel;

        RTCScene scene = embreeScene.GetEmbreeScene();
        const float maxDistance = embreeScene.GetMaxDistance();
        const auto& geometryIndex = embreeScene.GetEmbreeGeometryIndex();

        Vector3 albedo[LightmapTracingSettings::MaxBounces];
        Vector3 incomingSamples[LightmapTracingSettings::MaxBounces];
        float incomingFactors[LightmapTracingSettings::MaxBounces];

        RTCRayHit rayHit;
        IndirectTracingContext rayContext;
        rtcInitIntersectContext(&rayContext);
        rayContext.geometryIndex_ = &geometryIndex;
        rayContext.filter = TracingFilterIndirect;

        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.mask = EmbreeScene::PrimaryLODGeometry;
        rayHit.ray.flags = 0;

        for (unsigned elementIndex = fromIndex; elementIndex < toIndex; ++elementIndex)
        {
            auto element = kernel.BeginElement(elementIndex);
            if (!element)
                continue;

            for (unsigned sampleIndex = 0; sampleIndex < kernel.GetNumSamples(); ++sampleIndex)
            {
                Vector3 currentPosition;
                Vector3 currentFaceNormal;
                Vector3 currentSmoothNormal;
                Vector3 currentRayDirection;
                element.BeginSample(sampleIndex,
                    currentPosition, currentFaceNormal, currentSmoothNormal, currentRayDirection, albedo[0]);

                int numBounces = 0;
                for (unsigned bounceIndex = 0; bounceIndex < settings.numBounces_; ++bounceIndex)
                {
                    rayHit.ray.org_x = currentPosition.x_;
                    rayHit.ray.org_y = currentPosition.y_;
                    rayHit.ray.org_z = currentPosition.z_;
                    rayHit.ray.dir_x = currentRayDirection.x_;
                    rayHit.ray.dir_y = currentRayDirection.y_;
                    rayHit.ray.dir_z = currentRayDirection.z_;
                    rayHit.ray.tfar = maxDistance;
                    rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                    rtcIntersect1(scene, &rayContext, &rayHit);

                    if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                        break;

                    // Check normal orientation
                    if (currentRayDirection.DotProduct({ rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z }) > 0.0f)
                        break;

                    // Sample lightmap UV
                    const EmbreeGeometry& geometry = geometryIndex[rayHit.hit.geomID];
                    Vector2 lightmapUV;
                    rtcInterpolate0(geometry.embreeGeometry_, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                        RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 0, &lightmapUV.x_, 2);

                    // Modify incoming flux
                    const float probability = 1 / (2 * M_PI);
                    const float cosTheta = ea::max(0.0f, currentRayDirection.DotProduct(currentSmoothNormal));
                    const float reflectance = 1 / M_PI;
                    const float brdf = reflectance / M_PI;

                    // TODO: Use real index here
                    const unsigned lightmapIndex = geometry.lightmapIndex_;
                    const IntVector2 sampleLocation = bakedDirect[lightmapIndex]->GetNearestLocation(lightmapUV);
                    incomingSamples[bounceIndex] = bakedDirect[lightmapIndex]->GetSurfaceLight(sampleLocation);
                    incomingFactors[bounceIndex] = brdf * cosTheta / probability;
                    ++numBounces;

                    // Go to next hemisphere
                    if (numBounces < settings.numBounces_)
                    {
                        // Update albedo for hit surface
                        albedo[bounceIndex + 1] = bakedDirect[lightmapIndex]->GetAlbedo(sampleLocation);

                        // Move to hit position
                        currentPosition.x_ = rayHit.ray.org_x + rayHit.ray.dir_x * rayHit.ray.tfar;
                        currentPosition.y_ = rayHit.ray.org_y + rayHit.ray.dir_y * rayHit.ray.tfar;
                        currentPosition.z_ = rayHit.ray.org_z + rayHit.ray.dir_z * rayHit.ray.tfar;

                        // Offset position a bit
                        const Vector3 hitNormal = Vector3(rayHit.hit.Ng_x, rayHit.hit.Ng_y, rayHit.hit.Ng_z).Normalized();
                        currentPosition.x_ += hitNormal.x_ * settings.rayPositionOffset_;
                        currentPosition.y_ += hitNormal.y_ * settings.rayPositionOffset_;
                        currentPosition.z_ += hitNormal.z_ * settings.rayPositionOffset_;

                        // Update smooth normal
                        rtcInterpolate0(geometry.embreeGeometry_, rayHit.hit.primID, rayHit.hit.u, rayHit.hit.v,
                            RTC_BUFFER_TYPE_VERTEX_ATTRIBUTE, 1, &currentSmoothNormal.x_, 3);
                        currentSmoothNormal = currentSmoothNormal.Normalized();

                        // Update face normal and find new direction to sample
                        currentFaceNormal = hitNormal;
                        currentRayDirection = RandomHemisphereDirection(currentFaceNormal);
                    }
                }

                // Accumulate samples back-to-front
                Vector3 sampleIndirectLight;
                for (int bounceIndex = numBounces - 1; bounceIndex >= 0; --bounceIndex)
                {
                    if (albedo[bounceIndex] == Color::RED.ToVector3())
                        albedo[bounceIndex] = albedo[bounceIndex];
                    sampleIndirectLight += incomingSamples[bounceIndex];
                    sampleIndirectLight *= incomingFactors[bounceIndex];
                    sampleIndirectLight *= albedo[bounceIndex];
                }

                element.EndSample(sampleIndirectLight);
            }
            kernel.EndElement(elementIndex, element);
        }
    });
}

}

void PreprocessGeometryBuffer(LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const ea::vector<unsigned>& geometryBufferToEmbree,
    const LightmapTracingSettings& settings)
{
    RTCScene scene = embreeScene.GetEmbreeScene();
    const ea::vector<EmbreeGeometry>& embreeGeometryIndex = embreeScene.GetEmbreeGeometryIndex();
    ParallelFor(geometryBuffer.positions_.size(), settings.numTasks_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCRayHit rayHit;
        GeometryBufferPreprocessContext rayContext;
        rayContext.geometryIndex_ = &embreeGeometryIndex;
        rtcInitIntersectContext(&rayContext);
        rayContext.filter = GeometryBufferPreprocessFilter;

        rayHit.ray.mask = EmbreeScene::AllGeometry;
        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.flags = 0;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[i];
            if (!geometryId)
                continue;

            rayContext.currentGeometry_ = &embreeGeometryIndex[geometryBufferToEmbree[geometryId]];

            Vector3& mutablePosition = geometryBuffer.positions_[i];
            const Vector3 faceNormal = geometryBuffer.faceNormals_[i];
            const float texelRadius = geometryBuffer.texelRadiuses_[i];
            const Quaternion basis{ Vector3::FORWARD, faceNormal };

            rayHit.ray.org_x = mutablePosition.x_ + faceNormal.x_ * settings.shadowLeakBias_;
            rayHit.ray.org_y = mutablePosition.y_ + faceNormal.y_ * settings.shadowLeakBias_;
            rayHit.ray.org_z = mutablePosition.z_ + faceNormal.z_ * settings.shadowLeakBias_;

            static const unsigned NumSamples = 4;
            static const Vector3 sampleRays[NumSamples] = { Vector3::LEFT, Vector3::RIGHT, Vector3::UP, Vector3::DOWN };

            // Find closest backface hit
            float closestHitDistance = M_LARGE_VALUE;
            Vector3 closestHitDirection;

            for (unsigned i = 0; i < NumSamples; ++i)
            {
                const Vector3 rayDirection = basis * sampleRays[i];

                rayHit.ray.dir_x = rayDirection.x_;
                rayHit.ray.dir_y = rayDirection.y_;
                rayHit.ray.dir_z = rayDirection.z_;
                rayHit.ray.tfar = texelRadius;
                rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
                rtcIntersect1(scene, &rayContext, &rayHit);

                if (rayHit.hit.geomID == RTC_INVALID_GEOMETRY_ID)
                    continue;

                // Frontface if dot product is negative, i.e face and ray face each other
                const float dp = rayHit.hit.Ng_x * rayHit.ray.dir_x
                    + rayHit.hit.Ng_y * rayHit.ray.dir_y
                    + rayHit.hit.Ng_z * rayHit.ray.dir_z;

                // Normal is not normalized, so epsilon won't really help
                if (dp < 0.0f)
                    continue;

                // Find closest hit
                if (rayHit.ray.tfar < closestHitDistance)
                {
                    closestHitDistance = rayHit.ray.tfar;
                    closestHitDirection = rayDirection;
                }
            }

            // Push the position behind closest hit
            mutablePosition = { rayHit.ray.org_x, rayHit.ray.org_y, rayHit.ray.org_z };
            mutablePosition += closestHitDirection * (closestHitDistance + settings.shadowLeakOffset_);
        }
    });
}

void BakeEmissionLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const LightmapTracingSettings& settings)
{
    ParallelFor(bakedDirect.directLight_.size(), settings.numTasks_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[i];
            if (!geometryId)
                continue;

            const Vector3& albedo = geometryBuffer.albedo_[i];
            const Vector3& emission = geometryBuffer.emission_[i];

            if (emission != Vector3::ZERO)
                i = i;

            bakedDirect.directLight_[i] += Vector4(emission, 0.0f);
            bakedDirect.surfaceLight_[i] += emission;
            bakedDirect.albedo_[i] = albedo;
        }
    });
}

void BakeDirectionalLight(LightmapChartBakedDirect& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const EmbreeScene& embreeScene, const ea::vector<unsigned>& geometryBufferToEmbree,
    const DirectionalLightParameters& light, const LightmapTracingSettings& settings)
{
    const Vector3 rayDirection = light.direction_.Normalized();
    const float maxDistance = embreeScene.GetMaxDistance();
    const Vector3 lightColor = light.color_.ToVector3();
    RTCScene scene = embreeScene.GetEmbreeScene();
    const ea::vector<EmbreeGeometry>& embreeGeometryIndex = embreeScene.GetEmbreeGeometryIndex();

    ParallelFor(bakedDirect.directLight_.size(), settings.numTasks_,
        [&](unsigned fromIndex, unsigned toIndex)
    {
        RTCRayHit rayHit;
        DirectTracingContextForCharts rayContext;
        rtcInitIntersectContext(&rayContext);
        rayContext.geometryIndex_ = &embreeGeometryIndex;
        rayContext.filter = TracingFilterForChartsDirect;

        rayHit.ray.mask = EmbreeScene::AllGeometry;
        rayHit.ray.dir_x = rayDirection.x_;
        rayHit.ray.dir_y = rayDirection.y_;
        rayHit.ray.dir_z = rayDirection.z_;
        rayHit.ray.tnear = 0.0f;
        rayHit.ray.time = 0.0f;
        rayHit.ray.id = 0;
        rayHit.ray.flags = 0;

        for (unsigned i = fromIndex; i < toIndex; ++i)
        {
            const unsigned geometryId = geometryBuffer.geometryIds_[i];
            if (!geometryId)
                continue;

            const Vector3& position = geometryBuffer.positions_[i];
            const Vector3& faceNormal = geometryBuffer.faceNormals_[i];
            const Vector3& smoothNormal = geometryBuffer.smoothNormals_[i];

            const unsigned embreeGeometryId = geometryBufferToEmbree[geometryId];
            rayContext.currentGeometry_ = &embreeGeometryIndex[embreeGeometryId];

            Vector3 incomingLight = lightColor;
            rayContext.incomingLight_ = &incomingLight;

            // Cast direct ray
            rayHit.ray.org_x = position.x_ + faceNormal.x_ * settings.rayPositionOffset_ - rayDirection.x_ * maxDistance;
            rayHit.ray.org_y = position.y_ + faceNormal.y_ * settings.rayPositionOffset_ - rayDirection.y_ * maxDistance;
            rayHit.ray.org_z = position.z_ + faceNormal.z_ * settings.rayPositionOffset_ - rayDirection.z_ * maxDistance;
            rayHit.ray.tfar = maxDistance;
            rayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
            rtcIntersect1(scene, &rayContext, &rayHit);

            if (rayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
                continue;

            const float intensity = ea::max(0.0f, smoothNormal.DotProduct(-rayDirection));
            const Vector3 lightIntensity = incomingLight * intensity;


            if (light.bakeDirect_)
                bakedDirect.directLight_[i] += Vector4(lightIntensity, 0.0f);

            if (light.bakeIndirect_)
            {
                const Vector3& albedo = geometryBuffer.albedo_[i];
                bakedDirect.surfaceLight_[i] += albedo * lightIntensity;
            }
        }
    });
}

void BakeIndirectLightForCharts(LightmapChartBakedIndirect& bakedIndirect,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const TetrahedralMesh& lightProbesMesh, const LightProbeCollection& lightProbesData,
    const EmbreeScene& embreeScene, const ea::vector<unsigned>& geometryBufferToEmbree,
    const LightmapTracingSettings& settings)
{
    ChartIndirectTracingKernel kernel{ &bakedIndirect, &geometryBuffer, &lightProbesMesh, &lightProbesData,
        &geometryBufferToEmbree, &embreeScene.GetEmbreeGeometryIndex(), &settings };
    TraceIndirectLight(kernel, bakedDirect, embreeScene, settings);
}

void BakeIndirectLightForLightProbes(LightProbeCollection& collection,
    const ea::vector<const LightmapChartBakedDirect*>& bakedDirect,
    const EmbreeScene& embreeScene, const LightmapTracingSettings& settings)
{
    LightProbeIndirectTracingKernel kernel{ &collection, &settings };
    TraceIndirectLight(kernel, bakedDirect, embreeScene, settings);
}

void FilterIndirectLight(LightmapChartBakedIndirect& bakedIndirect, const LightmapChartGeometryBuffer& geometryBuffer,
    const IndirectFilterParameters& params, unsigned numTasks)
{
    const ea::span<const float> kernelWeights = GetKernel(params.kernelRadius_);
    ParallelFor(bakedIndirect.light_.size(), numTasks,
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
            const Vector3 centerPosition = geometryBuffer.positions_[index];
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
                        centerPosition, geometryBuffer.positions_[otherIndex], dxdy * params.positionSigma_,
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
