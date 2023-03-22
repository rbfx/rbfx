//
// Copyright (c) 2008-2022 the Urho3D project.
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

/// \file

#pragma once

#include "Urho3D/Core/Object.h"
#include "Urho3D/Core/Timer.h"
#include "Urho3D/IO/Archive.h"
#include "Urho3D/IO/ArchiveSerialization.h"
#include "Urho3D/IO/FileIdentifier.h"
#include "Urho3D/Resource/JSONValue.h"

#include <EASTL/array.h>
#include <EASTL/optional.h>

namespace Urho3D
{

class Deserializer;
class Serializer;
class XMLElement;

/// Internal file format of Resource.
enum class InternalResourceFormat
{
    /// Resource uses custom serialization logic. Format is unknown.
    Unknown,
    /// Resource is serialized as JSON or JSON Archive.
    Json,
    /// Resource is serialized as XML or XML Archive.
    Xml,
    /// Resource is serialized as binary Archive.
    Binary
};

/// Size of the magic number for binary resources.
static constexpr unsigned BinaryMagicSize = 4;
using BinaryMagic = ea::array<unsigned char, BinaryMagicSize>;
URHO3D_API extern const BinaryMagic DefaultBinaryMagic;

/// Peek into resource file and determine its internal format.
/// It's optimized for the case when the file is either Binary, JSON or XML.
/// Deserializer is left in the same state as it was before the call.
URHO3D_API InternalResourceFormat PeekResourceFormat(
    Deserializer& source, BinaryMagic binaryMagic = DefaultBinaryMagic);

/// Asynchronous loading state of a resource.
enum AsyncLoadState
{
    /// No async operation in progress.
    ASYNC_DONE = 0,
    /// Queued for asynchronous loading.
    ASYNC_QUEUED = 1,
    /// In progress of calling BeginLoad() in a worker thread.
    ASYNC_LOADING = 2,
    /// BeginLoad() succeeded. EndLoad() can be called in the main thread.
    ASYNC_SUCCESS = 3,
    /// BeginLoad() failed.
    ASYNC_FAIL = 4
};

/// Base class for resources.
/// @templateversion
class URHO3D_API Resource : public Object
{
    URHO3D_OBJECT(Resource, Object);

public:
    /// Construct.
    explicit Resource(Context* context);

    /// Load resource by reference.
    static Resource* LoadFromCache(Context* context, StringHash type, const ea::string& name);

    /// Load resource synchronously. Call both BeginLoad() & EndLoad() and return true if both succeeded.
    bool Load(Deserializer& source);
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();
    /// Save resource. Return true if successful.
    virtual bool Save(Serializer& dest) const;

    /// Load resource from file.
    bool LoadFile(const FileIdentifier& fileName);
    /// Save resource to file.
    virtual bool SaveFile(const FileIdentifier& fileName) const;

    /// Set name.
    /// @property
    void SetName(const ea::string& name);
    /// Set memory use in bytes, possibly approximate.
    void SetMemoryUse(unsigned size);
    /// Reset last used timer.
    void ResetUseTimer();
    /// Set the asynchronous loading state. Called by ResourceCache. Resources in the middle of asynchronous loading are not normally returned to user.
    void SetAsyncLoadState(AsyncLoadState newState);
    /// Set absolute file name.
    void SetAbsoluteFileName(const ea::string& fileName) { absoluteFileName_ = fileName; }

    /// Return name.
    /// @property
    const ea::string& GetName() const { return name_; }

    /// Return name hash.
    StringHash GetNameHash() const { return nameHash_; }

    /// Return memory use in bytes, possibly approximate.
    /// @property
    unsigned GetMemoryUse() const { return memoryUse_; }

    /// Return time since last use in milliseconds. If referred to elsewhere than in the resource cache, returns always zero.
    /// @property
    unsigned GetUseTimer();

    /// Return the asynchronous loading state.
    AsyncLoadState GetAsyncLoadState() const { return asyncLoadState_; }

    /// Return absolute file name.
    const ea::string& GetAbsoluteFileName() const { return absoluteFileName_; }

private:
    /// Name.
    ea::string name_;
    /// Name hash.
    StringHash nameHash_;
    /// Absolute file name.
    ea::string absoluteFileName_;
    /// Last used timer.
    Timer useTimer_;
    /// Memory use in bytes.
    unsigned memoryUse_;
    /// Asynchronous loading state.
    AsyncLoadState asyncLoadState_;
};

/// Base class for simple resource that uses Archive serialization.
class URHO3D_API SimpleResource : public Resource
{
    URHO3D_OBJECT(SimpleResource, Resource);

public:
    /// Construct.
    explicit SimpleResource(Context* context);

