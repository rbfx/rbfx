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

#include "../Engine/ApplicationSettings.h"
#include "../IO/ArchiveSerialization.h"

namespace Urho3D
{

ApplicationSettings::ApplicationSettings(Context* context)
    : Object(context)
{
}

bool ApplicationSettings::Serialize(Archive& archive)
{
    if (auto block = archive.OpenUnorderedBlock("settings"))
    {
        if (!archive.Serialize("defaultScene", defaultScene_))
            return false;

        if (!SerializeValue(archive, "platforms", platforms_))
            return false;

        if (auto block = archive.OpenMapBlock("settings", engineParameters_.size()))
        {
            if (archive.IsInput())
            {
                for (unsigned j = 0; j < block.GetSizeHint(); ++j)
                {
                    ea::string key;
                    if (!archive.SerializeKey(key))
                        return false;
                    if (!SerializeValue(archive, "value", engineParameters_[key]))
                        return false;
                }
            }
            else
            {
                for (auto& pair : engineParameters_)
                {
                    if (!archive.SerializeKey(const_cast<ea::string&>(pair.first)))
                        return false;
                    if (!SerializeValue(archive, "value", pair.second))
                        return false;
                }
            }
        }

#if URHO3D_PLUGINS
        if (!SerializeVector(archive, "plugins", "plugin", plugins_))
            return false;
#endif
    }

    return true;
}

}
