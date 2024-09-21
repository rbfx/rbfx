// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/RenderAPI/RenderScope.h"

#include "Urho3D/IO/Log.h"
#include "Urho3D/RenderAPI/GAPIIncludes.h"
#include "Urho3D/RenderAPI/RenderContext.h"
#include "Urho3D/RenderAPI/RenderDevice.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

namespace
{

void ConsumeOpenGLError(RenderDevice* renderDevice)
{
#if GL_SUPPORTED || GLES_SUPPORTED
    // Workaround for driver bug.
    if (renderDevice->GetBackend() == RenderBackend::OpenGL)
        (void)glGetError();
#endif
}

}

void RenderScope::BeginGroup(ea::string_view name)
{
    renderContext_->GetHandle()->BeginDebugGroup(name.data());
    ConsumeOpenGLError(renderContext_->GetRenderDevice());
}

void RenderScope::EndGroup()
{
    renderContext_->GetHandle()->EndDebugGroup();
    ConsumeOpenGLError(renderContext_->GetRenderDevice());
}

} // namespace Urho3D
