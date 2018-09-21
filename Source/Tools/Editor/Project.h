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

#pragma once


#include <Urho3D/Core/Object.h>

#include "Assets/AssetConverter.h"


namespace Urho3D
{

class Project : public Object
{
    URHO3D_OBJECT(Project, Object);
public:
    /// Construct.
    explicit Project(Context* context);
    /// Destruct.
    ~Project() override;
    /// Load existing project. Returns true if project was successfully loaded.
    bool LoadProject(const String& projectPath);
    /// Create a new project. Returns true if successful. Overwrites specified path unconditionally.
    bool SaveProject();
    /// Returns path to temporary asset cache.
    String GetCachePath() const;
    /// Returns path to permanent asset cache.
    String GetResourcePath() const;
    /// Returns a path from which project was loaded.
    const String& GetProjectFilePath() const { return projectFilePath_; }

protected:
    /// Path to a project file.
    String projectFilePath_;
    /// Directory containing project file.
    String projectFileDir_;
    /// Converter responsible for watching resource directories and converting assets to required formats.
    AssetConverter assetConverter_;
    /// Copy of engine resource paths that get unregistered when project is loaded.
    StringVector cachedEngineResourcePaths_;
    /// Path to imgui settings ini file.
    String uiConfigPath_;
};


}
