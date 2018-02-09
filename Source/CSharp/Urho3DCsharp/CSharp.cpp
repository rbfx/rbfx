#include <Urho3D/Urho3DAll.h>
#include <memory>


URHO3D_EXPORT_API void Urho3D__Free(void* ptr)
{
    free(ptr);
}
