//
// Copyright (c) 2022 the rbfx project.
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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR rhs
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR rhsWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR rhs DEALINGS IN
// THE SOFTWARE.
//

#include "ShaderParameterFromTo.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/Graphics/Material.h"

using namespace Urho3D;

namespace
{
class ShaderParameterFromToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;

public:
    ShaderParameterFromToState(ShaderParameterFromTo* action, Object* target)
        : FiniteTimeActionState(action, target)
        , from_(action->GetFrom())
        , to_(action->GetTo())
        , name_(action->GetName())
    {
    }

    ~ShaderParameterFromToState() override = default;

    void Update(float time) override
    {
        auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            material->SetShaderParameter(name_, from_.Lerp(to_, time));
        }
    }
};

} // namespace

/// Construct.
ShaderParameterFromTo::ShaderParameterFromTo(Context* context)
    : FiniteTimeAction(context)
{
}

/// Construct.
ShaderParameterFromTo::ShaderParameterFromTo(
    Context* context, float duration, const ea::string& name, const Variant& from, const Variant& to)
    : FiniteTimeAction(context, duration)
    , name_(name)
    , from_(from)
    , to_(to)
{
}

/// Destruct.
ShaderParameterFromTo::~ShaderParameterFromTo() {}

/// Register object factory.
void ShaderParameterFromTo::RegisterObject(Context* context) { context->RegisterFactory<ShaderParameterFromTo>(); }

SharedPtr<ActionState> ShaderParameterFromTo::StartAction(Object* target)
{
    return MakeShared<ShaderParameterFromToState>(this, target);
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterFromTo::Reverse() const { return MakeShared<ShaderParameterFromTo>(context_, GetDuration(), name_, to_, from_); }
