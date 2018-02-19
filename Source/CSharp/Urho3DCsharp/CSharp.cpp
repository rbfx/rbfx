#include "CSharp.h"
#include <memory>


ScriptSubsystem* script = new ScriptSubsystem();

extern "C"
{

URHO3D_EXPORT_API void Urho3D__Free(void* ptr)
{
    free(ptr);
}

}
