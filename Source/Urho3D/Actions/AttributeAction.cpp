//
// Copyright (c) 2015 Xamarin Inc.
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

#include "../Core/Context.h"
#include "../IO/Log.h"
#include "ActionManager.h"
#include "AttributeAction.h"
#include "../IO/ArchiveSerializationBasic.h"

namespace Urho3D
{
namespace Actions
{

/// Construct.
AttributeAction::AttributeAction(Context* context)
    : BaseClassName(context)
{
}

/// Construct.
AttributeAction::AttributeAction(Context* context, ea::string_view animatedAttribute)
    : BaseClassName(context)
    , animatedAttribute_(animatedAttribute)
{
}

/// Serialize content from/to archive. May throw ArchiveException.
void AttributeAction::SerializeInBlock(Archive& archive)
{
    BaseClassName::SerializeInBlock(archive);
    SerializeOptionalValue(archive, "attribute", animatedAttribute_, EMPTY_STRING);
}

const ea::string& AttributeAction::GetAttributeName() const
{
    return animatedAttribute_;
}

void AttributeAction::SetAttributeName(ea::string_view animatedAttribute)
{
    animatedAttribute_ = animatedAttribute;
}

AttributeInfo* AttributeAction::GetAttribute(Object* target)
{
    const auto serializable = target->Cast<Serializable>();
    if (!serializable)
    {
        URHO3D_LOGERROR(Format("Can animate only serializable class but {} is not serializable.", target->GetTypeName()));
        return nullptr;
    }

    const auto attribute = target->GetContext()->GetReflection(target->GetType())->GetAttribute(animatedAttribute_);
    if (!attribute)
    {
        URHO3D_LOGERROR(Format("Attribute {} not found in {}.", animatedAttribute_, target->GetTypeName()));
    }
    return attribute;
}


} // namespace Actions
} // namespace Urho3D




