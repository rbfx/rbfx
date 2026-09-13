// Copyright (c) 2008-2022 the Urho3D project.
// Copyright (c) 2022-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "Urho3D/Precompiled.h"

#include "Urho3D/Core/StringHashRegister.h"
#include "Urho3D/Core/Mutex.h"
#include "Urho3D/IO/Log.h"

#include <cstdio>

#include "Urho3D/DebugNew.h"

namespace Urho3D
{

StringHashRegister::StringHashRegister(bool threadSafe)
{
    if (threadSafe)
        mutex_ = ea::make_unique<Mutex>();
}


StringHashRegister::~StringHashRegister()       // NOLINT(hicpp-use-equals-default, modernize-use-equals-default)
{
    // Keep destructor here to let mutex_ destruct
}

StringHash StringHashRegister::RegisterString(const StringHash& hash, ea::string_view string)
{
    if (mutex_)
        mutex_->Acquire();

    auto iter = map_.find(hash);
    if (iter == map_.end())
    {
        map_.emplace(hash, ea::string(string));
    }
    else if (ea::string_view(iter->second) != string)
    {
        URHO3D_LOGWARNINGF("StringHash collision detected! Both \"%s\" and \"%s\" have hash #%s",
            string, iter->second.c_str(), hash.ToString().c_str());
    }

    if (mutex_)
        mutex_->Release();

    return hash;
}

StringHash StringHashRegister::RegisterString(ea::string_view string)
{
    StringHash hash(string);
    return RegisterString(hash, string);
}

ea::string StringHashRegister::GetStringCopy(const StringHash& hash) const
{
    if (mutex_)
        mutex_->Acquire();

    const ea::string copy = GetString(hash);

    if (mutex_)
        mutex_->Release();

    return copy;
}

bool StringHashRegister::Contains(const StringHash& hash) const
{
    if (mutex_)
        mutex_->Acquire();

    const bool contains = map_.contains(hash);

    if (mutex_)
        mutex_->Release();

    return contains;
}

const ea::string& StringHashRegister::GetString(const StringHash& hash) const
{
    auto iter = map_.find(hash);
    return iter == map_.end() ? EMPTY_STRING : iter->second;
}

}
