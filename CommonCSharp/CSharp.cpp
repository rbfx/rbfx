#include "CSharp.h"
#include <memory>


ScriptSubsystem* script = new ScriptSubsystem();

extern "C"
{

URHO3D_EXPORT_API void c_free(void* ptr)
{
    free(ptr);
}

URHO3D_EXPORT_API void CSharp_SetManagedAPI(ManagedInterface netApi)
{
    script->net_ = netApi;
}

}
