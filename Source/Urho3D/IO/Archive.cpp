// Copyright (c) 2017-2020 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include "../IO/Archive.h"

#include <cassert>

namespace Urho3D
{

ArchiveBlock::~ArchiveBlock()
{
    if (archive_)
        archive_->EndBlock();
}

bool Archive::ValidateName(ea::string_view name)
{
    // Empty names are not allowed
    if (name.empty())
        return false;

    // Name must start with letter or underscore.
    if (!isalpha(name[0]) && name[0] != '_')
        return false;

    // Name must contain only letters, digits, underscores, dots or colons.
    for (const char ch : name)
    {
        if (!isalnum(ch) && ch != '_' && ch != '.' && ch != ':')
            return false;
    }

    return true;
}

}
