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

#include "../Precompiled.h"

#include "../Graphics/AnimatedModel.h"
#include "../SystemUI/SerializableInspectorWidget.h"
#include "../SystemUI/StandardSerializableHooks.h"
#include "../SystemUI/SystemUI.h"
#include "../SystemUI/Widgets.h"

namespace Urho3D
{

void RegisterStandardSerializableHooks()
{
    SerializableInspectorWidget::RegisterHook({AnimatedModel::GetTypeNameStatic(), "Morphs"},
        [](const SerializableHookContext& ctx, Variant& boxedValue)
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
}

}