    /// Force override of SerializeInBlock.
    void SerializeInBlock(Archive& archive) override = 0;

    /// Save resource in specified internal format.
    bool Save(Serializer& dest, InternalResourceFormat format) const;
    /// Save file with specified internal format.
    bool SaveFile(const FileIdentifier& fileName, InternalResourceFormat format) const;

    /// Implement Resource.
    /// @{
    bool BeginLoad(Deserializer& source) override;
    bool EndLoad() override;
    bool Save(Serializer& dest) const override;
    bool SaveFile(const FileIdentifier& fileName) const override;
    /// @}

protected:
    /// Binary archive magic word. Should be 4 bytes.
    virtual BinaryMagic GetBinaryMagic() const { return DefaultBinaryMagic; }
    /// Root block name. Used for XML serialization only.
    virtual const char* GetRootBlockName() const { return "resource"; }
    /// Default internal resource format on save.
    virtual InternalResourceFormat GetDefaultInternalFormat() const { return InternalResourceFormat::Json; }
    /// Try to load legacy XML format, whatever it is.
    virtual bool LoadLegacyXML(const XMLElement& source) { return false; }

private:
    ea::optional<InternalResourceFormat> loadFormat_;
};

/// Base class for resources that support arbitrary metadata stored. Metadata serialization shall be implemented in derived classes.
class URHO3D_API ResourceWithMetadata : public Resource
{
    URHO3D_OBJECT(ResourceWithMetadata, Resource);

public:
    /// Construct.
    explicit ResourceWithMetadata(Context* context) : Resource(context) {}

    /// Add new metadata variable or overwrite old value.
    /// @property{set_metadata}
    void AddMetadata(const ea::string& name, const Variant& value);
    /// Remove metadata variable.
    void RemoveMetadata(const ea::string& name);
    /// Remove all metadata variables.
    void RemoveAllMetadata();
    /// Return all metadata keys.
    const StringVector& GetMetadataKeys() const { return metadataKeys_; }
    /// Return metadata variable.
    /// @property
    const Variant& GetMetadata(const ea::string& name) const;
    /// Return whether the resource has metadata.
    /// @property
    bool HasMetadata() const;

protected:
    /// Load metadata from <metadata> children of XML element.
    void LoadMetadataFromXML(const XMLElement& source);
    /// Load metadata from JSON array.
    void LoadMetadataFromJSON(const JSONArray& array);
    /// Save as <metadata> children of XML element.
    void SaveMetadataToXML(XMLElement& destination) const;
    /// Copy metadata from another resource.
    void CopyMetadata(const ResourceWithMetadata& source);

private:
    /// Animation metadata variables.
    VariantMap metadata_;
    /// Animation metadata keys.
    StringVector metadataKeys_;
};

/// Serialize reference to a resource.
template <class T, std::enable_if_t<std::is_base_of_v<Resource, T>, int> = 0>
inline void SerializeResource(Archive& archive, const char* name, SharedPtr<T>& value, ResourceRef& resourceRef)
{
    SerializeValue(archive, name, resourceRef);
    if (archive.IsInput())
        value = SharedPtr<T>(dynamic_cast<T*>(Resource::LoadFromCache(archive.GetContext(), resourceRef.type_, resourceRef.name_)));
}

inline const ea::string& GetResourceName(Resource* resource)
{
    return resource ? resource->GetName() : EMPTY_STRING;
}

inline StringHash GetResourceType(Resource* resource, StringHash defaultType)
{
    return resource ? resource->GetType() : defaultType;
}

inline ResourceRef GetResourceRef(Resource* resource, StringHash defaultType)
{
    return ResourceRef(GetResourceType(resource, defaultType), GetResourceName(resource));
}

template <class T> ea::vector<ea::string> GetResourceNames(const ea::vector<SharedPtr<T> >& resources)
{
    ea::vector<ea::string> ret(resources.size());
    for (unsigned i = 0; i < resources.size(); ++i)
        ret[i] = GetResourceName(resources[i]);

    return ret;
}

template <class T> ResourceRefList GetResourceRefList(const ea::vector<SharedPtr<T> >& resources)
{
    return ResourceRefList(T::GetTypeStatic(), GetResourceNames(resources));
}

}
