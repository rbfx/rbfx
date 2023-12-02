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

#include "../Core/Mutex.h"
#include "../Resource/JSONFile.h"
#include "../Resource/XMLElement.h"
#include "../Scene/Node.h"
#include "../Scene/SceneResolver.h"

#include <EASTL/span.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/unordered_set.h>

namespace Urho3D
{

class File;
class PackageFile;
class Texture2D;

/// TODO: Get rid of "replicated" word in the code. It is not used in the networking code anymore.
static const unsigned FIRST_REPLICATED_ID = 0x1;
static const unsigned LAST_REPLICATED_ID = 0xffffff;

/// Asynchronous scene loading mode.
enum LoadMode
{
    /// Preload resources used by a scene or object prefab file, but do not load any scene content.
    LOAD_RESOURCES_ONLY = 0,
    /// Load scene content without preloading. Resources will be requested synchronously when encountered.
    LOAD_SCENE,
    /// Default mode: preload resources used by the scene first, then load the scene content.
    LOAD_SCENE_AND_RESOURCES
};

/// Asynchronous loading progress of a scene.
struct AsyncProgress
{
    /// File for binary mode.
    AbstractFilePtr file_;
    /// XML file for XML mode.
    SharedPtr<XMLFile> xmlFile_;
    /// JSON file for JSON mode.
    SharedPtr<JSONFile> jsonFile_;

    /// Current XML element for XML mode.
    XMLElement xmlElement_;

    /// Current JSON child array and for JSON mode.
    unsigned jsonIndex_;

    /// Current load mode.
    LoadMode mode_;
    /// Resource name hashes left to load.
    ea::hash_set<StringHash> resources_;
    /// Loaded resources.
    unsigned loadedResources_;
    /// Total resources.
    unsigned totalResources_;
    /// Loaded root-level nodes.
    unsigned loadedNodes_;
    /// Total root-level nodes.
    unsigned totalNodes_;
};

/// Index of components in the Scene.
using SceneComponentIndex = ea::hash_set<Component*>;

/// Root scene node, represents the whole scene.
class URHO3D_API Scene : public Node
{
    URHO3D_OBJECT(Scene, Node);

public:
    using Node::GetComponent;
    using Node::Load;
    using Node::Save;
    using Node::SaveXML;
    using Node::SaveJSON;

    /// Construct.
    explicit Scene(Context* context);
    /// Destruct.
    ~Scene() override;
    /// Register object factory. Node must be registered first.
    /// @nobind
    static void RegisterObject(Context* context);

    /// Create component index. Scene must be empty.
    bool CreateComponentIndex(StringHash componentType);
    /// Create component index for template type. Scene must be empty.
    template <class T> void CreateComponentIndex() { CreateComponentIndex(T::GetTypeStatic()); }
    /// Return component index. Iterable. Invalidated when indexed component is added or removed!
    const SceneComponentIndex& GetComponentIndex(StringHash componentType);
    /// Return component index for template type. Invalidated when indexed component is added or removed!
    template <class T> const SceneComponentIndex& GetComponentIndex() { return GetComponentIndex(T::GetTypeStatic()); }

    /// Serialize object. May throw ArchiveException.
    void SerializeInBlock(Archive& archive) override;
    void SerializeInBlock(
        Archive& archive, bool serializeTemporary, PrefabSaveFlags saveFlags, PrefabLoadFlags loadFlags);

    /// Load from binary data. Removes all existing child nodes and components first. Return true if successful.
    bool Load(Deserializer& source) override;
    /// Save to binary data. Return true if successful.
    bool Save(Serializer& dest) const override;
    /// Load from XML data. Removes all existing child nodes and components first. Return true if successful.
    bool LoadXML(const XMLElement& source) override;
    /// Load from JSON data. Removes all existing child nodes and components first. Return true if successful.
    bool LoadJSON(const JSONValue& source) override;

    /// Return number of lightmaps.
    unsigned GetNumLightmaps() const { return lightmaps_.names_.size(); }
    /// Reset lightmaps.
    void ResetLightmaps();
    /// Add lightmap texture.
    void AddLightmap(const ea::string& lightmapTextureName);
    /// Return lightmap texture.
    Texture2D* GetLightmapTexture(unsigned index) const;

