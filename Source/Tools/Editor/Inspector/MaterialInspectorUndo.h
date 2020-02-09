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

#pragma once


#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Toolbox/Common/UndoStack.h>


namespace Urho3D
{

/// Tracks addition, removal and modification of techniques in material
class UndoTechniqueChanged
        : public UndoAction
{
public:
    struct TechniqueInfo
    {
        ea::string techniqueName_;
        MaterialQuality qualityLevel_;
        float lodDistance_;
    };

    UndoTechniqueChanged(const Material* material, unsigned index, const TechniqueEntry* oldEntry, const TechniqueEntry* newEntry);
    void RemoveTechnique();
    void AddTechnique(const TechniqueInfo& info);
    void SetTechnique(const TechniqueInfo& info);
    bool Undo(Context* context) override;
    bool Redo(Context* context) override;

private:
    Context* context_;
    ea::string materialName_;
    TechniqueInfo oldValue_;
    TechniqueInfo newValue_;
    unsigned index_;
};


/// Tracks addition, removal and modification of shader parameters in material
class UndoShaderParameterChanged
    : public UndoAction
{
public:
    struct ShaderParameterInfo
    {
        ea::string techniqueName_;
        MaterialQuality qualityLevel_;
        float lodDistance_;
    };

    UndoShaderParameterChanged(const Material* material, const ea::string& parameterName, const Variant& oldValue, const Variant& newValue);
    bool Undo(Context* context) override;
    bool Redo(Context* context) override;

private:
    Context* context_;
    ea::string materialName_;
    ea::string parameterName_;
    Variant oldValue_;
    Variant newValue_;
};

}
