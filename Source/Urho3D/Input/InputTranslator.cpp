//
// Copyright (c) 2023-2023 the rbfx project.
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

#include "Urho3D/Core/Context.h"
#include "Urho3D/Input/InputTranslator.h"

#include "Input.h"
#include "Urho3D/Core/CoreEvents.h"

namespace Urho3D
{

InputTranslator::InputTranslator(Context* context)
    : BaseClassName(context)
{
}

void InputTranslator::RegisterObject(Context* context)
{
    context->AddFactoryReflection<InputTranslator>();
}

void InputTranslator::SetEnabled(bool state)
{
    if (state != enabled_)
    {
        enabled_ = state;
        if (enabled_)
        {
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(InputTranslator, HandleUpdate));
        }
        else
        {
            UnsubscribeFromEvent(E_UPDATE);
        }
    }
}

void InputTranslator::SetMap(InputMap* map)
{
    map_ = map;
}

float InputTranslator::EvaluateActionState(const ea::string& action) const
{
    if (!map_)
        return 0.0f;
    auto& mapping = map_->GetMapping(action);
    return mapping.Evaluate(GetSubsystem<Input>());
}

void InputTranslator::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!map_)
        return;
    auto input = GetSubsystem<Input>();
}

} // namespace Urho3D
