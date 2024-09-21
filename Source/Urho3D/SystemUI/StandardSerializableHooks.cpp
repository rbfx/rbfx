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

#include "Urho3D/Precompiled.h"

#include "Urho3D/SystemUI/StandardSerializableHooks.h"

#include "Urho3D/Core/WorkQueue.h"
#include "Urho3D/Graphics/AnimatedModel.h"
#include "Urho3D/Graphics/Camera.h"
#include "Urho3D/RenderPipeline/RenderPipeline.h"
#include "Urho3D/SystemUI/SerializableInspectorWidget.h"
#include "Urho3D/SystemUI/SystemUI.h"
#include "Urho3D/SystemUI/Widgets.h"
#include "Urho3D/Utility/SceneRendererToTexture.h"

namespace Urho3D
{

void RegisterStandardSerializableHooks(Context* context)
{
    SerializableInspectorWidget::RegisterAttributeHook({AnimatedModel::GetTypeNameStatic(), "Morphs"},
        [](const AttributeHookContext& ctx, Variant& boxedValue)
    {
        if (boxedValue.GetType() != VAR_BUFFER)
            return false;

        ByteVector& value = *boxedValue.GetBufferPtr();

        Widgets::ItemLabel(ctx.info_->name_.c_str(), Widgets::GetItemLabelColor(ctx.isUndefined_, ctx.isDefaultValue_));
        const IdScopeGuard idScope(VAR_BUFFER);

        if (!value.empty())
            ui::NewLine();

        bool modified = false;
        for (unsigned morphIndex = 0; morphIndex < value.size(); ++morphIndex)
        {
            const IdScopeGuard elementIdScope(morphIndex);
            Widgets::ItemLabel(Format("> Morph #{}", morphIndex));

            float weight = value[morphIndex] / 255.0f;
            modified |= ui::DragFloat("", &weight, 1.0f / 255.0f, 0.0f, 1.0f, "%.3f");
            value[morphIndex] = static_cast<unsigned char>(Round(weight * 255.0f));
        }

        if (value.empty())
            ui::NewLine();

        return modified;
    });

    SerializableInspectorWidget::RegisterAttributeHook({RenderPipeline::GetTypeNameStatic(), "Render Passes"},
        [](const AttributeHookContext& ctx, Variant& boxedValue)
    {
        if (boxedValue.GetType() != VAR_VARIANTVECTOR)
            return false;

        VariantVector& value = *boxedValue.GetVariantVectorPtr();

        Widgets::ItemLabel(ctx.info_->name_.c_str(), Widgets::GetItemLabelColor(ctx.isUndefined_, ctx.isDefaultValue_));
        const IdScopeGuard idScope(VAR_BUFFER);

        if (!value.empty())
            ui::NewLine();

        ui::Indent();
        bool modified = false;
        for (unsigned passIndex = 0; passIndex < value.size() / 2; ++passIndex)
        {
            const IdScopeGuard elementIdScope(passIndex);

            const ea::string& name = value[passIndex * 2].GetString();
            bool enabled = value[passIndex * 2 + 1].GetBool();
            modified |= ui::Checkbox(name.c_str(), &enabled);
            value[passIndex * 2 + 1] = enabled;
        }
        ui::Unindent();

        if (value.empty())
            ui::NewLine();

        return modified;
    });


    SerializableInspectorWidget::RegisterObjectHook({Camera::GetTypeNameStatic(), ObjectHookType::Append},
        [context, widget = WeakPtr<SceneRendererToTexture>()](const WeakSerializableVector& objects) mutable
    {
        auto workQueue = context->GetSubsystem<WorkQueue>();

        if (objects.size() != 1)
            return;

        const auto camera = dynamic_cast<Camera*>(objects.front().Get());
        Scene* scene = camera ? camera->GetScene() : nullptr;
        if (!camera || !scene)
            return;

        if (!widget || widget->GetScene() != scene)
        {
            auto widgetHolder = MakeShared<SceneRendererToTexture>(scene);
            context->SetGlobalVar("Camera_Hook_Widget", MakeCustomValue(widgetHolder));
            widget = widgetHolder;
        }

        widget->SetActive(true);

        widget->GetCamera()->CopyAttributes(camera);
        widget->GetCamera()->SetDrawDebugGeometry(false);
        const auto reviewNode = widget->GetCameraNode();
        const auto cameraNode = camera->GetNode();
        reviewNode->CopyAttributes(cameraNode);
        reviewNode->SetWorldTransform(cameraNode->GetWorldTransform());

        workQueue->PostDelayedTaskForMainThread([widget] { widget->SetActive(false); });

        const float availableWidth = ui::GetContentRegionAvail().x;
        const float defaultAspectRatio = 16.0f / 9.0f;
        const float aspectRatio = camera->GetAutoAspectRatio() ? defaultAspectRatio : camera->GetAspectRatio();
        const auto textureSize = Vector2{availableWidth, availableWidth / aspectRatio}.ToIntVector2();

        widget->SetTextureSize(textureSize);
        widget->Update();

        Texture2D* sceneTexture = widget->GetTexture();
        Widgets::ImageItem(sceneTexture, ToImGui(sceneTexture->GetSize()));
    });
}

}
