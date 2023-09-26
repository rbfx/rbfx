// Copyright (c) 2023-2023 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Container/Ptr.h"
#include "Urho3D/SystemUI/3rdParty/ImGuiDiligentRenderer.hpp"

#include <EASTL/vector.h>

namespace Urho3D
{

class PipelineState;
class RenderDevice;

class ImGuiDiligentRendererEx : private Diligent::ImGuiDiligentRenderer
{
public:
    ImGuiDiligentRendererEx(RenderDevice* renderDevice);
    ~ImGuiDiligentRendererEx();
    void NewFrame();
    void RenderDrawData(ImDrawData* drawData);
    void RenderSecondaryWindows();

private:
    void RenderDrawDataWith(ImDrawData* drawData, PipelineState* pipelineState);

#if URHO3D_PLATFORM_WINDOWS || URHO3D_PLATFORM_LINUX || URHO3D_PLATFORM_MACOS
    void CreatePlatformWindow(ImGuiViewport* viewport);

    void CreateRendererWindow(ImGuiViewport* viewport);
    void DestroyRendererWindow(ImGuiViewport* viewport);
    void SetWindowSize(ImGuiViewport* viewport, ImVec2 size);
    void RenderWindow(ImGuiViewport* viewport, void* renderArg);
    void SwapBuffers(ImGuiViewport* viewport, void* renderArg);

    void CreateSwapChainForViewport(ImGuiViewport* viewport);
#endif

private:
    RenderDevice* renderDevice_{};
    void (*createPlatformWindow_)(ImGuiViewport* viewport);
    bool isCachedStateInvalid_{};

    SharedPtr<PipelineState> primaryPipelineState_;
    SharedPtr<PipelineState> secondaryPipelineState_;

    ea::vector<ImGuiViewport*> viewports_;
};

} // namespace Diligent
