//
// Copyright (c) 2017-2019 the rbfx project.
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

#include "Tabs/Tab.h"

namespace Urho3D
{

class Asset;

/// A base class for all asset importers. Classes that inherit from this class must be added to Pipeline::importers_ list.
class AssetImporter : public Serializable, public IInspectorProvider
{
    URHO3D_OBJECT(AssetImporter, Serializable);
public:
    ///
    explicit AssetImporter(Context* context);
    /// Renders inspector contents of asset importer.
    void RenderInspector(const char* filter) override;
    /// Returns `true` if importer is going to try importing specified path.
    virtual bool Accepts(const ea::string& path) const { return false; }
    /// May be called from non-main thread. Returns a list of produced files in `byproducts` vector and `true` on success.
    virtual bool Execute(Urho3D::Asset* input, const ea::string& inputFile, const ea::string& outputPath) = 0;
    ///
    bool SaveJSON(JSONValue& dest) const override;
    ///
    bool LoadJSON(const JSONValue& source) override;
    /// Returns true when importer is not required to run during editing session.
    bool IsOptional() const { return isOptional_; }
    ///
    bool IsModified();
    ///
    void OnGetAttribute(const AttributeInfo& attr, Variant& dest) const override;
    ///
    void OnSetAttribute(const AttributeInfo& attr, const Variant& src) override;
    ///
    const StringVector& GetByproducts() const { return byproducts_; }

protected:
    /// Sets needed asset information. Called after creating every importer.
    void Initialize(Asset* asset, const ea::string& flavor);
    ///
    void ClearByproducts();
    ///
    void AddByproduct(const ea::string& byproduct);
    ///
    bool SaveDefaultAttributes(const AttributeInfo& attr) const override;
    ///
    bool SetInheritedDefaultsForImporter(const AttributeInfo& attr, const Variant& src);
    ///
    void SetInheritedDefaultsIfSet();
    ///
    void SetInheritedDefaults(const AttributeInfo& attr, const Variant& src);

    /// Asset this importer belongs to.
    WeakPtr<Asset> asset_;
    /// Flavor this importer belongs to.
    ea::string flavor_{};
    /// Assets that were created by running this asset through conversion pipeline.
    StringVector byproducts_;
    /// Set to true when attributes are modified in inspector.
    bool attributesModified_ = false;
    ///
    AttributeInspector inspector_{context_};
    /// Flag indicating that project may function without running this importer.
    /// For example project may skip texture compression and load uncompressed textures.
    bool isOptional_ = false;
    ///
    ea::unordered_map<StringHash, bool> isAttributeSet_{};
public:

protected:

    friend class Asset;
};

}
