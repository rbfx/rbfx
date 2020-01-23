//
// Copyright (c) 2019-2020 the rbfx project.
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

#include "../Graphics/LightBaker.h"

#include "../Core/Context.h"
#include "../Core/CoreEvents.h"
#include "../Core/Timer.h"
#include "../Graphics/GlobalIllumination.h"
#include "../Graphics/Octree.h"
#include "../Graphics/Renderer.h"
#include "../Graphics/Skybox.h"
#include "../Graphics/Zone.h"
#include "../IO/Log.h"
#include "../Scene/Scene.h"

#if URHO3D_GLOW
#include "../Glow/IncrementalLightBaker.h"
#endif

namespace Urho3D
{

namespace
{

static const char* qualityNames[] =
{
    "Custom",
    "Low",
    "Medium",
    "High",
    nullptr
};

}

extern const char* SUBSYSTEM_CATEGORY;

LightBaker::LightBaker(Context* context) :
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

LightBaker::~LightBaker() = default;

void LightBaker::RegisterObject(Context* context)
{
    static const LightBakingSettings defaultSettings;
    context->RegisterFactory<LightBaker>(SUBSYSTEM_CATEGORY);

    auto getBake = [](const ClassName& self, Urho3D::Variant& value) { value = false; };
    auto setBake = [](ClassName& self, const Urho3D::Variant& value) { if (value.GetBool()) self.bakingScheduled_ = true; };
    URHO3D_CUSTOM_ACCESSOR_ATTRIBUTE("Bake!", getBake, setBake, bool, false, AM_EDIT);

    URHO3D_ATTRIBUTE("Output Directory", ea::string, settings_.incremental_.outputDirectory_, "", AM_DEFAULT);
    URHO3D_ATTRIBUTE("Lightmap Size", unsigned, settings_.charting_.lightmapSize_, defaultSettings.charting_.lightmapSize_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Texel Density", float, settings_.charting_.texelDensity_, defaultSettings.charting_.texelDensity_, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Quality", GetQuality, SetQuality, LightBakingQuality, qualityNames, LightBakingQuality::Custom, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Direct Samples (Lightmap)", unsigned, settings_.directChartTracing_.maxSamples_, defaultSettings.directChartTracing_.maxSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Direct Samples (Light Probes)", unsigned, settings_.directProbesTracing_.maxSamples_, defaultSettings.directProbesTracing_.maxSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Bounces", unsigned, settings_.indirectChartTracing_.maxBounces_, defaultSettings.indirectChartTracing_.maxBounces_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Samples (Texture)", unsigned, settings_.indirectChartTracing_.maxSamples_, defaultSettings.indirectChartTracing_.maxSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Indirect Samples (Light Probes)", unsigned, settings_.indirectProbesTracing_.maxSamples_, defaultSettings.indirectProbesTracing_.maxSamples_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Filter Radius (Direct)", unsigned, settings_.directFilter_.kernelRadius_, defaultSettings.directFilter_.kernelRadius_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Filter Radius (Indirect)", unsigned, settings_.indirectFilter_.kernelRadius_, defaultSettings.indirectFilter_.kernelRadius_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Size", Vector3, settings_.incremental_.chunkSize_, defaultSettings.incremental_.chunkSize_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Indirect Padding", float, settings_.incremental_.indirectPadding_, defaultSettings.incremental_.indirectPadding_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Chunk Shadow Distance", float, settings_.incremental_.directionalLightShadowDistance_, defaultSettings.incremental_.directionalLightShadowDistance_, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Stitch Iterations", unsigned, settings_.stitching_.numIterations_, defaultSettings.stitching_.numIterations_, AM_DEFAULT);
}

void LightBaker::SetQuality(LightBakingQuality quality)
{
    quality_ = quality;
    switch (quality)
    {
    case LightBakingQuality::Low:
        settings_.directChartTracing_.maxSamples_ = 10;
        settings_.directProbesTracing_.maxSamples_ = 32;
        settings_.indirectChartTracing_.maxSamples_ = 10;
        settings_.indirectProbesTracing_.maxSamples_ = 64;
        break;
    case LightBakingQuality::Medium:
        settings_.directChartTracing_.maxSamples_ = 32;
        settings_.directProbesTracing_.maxSamples_ = 32;
        settings_.indirectChartTracing_.maxSamples_ = 64;
        settings_.indirectProbesTracing_.maxSamples_ = 256;
        break;
    case LightBakingQuality::High:
        settings_.directChartTracing_.maxSamples_ = 32;
        settings_.directProbesTracing_.maxSamples_ = 32;
        settings_.indirectChartTracing_.maxSamples_ = 256;
        settings_.indirectProbesTracing_.maxSamples_ = 256;
        break;
    case LightBakingQuality::Custom:
    default:
        break;
    }
}

void LightBaker::Bake()
{
    Scene* scene = GetScene();
    auto octree = scene->GetComponent<Octree>();
    auto gi = scene->GetComponent<GlobalIllumination>();
    Zone* zone = octree->GetZone();
    Skybox* skybox = octree->GetSkybox();

    if (!gi)
    {
        URHO3D_LOGERROR("GlobalIllumination scene system is required to bake light");
        return;
    }

    // Fill settings
    settings_.indirectProbesTracing_.maxBounces_ = settings_.indirectChartTracing_.maxBounces_;

    const unsigned numTasks = settings_.charting_.lightmapSize_;
    settings_.geometryBufferPreprocessing_.numTasks_ = numTasks;
    settings_.emissionTracing_.numTasks_ = numTasks;
    settings_.directChartTracing_.numTasks_ = numTasks;
    settings_.directProbesTracing_.numTasks_ = numTasks;
    settings_.indirectChartTracing_.numTasks_ = numTasks;
    settings_.indirectProbesTracing_.numTasks_ = numTasks;

    settings_.properties_.backgroundImage_ = nullptr;
    if (gi->GetBackgroundStatic())
    {
        settings_.properties_.backgroundColor_ = zone->GetFogColor().ToVector3();
        settings_.properties_.backgroundBrightness_ = gi->GetBackgroundBrightness();
        if (skybox)
        {
            if (auto image = skybox->GetImage())
                settings_.properties_.backgroundImage_ = image->GetDecompressedImage();
        }
    }
    else
    {
        settings_.properties_.backgroundColor_ = Vector3::ZERO;
        settings_.properties_.backgroundBrightness_ = 0.0f;
    }

    // Bake light if possible
#if URHO3D_GLOW
    Timer timer;
    DefaultBakedSceneCollector sceneCollector;
    BakedLightMemoryCache lightmapCache;
    IncrementalLightBaker lightBaker;

    if (lightBaker.Initialize(settings_, GetScene(), &sceneCollector, &lightmapCache))
    {
        lightBaker.ProcessScene();
        lightBaker.Bake();
        lightBaker.CommitScene();
    }
#else
    URHO3D_LOGERROR("Enable URHO3D_GLOW in build options");
#endif

    // Compile light probes
    gi->CompileLightProbes();

#if URHO3D_GLOW
    // Log overall time
    const unsigned totalMSec = timer.GetMSec(true);
    URHO3D_LOGINFO("Light baking is finished in {} seconds", totalMSec / 1000);
#endif
}

}
