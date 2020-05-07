//
// Copyright (c) 2008-2020 the Urho3D project.
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

/// \file

#pragma once

#include "../Container/Hash.h"
#include "../Graphics/GraphicsDefs.h"
#include "../Resource/Resource.h"

namespace Urho3D
{

class ShaderVariation;

static const char* blendModeNames[] =
{
    "replace",
    "add",
    "multiply",
    "alpha",
    "addalpha",
    "premulalpha",
    "invdestalpha",
    "subtract",
    "subtractalpha",
    nullptr
};

static const char* compareModeNames[] =
{
    "always",
    "equal",
    "notequal",
    "less",
    "lessequal",
    "greater",
    "greaterequal",
    nullptr
};

static const char* lightingModeNames[] =
{
    "unlit",
    "pervertex",
    "perpixel",
    nullptr
};

/// Lighting mode of a pass.
enum PassLightingMode
{
    LIGHTING_UNLIT = 0,
    LIGHTING_PERVERTEX,
    LIGHTING_PERPIXEL
};

/// %Material rendering pass, which defines shaders and render state.
class URHO3D_API Pass : public RefCounted
{
public:
    /// Construct.
    explicit Pass(const ea::string& name);
    /// Destruct.
    ~Pass() override;

    /// Set blend mode.
    void SetBlendMode(BlendMode mode);
    /// Set culling mode override. By default culling mode is read from the material instead. Set the illegal culling mode MAX_CULLMODES to disable override again.
    void SetCullMode(CullMode mode);
    /// Set depth compare mode.
    void SetDepthTestMode(CompareMode mode);
    /// Set pass lighting mode, affects what shader variations will be attempted to be loaded.
    void SetLightingMode(PassLightingMode mode);
    /// Set depth write on/off.
    void SetDepthWrite(bool enable);
    /// Set alpha-to-coverage on/off.
    void SetAlphaToCoverage(bool enable);
    /// Set whether requires desktop level hardware.
    void SetIsDesktop(bool enable);
    /// Set vertex shader name.
    void SetVertexShader(const ea::string& name) { SetShader(VS, name); }
    /// Set pixel shader name.
    void SetPixelShader(const ea::string& name) { SetShader(PS, name); }
    /// Set vertex shader defines. Separate multiple defines with spaces.
    void SetVertexShaderDefines(const ea::string& defines) { SetShaderDefines(VS, defines); }
    /// Set pixel shader defines. Separate multiple defines with spaces.
    void SetPixelShaderDefines(const ea::string& defines) { SetShaderDefines(PS, defines); }
    /// Set vertex shader define excludes. Use to mark defines that the shader code will not recognize, to prevent compiling redundant shader variations.
    void SetVertexShaderDefineExcludes(const ea::string& excludes) { SetShaderExcludeDefines(VS, excludes); }
    /// Set pixel shader define excludes. Use to mark defines that the shader code will not recognize, to prevent compiling redundant shader variations.
    void SetPixelShaderDefineExcludes(const ea::string& excludes) { SetShaderExcludeDefines(PS, excludes); }
    /// Reset shader pointers.
    void ReleaseShaders();
    /// Mark shaders loaded this frame.
    void MarkShadersLoaded(unsigned frameNumber);

    /// Return pass name.
    const ea::string& GetName() const { return name_; }

    /// Return pass index. This is used for optimal render-time pass queries that avoid map lookups.
    unsigned GetIndex() const { return index_; }

    /// Return blend mode.
    BlendMode GetBlendMode() const { return blendMode_; }

    /// Return culling mode override. If pass is not overriding culling mode (default), the illegal mode MAX_CULLMODES is returned.
    CullMode GetCullMode() const { return cullMode_; }

    /// Return depth compare mode.
    CompareMode GetDepthTestMode() const { return depthTestMode_; }

    /// Return pass lighting mode.
    PassLightingMode GetLightingMode() const { return lightingMode_; }

    /// Return last shaders loaded frame number.
    unsigned GetShadersLoadedFrameNumber() const { return shadersLoadedFrameNumber_; }

    /// Return depth write mode.
    bool GetDepthWrite() const { return depthWrite_; }

    /// Return alpha-to-coverage mode.
    bool GetAlphaToCoverage() const { return alphaToCoverage_; }

    /// Return whether requires desktop level hardware.
    bool IsDesktop() const { return isDesktop_; }

    /// Return vertex shader name.
    const ea::string& GetVertexShader() const { return vertexShaderData_.shaderName_; }

