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

#pragma once

#include "../Core/Attribute.h"
#include "../Core/Signal.h"
#include "../Core/TypeInfo.h"
#include "../Container/Ptr.h"

#include <EASTL/functional.h>
#include <EASTL/type_traits.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_map.h>

namespace Urho3D
{

class Context;
class Object;
class TypeInfo;

/// Reflection of a class derived from Object.
class URHO3D_API ObjectReflection : public RefCounted
{
public:
    using ObjectFactoryCallback = SharedPtr<Object>(*)(const TypeInfo* typeInfo, Context* context);

    ObjectReflection(Context* context, const TypeInfo* typeInfo);
    ObjectReflection(Context* context, ea::unique_ptr<TypeInfo> typeInfo);

    /// @name Factory management
    /// @{
    void SetObjectFactory(ObjectFactoryCallback callback) { createObject_ = callback; }
    template <class T> void SetObjectFactory();
    SharedPtr<Object> CreateObject();
    bool HasObjectFactory() const { return createObject_ != nullptr; }
    /// @}

    /// @name Category management
    /// @{
    void SetCategory(ea::string_view category) { category_ = category; }
    const ea::string& GetCategory() const { return category_; }
    /// @}

    /// @name Return type info
    /// @{
    const TypeInfo* GetTypeInfo() const { return typeInfo_; }
    const ea::string& GetTypeName() const { return typeInfo_ ? typeInfo_->GetTypeName() : EMPTY_STRING; }
    StringHash GetTypeNameHash() const { return typeInfo_ ? typeInfo_->GetType() : StringHash::Empty; }
    /// @}

    /// @name Modify attributes
    /// @{
    AttributeHandle AddAttribute(const AttributeInfo& attr);
    void RemoveAttribute(StringHash nameHash);
    void RemoveAllAttributes();
    void CopyAttributesFrom(const ObjectReflection* other);
    void UpdateAttributeDefaultValue(StringHash nameHash, const Variant& defaultValue);
    /// @}

    /// @name Return attributes
    /// @{
    unsigned GetAttributeIndex(StringHash nameHash) const;
    unsigned GetAttributeIndex(StringHash nameHash, unsigned hintIndex) const;
    AttributeInfo* GetAttribute(StringHash nameHash);
    const AttributeInfo* GetAttribute(StringHash nameHash) const;
    const AttributeInfo& GetAttributeByIndex(unsigned index) const { return attributes_[index]; }
    const ea::vector<AttributeInfo>& GetAttributes() const { return attributes_; }
    unsigned GetNumAttributes() const { return attributes_.size(); }
    /// @}

    /// @name Metadata
    /// @{
    void SetMetadata(const StringHash& key, const Variant& value) { metadata_[key] = value; }
    const Variant& GetMetadata(const StringHash& key) const
    {
        auto elem = metadata_.find(key);
        return elem != metadata_.end() ? elem->second : Variant::EMPTY;
    }
    template <class T> T GetMetadata(const StringHash& key) const { return GetMetadata(key).Get<T>(); }
    void SetScopeHint(AttributeScopeHint hint) { scopeHint_ = hint; }
    AttributeScopeHint GetScopeHint() const { return scopeHint_; }
    AttributeScopeHint GetEffectiveScopeHint() const;
    /// @}

private:
    Context* context_{};

    /// Type info of reflected object.
    const TypeInfo* typeInfo_{};
    ea::unique_ptr<TypeInfo> ownedTypeInfo_;
    ObjectFactoryCallback createObject_{};
    /// Category of the object.
    ea::string category_;

    /// Reflection metadata.
    ea::unordered_map<StringHash, Variant> metadata_;
    /// Scope hint for the entire object.
    AttributeScopeHint scopeHint_{};

    /// Attributes of the Serializable.
    ea::vector<AttributeInfo> attributes_;
    ea::vector<StringHash> attributeNames_;
};

/// Registry of Object reflections.
class URHO3D_API ObjectReflectionRegistry
{
public:
    /// Object type is removed from reflection.
    /// All existing instances of the type shall be immediately destroyed.
    Signal<void(ObjectReflection*), ObjectReflectionRegistry> OnReflectionRemoved;

    explicit ObjectReflectionRegistry(Context* context);

    /// Return existing or new reflection for given type.
    ObjectReflection* Reflect(const TypeInfo* typeInfo);
    ObjectReflection* ReflectCustomType(ea::unique_ptr<TypeInfo> typeInfo);
    template <class T> ObjectReflection* Reflect();

    /// Add new object reflection with or without object creation factory and assign it to the category.
    template <class T> ObjectReflection* AddReflection(ea::string_view category = "") { return AddReflectionInternal<T, true, false>(category); }
    template <class T> ObjectReflection* AddFactoryReflection(ea::string_view category = "") { return AddReflectionInternal<T, true, true>(category); }
    template <class T> ObjectReflection* AddAbstractReflection(ea::string_view category = "") { return AddReflectionInternal<T, false, false>(category); }