    /// Load from an XML file. Return true if successful.
    bool LoadXML(Deserializer& source);
    /// Load from a JSON file. Return true if successful.
    bool LoadJSON(Deserializer& source);
    /// Save to an XML file. Return true if successful.
    bool SaveXML(Serializer& dest, const ea::string& indentation = "\t") const;
    /// Save to a JSON file. Return true if successful.
    bool SaveJSON(Serializer& dest, const ea::string& indentation = "\t") const;
    /// Load from a binary file asynchronously. Return true if started successfully. The LOAD_RESOURCES_ONLY mode can also be used to preload resources from object prefab files.
    bool LoadAsync(AbstractFilePtr file, LoadMode mode = LOAD_SCENE_AND_RESOURCES);
    /// Load from an XML file asynchronously. Return true if started successfully. The LOAD_RESOURCES_ONLY mode can also be used to preload resources from object prefab files.
    bool LoadAsyncXML(AbstractFilePtr file, LoadMode mode = LOAD_SCENE_AND_RESOURCES);
    /// Load from a JSON file asynchronously. Return true if started successfully. The LOAD_RESOURCES_ONLY mode can also be used to preload resources from object prefab files.
    bool LoadAsyncJSON(AbstractFilePtr file, LoadMode mode = LOAD_SCENE_AND_RESOURCES);
    /// Stop asynchronous loading.
    void StopAsyncLoading();
    /// Instantiate scene content from binary data. Return root node if successful.
    Node* Instantiate(Deserializer& source, const Vector3& position, const Quaternion& rotation);
    /// Instantiate scene content from XML data. Return root node if successful.
    Node* InstantiateXML
        (const XMLElement& source, const Vector3& position, const Quaternion& rotation);
    /// Instantiate scene content from XML data. Return root node if successful.
    Node* InstantiateXML(Deserializer& source, const Vector3& position, const Quaternion& rotation);
    /// Instantiate scene content from JSON data. Return root node if successful.
    Node* InstantiateJSON
        (const JSONValue& source, const Vector3& position, const Quaternion& rotation);
    /// Instantiate scene content from JSON data. Return root node if successful.
    Node* InstantiateJSON(Deserializer& source, const Vector3& position, const Quaternion& rotation);

    /// Clear scene completely.
    void Clear();
    /// Enable or disable scene update.
    /// @property
    void SetUpdateEnabled(bool enable);
    /// Set update time scale. 1.0 = real time (default).
    /// @property
    void SetTimeScale(float scale);
    /// Set elapsed time in seconds. This can be used to prevent inaccuracy in the timer if the scene runs for a long time.
    /// @property
    void SetElapsedTime(float time);
    /// Set maximum milliseconds per frame to spend on async scene loading.
    /// @property
    void SetAsyncLoadingMs(int ms);
    /// Add a required package file for networking. To be called on the server.
    void AddRequiredPackageFile(PackageFile* package);
    /// Clear required package files.
    void ClearRequiredPackageFiles();
    /// Register a node user variable hash reverse mapping (for editing).
    void RegisterVar(const ea::string& name);
    /// Unregister a node user variable hash reverse mapping.
    void UnregisterVar(const ea::string& name);
    /// Clear all registered node user variable hash reverse mappings.
    void UnregisterAllVars();

    /// Set source file name.
    void SetFileName(const ea::string_view fileName) { fileName_ = fileName; }

    /// Return whether the Scene is empty.
    bool IsEmpty(bool ignoreComponents = false) const;
    /// Return node from the whole scene by ID, or null if not found.
    Node* GetNode(unsigned id) const;
    /// Return component from the whole scene by ID, or null if not found.
    Component* GetComponent(unsigned id) const;
    /// Get nodes with specific tag from the whole scene, return false if empty.
    bool GetNodesWithTag(ea::vector<Node*>& dest, const ea::string& tag)  const;

    /// Return whether updates are enabled.
    /// @property
    bool IsUpdateEnabled() const { return updateEnabled_; }

    /// Return whether an asynchronous loading operation is in progress.
    /// @property
    bool IsAsyncLoading() const { return asyncLoading_; }

    /// Return asynchronous loading progress between 0.0 and 1.0, or 1.0 if not in progress.
    /// @property
    float GetAsyncProgress() const;

    /// Return the load mode of the current asynchronous loading operation.
    /// @property
    LoadMode GetAsyncLoadMode() const { return asyncProgress_.mode_; }

    /// Return source file name.
    /// @property
    const ea::string& GetFileName() const { return fileName_; }

    /// Return source file checksum.
    /// @property
    unsigned GetChecksum() const { return checksum_; }

    /// Return update time scale.
    /// @property
    float GetTimeScale() const { return timeScale_; }

    /// Return elapsed time in seconds.
    /// @property
    float GetElapsedTime() const { return elapsedTime_; }

    /// Return maximum milliseconds per frame to spend on async loading.
    /// @property
    int GetAsyncLoadingMs() const { return asyncLoadingMs_; }

    /// Return required package files.
    /// @property
    const ea::vector<SharedPtr<PackageFile> >& GetRequiredPackageFiles() const { return requiredPackageFiles_; }

    /// Return all nodes parented by this Scene.
    const ea::unordered_map<unsigned, Node*>& GetAllNodes() const { return replicatedNodes_; }

    /// Return all components parented by this Scene.
    const ea::unordered_map<unsigned, Component*>& GetAllComponents() const { return replicatedComponents_; }

