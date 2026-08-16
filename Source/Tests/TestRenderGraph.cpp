// SPDX-License-Identifier: MIT

#include <catch2/catch_amalgamated.hpp>

#include <Urho3D/RenderPipeline/RenderGraph.h>

using namespace Urho3D;

TEST_CASE("RenderGraph compiles dependencies and executes passes", "[rendergraph][render]")
{
    RenderGraph graph;
    RenderGraphResourceDesc colorDesc;
    colorDesc.name = "SceneColor";
    colorDesc.width = 1920;
    colorDesc.height = 1080;
    colorDesc.format = 1;
    const RenderGraphResourceHandle color = graph.CreateResource(colorDesc);

    RenderGraphResourceDesc lightingDesc = colorDesc;
    lightingDesc.name = "Lighting";
    const RenderGraphResourceHandle lighting = graph.CreateResource(lightingDesc);

    ea::vector<ea::string> executed;
    graph.AddPass({"GBuffer", {{color, true}}, [&](const RenderGraphPassContext& context)
        {
            REQUIRE(context.GetFrameIndex() == 7);
            executed.push_back("GBuffer");
        }});
    graph.AddPass({"Lighting", {{color, false}, {lighting, true}}, [&](const RenderGraphPassContext&)
        {
            executed.push_back("Lighting");
        }});
    graph.AddPass({"PostProcess", {{lighting, false}}, [&](const RenderGraphPassContext&)
        {
            executed.push_back("PostProcess");
        }});

    REQUIRE(graph.Compile());
    REQUIRE(graph.GetLastError().empty());
    REQUIRE(graph.GetExecutionOrder().size() == 3);
    REQUIRE(graph.Execute(7));
    REQUIRE(executed.size() == 3);
    REQUIRE(executed[0] == "GBuffer");
    REQUIRE(executed[1] == "Lighting");
    REQUIRE(executed[2] == "PostProcess");
}

TEST_CASE("RenderGraph aliases compatible non-overlapping transient resources", "[rendergraph][memory]")
{
    RenderGraph graph;
    RenderGraphResourceDesc desc;
    desc.name = "Transient";
    desc.width = 256;
    desc.height = 256;
    desc.format = 2;
    const RenderGraphResourceHandle first = graph.CreateResource(desc);
    const RenderGraphResourceHandle second = graph.CreateResource(desc);

    graph.AddPass({"First", {{first, true}}, {}});
    graph.AddPass({"Second", {{second, true}}, {}});

    REQUIRE(graph.Compile());
    REQUIRE(graph.GetTransientAliasGroupCount() == 1);
    REQUIRE(graph.GetCompiledResources().size() == 2);
    REQUIRE(graph.GetCompiledResources()[0].aliasGroup == graph.GetCompiledResources()[1].aliasGroup);
}

TEST_CASE("RenderGraph rejects invalid and duplicate resource uses", "[rendergraph][validation]")
{
    RenderGraph graph;
    RenderGraphResourceDesc desc;
    desc.name = "Color";
    const RenderGraphResourceHandle color = graph.CreateResource(desc);

    graph.AddPass({"Invalid", {{InvalidRenderGraphResource, true}}, {}});
    REQUIRE_FALSE(graph.Compile());
    REQUIRE_FALSE(graph.GetLastError().empty());

    graph.Reset();
    const RenderGraphResourceHandle restoredColor = graph.CreateResource(desc);
    graph.AddPass({"Duplicate", {{restoredColor, false}, {restoredColor, true}}, {}});
    REQUIRE_FALSE(graph.Compile());
    REQUIRE_FALSE(graph.GetLastError().empty());
}

TEST_CASE("RenderGraph keeps imported resources out of transient aliasing", "[rendergraph][memory]")
{
    RenderGraph graph;
    RenderGraphResourceDesc desc;
    desc.name = "Backbuffer";
    const RenderGraphResourceHandle backbuffer = graph.ImportResource(desc);
    graph.AddPass({"Present", {{backbuffer, true}}, {}});

    REQUIRE(graph.Compile());
    REQUIRE(graph.GetTransientAliasGroupCount() == 0);
    REQUIRE(graph.GetCompiledResources()[0].aliasGroup == InvalidRenderGraphResource);
    REQUIRE(graph.GetCompiledResources()[0].desc.imported);
}

TEST_CASE("RenderGraph reports pass durations to the production profiler", "[rendergraph][profiler]")
{
    RenderGraph graph;
    RenderGraphResourceDesc desc;
    desc.name = "Color";
    const RenderGraphResourceHandle color = graph.CreateResource(desc);

    ea::vector<ea::string> names;
    ea::vector<double> durations;
    graph.SetPassProfiler([&](const ea::string& name, double milliseconds)
    {
        names.push_back(name);
        durations.push_back(milliseconds);
    });
    graph.AddPass({"Measured", {{color, true}}, [](const RenderGraphPassContext&) {}});

    REQUIRE(graph.Execute(3));
    REQUIRE(names.size() == 1);
    CHECK(names.front() == "Measured");
    CHECK(durations.front() >= 0.0);
}
