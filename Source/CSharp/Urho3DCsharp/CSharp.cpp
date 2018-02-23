#include "CSharp.h"
#include <memory>


ScriptSubsystem* script = new ScriptSubsystem();

extern "C"
{

URHO3D_EXPORT_API void Urho3D__Free(void* ptr)
{
    free(ptr);
}

URHO3D_EXPORT_API unsigned Urho3D_HashMap_StringHash_Variant_GetKey(Urho3D::VariantMap::Iterator it)
{
    return it->first_.Value();
}

URHO3D_EXPORT_API void* Urho3D_HashMap_StringHash_Variant_GetValue(Urho3D::VariantMap::Iterator it)
{
    return (void*)script->AddRef<Variant>(it->second_);
}

URHO3D_EXPORT_API void Urho3D_HashMap_StringHash_Variant_Add(Urho3D::VariantMap* map, unsigned key, Urho3D::Variant* value)
{
    map->Insert(Urho3D::Pair<StringHash, Variant>(StringHash(key), *value));
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_Remove(Urho3D::VariantMap* map, unsigned key)
{
    map->Erase(StringHash(key));
}

URHO3D_EXPORT_API bool Urho3D_HashMap_StringHash_Variant_First(Urho3D::VariantMap* map, Urho3D::VariantMap::Iterator& it)
{
    if (it == map->End())
        return false;
    it = map->Begin();
    return true;
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
        return script->AddRef<Variant>(std::move(result));
    return nullptr;
}

}
