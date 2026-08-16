//
// Copyright (c) 2026 the rbfx-blueprint project.
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

#include <EASTL/functional.h>
#include <EASTL/string.h>
#include <EASTL/vector.h>

#include "Urho3D/Core/Attribute.h"

namespace Urho3D
{

/// Stable handle returned by RenderGraph when a resource is declared.
using RenderGraphResourceHandle = unsigned;
using RenderGraphPassHandle = unsigned;

static constexpr RenderGraphResourceHandle InvalidRenderGraphResource = M_MAX_UNSIGNED;
static constexpr RenderGraphPassHandle InvalidRenderGraphPass = M_MAX_UNSIGNED;

/// Kind of GPU resource managed by the graph allocator.
enum class RenderGraphResourceKind
{
    Texture,
    Buffer
};

/// Backend-neutral description used by the graph compiler and transient allocator.
struct URHO3D_API RenderGraphResourceDesc
{
    ea::string name;
    RenderGraphResourceKind kind{RenderGraphResourceKind::Texture};
    unsigned width{};
    unsigned height{};
    unsigned depth{1};
    unsigned format{};
    unsigned bindFlags{};
    bool transient{true};
    bool imported{};

    bool operator==(const RenderGraphResourceDesc& rhs) const
    {
        return kind == rhs.kind && width == rhs.width && height == rhs.height && depth == rhs.depth
            && format == rhs.format && bindFlags == rhs.bindFlags;
    }
};

/// One read or write edge between a pass and a graph resource.
struct URHO3D_API RenderGraphResourceUse
{
    RenderGraphResourceHandle resource{InvalidRenderGraphResource};
    bool write{};
};

/// Read-only context passed to a compiled pass callback.
class URHO3D_API RenderGraphPassContext
{
public:
    RenderGraphPassContext(const class RenderGraph& graph, RenderGraphPassHandle pass, unsigned frameIndex)
        : graph_(graph)
        , pass_(pass)
        , frameIndex_(frameIndex)
    {
    }

    const class RenderGraph& GetGraph() const { return graph_; }
    RenderGraphPassHandle GetPass() const { return pass_; }
    unsigned GetFrameIndex() const { return frameIndex_; }

private:
    const class RenderGraph& graph_;
    RenderGraphPassHandle pass_{};
    unsigned frameIndex_{};
};

/// Description of one render pass. Callbacks issue backend commands through the graph context.
using RenderGraphPassProfiler = ea::function<void(const ea::string&, double)>;

struct URHO3D_API RenderGraphPassDesc
{
    ea::string name;
    ea::vector<RenderGraphResourceUse> resources;
    ea::function<void(const RenderGraphPassContext&)> execute;
};

/// Compiled lifetime and aliasing information for a declared resource.
struct URHO3D_API RenderGraphCompiledResource
{
    RenderGraphResourceHandle handle{InvalidRenderGraphResource};
    RenderGraphResourceDesc desc;
    unsigned firstPass{InvalidRenderGraphPass};
    unsigned lastPass{InvalidRenderGraphPass};
    unsigned aliasGroup{InvalidRenderGraphResource};
};

/// Backend-neutral render graph compiler.
///
/// The graph records resource hazards, produces a deterministic topological order,
/// computes transient lifetimes and assigns compatible non-overlapping resources to
/// alias groups. A renderer backend can use the compiled data to allocate actual
/// textures and buffers without duplicating scheduling logic.
class URHO3D_API RenderGraph
{
public:
    RenderGraph() = default;

    /// Remove all passes, resources and compiled state.
    void Reset();

    /// Declare a transient resource owned by the graph.
    RenderGraphResourceHandle CreateResource(const RenderGraphResourceDesc& desc);
    /// Declare an external resource that must not be aliased or destroyed by the graph.
    RenderGraphResourceHandle ImportResource(const RenderGraphResourceDesc& desc);
    /// Return a resource description, or nullptr for an invalid handle.
    const RenderGraphResourceDesc* GetResource(RenderGraphResourceHandle handle) const;

    /// Add a pass in declaration order. Names must be non-empty and unique.
    RenderGraphPassHandle AddPass(const RenderGraphPassDesc& desc);

    /// Compile hazards, execution order, lifetimes and transient alias groups.
    bool Compile();
    /// Execute compiled callbacks in dependency order.
    bool Execute(unsigned frameIndex = 0);
    /// Install an optional duration sink for pass-level GPU/renderer instrumentation.
    void SetPassProfiler(RenderGraphPassProfiler profiler) { passProfiler_ = ea::move(profiler); }

    bool IsCompiled() const { return compiled_; }
    const ea::string& GetLastError() const { return lastError_; }
    const ea::vector<RenderGraphPassHandle>& GetExecutionOrder() const { return executionOrder_; }
    const ea::vector<RenderGraphCompiledResource>& GetCompiledResources() const { return compiledResources_; }
    unsigned GetTransientAliasGroupCount() const { return transientAliasGroupCount_; }

private:
    bool IsValidResource(RenderGraphResourceHandle handle) const;
    bool ValidatePasses();
    void SetError(const ea::string& message);

    struct PassRecord
    {
        RenderGraphPassDesc desc;
        ea::vector<RenderGraphPassHandle> outgoing;
        unsigned incoming{};
    };

    ea::vector<RenderGraphResourceDesc> resources_;
    ea::vector<PassRecord> passes_;
    ea::vector<RenderGraphPassHandle> executionOrder_;
    ea::vector<RenderGraphCompiledResource> compiledResources_;
    ea::string lastError_;
    unsigned transientAliasGroupCount_{};
    RenderGraphPassProfiler passProfiler_;
    bool compiled_{};
};

} // namespace Urho3D
