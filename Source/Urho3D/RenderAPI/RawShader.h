//
// Copyright (c) 2008-2022 the Urho3D project.
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
