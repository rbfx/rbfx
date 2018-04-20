#include "CSharp.h"

extern "C"
{

////////////////////////////////////////////////////// VariantMap //////////////////////////////////////////////////////
URHO3D_EXPORT_API unsigned Urho3D_HashMap_StringHash_Variant_GetKey(Urho3D::VariantMap::Iterator it)
{
    return it->first_.Value();
}

URHO3D_EXPORT_API void* Urho3D_HashMap_StringHash_Variant_GetValue(Urho3D::VariantMap::Iterator it)
{
    return (void*) &it->second_;
}

URHO3D_EXPORT_API void Urho3D_HashMap_StringHash_Variant_Add(Urho3D::VariantMap* map, unsigned key,
    Urho3D::Variant* value)
{
    map->Insert(Urho3D::Pair<StringHash, Variant>(StringHash(key), *value));
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Remove(Urho3D::VariantMap* map, unsigned key)
{
    return map->Erase(StringHash(key));
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_First(Urho3D::VariantMap* map,
    Urho3D::VariantMap::Iterator& it)
{
    it = map->Begin();
    return it != map->End();
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Next(Urho3D::VariantMap* map, Urho3D::VariantMap::Iterator& it)
{
    it++;
    return it != map->End();
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Contains(Urho3D::VariantMap* map, unsigned key)
{
    return map->Contains(StringHash(key));
}

URHO3D_EXPORT_API Variant* Urho3D_HashMap_StringHash_Variant_TryGet(Urho3D::VariantMap* map, unsigned key)
{
    Variant result;
    if (map->TryGetValue(StringHash(key), result))
        return new Variant(result);
    return nullptr;
}

URHO3D_EXPORT_API void Urho3D_HashMap_StringHash_Variant_destructor(Urho3D::VariantMap* map)
{
    delete map;
}

void RegisterVariantMapInternalCalls(Context* context)
{
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_GetKey);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_GetValue);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_Add);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_Remove);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_First);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_Next);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_Contains);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_TryGet);
    MONO_INTERNAL_CALL(Urho3D.VariantMap, Urho3D_HashMap_StringHash_Variant_destructor);
}

}