    /// Return pixel shader name.
    const ea::string& GetPixelShader() const { return pixelShaderData_.shaderName_; }

    /// Return vertex shader defines.
    const ea::string& GetVertexShaderDefines() const { return vertexShaderData_.defines_; }

    /// Return pixel shader defines.
    const ea::string& GetPixelShaderDefines() const { return pixelShaderData_.defines_; }

    /// Return vertex shader define excludes.
    const ea::string& GetVertexShaderDefineExcludes() const { return vertexShaderData_.defineExcludes_; }

    /// Return pixel shader define excludes.
    const ea::string& GetPixelShaderDefineExcludes() const { return pixelShaderData_.defineExcludes_; }

    /// Return vertex shaders.
    ea::vector<SharedPtr<ShaderVariation> >& GetVertexShaders() { return vertexShaderData_.shaders_; }

    /// Return pixel shaders.
    ea::vector<SharedPtr<ShaderVariation> >& GetPixelShaders() { return pixelShaderData_.shaders_; }

    /// Return vertex shaders with extra defines from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetVertexShaders(const StringHash& extraDefinesHash) { return GetShaders(VS, extraDefinesHash); }
    /// Return pixel shaders with extra defines from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetPixelShaders(const StringHash& extraDefinesHash) { return GetShaders(PS, extraDefinesHash); }

    /// Return the effective vertex shader defines, accounting for excludes. Called internally by Renderer.
    ea::string GetEffectiveVertexShaderDefines() const { return GetEffectiveShaderDefines(VS); }
    /// Return the effective pixel shader defines, accounting for excludes. Called internally by Renderer.
    ea::string GetEffectivePixelShaderDefines() const { return GetEffectiveShaderDefines(PS); }

    /// Set geometry shader name.
    void SetGeometryShader(const ea::string& name) { SetShader(GS, name); }
    /// Set geometry shader defines. Separate multiple defines with spaces.
    void SetGeometryShaderDefines(const ea::string& defines) { SetShaderDefines(GS, defines); }
    /// Set geometry shader define excludes. Use to mark defines that the shader code will not recognize, to prevent compiling redundant shader variations.
    void SetGeometryShaderDefineExcludes(const ea::string& excludes) { SetShaderExcludeDefines(GS, excludes); }
    /// Return geometry shader name.
    const ea::string& GetGeometryShader() const { return geometryShaderData_.shaderName_; }
    /// Return geometry shader defines.
    const ea::string& GetGeometryShaderDefines() const { return geometryShaderData_.defines_; }
    /// Return geometry shader define excludes.
    const ea::string& GetGeometryShaderDefineExcludes() const { return geometryShaderData_.defineExcludes_; }
    /// Return pixel shaders.
    ea::vector<SharedPtr<ShaderVariation> >& GetGeometryShaders() { return geometryShaderData_.shaders_; }
    /// Return geometry shaders with extra defines from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetGeometryShaders(const StringHash& extraDefinesHash) { return GetShaders(GS, extraDefinesHash); }
    /// Return the effective geometry shader defines, accounting for excludes. Called internally by Renderer.
    ea::string GetEffectiveGeometryShaderDefines() const { return GetEffectiveShaderDefines(GS); }