    /// Return a node user variable name, or empty if not registered.
    const ea::string& GetVarName(StringHash hash) const;

    /// Update scene. Called by HandleUpdate.
    void Update(float timeStep);
    /// Begin a threaded update. During threaded update components can choose to delay dirty processing.
    void BeginThreadedUpdate();
    /// End a threaded update. Notify components that marked themselves for delayed dirty processing.
    void EndThreadedUpdate();
    /// Add a component to the delayed dirty notify queue. Is thread-safe.
    void DelayedMarkedDirty(Component* component);

    /// Return threaded update flag.
    bool IsThreadedUpdate() const { return threadedUpdate_; }

    /// Get free node ID.
    unsigned GetFreeNodeID();
    /// Get free component ID.
    unsigned GetFreeComponentID();

    /// Cache node by tag if tag not zero, no checking if already added. Used internaly in Node::AddTag.
    void NodeTagAdded(Node* node, const ea::string& tag);
    /// Cache node by tag if tag not zero.
    void NodeTagRemoved(Node* node, const ea::string& tag);

    /// Node added. Assign scene pointer and add to ID map.
    void NodeAdded(Node* node);
    /// Node removed. Remove from ID map.
    void NodeRemoved(Node* node);
    /// Component added. Add to ID map.
    void ComponentAdded(Component* component);
    /// Component removed. Remove from ID map.
    void ComponentRemoved(Component* component);
    /// Set node user variable reverse mappings.
    void SetVarNamesAttr(const ea::string& value);
    /// Return node user variable reverse mappings.
    ea::string GetVarNamesAttr() const;

private:
    /// Handle the logic update event to update the scene, if active.
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    /// Handle a background loaded resource completing.
    void HandleResourceBackgroundLoaded(StringHash eventType, VariantMap& eventData);
    /// Update asynchronous loading.
    void UpdateAsyncLoading();
    /// Finish asynchronous loading.
    void FinishAsyncLoading();
    /// Finish loading. Sets the scene filename and checksum.
    void FinishLoading(Deserializer* source);
    /// Finish saving. Sets the scene filename and checksum.
    void FinishSaving(Serializer* dest) const;
    /// Preload resources from a binary scene or object prefab file.
    void PreloadResources(AbstractFilePtr file, bool isSceneFile);
    /// Preload resources from an XML scene or object prefab file.
    void PreloadResourcesXML(const XMLElement& element);
    /// Preload resources from a JSON scene or object prefab file.
    void PreloadResourcesJSON(const JSONValue& value);
    /// Return component index storage for given type.
    SceneComponentIndex* GetMutableComponentIndex(StringHash componentType);
    /// Reload lightmap textures.
    void ReloadLightmaps();

    /// Types of components that should be indexed.
    ea::vector<StringHash> indexedComponentTypes_;
    /// Indexes of components.
    ea::vector<SceneComponentIndex> componentIndexes_;

    /// Replicated scene nodes by ID.
    ea::unordered_map<unsigned, Node*> replicatedNodes_;
    /// Replicated components by ID.
    ea::unordered_map<unsigned, Component*> replicatedComponents_;
    /// Cached tagged nodes by tag.
    ea::unordered_map<StringHash, ea::vector<Node*> > taggedNodes_;
    /// Asynchronous loading progress.
    AsyncProgress asyncProgress_;
    /// Node and component ID resolver for asynchronous loading.
    SceneResolver resolver_;
    /// Source file name.
    mutable ea::string fileName_;
    /// Required package files for networking.
    ea::vector<SharedPtr<PackageFile> > requiredPackageFiles_;
    /// Registered node user variable reverse mappings.
    ea::unordered_map<StringHash, ea::string> varNames_;
    /// Delayed dirty notification queue for components.
    ea::vector<Component*> delayedDirtyComponents_;
    /// Mutex for the delayed dirty notification queue.
    Mutex sceneMutex_;
    /// Next free non-local node ID.
    unsigned replicatedNodeID_;
    /// Next free non-local component ID.
    unsigned replicatedComponentID_;
    /// Scene source file checksum.
    mutable unsigned checksum_;
    /// Maximum milliseconds per frame to spend on async scene loading.
    int asyncLoadingMs_;
    /// Scene update time scale.
    float timeScale_;
    /// Elapsed time accumulator.
    float elapsedTime_;
    /// Update enabled flag.
    bool updateEnabled_;
    /// Asynchronous loading flag.
    bool asyncLoading_;
    /// Threaded update flag.
    bool threadedUpdate_;

    /// Lightmap textures names.
    ResourceRefList lightmaps_;
    /// Loaded lightmap textures.
    ea::vector<SharedPtr<Texture2D>> lightmapTextures_;
};

/// Register Scene library objects.
/// @nobind
void URHO3D_API RegisterSceneLibrary(Context* context);

}
