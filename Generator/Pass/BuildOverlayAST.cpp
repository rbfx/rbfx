#include "BuildOverlayAST.h"
#include "GeneratorContext.h"

namespace Urho3D
{

void MetaEntity::Register()
{
    generator->symbols_[uniqueName_] = this;
}

void MetaEntity::Unregister()
{
    generator->symbols_.Erase(uniqueName_);
}

}
