#pragma once
#include "../Core/Object.h"
#include "GraphicsDefs.h"
namespace Urho3D
{
    namespace Utils
    {
        URHO3D_API ea::vector<VertexElement> GetVertexElements(unsigned elementMask);
    }
}
