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

#include "../Precompiled.h"

#include "../Graphics/LightmapManager.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Graphics/GlobalIllumination.h"
#include "../Scene/Scene.h"

#if URHO3D_GLOW
#include "../Glow/IncrementalLightmapper.h"
#endif

namespace Urho3D
{

extern const char* SUBSYSTEM_CATEGORY;

LightmapManager::LightmapManager(Context* context) :
    Component(context)
{
    SubscribeToEvent(E_UPDATE, [this](StringHash eventType, VariantMap& eventData)
    {
        if (bakingScheduled_)
        {
            bakingScheduled_ = false;
            Bake();
        }
    });
}

LightmapManager::~LightmapManager() = default;

void LightmapManager::RegisterObject(Context* context)
{
    static const IncrementalLightmapperSettings defaultIncrementalSettings;
    static const LightmapSettings defaultLightmapSettings;
    context->RegisterFactory<LightmapManager>(SUBSYSTEM_CATEGORY);

    auto getBake = [](const ClassName& self, Urho3D::Variant& value) { value = false; };
    auto setBake = [](ClassName& self, const Urho3D::Variant& value) { if (value.GetBool()) self.bakingScheduled_ = true; };
    URHO3D_CUSTOM_ACCESSOR_ATTRIBUTE("Bake!", getBake, setBake, bool, false, AM_EDIT);

    URHO3D_ATTRIBUTE("Output Directory", ea::string, incrementalBakingSettings_.outputDirectory_, "", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Lightmap Size", unsigned, lightmapSettings_.charting_.lightmapSize_, defaultLightmapSettings.charting_.lightmapSize_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Texel Density", float, lightmapSettings_.charting_.texelDensity_, defaultLightmapSettings.charting_.texelDensity_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Bounces", unsigned, lightmapSettings_.tracing_.numBounces_, defaultLightmapSettings.tracing_.numBounces_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Samples (Texture)", unsigned, lightmapSettings_.tracing_.numIndirectChartSamples_, defaultLightmapSettings.tracing_.numIndirectChartSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Samples (Light Probes)", unsigned, lightmapSettings_.tracing_.numIndirectProbeSamples_, defaultLightmapSettings.tracing_.numIndirectProbeSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Size", Vector3, incrementalBakingSettings_.chunkSize_, defaultIncrementalSettings.chunkSize_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Indirect Padding", float, incrementalBakingSettings_.indirectPadding_, defaultIncrementalSettings.indirectPadding_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Shadow Distance", float, incrementalBakingSettings_.directionalLightShadowDistance_, defaultIncrementalSettings.directionalLightShadowDistance_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Stitch Iterations", unsigned, lightmapSettings_.stitching_.numIterations_, defaultLightmapSettings.stitching_.numIterations_, AM_DEFAULT);
}

void LightmapManager::Bake()
{
    // Bake lightmaps if possible
#if URHO3D_GLOW
    DefaultBakedSceneCollector sceneCollector;
    BakedLightMemoryCache lightmapCache;
    IncrementalLightmapper lightmapper;

    if (lightmapper.Initialize(lightmapSettings_, incrementalBakingSettings_, GetScene(), &sceneCollector, &lightmapCache))
    {
        lightmapper.ProcessScene();
        lightmapper.Bake();
        lightmapper.CommitScene();
    }
#endif

    // Compile light probes
    if (auto gi = GetScene()->GetComponent<GlobalIllumination>())
    {
        gi->CompileLightProbes();
    }
}

}
