#include "BuildMetaAST.h"
#include "GeneratorContext.h"

namespace Urho3D
{

void MetaEntity::Register()
{
    if (uniqueName_.empty())
        // Could be stuff injected into AST.
        return;
    generator->symbols_[uniqueName_] = shared_from_this();
}

void MetaEntity::Unregister()
{
    generator->symbols_.erase(uniqueName_);
}

}
