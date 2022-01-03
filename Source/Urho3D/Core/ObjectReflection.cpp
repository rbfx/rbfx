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

#include "../Core/Assert.h"
#include "../Core/Object.h"
#include "../Core/ObjectReflection.h"
#include "../IO/Log.h"

#include "../DebugNew.h"

namespace Urho3D
{

namespace
{

unsigned FindIndexInCategory(const ea::vector<StringHash>& objectsInCategory, StringHash typeNameHash)
{
    // Optimize for most recent object
    if (!objectsInCategory.empty() && objectsInCategory.back() == typeNameHash)
        return objectsInCategory.size() - 1;

    return objectsInCategory.index_of(typeNameHash);
}

}

ObjectReflection::ObjectReflection(Context* context, const TypeInfo* typeInfo)
    : context_(context)
    , typeInfo_(typeInfo)
{
}

ObjectReflection::ObjectReflection(Context* context, ea::unique_ptr<TypeInfo> typeInfo)
    : context_(context)
    , typeInfo_(typeInfo.get())
    , ownedTypeInfo_(ea::move(typeInfo))
{
}

SharedPtr<Object> ObjectReflection::CreateObject()
{
    return createObject_ ? createObject_(typeInfo_, context_) : nullptr;
}

AttributeHandle ObjectReflection::AddAttribute(const AttributeInfo& attr)
{
    // None or pointer types can not be supported
    if (attr.type_ == VAR_NONE || attr.type_ == VAR_VOIDPTR || attr.type_ == VAR_PTR)
    {
        URHO3D_LOGWARNING("Attempt to register unsupported attribute type {} to class {}",
            Variant::GetTypeName(attr.type_), GetTypeName());
        return AttributeHandle();
    }

    // Only SharedPtr<> of Serializable or it's subclasses are supported in attributes
    if (attr.type_ == VAR_CUSTOM && !attr.defaultValue_.IsCustomType<SharedPtr<Serializable>>())
    {
        URHO3D_LOGWARNING("Attempt to register unsupported attribute of custom type to class {}", GetTypeName());
        return AttributeHandle();
    }

    AttributeHandle handle;
    attributes_.push_back(attr);
    attributeNames_.push_back(attr.nameHash_);
    handle.attributeInfo_ = &attributes_.back();
    if (attr.mode_ & AM_NET)
    {
        networkAttributes_.push_back(attr);
        handle.networkAttributeInfo_ = &networkAttributes_.back();
    }
    return handle;

}

void ObjectReflection::RemoveAttribute(StringHash nameHash)
{
    const unsigned index = attributeNames_.index_of(nameHash);
    if (index >= attributeNames_.size())
    {
        URHO3D_LOGWARNING("Cannot find attribute {}", nameHash.ToDebugString());
        return;
    }

    const bool isNetwork = attributes_[index].mode_.Test(AM_NET);

    attributes_.erase_at(index);
    attributeNames_.erase_at(index);

    if (isNetwork)
    {
        const auto iter = ea::find_if(networkAttributes_.begin(), networkAttributes_.end(),
            [&](const AttributeInfo& info) { return info.nameHash_ == nameHash; });

        if (iter == networkAttributes_.end())
            URHO3D_LOGERROR("Cannot find network attribute {}", nameHash.ToDebugString());
        else
            networkAttributes_.erase(iter);
    }
}

void ObjectReflection::RemoveAllAttributes()
{
    attributes_.clear();
    attributeNames_.clear();
    networkAttributes_.clear();
}

void ObjectReflection::CopyAttributesFrom(const ObjectReflection* other)
{
    if (!other)
    {
        URHO3D_LOGWARNING("Attempt to copy base attributes from unknown base class for class " + GetTypeName());
        return;
    }

    // Prevent endless loop if mistakenly copying attributes from same class as derived
    if (other == this)
    {
        URHO3D_LOGWARNING("Attempt to copy base attributes to itself for class " + GetTypeName());
        return;
    }

    attributes_.append(other->attributes_);
    attributeNames_.append(other->attributeNames_);
    networkAttributes_.append(other->networkAttributes_);
}

void ObjectReflection::UpdateAttributeDefaultValue(StringHash nameHash, const Variant& defaultValue)
{
    AttributeInfo* info = GetAttribute(nameHash);
    if (info)
        info->defaultValue_ = defaultValue;
    else
        URHO3D_LOGWARNING("Cannot find attribute {}", nameHash.ToDebugString());
}

unsigned ObjectReflection::GetAttributeIndex(StringHash nameHash) const
{
    const unsigned index = attributeNames_.index_of(nameHash);
    return index < attributeNames_.size() ? index : M_MAX_UNSIGNED;
}

unsigned ObjectReflection::GetAttributeIndex(StringHash nameHash, unsigned hintIndex) const
{
    const unsigned numAttributes = attributeNames_.size();
    hintIndex = ea::min(hintIndex, numAttributes);

    for (unsigned i = hintIndex; i < numAttributes; ++i)
    {
        if (attributeNames_[i] == nameHash)
            return i;
    }
    for (unsigned i = 0; i < hintIndex; ++i)
    {
        if (attributeNames_[i] == nameHash)
            return i;
    }
    return M_MAX_UNSIGNED;
}

AttributeInfo* ObjectReflection::GetAttribute(StringHash nameHash)
{
    return const_cast<AttributeInfo*>(const_cast<const ObjectReflection*>(this)->GetAttribute(nameHash));
}

const AttributeInfo* ObjectReflection::GetAttribute(StringHash nameHash) const
{
    const unsigned index = attributeNames_.index_of(nameHash);
    return index < attributeNames_.size() ? &attributes_[index] : nullptr;
}

ObjectReflectionRegistry::ObjectReflectionRegistry(Context* context)
    : context_(context)
{
}

ObjectReflection* ObjectReflectionRegistry::Reflect(const TypeInfo* typeInfo)
{
    if (!typeInfo)
    {
        URHO3D_LOGWARNING("Attempt to reflect class without TypeInfo");
        return nullptr;
    }

    const StringHash typeNameHash = typeInfo->GetType();
    const auto iter = reflections_.find(typeNameHash);
    if (iter == reflections_.end())
    {
        const auto reflection = MakeShared<ObjectReflection>(context_, typeInfo);
        reflections_.emplace(typeNameHash, reflection);
        AddReflectionToCurrentCategory(reflection);
        return reflection;
    }

    return iter->second;
}

ObjectReflection* ObjectReflectionRegistry::ReflectCustomType(ea::unique_ptr<TypeInfo> typeInfo)
{
    if (!typeInfo)
    {
        URHO3D_LOGWARNING("Attempt to reflect class without TypeInfo");
        return nullptr;
    }

    const StringHash typeNameHash = typeInfo->GetType();
    const auto iter = reflections_.find(typeNameHash);
    if (iter == reflections_.end())
    {
        const auto reflection = MakeShared<ObjectReflection>(context_, ea::move(typeInfo));
        reflections_.emplace(typeNameHash, reflection);
        AddReflectionToCurrentCategory(reflection);
        return reflection;
    }

    return iter->second;
}

ObjectReflection* ObjectReflectionRegistry::GetReflection(StringHash typeNameHash)
{
    return const_cast<ObjectReflection*>(const_cast<const ObjectReflectionRegistry*>(this)->GetReflection(typeNameHash));
}

const ObjectReflection* ObjectReflectionRegistry::GetReflection(StringHash typeNameHash) const
{
    const auto iter = reflections_.find(typeNameHash);
    return iter != reflections_.end() ? iter->second : nullptr;
}

void ObjectReflectionRegistry::SetReflectionCategory(const TypeInfo* typeInfo, ea::string_view category)
{
    const auto reflection = Reflect(typeInfo);

    RemoveReflectionFromCurrentCategory(reflection);
    reflection->SetCategory(category);
    AddReflectionToCurrentCategory(reflection);
}

void ObjectReflectionRegistry::RemoveReflection(StringHash typeNameHash)
{
    const auto iter = reflections_.find(typeNameHash);
    if (iter == reflections_.end())
    {
        ErrorReflectionNotFound(typeNameHash);
        return;
    }

    const auto reflection = iter->second;
    RemoveReflectionFromCurrentCategory(reflection);
    reflections_.erase(iter);
}

SharedPtr<Object> ObjectReflectionRegistry::CreateObject(StringHash typeNameHash)
{
    const auto iter = reflections_.find(typeNameHash);
    return iter != reflections_.end() ? iter->second->CreateObject() : nullptr;
}

void ObjectReflectionRegistry::ErrorReflectionNotFound(StringHash typeNameHash) const
{
    URHO3D_LOGWARNING("Reflection of object {} is not found", typeNameHash.ToDebugString());
}

void ObjectReflectionRegistry::ErrorDuplicateReflection(StringHash typeNameHash) const
{
    URHO3D_ASSERTLOG(0, "Object {} is reflectied multiple times.", typeNameHash.ToDebugString());
}

void ObjectReflectionRegistry::AddReflectionToCurrentCategory(ObjectReflection* reflection)
{
    const ea::string& category = reflection->GetCategory();
    auto& objectsInCategory = categories_[category];
    objectsInCategory.push_back(reflection->GetTypeNameHash());
}

void ObjectReflectionRegistry::RemoveReflectionFromCurrentCategory(ObjectReflection* reflection)
{
    const ea::string& oldCategory = reflection->GetCategory();
    auto& objectsInOldCategory = categories_[oldCategory];

    const unsigned oldIndex = FindIndexInCategory(objectsInOldCategory, reflection->GetTypeNameHash());
    if (oldIndex < objectsInOldCategory.size())
        objectsInOldCategory.erase_at(oldIndex);
    else
        URHO3D_ASSERTLOG(0, "Object {} is not found in category '{}'", reflection->GetTypeName(), oldCategory);
}

}
