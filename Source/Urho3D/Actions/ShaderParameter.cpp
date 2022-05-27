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

#include "ShaderParameter.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "FiniteTimeActionState.h"
#include "Urho3D/Graphics/Material.h"

namespace Urho3D
{
namespace Actions
{

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

    void Update(float time) override
    {
        auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            material->SetShaderParameter(name_, from_.Lerp(to_, time));
        }
    }
};

class ShaderParameterToState : public FiniteTimeActionState
{
    Variant from_;
    Variant to_;
    ea::string name_;

public:
    ShaderParameterToState(ShaderParameterTo* action, Object* target)
        : FiniteTimeActionState(action, target)
        , to_(action->GetTo())
        , name_(action->GetName())
    {
        const auto* material = GetTarget()->Cast<Material>();
        if (material)
        {
            from_ = material->GetShaderParameter(name_);
            if (from_.GetType() != to_.GetType())
            {
                from_ = to_;
            }
        }
    }

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

/// --------------------------------------------------
/// Construct.
ShaderParameterAction::ShaderParameterAction(Context* context)
    : BaseClassName(context)
{
}

// Get shader parameter name
void ShaderParameterAction::SetName(ea::string_view name) { name_ = name; }

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterAction::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeValue(archive, "name", name_);
}


/// --------------------------------------------------
/// Construct.
ShaderParameterTo::ShaderParameterTo(Context* context)
    : BaseClassName(context)
{
}

// Get "to" value.
void ShaderParameterTo::SetTo(const Variant& variant) { to_ = variant; }

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "to", to_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterTo::StartAction(Object* target)
{
    return MakeShared<ShaderParameterToState>(this, target);
}

/// --------------------------------------------------
/// Construct.
ShaderParameterFromTo::ShaderParameterFromTo(Context* context)
    : BaseClassName(context)
{
}

// Set "from" value.
void ShaderParameterFromTo::SetFrom(const Variant& variant) { from_ = variant; }

/// Create reversed action.
SharedPtr<FiniteTimeAction> ShaderParameterFromTo::Reverse() const
{
    auto result = MakeShared<ShaderParameterFromTo>(context_);
    result->SetDuration(GetDuration());
    result->SetName(GetName());
    result->SetFrom(GetTo());
    result->SetTo(from_);
    return result;
}

/// Serialize content from/to archive. May throw ArchiveException.
void ShaderParameterFromTo::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "from", from_, Variant::EMPTY);
}

/// Create new action state from the action.
SharedPtr<ActionState> ShaderParameterFromTo::StartAction(Object* target)
{
    return MakeShared<ShaderParameterFromToState>(this, target);
}

} // namespace Actions
} // namespace Urho3D
