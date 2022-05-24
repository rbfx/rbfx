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

#include "FiniteTimeAction.h"

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "Urho3D/IO/ArchiveSerializationBasic.h"

using namespace Urho3D;

namespace 
{
/// "No-operation" finite time action for irreversible actions.
class URHO3D_API NoAction : public FiniteTimeAction
{
public:
    /// Construct.
    NoAction(Context* context, FiniteTimeAction* reversed);
    /// Destruct.
    ~NoAction() override;
    /// Create reversed action.
    virtual SharedPtr<FiniteTimeAction> Reverse() const;

private:
    SharedPtr<FiniteTimeAction> reversed_;
};
}

/// Construct.
FiniteTimeAction::FiniteTimeAction(Context* context)
    : BaseAction(context)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void FiniteTimeAction::SerializeInBlock(Archive& archive)
{
    BaseAction::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "duration", duration_, ea::numeric_limits<float>::epsilon());
}

float FiniteTimeAction::GetDuration() const { return duration_; }

void FiniteTimeAction::SetDuration(float duration)
{
    // Prevent division by 0
    if (duration < ea::numeric_limits<float>::epsilon())
        duration = ea::numeric_limits<float>::epsilon();

    duration_ = duration;
}

/// Create reversed action.
SharedPtr<FiniteTimeAction> FiniteTimeAction::Reverse() const { return MakeShared<NoAction>(context_, const_cast<FiniteTimeAction*>(this)); }

/// Construct.
NoAction::NoAction(Context* context, FiniteTimeAction* reversed)
    : FiniteTimeAction(context)
    , reversed_(reversed)
{
}

/// Destruct.
NoAction::~NoAction() {}

/// Create reversed action.
SharedPtr<FiniteTimeAction> NoAction::Reverse() const { return reversed_; }

void Urho3D::SerializeValue(Archive& archive, const char* name, SharedPtr<FiniteTimeAction>& value)
{
    SharedPtr<BaseAction> baseAction = value;
    SerializeValue(archive, name, baseAction);
    if (archive.IsInput())
    {
        value.DynamicCast(baseAction);
    }
}
