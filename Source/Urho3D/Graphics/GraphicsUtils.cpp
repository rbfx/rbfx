#include "GraphicsUtils.h"

namespace Urho3D
{
    namespace Utils
    {
        ea::vector<VertexElement> GetVertexElements(unsigned elementMask) {
            ea::vector<VertexElement> ret;

            for (unsigned i = 0; i < MAX_LEGACY_VERTEX_ELEMENTS; ++i)
            {
                if (elementMask & (1u << i))
                    ret.push_back(LEGACY_VERTEXELEMENTS[i]);
            }

            return ret;
        }
    }
}
