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

#include "VariantTypeRegistry.h"

namespace Urho3D
{
VariantCustomValueInitializer::VariantCustomValueInitializer(const ea::string_view hint)
    : name_(hint)
    , nameHash_(hint)
{
}

VariantCustomValueInitializer::~VariantCustomValueInitializer() = default;

VariantTypeRegistry::VariantTypeRegistry(Context* context)
    : Object(context)
{   
}

VariantTypeRegistry::~VariantTypeRegistry() = default;

void VariantTypeRegistry::RegisterInitializer(VariantCustomValueInitializer* initializer)
{
    initializersByHint_[initializer->GetHintHash()] = initializer;
    initializersByType_[initializer->GetTypeHash()] = initializer;
}

const ea::string* VariantTypeRegistry::GetHint(const Variant& variant)
{
    if (variant.GetType() != VAR_CUSTOM)
        return nullptr;
    auto val = variant.GetCustomVariantValuePtr();
    const size_t hintHash = val->GetTypeInfo().hash_code();
    auto res = initializersByType_.find(hintHash);
    if (res == initializersByType_.end())
        return nullptr;
    return &res->second->GetHint();
}

ea::tuple<bool, StringHash> VariantTypeRegistry::GetHintHash(const Variant& variant)
{
    if (variant.GetType() != VAR_CUSTOM)
        return ea::make_tuple<bool, StringHash>(false, 0);
    auto val = variant.GetCustomVariantValuePtr();
    const size_t hintHash = val->GetTypeInfo().hash_code();
    auto res = initializersByType_.find(hintHash);
    if (res == initializersByType_.end())
        return ea::make_tuple<bool, StringHash>(false, 0);
    return ea::make_tuple<bool, StringHash>(true, res->second->GetHintHash());
}

bool VariantTypeRegistry::InitializeValue(const ea::string_view hint, Variant& variant)
{
    auto res = initializersByHint_.find(hint);
    if (res == initializersByHint_.end())
        return false;
    return res->second->Initialize(variant);
}
}
