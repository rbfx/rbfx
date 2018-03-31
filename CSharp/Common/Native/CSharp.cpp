#include "CSharp.h"
#include <memory>


ManagedInterface managedAPI;
ScriptSubsystem* script = new ScriptSubsystem();

extern "C"
{

URHO3D_EXPORT_API void c_free(void* ptr)
{
    free(ptr);
}

URHO3D_EXPORT_API void* c_alloc(int size)
{
    return malloc(size);
}

URHO3D_EXPORT_API void CSharp_SetManagedAPI(ManagedInterface netApi)
{
    managedAPI = netApi;
}

}
