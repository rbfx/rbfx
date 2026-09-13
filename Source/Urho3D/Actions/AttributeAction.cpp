// Copyright (c) 2015 Xamarin Inc.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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




