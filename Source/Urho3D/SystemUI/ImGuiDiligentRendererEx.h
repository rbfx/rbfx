// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include <Urho3D/Urho3D.h>

#include "Urho3D/SystemUI/3rdParty/ImGuiDiligentRenderer.hpp"

#ifdef CreateWindow
#    undef CreateWindow
#endif

namespace Urho3D
{

class RenderDevice;

class ImGuiDiligentRendererEx : private Diligent::ImGuiDiligentRenderer
{
public:
    ImGuiDiligentRendererEx(RenderDevice* renderDevice);
    ~ImGuiDiligentRendererEx();
    void NewFrame();
    void EndFrame();
    void RenderDrawData(ImDrawData* drawData);
    void RenderSecondaryWindows();

private:
#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
    void CreatePlatformWindow(ImGuiViewport* viewport);

    void CreateWindow(ImGuiViewport* viewport);
    void DestroyWindow(ImGuiViewport* viewport);
    void SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    void RenderWindow(ImGuiViewport* viewport, void* renderArg);
    void SwapBuffers(ImGuiViewport* viewport, void* renderArg);
#endif

private:
    RenderDevice* renderDevice_{};
    void (*createPlatformWindow_)(ImGuiViewport* viewport);
};

} // namespace Diligent
