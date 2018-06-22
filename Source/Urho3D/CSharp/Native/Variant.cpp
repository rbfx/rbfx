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

#include <Urho3D/Core/Object.h>
#include <Urho3D/Script/ScriptSubsystem.h>
#include "CSharp.h"


namespace Urho3D
{

extern "C"
{

EXPORT_API Variant* Urho3D_Variant__Variant_object(gchandle handle)
{
    auto* result = new Variant(MakeCustomValue(GcHandleContainer(handle)));
    return result;
}

EXPORT_API gchandle Urho3D_Variant__GetObject(Variant* variant)
{
    if (variant == nullptr)
        return nullptr;

    auto* storage = variant->GetCustomPtr<GcHandleContainer>();
    if (storage == nullptr)
        return nullptr;
    return storage->handle_;
}

EXPORT_API VariantType Urho3D_Variant__GetValueType(Variant* variant)
{
    if (variant == nullptr)
        return VAR_NONE;
    return variant->GetType();
}

EXPORT_API void Urho3D_Variant__SetObject(Variant* variant, gchandle handle)
{
    if (variant == nullptr)
        return;
    variant->SetCustom(GcHandleContainer(handle));
}

}

}
