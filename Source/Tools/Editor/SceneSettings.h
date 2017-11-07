//
// Copyright (c) 2008-2017 the Urho3D project.
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


#include <Urho3D/Scene/Serializable.h>
#include <Urho3D/Resource/XMLElement.h>


namespace Urho3D
{

class SceneTab;

/// Class handling common scene settings
class SceneSettings : public Serializable
{
URHO3D_OBJECT(SceneSettings, Serializable);
public:
    /// Construct
    explicit SceneSettings(Context* context);
    /// Save settings into project file.
    void SaveProject(XMLElement scene);
    /// Load settings from a project file.
    void LoadProject(XMLElement scene);

    /// Flag which determines if "Elapsed Time" attribute of a scene should be saved.
    bool saveElapsedTime_ = false;
};

/// Class handling scene postprocess effect settings
class SceneEffects : public Serializable
{
    URHO3D_OBJECT(SceneEffects, Serializable);
public:
    /// Construct
    explicit SceneEffects(SceneTab* tab);
    /// This method should be called before rendering attributes. It handles rebuilding of attribute cache.
    void Prepare(bool force=false);
    /// Save settings into project file.
    void SaveProject(XMLElement scene);
    /// Load settings from a project file.
    void LoadProject(XMLElement scene);

protected:
    /// Flag which signals that attributes should be rebuilt.
    bool rebuild_ = true;
    /// Pointer to tab which owns this object.
    WeakPtr<SceneTab> tab_;

    struct PostProcess
    {
        /// List of postprocess tags present in the file.
        StringVector tags_;
        /// Fake enum name array for attribute when there are more than one tag.
        PODVector<const char*> tagEnumNames_;
        /// Variable names mapped to number of floats variable contains.
        HashMap<String, int> variables_;
    };
    /// Cached effect data so we do not read disk on every frame.
    HashMap<String, PostProcess> effects_;
    /// Cached list of renderpaths.
    StringVector renderPaths_;
    /// Fake enum name array of renderpaths.
    PODVector<const char*> renderPathsEnumNames_;
    /// Index of current renderpath
    int currentRenderPath_ = -1;
};

}

