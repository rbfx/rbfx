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
#include "../Core/CoreEvents.h"

#include "ValueCache.h"


namespace Urho3D
{

ValueCache::ValueCache(Context* context)
    : Object(context)
{
    SubscribeToEvent(E_ENDFRAME, URHO3D_HANDLER(ValueCache, OnEndFrame));
}

ValueCache::~ValueCache()
{
    Clear();
}

void ValueCache::Expire()
{
    if (expireTimer_.GetMSec(false) < 10000)
        return;
    expireTimer_.Reset();

    auto time = GetSubsystem<Time>();
    unsigned framesNow = time->GetFrameNumber();
    for (auto it = cache_.begin(); it != cache_.end();)
    {
        CacheEntry& entry = it->second;
        if ((framesNow - entry.lastUsed_) > expireFrames_)
        {
            entry.deleter_(entry.value_);
            entry.value_ = nullptr;
            it = cache_.erase(it);
        }
        else
            ++it;
    }
}

void ValueCache::Clear()
{
    for (auto& pair : cache_)
    {
        CacheEntry& entry = pair.second;
        entry.deleter_(entry.value_);
        entry.value_ = nullptr;
    }
    cache_.clear();
}

void ValueCache::OnEndFrame(StringHash, VariantMap&)
{
    Expire();
}

}