    /// Set hull shader name.
    void SetHullShader(const ea::string& name) { SetShader(HS, name); }
    /// Set domain shader name.
    void SetDomainShader(const ea::string& name) { SetShader(DS, name); }
    /// Set hull shader defines. Separate multiple defines with spaces.
    void SetHullShaderDefines(const ea::string& defines) { SetShaderDefines(HS, defines); }
    /// Set domain shader defines. Separate multiple defines with spaces.
    void SetDomainShaderDefines(const ea::string& defines) { SetShaderDefines(DS, defines); }
    /// Set hull shader define excludes. Use to mark defines that the shader code will not recognize, to prevent compiling redundant shader variations.
    void SetHullShaderDefineExcludes(const ea::string& excludes) { SetShaderExcludeDefines(HS, excludes); }
    /// Set domain shader define excludes. Use to mark defines that the shader code will not recognize, to prevent compiling redundant shader variations.
    void SetDomainShaderDefineExcludes(const ea::string& excludes) { SetShaderExcludeDefines(DS, excludes); }
    /// Return hull shader name.
    const ea::string& GetHullShader() const { return hullShaderData_.shaderName_; }
    /// Return domain shader name.
    const ea::string& GetDomainShader() const { return domainShaderData_.shaderName_; }
    /// Return hull shader defines.
    const ea::string& GetHullShaderDefines() const { return hullShaderData_.defines_; }
    /// Return domain shader defines.
    const ea::string& GetDomainShaderDefines() const { return domainShaderData_.defines_; }
    /// Return hull shader define excludes.
    const ea::string& GetHullShaderDefineExcludes() const { return hullShaderData_.defineExcludes_; }
    /// Return domain shader define excludes.
    const ea::string& GetDomainShaderDefineExcludes() const { return domainShaderData_.defineExcludes_; }
    /// Return hull shaders.
    ea::vector<SharedPtr<ShaderVariation> >& GetHullShaders() { return hullShaderData_.shaders_; }
    /// Return domain shaders.
    ea::vector<SharedPtr<ShaderVariation> >& GetDomainShaders() { return domainShaderData_.shaders_; }
    /// Return hull shaders with extra defines from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetHullShaders(const StringHash& extraDefinesHash) { return GetShaders(HS, extraDefinesHash); }
    /// Return domain shaders with extra defines from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetDomainShaders(const StringHash& extraDefinesHash) { return GetShaders(DS, extraDefinesHash); }
    /// Return the effective hull shader defines, accounting for excludes. Called internally by Renderer.
    ea::string GetEffectiveHullShaderDefines() const { return GetEffectiveShaderDefines(HS); }
    /// Return the effective domain shader defines, accounting for excludes. Called internally by Renderer.
    ea::string GetEffectiveDomainShaderDefines() const { return GetEffectiveShaderDefines(DS); }

private:
    /// Set the name of specific shader stage.
    void SetShader(ShaderType type, const ea::string& name);
    /// Set the preprocessor definitions for a shader stage.
    void SetShaderDefines(ShaderType type, const ea::string& defines);
    /// Set the preprocessor exclusions for a shader stage.
    void SetShaderExcludeDefines(ShaderType type, const ea::string& excludeDefines);
    /// Return the requested shaders with the provided additional definitions from the renderpath.
    ea::vector<SharedPtr<ShaderVariation> >& GetShaders(ShaderType type, const StringHash& extraDefinesHash);
    /// Returns the effective preprocessor definitions for a shader stage.
    ea::string GetEffectiveShaderDefines(ShaderType type) const;

    /// Encapsulates shader data for a pipeline stage.
    struct ShaderData
    {
        /// Name of the shader.
        ea::string shaderName_;
        /// Preprocessor definitions for the shader.
        ea::string defines_;
        /// Excluded preprocessor definitions.
        ea::string defineExcludes_;
        /// List of shader permutations.
        ea::vector<SharedPtr<ShaderVariation> > shaders_;
        /// Additional shaders with extra defines from the renderpath.
        ea::unordered_map<StringHash, ea::vector<SharedPtr<ShaderVariation> > > extraShaders_;
        /// Indicates whether a shader is used for the stage this data represents.
        bool exists_{};
    };

    /// Returns the appropriate shader-data to use internally for defines/excludes.
    ShaderData& GetShaderData(ShaderType type);
    /// Returns the appropriate shader-data to use internally for defines/excludes.
    const ShaderData& GetShaderData(ShaderType type) const;

    /// Pass index.
    unsigned index_;
    /// Blend mode.
    BlendMode blendMode_;
    /// Culling mode.
    CullMode cullMode_;
    /// Depth compare mode.
    CompareMode depthTestMode_;
    /// Lighting mode.
    PassLightingMode lightingMode_;
    /// Last shaders loaded frame number.
    unsigned shadersLoadedFrameNumber_;
    /// Depth write mode.
    bool depthWrite_;
    /// Alpha-to-coverage mode.
    bool alphaToCoverage_;
    /// Require desktop level hardware flag.
    bool isDesktop_;
    /// Vertex shader data.
    ShaderData vertexShaderData_;
    /// Pixel shader data.
    ShaderData pixelShaderData_;
    /// Geometry shader data.
    ShaderData geometryShaderData_;
    /// TCS shader data.
    ShaderData hullShaderData_;
    /// TES shader data.
    ShaderData domainShaderData_;

    /// Pass name.
    ea::string name_;
};

/// %Material technique. Consists of several passes.
class URHO3D_API Technique : public Resource
{
    URHO3D_OBJECT(Technique, Resource);

