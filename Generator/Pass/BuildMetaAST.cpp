#include "BuildMetaAST.h"
#include "GeneratorContext.h"

namespace Urho3D
{

void MetaEntity::Register()
{
    if (uniqueName_.empty())
        // Could be stuff injected into AST.
        return;
    generator->symbols_[uniqueName_] = this;
}

void MetaEntity::Unregister()
{
    generator->symbols_.Erase(uniqueName_);
}

}
