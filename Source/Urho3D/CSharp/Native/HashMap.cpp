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

extern "C"
{

////////////////////////////////////////////////////// VariantMap //////////////////////////////////////////////////////
EXPORT_API unsigned Urho3D_HashMap_StringHash_Variant_GetKey(Urho3D::VariantMap::Iterator it)
{
    return it->first_.Value();
}

EXPORT_API void* Urho3D_HashMap_StringHash_Variant_GetValue(Urho3D::VariantMap::Iterator it)
{
    return (void*) &it->second_;
}

EXPORT_API void Urho3D_HashMap_StringHash_Variant_Add(Urho3D::VariantMap* map, unsigned key,
    Urho3D::Variant* value)
{
    map->Insert(Urho3D::Pair<StringHash, Variant>(StringHash(key), *value));
}

EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Remove(Urho3D::VariantMap* map, unsigned key)
{
    return map->Erase(StringHash(key));
}

EXPORT_API bool Urho3D_HashMap_StringHash_Variant_First(Urho3D::VariantMap* map,
    Urho3D::VariantMap::Iterator& it)
{
    it = map->Begin();
    return it != map->End();
}

EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Next(Urho3D::VariantMap* map, Urho3D::VariantMap::Iterator& it)
{
    it++;
    return it != map->End();
}

EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Contains(Urho3D::VariantMap* map, unsigned key)
{
    return map->Contains(StringHash(key));
}

EXPORT_API Variant* Urho3D_HashMap_StringHash_Variant_TryGet(Urho3D::VariantMap* map, unsigned key)
{
    Variant result;
    if (map->TryGetValue(StringHash(key), result))
        return new Variant(result);
    return nullptr;
}

EXPORT_API void Urho3D_HashMap_StringHash_Variant_destructor(Urho3D::VariantMap* map)
{
    delete map;
}

}
