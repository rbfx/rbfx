//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include "../Container/FlagSet.h"
#include "../Container/Ptr.h"
#include "../Core/Variant.h"

#include <EASTL/optional.h>

namespace Urho3D
{

enum AttributeMode
{
    /// Attribute shown only in the editor, but not serialized.
    AM_EDIT = 0,
    /// Attribute used for file serialization.
    AM_FILE = 1 << 0,
    /// Attribute should not be shown in the editor.
    AM_NOEDIT = 1 << 1,
    /// Attribute is a node ID and may need rewriting.
    AM_NODEID = 1 << 2,
    /// Attribute is a component ID and may need rewriting.
    AM_COMPONENTID = 1 << 3,
    /// Attribute is a node ID vector where first element is the amount of nodes.
    AM_NODEIDVECTOR = 1 << 4,
    /// Attribute is readonly. Can't be used with binary serialized objects.
    AM_READONLY = 1 << 5,
    /// Attribute should be saved in prefab.
    AM_PREFAB = 1 << 6,
    /// Attribute should be saved in temporary storages even when other serialization is disabled.
    AM_TEMPORARY = 1 << 7,

    /// Default mode, same as AM_FILE and AM_PREFAB.
    AM_DEFAULT = 1 << 0 | 1 << 6,
};
URHO3D_FLAGSET(AttributeMode, AttributeModeFlags);

/// Attribute scope hint.
/// Indicates the scope of changes caused by an attribute. Used for undo/redo in the Editor.
enum class AttributeScopeHint
{
    /// Attribute change doesn't affect any other attributes.
    Attribute,
    /// Attribute change may affect other attributes in the same object (node or component).
    Serializable,
    /// Attribute change may affect other attributes, components or children nodes in the owner node.
    Node,
    /// Attribute change may affect anything in the scene.
    /// \warning It is not fully supported by the Editor yet!
    Scene,
};

class Serializable;

/// Abstract base class for invoking attribute accessors.
class URHO3D_API AttributeAccessor : public RefCounted
{
public:
    /// Construct.
    AttributeAccessor() = default;
    /// Get the attribute.
    virtual void Get(const Serializable* ptr, Variant& dest) const = 0;
    /// Set the attribute.
    virtual void Set(Serializable* ptr, const Variant& src) = 0;
};

/// Description of an automatically serializable variable.
struct AttributeInfo
{
    AttributeInfo() = default;
    AttributeInfo(const AttributeInfo& other) = default;

#ifndef SWIG
    /// Construct attribute.
    AttributeInfo(VariantType type, const char* name, const SharedPtr<AttributeAccessor>& accessor, const char** enumNames, const Variant& defaultValue, AttributeModeFlags mode) :
        type_(type),
        name_(name),
        nameHash_(name_),
        enumNames_(ToVector(enumNames)),
        accessor_(accessor),
        defaultValue_(defaultValue),
        mode_(mode)
    {
    }
#endif
    /// Construct attribute.
    AttributeInfo(VariantType type, const char* name, const SharedPtr<AttributeAccessor>& accessor, const StringVector& enumNames, const Variant& defaultValue, AttributeModeFlags mode) :
        type_(type),
        name_(name),
        nameHash_(name_),
        enumNames_(enumNames),
        accessor_(accessor),
        defaultValue_(defaultValue),
        mode_(mode)
    {
    }

    /// Return attribute metadata.
    const Variant& GetMetadata(const StringHash& key) const
    {
        auto elem = metadata_.find(key);
        return elem != metadata_.end() ? elem->second : Variant::EMPTY;
    }

    /// Return attribute metadata of specified type.
    template <class T> T GetMetadata(const StringHash& key) const
    {
        return GetMetadata(key).Get<T>();
    }

    /// Return whether the attribute should be saved.
    bool ShouldSave() const { return (mode_ & AM_FILE) && !(mode_ & AM_READONLY); }

    /// Return whether the attribute should be loaded.
    bool ShouldLoad() const { return !!(mode_ & AM_FILE); }

    /// Convert enum value to string.
    const ea::string& ConvertEnumToString(unsigned value) const { return enumNames_[value]; }

    /// Convert enum value to integer.
    unsigned ConvertEnumToUInt(ea::string_view value) const
    {
        unsigned index = 0;
        for (const ea::string& name : enumNames_)
        {
            if (Compare(name, value, false) == 0)
                return index;
            ++index;
        }
        return M_MAX_UNSIGNED;
    }

    /// Attribute type.
    VariantType type_ = VAR_NONE;
    /// Name.
    ea::string name_;
    /// Name hash.
    StringHash nameHash_;
    /// Enum names.
    StringVector enumNames_;
    /// Helper object for accessor mode.
    SharedPtr<AttributeAccessor> accessor_;
    /// Default value for network replication.
    Variant defaultValue_;
    /// Attribute mode: whether to use for serialization, network replication, or both.
    AttributeModeFlags mode_ = AM_DEFAULT;
    /// Attribute metadata.
    VariantMap metadata_;
    /// Scope hint.
    AttributeScopeHint scopeHint_{};

private:
    static StringVector ToVector(const char* const* strings)
    {
        StringVector vector;
        if (strings)
        {
            for (const char* const* ptr = strings; *ptr; ++ptr)
                vector.push_back(*ptr);
        }
        return vector;
    }
};

/// Attribute handle returned by Context::RegisterAttribute and used to chain attribute setup calls.
/// @nobind
struct AttributeHandle
{
    friend class ObjectReflection;

public:
    AttributeHandle& SetScopeHint(AttributeScopeHint scopeHint)
    {
        if (attributeInfo_)
            attributeInfo_->scopeHint_ = scopeHint;
        return *this;
    }

    AttributeHandle& SetMetadata(StringHash key, const Variant& value)
    {
        if (attributeInfo_)
            attributeInfo_->metadata_[key] = value;
        return *this;
    }

private:
    AttributeInfo* attributeInfo_ = nullptr;
};

}
