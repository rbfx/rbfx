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

#include "../Core/Object.h"
#include "../RenderPipeline/RenderPipelineDefs.h"

#include <EASTL/utility.h>

namespace Urho3D
{

class Light;
class CameraProcessor;

/// Description of shader program used for rendering.
///
/// Shader name may be different for different stages,
/// but this use case is obsolete and may not be fully supported by tools.
/// For best compatibility use same shader for all stages.
///
/// Shader defines should be the same for all stages, with one exception:
/// Defines that are ignored by the stage may be omitted in the stage defines list.
///
/// These restrictions are imposed to simplify possible shader preprocessing.
///
/// TODO: Consider replacing define string with tokenized define lists
struct ShaderProgramDesc
{
    ea::string shaderName_[MAX_SHADER_TYPES];
    ea::string shaderDefines_[MAX_SHADER_TYPES];
    ea::string commonShaderDefines_;

    /// Hints about what the shader program is
    /// @{
    bool isInstancingUsed_{};
    /// @}

    void Clear()
    {
        for (ea::string& shaderName : shaderName_)
            shaderName.clear();

        for (ea::string& shaderDefines : shaderDefines_)
            shaderDefines.clear();
        commonShaderDefines_.clear();

        isInstancingUsed_ = false;
    }

    void AddCommonShaderDefines(ea::string_view defines)
    {
        commonShaderDefines_.append(defines.begin(), defines.end());
        if (!defines.ends_with(' '))
            commonShaderDefines_ += ' ';
    }

    void AddShaderDefines(ShaderType type, ea::string_view defines)
    {
        shaderDefines_[type].append(defines.begin(), defines.end());
        if (!defines.ends_with(' '))
            shaderDefines_[type] += ' ';
    }
};

/// Generates shader program descritpions for scene and light volume batches.
class URHO3D_API ShaderProgramCompositor : public Object
{
    URHO3D_OBJECT(ShaderProgramCompositor, Object);

public:
    explicit ShaderProgramCompositor(Context* context);
    void SetSettings(const ShaderProgramCompositorSettings& settings);
    void SetFrameSettings(const CameraProcessor* cameraProcessor, bool linearColorSpace);

    /// Process batches
    /// @{
    void ProcessUserBatch(ShaderProgramDesc& result, const DrawableProcessorPassFlags flags,
        Drawable* drawable, Geometry* geometry, GeometryType geometryType, Material* material, Pass* pass,
        Light* light, bool hasShadow, BatchCompositorSubpass subpass);
    void ProcessShadowBatch(ShaderProgramDesc& result,
        Geometry* geometry, GeometryType geometryType, Material* material, Pass* pass, Light* light);
    void ProcessLightVolumeBatch(ShaderProgramDesc& result, Geometry* geometry, GeometryType geometryType,
        Pass* pass, Light* light, bool hasShadow);
    /// @}

private:
    /// Apply common defines
    /// @{
    void SetupShaders(ShaderProgramDesc& result, Pass* pass);
    void ApplyCommonDefines(ShaderProgramDesc& result, DrawableProcessorPassFlags flags, Pass* pass) const;
    void ApplyGeometryVertexDefines(ShaderProgramDesc& result,
        DrawableProcessorPassFlags flags, Geometry* geometry, GeometryType geometryType) const;
    void ApplyPixelLightPixelAndCommonDefines(ShaderProgramDesc& result,
        Light* light, bool hasShadow, bool materialHasSpecular) const;
    void ApplyNormalTangentSpaceDefines(
        ShaderProgramDesc& result, GeometryType geometryType, VertexBuffer* vertexBuffer) const;
    /// @}

    bool IsInstancingUsed(DrawableProcessorPassFlags flags, Geometry* geometry, GeometryType geometryType) const;
    ea::pair<bool, bool> IsNormalAndTangentAvailable(GeometryType geometryType, VertexBuffer* vertexBuffer) const;

    /// Apply user pass defines
    /// @{
    void ApplyLayoutVertexAndCommonDefinesForUserPass(ShaderProgramDesc& result, VertexBuffer* vertexBuffer) const;
    void ApplyMaterialPixelDefinesForUserPass(ShaderProgramDesc& result, Material* material, Pass* pass) const;
    void ApplyAmbientLightingVertexAndCommonDefinesForUserPass(ShaderProgramDesc& result,
        Drawable* drawable, bool isGeometryBufferPass) const;
    /// @}

    /// Apply internal pass defines
    /// @{
    void ApplyDefinesForShadowPass(ShaderProgramDesc& result,
        Light* light, VertexBuffer* vertexBuffer, Material* material, Pass* pass) const;
    void ApplyDefinesForLightVolumePass(ShaderProgramDesc& result) const;
    /// @}

    /// External configuration
    /// @{
    ShaderProgramCompositorSettings settings_;
    bool isCameraOrthographic_{};
    bool isCameraClipped_{};
    bool isCameraReversed_{};
    bool linearColorSpace_{};
    /// @}
};

}
