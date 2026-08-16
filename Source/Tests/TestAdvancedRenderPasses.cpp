#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RenderPipeline/Passes/AdvancedRenderPasses.h>
#include <Urho3D/RenderPipeline/RenderPipelineDefs.h>

#include "CommonUtils.h"

using namespace Urho3D;

TEST_CASE("Temporal upscaling pass clamps parameters and tracks history", "[render][postprocess]")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto pass = MakeShared<TemporalUpscalingPass>(context);

    StringVariantMap params;
    pass->CollectParameters(params);
    params["Temporal Upscaling: Feedback"] = 4.0f;
    params["Temporal Upscaling: Sharpness"] = -1.0f;
    params["Temporal Upscaling: JitterScale"] = 99.0f;
    pass->UpdateParameters(RenderPipelineSettings{}, params);

    REQUIRE(pass->GetFeedback() == Catch::Approx(0.99f));
    REQUIRE(pass->GetSharpness() == Catch::Approx(0.0f));
    REQUIRE(pass->GetJitterScale() == Catch::Approx(4.0f));
    REQUIRE(pass->GetHistoryGeneration() == 0);
}

TEST_CASE("Volumetric fog pass clamps physical controls", "[render][postprocess]")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto pass = MakeShared<VolumetricFogPass>(context);

    StringVariantMap params;
    pass->CollectParameters(params);
    params["Volumetric Fog: Density"] = -1.0f;
    params["Volumetric Fog: HeightFalloff"] = 50.0f;
    params["Volumetric Fog: Anisotropy"] = 2.0f;
    pass->UpdateParameters(RenderPipelineSettings{}, params);

    REQUIRE(pass->GetDensity() == Catch::Approx(0.0f));
    REQUIRE(pass->GetHeightFalloff() == Catch::Approx(10.0f));
    REQUIRE(pass->GetAnisotropy() == Catch::Approx(0.99f));
}

TEST_CASE("Screen space reflections pass bounds work budget", "[render][postprocess]")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto pass = MakeShared<ScreenSpaceReflectionsPass>(context);

    StringVariantMap params;
    pass->CollectParameters(params);
    params["SSR: MaxSteps"] = 1u;
    params["SSR: MaxDistance"] = 0.0f;
    params["SSR: Thickness"] = 100.0f;
    pass->UpdateParameters(RenderPipelineSettings{}, params);

    REQUIRE(pass->GetMaxSteps() == 4);
    REQUIRE(pass->GetMaxDistance() == Catch::Approx(0.1f));
    REQUIRE(pass->GetThickness() == Catch::Approx(10.0f));
}

TEST_CASE("Cascaded shadow pass bounds split policy", "[render][shadows]")
{
    auto context = Tests::GetOrCreateContext(Tests::CreateCompleteContext);
    auto pass = MakeShared<CascadedShadowMapsPass>(context);

    StringVariantMap params;
    pass->CollectParameters(params);
    params["Cascaded Shadows: Cascade Count"] = 99u;
    params["Cascaded Shadows: Split Lambda"] = -1.0f;
    params["Cascaded Shadows: Max Distance"] = 0.0f;
    params["Cascaded Shadows: Bias"] = 99.0f;
    pass->UpdateParameters(RenderPipelineSettings{}, params);

    REQUIRE(pass->GetCascadeCount() == 8);
    REQUIRE(pass->GetSplitLambda() == Catch::Approx(0.0f));
    REQUIRE(pass->GetMaxDistance() == Catch::Approx(1.0f));
    REQUIRE(pass->GetShadowBias() == Catch::Approx(1.0f));
}
