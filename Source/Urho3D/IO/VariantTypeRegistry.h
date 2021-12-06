//
// Copyright (c) 2021 the rbfx project.
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

#include "../Core/Variant.h"
#include "../Core/Object.h"

#include <EASTL/unordered_map.h>

namespace Urho3D
{
struct URHO3D_API VariantCustomValueInitializer : public RefCounted
{
    VariantCustomValueInitializer(const ea::string_view hint);

    /// Destruct. Free all resources.
    ~VariantCustomValueInitializer() override;

    /// Initialize variant.
    virtual bool Initialize(Variant& variant) const = 0;

    /// Get hint.
    virtual const ea::string& GetHint() const { return name_; }

    /// Get hint hash.
    virtual StringHash GetHintHash() const { return nameHash_; }

    /// Get type hash.
    virtual size_t GetTypeHash() const = 0;

private:
    ea::string name_;
    StringHash nameHash_;
};

template <typename T> struct VariantCustomValueInitializerImpl : public VariantCustomValueInitializer
{
    VariantCustomValueInitializerImpl(const ea::string_view hint)
        : VariantCustomValueInitializer(hint)
    {
    }

    /// Get type hash.
    size_t GetTypeHash() const override { return typeid(T).hash_code(); }
    
    /// Initialize variant.
    bool Initialize(Variant& variant) const override
    {
        variant.SetCustom(T());
        return true;
    }
};

template <typename U> struct VariantCustomValueInitializerImpl<ea::unique_ptr<U>> : public VariantCustomValueInitializer
{
    VariantCustomValueInitializerImpl(const ea::string_view hint)
        : VariantCustomValueInitializer(hint)
    {
    }


    /// Get type hash code.
    size_t GetTypeHash() const override { return typeid(ea::unique_ptr<U>).hash_code(); }

    /// Initialize variant.
    bool Initialize(Variant& variant) const override
    {
        variant.SetCustom(ea::make_unique<U>());
        return true;
    }
};

/// Factory registry for graph custom properties.
class URHO3D_API VariantTypeRegistry : public Object
{
    URHO3D_OBJECT(VariantTypeRegistry, Object);

public:
    /// Construct.
    explicit VariantTypeRegistry(Context* context);
    /// Destruct. Free all resources.
    ~VariantTypeRegistry() override;

    /// Register initializer.
    void RegisterInitializer(VariantCustomValueInitializer* initializer);

    /// Register initializer.
    template <typename T>
    void RegisterInitializer(const ea::string_view hint)
    {
        RegisterInitializer(new VariantCustomValueInitializerImpl<T>(hint));
    }

    /// Get custom type hint by variant custom type. Returns nullptr if type is not registered.
    const ea::string* GetHint(const Variant& variant);

    /// Get custom type hint by variant custom type.
    ea::tuple<bool, StringHash> GetHintHash(const Variant& variant);

    /// Create custom value.
    bool InitializeValue(const ea::string_view hint, Variant& variant);

private:
    ea::unordered_map<StringHash, SharedPtr<VariantCustomValueInitializer>> initializersByHint_;
    ea::unordered_map<size_t, SharedPtr<VariantCustomValueInitializer>> initializersByType_;
};

}