    friend class Renderer;

public:
    /// Construct.
    explicit Technique(Context* context);
    /// Destruct.
    ~Technique() override;
    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    bool BeginLoad(Deserializer& source) override;

    /// Set whether requires desktop level hardware.
    void SetIsDesktop(bool enable);
    /// Create a new pass.
    Pass* CreatePass(const ea::string& name);
    /// Remove a pass.
    void RemovePass(const ea::string& name);
    /// Reset shader pointers in all passes.
    void ReleaseShaders();
    /// Clone the technique. Passes will be deep copied to allow independent modification.
    SharedPtr<Technique> Clone(const ea::string& cloneName = EMPTY_STRING) const;

    /// Return whether requires desktop level hardware.
    bool IsDesktop() const { return isDesktop_; }
    /// Return whether geometry shader functionality is required.
    bool RequiresGeometryShader() const { return requireGeometryShaderSupport_; }
    /// Return whether tessellation shader functionality is required.
    bool RequiresTessellation() const { return requireTessellationSupport_; }

    /// Return whether technique is supported by the current hardware.
    bool IsSupported() const { return (!isDesktop_ || desktopSupport_) && (!requireGeometryShaderSupport_ || geometryShaderSupport_) && (!requireTessellationSupport_ || tessellationSupport_); }

    /// Return whether has a pass.
    bool HasPass(unsigned passIndex) const { return passIndex < passes_.size() && passes_[passIndex] != nullptr; }

    /// Return whether has a pass by name. This overload should not be called in time-critical rendering loops; use a pre-acquired pass index instead.
    bool HasPass(const ea::string& name) const;

    /// Return a pass, or null if not found.
    Pass* GetPass(unsigned passIndex) const { return passIndex < passes_.size() ? passes_[passIndex] : nullptr; }

    /// Return a pass by name, or null if not found. This overload should not be called in time-critical rendering loops; use a pre-acquired pass index instead.
    Pass* GetPass(const ea::string& name) const;

    /// Return a pass that is supported for rendering, or null if not found.
    Pass* GetSupportedPass(unsigned passIndex) const
    {
        Pass* pass = passIndex < passes_.size() ? passes_[passIndex] : nullptr;
        return pass && (!pass->IsDesktop() || desktopSupport_) ? pass : nullptr;
    }

    /// Return a supported pass by name. This overload should not be called in time-critical rendering loops; use a pre-acquired pass index instead.
    Pass* GetSupportedPass(const ea::string& name) const;

    /// Return number of passes.
    unsigned GetNumPasses() const;
    /// Return all pass names.
    ea::vector<ea::string> GetPassNames() const;
    /// Return all passes.
    ea::vector<Pass*> GetPasses() const;

    /// Return a clone with added shader compilation defines. Called internally by Material.
    SharedPtr<Technique> CloneWithDefines(const ea::string& vsDefines, const ea::string& psDefines, const ea::string& gsDefines = ea::string(), const ea::string& tcsDefines = ea::string(), const ea::string& tesDefines = ea::string());

    /// Return a pass type index by name. Allocate new if not used yet.
    static unsigned GetPassIndex(const ea::string& passName);

    /// Index for base pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned basePassIndex;
    /// Index for alpha pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned alphaPassIndex;
    /// Index for prepass material pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned materialPassIndex;
    /// Index for deferred G-buffer pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned deferredPassIndex;
    /// Index for per-pixel light pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned lightPassIndex;
    /// Index for lit base pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned litBasePassIndex;
    /// Index for lit alpha pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned litAlphaPassIndex;
    /// Index for shadow pass. Initialized once GetPassIndex() has been called for the first time.
    static unsigned shadowPassIndex;

private:
    /// Require desktop GPU flag.
    bool isDesktop_;
    /// Requires GS support to use.
    bool requireGeometryShaderSupport_;
    /// Requires tessellation support to use.
    bool requireTessellationSupport_;
    /// Cached desktop GPU support flag.
    bool desktopSupport_;
    /// Cached GS support flag.
    bool geometryShaderSupport_;
    /// Cached tessellation support flag.
    bool tessellationSupport_;
    /// Passes.
    ea::vector<SharedPtr<Pass> > passes_;
    /// Cached clones with added shader compilation defines.
    ea::unordered_map<ea::pair<StringHash, StringHash>, SharedPtr<Technique> > cloneTechniques_;

    /// Pass index assignments.
    static ea::unordered_map<ea::string, unsigned> passIndices;
};

}
