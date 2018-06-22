//
// Copyright (c) 2018 Rokas Kupstys
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

#include "CSharp.h"
#include <Urho3D/Scene/Serializable.h>


namespace Urho3D
{

extern "C"
{

EXPORT_API void Urho3D_Serializable_RegisterAttribute(Context* context, unsigned typeHash, VariantType valueType,
                                                      MarshalAllocator::Block* name, Variant* defaultValue,
                                                      AttributeMode mode, MarshalAllocator::Block* enumNames,
                                                      Variant*(*getter)(const Serializable*),
                                                      void(*setter)(Serializable*, Variant*))
{
    auto enumNamesList = CSharpConverter<StringVector>::FromCSharp(enumNames);
    auto accessor = MakeVariantAttributeAccessor<Serializable>(
        [getter](const Serializable& self, Urho3D::Variant& value) {
            auto* result = getter(&self);
            if (result == nullptr)
                value.Clear();
            else
                value = *result;
        },
        [setter](Serializable& self, const Urho3D::Variant& value)
        {
            setter(&self, CSharpObjConverter::ToCSharp<Variant>(value));
        });
    AttributeInfo info(valueType, CSharpConverter<const char*>::FromCSharp(name), accessor, enumNamesList,
                       defaultValue == nullptr ? Variant::EMPTY : *defaultValue, mode);
    context->RegisterAttribute(StringHash(typeHash), info);
}

}

}