    /// Return existing reflection for given type.
    ObjectReflection* GetReflection(StringHash typeNameHash);
    const ObjectReflection* GetReflection(StringHash typeNameHash) const;
    bool IsReflected(StringHash typeNameHash) const { return GetReflection(typeNameHash) != nullptr; }
    template <class T> ObjectReflection* GetReflection();
    template <class T> const ObjectReflection* GetReflection() const;
    template <class T> bool IsReflected() const;

    /// @name Shortcuts to call ObjectReflection methods without creating new reflection if one doesn't exist
    /// @{
    template <class T> void RemoveAttribute(StringHash nameHash);
    template <class T> void UpdateAttributeDefaultValue(StringHash nameHash, const Variant& defaultValue);
    /// @}

    /// Assign object reflection to the category.
    void SetReflectionCategory(const TypeInfo* typeInfo, ea::string_view category);
    template <class T> void SetReflectionCategory(ea::string_view category);

    /// Remove reflection.
    void RemoveReflection(StringHash typeNameHash);
    template <class T> void RemoveReflection();

    /// Create an object by type. Return pointer to it or null if no reflection is found.
    SharedPtr<Object> CreateObject(StringHash typeNameHash);

    /// Return reflections of all objects.
    const ea::unordered_map<StringHash, SharedPtr<ObjectReflection>>& GetObjectReflections() const { return reflections_; }
    /// Return categories of reflected objects.
    const ea::unordered_map<ea::string, ea::vector<StringHash>>& GetObjectCategories() const { return categories_; }

private:
    template <class T, bool EnableFactory, bool RequireFactory> ObjectReflection* AddReflectionInternal(ea::string_view category);

    void ErrorReflectionNotFound(StringHash typeNameHash) const;
    void ErrorDuplicateReflection(StringHash typeNameHash) const;
    void AddReflectionToCurrentCategory(ObjectReflection* reflection);
    void RemoveReflectionFromCurrentCategory(ObjectReflection* reflection);

    Context* context_{};

    ea::unordered_map<StringHash, SharedPtr<ObjectReflection>> reflections_;
    ea::unordered_map<ea::string, ea::vector<StringHash>> categories_;
};

template <class T>
void ObjectReflection::SetObjectFactory()
{
    createObject_ = +[](const TypeInfo* typeInfo, Context* context) { return StaticCast<Object>(MakeShared<T>(context)); };
}

template <class T>
ObjectReflection* ObjectReflectionRegistry::Reflect()
{
    return Reflect(T::GetTypeInfoStatic());
}

template <class T, bool EnableFactory, bool RequireFactory>
ObjectReflection* ObjectReflectionRegistry::AddReflectionInternal(ea::string_view category)
{
    if (IsReflected<T>())
    {
        ErrorDuplicateReflection(T::GetTypeStatic());
        return nullptr;
    }
    else
    {
        ObjectReflection* reflection = Reflect<T>();
        constexpr bool isConstructible = ea::is_constructible_v<T, Context*>;
        constexpr bool isAbstract = ea::is_abstract_v<T>;
        static_assert(!isAbstract || !RequireFactory, "Object should not be abstract");
        static_assert(isConstructible || !RequireFactory, "Object should be constructible from Context*");

        if constexpr(EnableFactory && isConstructible)
            reflection->SetObjectFactory<T>();

        if (!category.empty())
            SetReflectionCategory<T>(category);
        return reflection;
    }
}

template <class T>
ObjectReflection* ObjectReflectionRegistry::GetReflection()
{
    return const_cast<ObjectReflection*>(const_cast<const ObjectReflectionRegistry*>(this)->GetReflection<T>());
}

template <class T>
const ObjectReflection* ObjectReflectionRegistry::GetReflection() const
{
    return GetReflection(T::GetTypeStatic());
}

template <class T>
bool ObjectReflectionRegistry::IsReflected() const
{
    return IsReflected(T::GetTypeStatic());
}

template <class T>
void ObjectReflectionRegistry::RemoveAttribute(StringHash nameHash)
{
    if (ObjectReflection* reflection = GetReflection<T>())
        reflection->RemoveAttribute(nameHash);
    else
        ErrorReflectionNotFound(T::GetTypeStatic());
}

template <class T>
void ObjectReflectionRegistry::UpdateAttributeDefaultValue(StringHash nameHash, const Variant& defaultValue)
{
    if (ObjectReflection* reflection = GetReflection<T>())
        reflection->UpdateAttributeDefaultValue(nameHash, defaultValue);
    else
        ErrorReflectionNotFound(T::GetTypeStatic());
}

template <class T>
void ObjectReflectionRegistry::SetReflectionCategory(ea::string_view category)
{
    SetReflectionCategory(T::GetTypeInfoStatic(), category);
}

template <class T>
void ObjectReflectionRegistry::RemoveReflection()
{
    RemoveReflection(T::GetTypeStatic());
}

}
