// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Signal.h"
#include "Urho3D/RenderAPI/ShaderBytecode.h"
#include "Urho3D/RenderAPI/DeviceObject.h"
#include "Urho3D/RenderAPI/RenderAPIDefs.h"

#include <Diligent/Common/interface/RefCntAutoPtr.hpp>
#include <Diligent/Graphics/GraphicsEngine/interface/Shader.h>

namespace Urho3D
{

/// Base class for GPU shader.
/// It should be kept alive as long as the shader is used by any pipeline state.
/// Shader handle is null if shader is not loaded or failed to load.
class URHO3D_API RawShader : public Object, public DeviceObject
{
    URHO3D_OBJECT(RawShader, Object);

public:
    /// Signals that the shader has been reloaded and dependent pipeline states should be recreated.
    Signal<void()> OnReloaded;

    RawShader(Context* context, ShaderBytecode bytecode);
    ~RawShader() override;

    /// Implement DeviceObject.
    /// @{
    void Invalidate() override;
    void Restore() override;
    void Destroy() override;
    /// @}

    /// Getters.
    /// @{
    const ShaderBytecode& GetBytecode() const { return bytecode_; }
    ShaderType GetShaderType() const { return bytecode_.type_; }
    Diligent::IShader* GetHandle() const { return handle_; }
    /// @}

protected:
    RawShader(Context* context, ShaderType type);

    /// Create shader from platform-specific binary.
    void CreateFromBinary(ShaderBytecode bytecode);

private:
    void CreateGPU();
    void DestroyGPU();

    ShaderBytecode bytecode_;
    Diligent::RefCntAutoPtr<Diligent::IShader> handle_;

};

} // namespace Urho3D
