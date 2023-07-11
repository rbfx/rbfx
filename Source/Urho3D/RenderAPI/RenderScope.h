// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Context.h"
#include "Urho3D/Core/NonCopyable.h"
#include "Urho3D/RenderAPI/RenderContext.h"

#include <Diligent/Graphics/GraphicsEngine/interface/DeviceContext.h>

namespace Urho3D
{

/// Utility class to add debug scope markers.
class URHO3D_API RenderScope : public NonCopyable
{
public:
    explicit RenderScope(RenderContext* renderContext, ea::string_view name)
        : renderContext_(renderContext)
    {
        if (renderContext_)
            BeginGroup(name);
    }

    template <class ... T>
    explicit RenderScope(RenderContext* renderContext, ea::string_view format, T&& ... args)
        : renderContext_(renderContext)
    {
        if (renderContext_)
            BeginGroup(Format(format, args...));
    }

    ~RenderScope()
    {
        if (renderContext_)
            renderContext_->GetHandle()->EndDebugGroup();
    }

private:
    void BeginGroup(ea::string_view name)
    {
        renderContext_->GetHandle()->BeginDebugGroup(name.data());
    }

    RenderContext* renderContext_{};
};

} // namespace Urho3D
