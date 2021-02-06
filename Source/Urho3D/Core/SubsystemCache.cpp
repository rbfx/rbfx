//
// Copyright (c) 2017-2020 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include "../Precompiled.h"

#include "../Core/SubsystemCache.h"

#include "../Audio/Audio.h"
#include "../Engine/Engine.h"
#include "../Core/WorkQueue.h"
#include "../Graphics/Graphics.h"
#include "../Graphics/Renderer.h"
#include "../IO/FileSystem.h"
#if URHO3D_LOGGING
#include "../IO/Log.h"
#endif
#include "../Resource/ResourceCache.h"
#include "../Resource/Localization.h"
#if URHO3D_NETWORK
#include "../Network/Network.h"
#endif
#include "../Input/Input.h"
#include "../UI/UI.h"
#if URHO3D_SYSTEMUI
#include "../SystemUI/SystemUI.h"
#endif

#include "../DebugNew.h"

namespace Urho3D
{

SubsystemCache::SubsystemCache()
    : cachedSubsystemTypes_(GetCachedSubsystemTypes(static_cast<CachedSubsystemList*>(nullptr)))
{
}

SubsystemCache::~SubsystemCache()
{
}

unsigned SubsystemCache::GetCacheIndex(StringHash type) const
{
    const auto iter = ea::find(cachedSubsystemTypes_.begin(), cachedSubsystemTypes_.end(), type);
    return static_cast<unsigned>(iter - cachedSubsystemTypes_.begin());
}

void SubsystemCache::Add(StringHash type, Object* subsystem)
{
    const unsigned index = GetCacheIndex(type);
    if (index < NumCachedSubsystems)
        cachedSubsystems_[index] = subsystem;
    subsystems_[type] = subsystem;
}

void SubsystemCache::Remove(StringHash type)
{
    const unsigned index = GetCacheIndex(type);
    if (index < NumCachedSubsystems)
        cachedSubsystems_[index] = nullptr;
    subsystems_.erase(type);
}

void SubsystemCache::Clear()
{
    cachedSubsystems_.fill(nullptr);
    subsystems_.clear();
}

}
