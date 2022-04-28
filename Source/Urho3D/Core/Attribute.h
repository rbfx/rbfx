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

    /// Default mode, same as AM_FILE.
    AM_DEFAULT = 1 << 0,
};
URHO3D_FLAGSET(AttributeMode, AttributeModeFlags);

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
    /// Construct empty.
    AttributeInfo() = default;
#ifndef SWIG
    /// Construct attribute.
    AttributeInfo(VariantType type, const char* name, const SharedPtr<AttributeAccessor>& accessor, const char** enumNames, const Variant& defaultValue, AttributeModeFlags mode) :
        type_(type),
        name_(name),
        nameHash_(name_),
        enumNames_(enumNames),
        accessor_(accessor),
        defaultValue_(defaultValue),
        mode_(mode)
    {
    }
#endif
    /// Construct attribute.
    AttributeInfo(VariantType type, const char* name, const SharedPtr<AttributeAccessor>& accessor, const ea::vector<ea::string>& enumNames, const Variant& defaultValue, AttributeModeFlags mode) :
        type_(type),
        name_(name),
        nameHash_(name_),
        enumNames_(nullptr),
        enumNamesStorage_(enumNames),
        accessor_(accessor),
        defaultValue_(defaultValue),
        mode_(mode)
    {
        InitializeEnumNamesFromStorage();
    }

    /// Copy attribute info.
    AttributeInfo(const AttributeInfo& other)
    {
        type_ = other.type_;
        name_ = other.name_;
        nameHash_ = other.nameHash_;
        enumNames_ = other.enumNames_;
        accessor_ = other.accessor_;
        defaultValue_ = other.defaultValue_;
        mode_ = other.mode_;
        metadata_ = other.metadata_;
        ptr_ = other.ptr_;
        enumNamesStorage_ = other.enumNamesStorage_;

        if (!enumNamesStorage_.empty())
            InitializeEnumNamesFromStorage();
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
    const char* ConvertEnumToString(unsigned value) const { return enumNames_[value]; }

    /// Convert enum value to integer.
    unsigned ConvertEnumToUInt(ea::string_view value) const
    {
        unsigned index = 0;
        for (const char** namePtr = enumNames_; *namePtr; ++namePtr)
        {
            if (Compare(ea::string_view(*namePtr), value, false) == 0)
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
    const char** enumNames_ = nullptr;
    /// Helper object for accessor mode.
    SharedPtr<AttributeAccessor> accessor_;
    /// Default value for network replication.
    Variant defaultValue_;
    /// Attribute mode: whether to use for serialization, network replication, or both.
    AttributeModeFlags mode_ = AM_DEFAULT;
    /// Attribute metadata.
    VariantMap metadata_;
    /// Attribute data pointer if elsewhere than in the Serializable.
    void* ptr_ = nullptr;
    /// List of enum names. Used when names can not be stored externally.
    ea::vector<ea::string> enumNamesStorage_;
    /// List of enum name pointers. Front of this vector will be assigned to enumNames_ when enumNamesStorage_ is in use.
    ea::vector<const char*> enumNamesPointers_;

private:
    void InitializeEnumNamesFromStorage()
    {
        if (enumNamesStorage_.empty())
            enumNames_ = nullptr;
        else
        {
            for (const auto& enumName : enumNamesStorage_)
                enumNamesPointers_.emplace_back(enumName.c_str());
            enumNamesPointers_.emplace_back(nullptr);
            enumNames_ = &enumNamesPointers_.front();
        }
    }
};

/// Attribute handle returned by Context::RegisterAttribute and used to chain attribute setup calls.
/// @nobind
struct AttributeHandle
{
    friend class ObjectReflection;

public:
    /// Set metadata.
    AttributeHandle& SetMetadata(StringHash key, const Variant& value)
    {
        if (attributeInfo_)
            attributeInfo_->metadata_[key] = value;
        return *this;
    }

private:
    /// Attribute info.
    AttributeInfo* attributeInfo_ = nullptr;
};

}
