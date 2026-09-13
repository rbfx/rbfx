// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/Graphics/GraphicsDefs.h"
#include "Urho3D/Graphics/Graphics.h"

#include <EASTL/span.h>

namespace Urho3D
{

struct FrameInfo;

/// Utility to process render camera (not cull camera!).
class URHO3D_API CameraProcessor : public Object
{
    URHO3D_OBJECT(CameraProcessor, Object);

public:
    explicit CameraProcessor(Context* context);
    void SetCameras(ea::span<Camera* const> cameras);

    unsigned GetPipelineStateHash() const;
    bool IsCameraReversed() const;
    bool IsCameraOrthographic() const { return isCameraOrthographic_; }
    bool IsCameraClipped() const { return isCameraClipped_; }
    FillMode GetCameraFillMode() const { return cameraFillMode_; }

    /// Callbacks from SceneProcessor
    /// @{
    void OnUpdateBegin(const FrameInfo& frameInfo);
    void OnRenderBegin(const FrameInfo& frameInfo);
    void OnRenderEnd(const FrameInfo& frameInfo);
    /// @}

private:
    void UpdateCamera(const FrameInfo& frameInfo, Camera* camera);

    bool isCameraOrthographic_{};
    bool isCameraFlippedByUser_{};
    bool isReflectionCamera_{};
    bool isCameraClipped_{};
    bool flipCameraForRendering_{};
    FillMode cameraFillMode_{};
    ea::vector<WeakPtr<Camera>> cameras_{};

    /// Graphics instance
    WeakPtr<Graphics> graphics_;
};

}
