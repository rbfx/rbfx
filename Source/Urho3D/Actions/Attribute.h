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

#pragma once

#include "AttributeAction.h"

namespace Urho3D
{
namespace Actions
{
/// Animate attribute between two values.
class URHO3D_API AttributeFromTo : public AttributeAction
{
    URHO3D_OBJECT(AttributeFromTo, AttributeAction)

public:
    /// Construct.
    explicit AttributeFromTo(Context* context);

    // Set "from" value.
    void SetFrom(const Variant&);
    // Get "to" value.
    void SetTo(const Variant&);

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }
    // Get "to" value.
    const Variant& GetTo() const { return to_; }

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant from_;
    Variant to_;
};

/// Animate attribute between current and provided value.
class URHO3D_API AttributeTo : public AttributeAction
{
    URHO3D_OBJECT(AttributeTo, AttributeAction)

public:
    /// Construct.
    explicit AttributeTo(Context* context);

    // Get "to" value.
    void SetTo(const Variant&);

    // Get "to" value.
    const Variant& GetTo() const { return to_; }

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant to_;
};

/// Animate attribute between two values.
class URHO3D_API AttributeBlink : public AttributeAction
{
    URHO3D_OBJECT(AttributeBlink, AttributeAction)

public:
    /// Construct.
    explicit AttributeBlink(Context* context);

    // Set "from" value.
    void SetFrom(const Variant&);
    // Get "to" value.
    void SetTo(const Variant&);
    /// Set number of blinks.
    void SetNumOfBlinks(unsigned times) { times_ = Max(1, times); };

    // Get "from" value.
    const Variant& GetFrom() const { return from_; }
    // Get "to" value.
    const Variant& GetTo() const { return to_; }
    /// Get number of blinks.
    unsigned GetNumOfBlinks() const { return times_; };

    /// Create reversed action.
    SharedPtr<FiniteTimeAction> Reverse() const override;

    /// Serialize content from/to archive. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;

protected:
    /// Create new action state from the action.
    SharedPtr<ActionState> StartAction(Object* target) override;

private:
    Variant from_;
    Variant to_;
    unsigned times_{1};
};

} // namespace Actions
} // namespace Urho3D
