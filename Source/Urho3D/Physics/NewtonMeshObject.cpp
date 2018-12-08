#include "NewtonMeshObject.h"
#include "Newton.h"
#include "Core/Context.h"
namespace Urho3D {

    NewtonMeshObject::NewtonMeshObject(Context* context) : Object(context)
    {
    }

    NewtonMeshObject::~NewtonMeshObject()
    {
        if (mesh != nullptr)
            NewtonMeshDestroy(mesh);
    }

    void NewtonMeshObject::RegisterObject(Context* context)
    {
        context->RegisterFactory<NewtonMeshObject>();
    }
}
