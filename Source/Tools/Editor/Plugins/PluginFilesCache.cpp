//
// Copyright (c) 2018 Rokas Kupstys
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

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/FileSystem.h>
#include "Plugins/PluginManager.h"
#include "PluginFilesCache.h"


namespace Urho3D
{

PluginFilesCache::PluginFilesCache(Context* context)
    : Object(context)
{
}

const StringVector& PluginFilesCache::GetPluginNames()
{
    if (updateTimer_.GetMSec(false) > 2000)
    {
        StringVector files;
        HashMap<String, String> nameToPath;
        GetFileSystem()->ScanDir(files, GetFileSystem()->GetProgramDir(), "*.*", SCAN_FILES, false);
        // Remove definitely not plugins.
        for (auto it = files.Begin(); it != files.End(); ++it)
        {
            String baseName = PluginManager::PathToName(*it);
            // Native plugins will rename main file and append version after base name.
            if (baseName.Empty() || IsDigit(static_cast<unsigned int>(baseName.Back())))
                continue;
            nameToPath[baseName] = GetFileSystem()->GetProgramDir() + "/" + *it;
        }

        // Remove deleted plugins.
        for (auto it = modificationTimes_.Begin(); it != modificationTimes_.End();)
        {
            if (!nameToPath.Contains(it->first_))
            {
                names_.Remove(it->first_);
                it = modificationTimes_.Erase(it);
            }
            else
                ++it;
        }

        // Check file types of modified files.
        for (auto it = nameToPath.Begin(); it != nameToPath.End(); ++it)
        {
            const String& pluginName = it->first_;
            const String& pluginPath = it->second_;

            unsigned mtime = GetFileSystem()->GetLastModifiedTime(pluginPath);
            if (modificationTimes_.Contains(pluginName) && modificationTimes_[pluginName] == mtime)
                // File was not changed.
                continue;

            if (PluginManager::GetPluginType(pluginPath) == PLUGIN_INVALID)
                names_.Remove(pluginName);
            else
            {
                if (!names_.Contains(pluginName))
                    names_.Push(pluginName);
                Sort(names_.Begin(), names_.End());
            }
            modificationTimes_[pluginName] = mtime;
        }

        updateTimer_.Reset();
    }

    return names_;
}

}
