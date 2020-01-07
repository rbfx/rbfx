//
// Copyright (c) 2008-2020 the Urho3D project.
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

#include <EASTL/sort.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Graphics/Tangent.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/XMLFile.h>

#include "OgreImporterUtils.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <Urho3D/DebugNew.h>


static const int VERTEX_CACHE_SIZE = 32;

SharedPtr<Context> context_(new Context());
SharedPtr<XMLFile> meshFile_(new XMLFile(context_));
SharedPtr<XMLFile> skelFile_(new XMLFile(context_));
ea::vector<ModelIndexBuffer> indexBuffers_;
ea::vector<ModelVertexBuffer> vertexBuffers_;
ea::vector<ea::vector<ModelSubGeometryLodLevel> > subGeometries_;
ea::vector<Vector3> subGeometryCenters_;
ea::vector<ModelBone> bones_;
ea::vector<ModelMorph> morphs_;
ea::vector<ea::string> materialNames_;
BoundingBox boundingBox_;
unsigned maxBones_ = 64;
unsigned numSubMeshes_ = 0;
bool useOneBuffer_ = true;

int main(int argc, char** argv);
void Run(const ea::vector<ea::string>& arguments);
void LoadSkeleton(const ea::string& skeletonFileName);
void LoadMesh(const ea::string& inputFileName, bool generateTangents, bool splitSubMeshes, bool exportMorphs);
void WriteOutput(const ea::string& outputFileName, bool exportAnimations, bool rotationsOnly, bool saveMaterialList);
void OptimizeIndices(ModelSubGeometryLodLevel* subGeom, ModelVertexBuffer* vb, ModelIndexBuffer* ib);
void CalculateScore(ModelVertex& vertex);
ea::string SanitateAssetName(const ea::string& name);

int main(int argc, char** argv)
{
    ea::vector<ea::string> arguments;

    #ifdef WIN32
    arguments = ParseArguments(GetCommandLineW());
    #else
    arguments = ParseArguments(argc, argv);
    #endif

    Run(arguments);
    return 0;
}

void Run(const ea::vector<ea::string>& arguments)
{
    if (arguments.size() < 2)
    {
        ErrorExit(
            "Usage: OgreImporter <input file> <output file> [options]\n\n"
            "Options:\n"
            "-l      Output a material list file\n"
            "-na     Do not output animations\n"
            "-nm     Do not output morphs\n"
            "-r      Output only rotations from animations\n"
            "-s      Split each submesh into own vertex buffer\n"
            "-t      Generate tangents\n"
            "-mb <x> Maximum number of bones per submesh, default 64\n"
        );
    }

    bool generateTangents = false;
    bool splitSubMeshes = false;
    bool exportAnimations = true;
    bool exportMorphs = true;
    bool rotationsOnly = false;
    bool saveMaterialList = false;

    if (arguments.size() > 2)
    {
        for (unsigned i = 2; i < arguments.size(); ++i)
        {
            if (arguments[i].length() > 1 && arguments[i][0] == '-')
            {
                ea::string argument = arguments[i].substr(1).to_lower();
                if (argument == "l")
                    saveMaterialList = true;
                else if (argument == "r")
                    rotationsOnly = true;
                else if (argument == "s")
                    splitSubMeshes = true;
                else if (argument == "t")
                    generateTangents = true;
                else if (argument.length() == 2 && argument[0] == 'n')
                {
                    switch (tolower(argument[1]))
                    {
                    case 'a':
                        exportAnimations = false;
                        break;

                    case 'm':
                        exportMorphs = false;
                        break;
                    }
                    break;
                }
                else if (argument == "mb" && i < arguments.size() - 1)
                {
                    maxBones_ = ToUInt(arguments[i + 1]);
                    if (maxBones_ < 1)
                        maxBones_ = 1;
                    ++i;
                }
            }
        }
    }

    LoadMesh(arguments[0], generateTangents, splitSubMeshes, exportMorphs);
    WriteOutput(arguments[1], exportAnimations, rotationsOnly, saveMaterialList);

    PrintLine("Finished");
}

void LoadSkeleton(const ea::string& skeletonFileName)
{
    // Process skeleton first (if found)
    XMLElement skeletonRoot;
    File skeletonFileSource(context_);
    skeletonFileSource.Open(skeletonFileName);
    if (!skelFile_->Load(skeletonFileSource))
        PrintLine("Failed to load skeleton " + skeletonFileName);
    skeletonRoot = skelFile_->GetRoot();

    if (skeletonRoot)
    {
        XMLElement bonesRoot = skeletonRoot.GetChild("bones");
        XMLElement bone = bonesRoot.GetChild("bone");
        while (bone)
        {
            unsigned index = bone.GetInt("id");
            ea::string name = bone.GetAttribute("name");
            if (index >= bones_.size())
                bones_.resize(index + 1);

            // Convert from right- to left-handed
            XMLElement position = bone.GetChild("position");
            float x = position.GetFloat("x");
            float y = position.GetFloat("y");
            float z = position.GetFloat("z");
            Vector3 pos(x, y, -z);

            XMLElement rotation = bone.GetChild("rotation");
            XMLElement axis = rotation.GetChild("axis");
            float angle = -rotation.GetFloat("angle") * M_RADTODEG;
            x = axis.GetFloat("x");
            y = axis.GetFloat("y");
            z = axis.GetFloat("z");
            Vector3 axisVec(x, y, -z);
            Quaternion rot(angle, axisVec);

            bones_[index].name_ = name;
            bones_[index].parentIndex_ = index; // Fill in the correct parent later
            bones_[index].bindPosition_ = pos;
            bones_[index].bindRotation_ = rot;
            bones_[index].bindScale_ = Vector3::ONE;
            bones_[index].collisionMask_ = 0;
            bones_[index].radius_ = 0.0f;

            bone = bone.GetNext("bone");
        }

        // Go through the bone hierarchy
        XMLElement boneHierarchy = skeletonRoot.GetChild("bonehierarchy");
        XMLElement boneParent = boneHierarchy.GetChild("boneparent");
        while (boneParent)
        {
            ea::string bone = boneParent.GetAttribute("bone");
            ea::string parent = boneParent.GetAttribute("parent");
            unsigned i = 0, j = 0;
            for (i = 0; i < bones_.size() && bones_[i].name_ != bone; ++i);
            for (j = 0; j < bones_.size() && bones_[j].name_ != parent; ++j);

            if (i >= bones_.size() || j >= bones_.size())
                ErrorExit("Found indeterminate parent bone assignment");
            bones_[i].parentIndex_ = j;

            boneParent = boneParent.GetNext("boneparent");
        }

        // Calculate bone derived positions
        for (unsigned i = 0; i < bones_.size(); ++i)
        {
            Vector3 derivedPosition = bones_[i].bindPosition_;
            Quaternion derivedRotation = bones_[i].bindRotation_;
            Vector3 derivedScale = bones_[i].bindScale_;

            unsigned index = bones_[i].parentIndex_;
            if (index != i)
            {
                for (;;)
                {
                    derivedPosition = bones_[index].bindPosition_ + (bones_[index].bindRotation_ * (bones_[index].bindScale_ * derivedPosition));
                    derivedRotation = bones_[index].bindRotation_ * derivedRotation;
                    derivedScale = bones_[index].bindScale_ * derivedScale;
                    if (bones_[index].parentIndex_ != index)
                        index = bones_[index].parentIndex_;
                    else
                        break;
                }
            }

            bones_[i].derivedPosition_ = derivedPosition;
            bones_[i].derivedRotation_ = derivedRotation;
            bones_[i].derivedScale_ = derivedScale;
            bones_[i].worldTransform_ = Matrix3x4(derivedPosition, derivedRotation, derivedScale);
            bones_[i].inverseWorldTransform_ = bones_[i].worldTransform_.Inverse();
        }

        PrintLine("Processed skeleton");
    }
}

void LoadMesh(const ea::string& inputFileName, bool generateTangents, bool splitSubMeshes, bool exportMorphs)
{
    File meshFileSource(context_);
    meshFileSource.Open(inputFileName);
    if (!meshFile_->Load(meshFileSource))
        ErrorExit("Could not load input file " + inputFileName);

    XMLElement root = meshFile_->GetRoot("mesh");
    XMLElement subMeshes = root.GetChild("submeshes");
    XMLElement skeletonLink = root.GetChild("skeletonlink");
    if (root.IsNull())
        ErrorExit("Could not load input file " + inputFileName);

    ea::string skeletonName = skeletonLink.GetAttribute("name");
    if (!skeletonName.empty())
        LoadSkeleton(GetPath(inputFileName) + GetFileName(skeletonName) + ".skeleton.xml");

    // Check whether there's benefit of avoiding 32bit indices by splitting each submesh into own buffer
    XMLElement subMesh = subMeshes.GetChild("submesh");
    unsigned totalVertices = 0;
    unsigned maxSubMeshVertices = 0;
    while (subMesh)
    {
        materialNames_.push_back(subMesh.GetAttribute("material"));
        XMLElement geometry = subMesh.GetChild("geometry");
        if (geometry)
        {
            unsigned vertices = geometry.GetInt("vertexcount");
            totalVertices += vertices;
            if (maxSubMeshVertices < vertices)
                maxSubMeshVertices = vertices;
        }
        ++numSubMeshes_;

        subMesh = subMesh.GetNext("submesh");
    }

    XMLElement sharedGeometry = root.GetChild("sharedgeometry");
    if (sharedGeometry)
    {
        unsigned vertices = sharedGeometry.GetInt("vertexcount");
        totalVertices += vertices;
        if (maxSubMeshVertices < vertices)
            maxSubMeshVertices = vertices;
    }

    if (!sharedGeometry && (splitSubMeshes || (totalVertices > 65535 && maxSubMeshVertices <= 65535)))
    {
        useOneBuffer_ = false;
        vertexBuffers_.resize(numSubMeshes_);
        indexBuffers_.resize(numSubMeshes_);
    }
    else
    {
        vertexBuffers_.resize(1);
        indexBuffers_.resize(1);
    }

    subMesh = subMeshes.GetChild("submesh");
    unsigned indexStart = 0;
    unsigned vertexStart = 0;
    unsigned subMeshIndex = 0;

    ea::vector<unsigned> vertexStarts;
    vertexStarts.resize(numSubMeshes_);

    while (subMesh)
    {
        XMLElement geometry = subMesh.GetChild("geometry");
        XMLElement faces = subMesh.GetChild("faces");

        // If no submesh vertexbuffer, process the shared geometry, but do it only once
        unsigned vertices = 0;
        if (!geometry)
        {
            vertexStart = 0;
            if (!subMeshIndex)
                geometry = root.GetChild("sharedgeometry");
        }

        if (geometry)
            vertices = geometry.GetInt("vertexcount");

        ModelSubGeometryLodLevel subGeometryLodLevel;
        ModelVertexBuffer* vBuf;
        ModelIndexBuffer* iBuf;

        if (useOneBuffer_)
        {
            vBuf = &vertexBuffers_[0];
            if (vertices)
                vBuf->vertices_.resize(vertexStart + vertices);
            iBuf = &indexBuffers_[0];

            subGeometryLodLevel.vertexBuffer_ = 0;
            subGeometryLodLevel.indexBuffer_ = 0;
        }
        else
        {
            vertexStart = 0;
            indexStart = 0;

            vBuf = &vertexBuffers_[subMeshIndex];
            vBuf->vertices_.resize(vertices);
            iBuf = &indexBuffers_[subMeshIndex];

            subGeometryLodLevel.vertexBuffer_ = subMeshIndex;
            subGeometryLodLevel.indexBuffer_ = subMeshIndex;
        }

        // Store the start vertex for later use
        vertexStarts[subMeshIndex] = vertexStart;

        // Ogre may have multiple buffers in one submesh. These will be merged into one
        XMLElement bufferDef;
        if (geometry)
            bufferDef = geometry.GetChild("vertexbuffer");

        while (bufferDef)
        {
            if (bufferDef.HasAttribute("positions"))
                vBuf->elementMask_ |= MASK_POSITION;
            if (bufferDef.HasAttribute("normals"))
                vBuf->elementMask_ |= MASK_NORMAL;
            if (bufferDef.HasAttribute("texture_coords"))
            {
                vBuf->elementMask_ |= MASK_TEXCOORD1;
                if (bufferDef.GetInt("texture_coords") > 1)
                    vBuf->elementMask_ |= MASK_TEXCOORD2;
            }

            unsigned vertexNum = vertexStart;
            if (vertices)
            {
                XMLElement vertex = bufferDef.GetChild("vertex");
                while (vertex)
                {
                    XMLElement position = vertex.GetChild("position");
                    if (position)
                    {
                        // Convert from right- to left-handed
                        float x = position.GetFloat("x");
                        float y = position.GetFloat("y");
                        float z = position.GetFloat("z");
                        Vector3 vec(x, y, -z);

                        vBuf->vertices_[vertexNum].position_ = vec;
                        boundingBox_.Merge(vec);
                    }
                    XMLElement normal = vertex.GetChild("normal");
                    if (normal)
                    {
                        // Convert from right- to left-handed
                        float x = normal.GetFloat("x");
                        float y = normal.GetFloat("y");
                        float z = normal.GetFloat("z");
                        Vector3 vec(x, y, -z);

                        vBuf->vertices_[vertexNum].normal_ = vec;
                    }
                    XMLElement uv = vertex.GetChild("texcoord");
                    if (uv)
                    {
                        float x = uv.GetFloat("u");
                        float y = uv.GetFloat("v");
                        Vector2 vec(x, y);

                        vBuf->vertices_[vertexNum].texCoord1_ = vec;

                        if (vBuf->elementMask_ & MASK_TEXCOORD2)
                        {
                            uv = uv.GetNext("texcoord");
                            if (uv)
                            {
                                float x = uv.GetFloat("u");
                                float y = uv.GetFloat("v");
                                Vector2 vec(x, y);

                                vBuf->vertices_[vertexNum].texCoord2_ = vec;
                            }
                        }
                    }

                    vertexNum++;
                    vertex = vertex.GetNext("vertex");
                }
            }

            bufferDef = bufferDef.GetNext("vertexbuffer");
        }

        unsigned triangles = faces.GetInt("count");
        unsigned indices = triangles * 3;

        XMLElement triangle = faces.GetChild("face");
        while (triangle)
        {
            unsigned v1 = triangle.GetInt("v1");
            unsigned v2 = triangle.GetInt("v2");
            unsigned v3 = triangle.GetInt("v3");
            iBuf->indices_.push_back(v3 + vertexStart);
            iBuf->indices_.push_back(v2 + vertexStart);
            iBuf->indices_.push_back(v1 + vertexStart);
            triangle = triangle.GetNext("face");
        }

        subGeometryLodLevel.indexStart_ = indexStart;
        subGeometryLodLevel.indexCount_ = indices;
        if (vertexStart + vertices > 65535)
            iBuf->indexSize_ = sizeof(unsigned);

        XMLElement boneAssignments = subMesh.GetChild("boneassignments");
        if (bones_.size())
        {
            if (boneAssignments)
            {
                XMLElement boneAssignment = boneAssignments.GetChild("vertexboneassignment");
                while (boneAssignment)
                {
                    unsigned vertex = boneAssignment.GetInt("vertexindex") + vertexStart;
                    unsigned bone = boneAssignment.GetInt("boneindex");
                    float weight = boneAssignment.GetFloat("weight");

                    BoneWeightAssignment assign{static_cast<unsigned char>(bone), weight};
                    // Source data might have 0 weights. Disregard these
                    if (assign.weight_ > 0.0f)
                    {
                        subGeometryLodLevel.boneWeights_[vertex].push_back(assign);

                        // Require skinning weight to be sufficiently large before vertex contributes to bone hitbox
                        if (assign.weight_ > 0.33f)
                        {
                            // Check distance of vertex from bone to get bone max. radius information
                            Vector3 bonePos = bones_[bone].derivedPosition_;
                            Vector3 vertexPos = vBuf->vertices_[vertex].position_;
                            float distance = (bonePos - vertexPos).Length();
                            if (distance > bones_[bone].radius_)
                            {
                                bones_[bone].collisionMask_ |= 1;
                                bones_[bone].radius_ = distance;
                            }
                            // Build the hitbox for the bone
                            bones_[bone].boundingBox_.Merge(bones_[bone].inverseWorldTransform_ * (vertexPos));
                            bones_[bone].collisionMask_ |= 2;
                        }
                    }
                    boneAssignment = boneAssignment.GetNext("vertexboneassignment");
                }
            }

            if ((subGeometryLodLevel.boneWeights_.size()) && bones_.size())
            {
                vBuf->elementMask_ |= MASK_BLENDWEIGHTS | MASK_BLENDINDICES;
                bool sorted = false;

                // If amount of bones is larger than supported by HW skinning, must remap per submesh
                if (bones_.size() > maxBones_)
                {
                    ea::unordered_map<unsigned, unsigned> usedBoneMap;
                    unsigned remapIndex = 0;
                    for (auto i = subGeometryLodLevel.boneWeights_.begin(); i !=
                        subGeometryLodLevel.boneWeights_.end(); ++i)
                    {
                        // Sort the bone assigns by weight
                        ea::quick_sort(i->second.begin(), i->second.end(), CompareWeights);

                        // Use only the first 4 weights
                        for (unsigned j = 0; j < i->second.size() && j < 4; ++j)
                        {
                            unsigned originalIndex = i->second[j].boneIndex_;
                            if (!usedBoneMap.contains(originalIndex))
                            {
                                usedBoneMap[originalIndex] = remapIndex;
                                remapIndex++;
                            }
                            i->second[j].boneIndex_ = usedBoneMap[originalIndex];
                        }
                    }

                    // If still too many bones in one subgeometry, error
                    if (usedBoneMap.size() > maxBones_)
                        ErrorExit("Too many bones (limit " + ea::to_string(maxBones_) + ") in submesh " + ea::to_string(subMeshIndex + 1));

                    // Write mapping of vertex buffer bone indices to original bone indices
                    subGeometryLodLevel.boneMapping_.resize(usedBoneMap.size());
                    for (auto j = usedBoneMap.begin(); j != usedBoneMap.end(); ++j)
                        subGeometryLodLevel.boneMapping_[j->second] = j->first;

                    sorted = true;
                }

                for (auto i = subGeometryLodLevel.boneWeights_.begin(); i != subGeometryLodLevel.boneWeights_.end(); ++i)
                {
                    // Sort the bone assigns by weight, if not sorted yet in bone remapping pass
                    if (!sorted)
                        ea::quick_sort(i->second.begin(), i->second.end(), CompareWeights);

                    float totalWeight = 0.0f;
                    float normalizationFactor = 0.0f;

                    // Calculate normalization factor in case there are more than 4 blend weights, or they do not add up to 1
                    for (unsigned j = 0; j < i->second.size() && j < 4; ++j)
                        totalWeight += i->second[j].weight_;
                    if (totalWeight > 0.0f)
                        normalizationFactor = 1.0f / totalWeight;

                    for (unsigned j = 0; j < i->second.size() && j < 4; ++j)
                    {
                        vBuf->vertices_[i->first].blendIndices_[j] = i->second[j].boneIndex_;
                        vBuf->vertices_[i->first].blendWeights_[j] = i->second[j].weight_ * normalizationFactor;
                    }

                    // If there are less than 4 blend weights, fill rest with zero
                    for (unsigned j = i->second.size(); j < 4; ++j)
                    {
                        vBuf->vertices_[i->first].blendIndices_[j] = 0;
                        vBuf->vertices_[i->first].blendWeights_[j] = 0.0f;
                    }

                    vBuf->vertices_[i->first].hasBlendWeights_ = true;
                }
            }
        }
        else if (boneAssignments)
            PrintLine("No skeleton loaded, skipping skinning information");

        // Calculate center for the subgeometry
        Vector3 center = Vector3::ZERO;
        for (unsigned i = 0; i < iBuf->indices_.size(); i += 3)
        {
            center += vBuf->vertices_[iBuf->indices_[i]].position_;
            center += vBuf->vertices_[iBuf->indices_[i + 1]].position_;
            center += vBuf->vertices_[iBuf->indices_[i + 2]].position_;
        }
        if (iBuf->indices_.size())
            center /= (float) iBuf->indices_.size();
        subGeometryCenters_.push_back(center);

        indexStart += indices;
        vertexStart += vertices;

        OptimizeIndices(&subGeometryLodLevel, vBuf, iBuf);

        PrintLine("Processed submesh " + ea::to_string(subMeshIndex + 1) + ": " + ea::to_string(vertices) + " vertices " +
            ea::to_string(triangles) + " triangles");
        ea::vector<ModelSubGeometryLodLevel> thisSubGeometry;
        thisSubGeometry.push_back(subGeometryLodLevel);
        subGeometries_.push_back(thisSubGeometry);

        subMesh = subMesh.GetNext("submesh");
        subMeshIndex++;
    }

    // Process LOD levels, if any
    XMLElement lods = root.GetChild("levelofdetail");
    if (lods)
    {
        try
        {
            // For now, support only generated LODs, where the vertices are the same
            XMLElement lod = lods.GetChild("lodgenerated");
            while (lod)
            {
                float distance = M_EPSILON;
                if (lod.HasAttribute("fromdepthsquared"))
                    distance = sqrtf(lod.GetFloat("fromdepthsquared"));
                if (lod.HasAttribute("value"))
                    distance = lod.GetFloat("value");
                XMLElement lodSubMesh = lod.GetChild("lodfacelist");
                while (lodSubMesh)
                {
                    unsigned subMeshIndex = lodSubMesh.GetInt("submeshindex");
                    unsigned triangles = lodSubMesh.GetInt("numfaces");

                    ModelSubGeometryLodLevel newLodLevel;
                    ModelSubGeometryLodLevel& originalLodLevel = subGeometries_[subMeshIndex][0];

                    // Copy all initial values
                    newLodLevel = originalLodLevel;

                    ModelVertexBuffer* vBuf;
                    ModelIndexBuffer* iBuf;

                    if (useOneBuffer_)
                    {
                        vBuf = &vertexBuffers_[0];
                        iBuf = &indexBuffers_[0];
                    }
                    else
                    {
                        vBuf = &vertexBuffers_[subMeshIndex];
                        iBuf = &indexBuffers_[subMeshIndex];
                    }

                    unsigned indexStart = iBuf->indices_.size();
                    unsigned indexCount = triangles * 3;
                    unsigned vertexStart = vertexStarts[subMeshIndex];

                    newLodLevel.distance_ = distance;
                    newLodLevel.indexStart_ = indexStart;
                    newLodLevel.indexCount_ = indexCount;

                    // Append indices to the original index buffer
                    XMLElement triangle = lodSubMesh.GetChild("face");
                    while (triangle)
                    {
                        unsigned v1 = triangle.GetInt("v1");
                        unsigned v2 = triangle.GetInt("v2");
                        unsigned v3 = triangle.GetInt("v3");
                        iBuf->indices_.push_back(v3 + vertexStart);
                        iBuf->indices_.push_back(v2 + vertexStart);
                        iBuf->indices_.push_back(v1 + vertexStart);
                        triangle = triangle.GetNext("face");
                    }

                    OptimizeIndices(&newLodLevel, vBuf, iBuf);

                    subGeometries_[subMeshIndex].push_back(newLodLevel);
                    PrintLine("Processed LOD level for submesh " + ea::to_string(subMeshIndex + 1) + ": distance " + ea::to_string(distance));

                    lodSubMesh = lodSubMesh.GetNext("lodfacelist");
                }
                lod = lod.GetNext("lodgenerated");
            }
        }
        catch (...) {}
    }

    // Process poses/morphs
    // First find out all pose definitions
    if (exportMorphs)
    {
        try
        {
            ea::vector<XMLElement> poses;
            XMLElement posesRoot = root.GetChild("poses");
            if (posesRoot)
            {
                XMLElement pose = posesRoot.GetChild("pose");
                while (pose)
                {
                    poses.push_back(pose);
                    pose = pose.GetNext("pose");
                }
            }

            // Then process animations using the poses
            XMLElement animsRoot = root.GetChild("animations");
            if (animsRoot)
            {
                XMLElement anim = animsRoot.GetChild("animation");
                while (anim)
                {
                    ea::string name = anim.GetAttribute("name");
                    float length = anim.GetFloat("length");
                    ea::hash_set<unsigned> usedPoses;
                    XMLElement tracks = anim.GetChild("tracks");
                    if (tracks)
                    {
                        XMLElement track = tracks.GetChild("track");
                        while (track)
                        {
                            XMLElement keyframes = track.GetChild("keyframes");
                            if (keyframes)
                            {
                                XMLElement keyframe = keyframes.GetChild("keyframe");
                                while (keyframe)
                                {
                                    float time = keyframe.GetFloat("time");
                                    XMLElement poseref = keyframe.GetChild("poseref");
                                    // Get only the end pose
                                    if (poseref && time == length)
                                        usedPoses.insert(poseref.GetInt("poseindex"));

                                    keyframe = keyframe.GetNext("keyframe");
                                }
                            }
                            track = track.GetNext("track");
                        }
                    }

                    if (usedPoses.size())
                    {
                        ModelMorph newMorph;
                        newMorph.name_ = name;

                        if (useOneBuffer_)
                            newMorph.buffers_.resize(1);
                        else
                            newMorph.buffers_.resize(usedPoses.size());

                        unsigned bufIndex = 0;

                        for (auto i = usedPoses.begin(); i != usedPoses.end(); ++i)
                        {
                            XMLElement pose = poses[*i];
                            unsigned targetSubMesh = pose.GetInt("index");
                            XMLElement poseOffset = pose.GetChild("poseoffset");

                            if (useOneBuffer_)
                                newMorph.buffers_[bufIndex].vertexBuffer_ = 0;
                            else
                                newMorph.buffers_[bufIndex].vertexBuffer_ = targetSubMesh;

                            newMorph.buffers_[bufIndex].elementMask_ = MASK_POSITION;

                            ModelVertexBuffer* vBuf = &vertexBuffers_[newMorph.buffers_[bufIndex].vertexBuffer_];

                            while (poseOffset)
                            {
                                // Convert from right- to left-handed
                                unsigned vertexIndex = poseOffset.GetInt("index") + vertexStarts[targetSubMesh];
                                float x = poseOffset.GetFloat("x");
                                float y = poseOffset.GetFloat("y");
                                float z = poseOffset.GetFloat("z");
                                Vector3 vec(x, y, -z);

                                if (vBuf->morphCount_ == 0)
                                {
                                    vBuf->morphStart_ = vertexIndex;
                                    vBuf->morphCount_ = 1;
                                }
                                else
                                {
                                    unsigned first = vBuf->morphStart_;
                                    unsigned last = first + vBuf->morphCount_ - 1;
                                    if (vertexIndex < first)
                                        first = vertexIndex;
                                    if (vertexIndex > last)
                                        last = vertexIndex;
                                    vBuf->morphStart_ = first;
                                    vBuf->morphCount_ = last - first + 1;
                                }

                                ModelVertex newVertex;
                                newVertex.position_ = vec;
                                newMorph.buffers_[bufIndex].vertices_.push_back(ea::make_pair(vertexIndex, newVertex));
                                poseOffset = poseOffset.GetNext("poseoffset");
                            }

                            if (!useOneBuffer_)
                                ++bufIndex;
                        }
                        morphs_.push_back(newMorph);
                        PrintLine("Processed morph " + name + " with " + ea::to_string(usedPoses.size()) + " sub-poses");
                    }

                    anim = anim.GetNext("animation");
                }
            }
        }
        catch (...) {}
    }

    // Check any of the buffers for vertices with missing blend weight assignments
    for (unsigned i = 0; i < vertexBuffers_.size(); ++i)
    {
        if (vertexBuffers_[i].elementMask_ & MASK_BLENDWEIGHTS)
        {
            for (unsigned j = 0; j < vertexBuffers_[i].vertices_.size(); ++j)
                if (!vertexBuffers_[i].vertices_[j].hasBlendWeights_)
                    ErrorExit("Found a vertex with missing skinning information");
        }
    }

    // Tangent generation
    if (generateTangents)
    {
        for (unsigned i = 0; i < subGeometries_.size(); ++i)
        {
            for (unsigned j = 0; j < subGeometries_[i].size(); ++j)
            {
                ModelVertexBuffer& vBuf = vertexBuffers_[subGeometries_[i][j].vertexBuffer_];
                ModelIndexBuffer& iBuf = indexBuffers_[subGeometries_[i][j].indexBuffer_];
                unsigned indexStart = subGeometries_[i][j].indexStart_;
                unsigned indexCount = subGeometries_[i][j].indexCount_;

                // If already has tangents, do not regenerate
                if (vBuf.elementMask_ & MASK_TANGENT || vBuf.vertices_.empty() || iBuf.indices_.empty())
                    continue;

                vBuf.elementMask_ |= MASK_TANGENT;

                if ((vBuf.elementMask_ & (MASK_POSITION | MASK_NORMAL | MASK_TEXCOORD1)) != (MASK_POSITION | MASK_NORMAL |
                    MASK_TEXCOORD1))
                    ErrorExit("To generate tangents, positions normals and texcoords are required");

                GenerateTangents(&vBuf.vertices_[0], sizeof(ModelVertex), &iBuf.indices_[0], sizeof(unsigned), indexStart,
                    indexCount, offsetof(ModelVertex, normal_), offsetof(ModelVertex, texCoord1_), offsetof(ModelVertex,
                    tangent_));

                PrintLine("Generated tangents");
            }
        }
    }
}

void WriteOutput(const ea::string& outputFileName, bool exportAnimations, bool rotationsOnly, bool saveMaterialList)
{
    /// \todo Use save functions of Model & Animation classes

    // Begin serialization
    {
        File dest(context_);
        if (!dest.Open(outputFileName, FILE_WRITE))
            ErrorExit("Could not open output file " + outputFileName);

        // ID
        dest.WriteFileID("UMD2");

        // Vertexbuffers
        dest.WriteUInt(vertexBuffers_.size());
        for (unsigned i = 0; i < vertexBuffers_.size(); ++i)
            vertexBuffers_[i].WriteData(dest);

        // Indexbuffers
        dest.WriteUInt(indexBuffers_.size());
        for (unsigned i = 0; i < indexBuffers_.size(); ++i)
            indexBuffers_[i].WriteData(dest);

        // Subgeometries
        dest.WriteUInt(subGeometries_.size());
        for (unsigned i = 0; i < subGeometries_.size(); ++i)
        {
            // Write bone mapping info from the first LOD level. It does not change for further LODs
            dest.WriteUInt(subGeometries_[i][0].boneMapping_.size());
            for (unsigned k = 0; k < subGeometries_[i][0].boneMapping_.size(); ++k)
                dest.WriteUInt(subGeometries_[i][0].boneMapping_[k]);

            // Lod levels for this subgeometry
            dest.WriteUInt(subGeometries_[i].size());
            for (unsigned j = 0; j < subGeometries_[i].size(); ++j)
            {
                dest.WriteFloat(subGeometries_[i][j].distance_);
                dest.WriteUInt((unsigned)subGeometries_[i][j].primitiveType_);
                dest.WriteUInt(subGeometries_[i][j].vertexBuffer_);
                dest.WriteUInt(subGeometries_[i][j].indexBuffer_);
                dest.WriteUInt(subGeometries_[i][j].indexStart_);
                dest.WriteUInt(subGeometries_[i][j].indexCount_);
            }
        }

        // Morphs
        dest.WriteUInt(morphs_.size());
        for (unsigned i = 0; i < morphs_.size(); ++i)
            morphs_[i].WriteData(dest);

        // Skeleton
        dest.WriteUInt(bones_.size());
        for (unsigned i = 0; i < bones_.size(); ++i)
        {
            dest.WriteString(bones_[i].name_);
            dest.WriteUInt(bones_[i].parentIndex_);
            dest.WriteVector3(bones_[i].bindPosition_);
            dest.WriteQuaternion(bones_[i].bindRotation_);
            dest.WriteVector3(bones_[i].bindScale_);

            Matrix3x4 offsetMatrix(bones_[i].derivedPosition_, bones_[i].derivedRotation_, bones_[i].derivedScale_);
            offsetMatrix = offsetMatrix.Inverse();
            dest.Write(offsetMatrix.Data(), sizeof(Matrix3x4));

            dest.WriteUByte(bones_[i].collisionMask_);
            if (bones_[i].collisionMask_ & 1u)
                dest.WriteFloat(bones_[i].radius_);
            if (bones_[i].collisionMask_ & 2u)
                dest.WriteBoundingBox(bones_[i].boundingBox_);
        }

        // Bounding box
        dest.WriteBoundingBox(boundingBox_);

        // Geometry centers
        for (unsigned i = 0; i < subGeometryCenters_.size(); ++i)
            dest.WriteVector3(subGeometryCenters_[i]);
    }

    if (saveMaterialList)
    {
        ea::string materialListName = ReplaceExtension(outputFileName, ".txt");
        File listFile(context_);
        if (listFile.Open(materialListName, FILE_WRITE))
        {
            for (unsigned i = 0; i < materialNames_.size(); ++i)
            {
                // Assume the materials will be located inside the standard Materials subdirectory
                listFile.WriteLine("Materials/" + ReplaceExtension(SanitateAssetName(materialNames_[i]), ".xml"));
            }
        }
        else
            PrintLine("Warning: could not write material list file " + materialListName);
    }

    XMLElement skeletonRoot = skelFile_->GetRoot("skeleton");
    if (skeletonRoot && exportAnimations)
    {
        // Go through animations
        XMLElement animationsRoot = skeletonRoot.GetChild("animations");
        if (animationsRoot)
        {
            XMLElement animation = animationsRoot.GetChild("animation");
            while (animation)
            {
                ModelAnimation newAnimation;
                newAnimation.name_ = animation.GetAttribute("name");
                newAnimation.length_ = animation.GetFloat("length");

                XMLElement tracksRoot = animation.GetChild("tracks");
                XMLElement track = tracksRoot.GetChild("track");
                while (track)
                {
                    ea::string trackName = track.GetAttribute("bone");
                    ModelBone* bone = nullptr;
                    for (unsigned i = 0; i < bones_.size(); ++i)
                    {
                        if (bones_[i].name_ == trackName)
                        {
                            bone = &bones_[i];
                            break;
                        }
                    }
                    if (!bone)
                        ErrorExit("Found animation track for unknown bone " + trackName);

                    AnimationTrack newAnimationTrack;
                    newAnimationTrack.name_ = trackName;
                    if (!rotationsOnly)
                        newAnimationTrack.channelMask_ = CHANNEL_POSITION | CHANNEL_ROTATION;
                    else
                        newAnimationTrack.channelMask_ = CHANNEL_ROTATION;

                    XMLElement keyFramesRoot = track.GetChild("keyframes");
                    XMLElement keyFrame = keyFramesRoot.GetChild("keyframe");
                    while (keyFrame)
                    {
                        AnimationKeyFrame newKeyFrame;

                        // Convert from right- to left-handed
                        XMLElement position = keyFrame.GetChild("translate");
                        float x = position.GetFloat("x");
                        float y = position.GetFloat("y");
                        float z = position.GetFloat("z");
                        Vector3 pos(x, y, -z);

                        XMLElement rotation = keyFrame.GetChild("rotate");
                        XMLElement axis = rotation.GetChild("axis");
                        float angle = -rotation.GetFloat("angle") * M_RADTODEG;
                        x = axis.GetFloat("x");
                        y = axis.GetFloat("y");
                        z = axis.GetFloat("z");
                        Vector3 axisVec(x, y, -z);
                        Quaternion rot(angle, axisVec);

                        // Transform from bind-pose relative into absolute
                        pos = bone->bindPosition_ + pos;
                        rot = bone->bindRotation_ * rot;

                        newKeyFrame.time_ = keyFrame.GetFloat("time");
                        newKeyFrame.position_ = pos;
                        newKeyFrame.rotation_ = rot;

                        newAnimationTrack.keyFrames_.push_back(newKeyFrame);
                        keyFrame = keyFrame.GetNext("keyframe");
                    }

                    // Make sure keyframes are sorted from beginning to end
                    ea::quick_sort(newAnimationTrack.keyFrames_.begin(), newAnimationTrack.keyFrames_.end(), CompareKeyFrames);

                    // Do not add tracks with no keyframes
                    if (newAnimationTrack.keyFrames_.size())
                        newAnimation.tracks_.push_back(newAnimationTrack);

                    track = track.GetNext("track");
                }

                // Write each animation into a separate file
                ea::string animationFileName = outputFileName.replaced(".mdl", "");
                animationFileName += "_" + newAnimation.name_ + ".ani";

                File dest(context_);
                if (!dest.Open(animationFileName, FILE_WRITE))
                    ErrorExit("Could not open output file " + animationFileName);

                dest.WriteFileID("UANI");
                dest.WriteString(newAnimation.name_);
                dest.WriteFloat(newAnimation.length_);
                dest.WriteUInt(newAnimation.tracks_.size());
                for (unsigned i = 0; i < newAnimation.tracks_.size(); ++i)
                {
                    AnimationTrack& track = newAnimation.tracks_[i];
                    dest.WriteString(track.name_);
                    dest.WriteUByte(track.channelMask_);
                    dest.WriteUInt(track.keyFrames_.size());
                    for (unsigned j = 0; j < track.keyFrames_.size(); ++j)
                    {
                        AnimationKeyFrame& keyFrame = track.keyFrames_[j];
                        dest.WriteFloat(keyFrame.time_);
                        if (track.channelMask_ & CHANNEL_POSITION)
                            dest.WriteVector3(keyFrame.position_);
                        if (track.channelMask_ & CHANNEL_ROTATION)
                            dest.WriteQuaternion(keyFrame.rotation_);
                        if (track.channelMask_ & CHANNEL_SCALE)
                            dest.WriteVector3(keyFrame.scale_);
                    }
                }

                animation = animation.GetNext("animation");
                PrintLine("Processed animation " + newAnimation.name_);
            }
        }
    }
}

void OptimizeIndices(ModelSubGeometryLodLevel* subGeom, ModelVertexBuffer* vb, ModelIndexBuffer* ib)
{
    ea::vector<Triangle> oldTriangles;
    ea::vector<Triangle> newTriangles;

    if (subGeom->indexCount_ % 3)
    {
        PrintLine("Index count is not divisible by 3, skipping index optimization");
        return;
    }

    for (unsigned i = 0; i < vb->vertices_.size(); ++i)
    {
        vb->vertices_[i].useCount_ = 0;
        vb->vertices_[i].cachePosition_ = -1;
    }

    for (unsigned i = subGeom->indexStart_; i < subGeom->indexStart_ + subGeom->indexCount_; i += 3)
    {
        Triangle triangle{ib->indices_[i], ib->indices_[i + 1], ib->indices_[i + 2]};
        vb->vertices_[triangle.v0_].useCount_++;
        vb->vertices_[triangle.v1_].useCount_++;
        vb->vertices_[triangle.v2_].useCount_++;
        oldTriangles.push_back(triangle);
    }

    for (unsigned i = 0; i < vb->vertices_.size(); ++i)
        CalculateScore(vb->vertices_[i]);

    ea::vector<unsigned> vertexCache;

    while (oldTriangles.size())
    {
        unsigned bestTriangle = M_MAX_UNSIGNED;
        float bestTriangleScore = -1.0f;

        // Find the best triangle at this point
        for (unsigned i = 0; i < oldTriangles.size(); ++i)
        {
            Triangle& triangle = oldTriangles[i];
            float triangleScore =
                vb->vertices_[triangle.v0_].score_ +
                vb->vertices_[triangle.v1_].score_ +
                vb->vertices_[triangle.v2_].score_;

            if (triangleScore > bestTriangleScore)
            {
                bestTriangle = i;
                bestTriangleScore = triangleScore;
            }
        }

        if (bestTriangle == M_MAX_UNSIGNED)
        {
            PrintLine("Could not find next triangle, aborting index optimization");
            return;
        }

        // Add the best triangle
        Triangle triangleCopy = oldTriangles[bestTriangle];
        newTriangles.push_back(triangleCopy);
        oldTriangles.erase(oldTriangles.begin() + bestTriangle);

        // Reduce the use count
        vb->vertices_[triangleCopy.v0_].useCount_--;
        vb->vertices_[triangleCopy.v1_].useCount_--;
        vb->vertices_[triangleCopy.v2_].useCount_--;

        // Model the LRU cache behaviour
        // Erase the triangle vertices from the middle of the cache, if they were there
        for (unsigned i = 0; i < vertexCache.size(); ++i)
        {
            if ((vertexCache[i] == triangleCopy.v0_) ||
                (vertexCache[i] == triangleCopy.v1_) ||
                (vertexCache[i] == triangleCopy.v2_))
            {
                vertexCache.erase(vertexCache.begin() + i);
                --i;
            }
        }

        // Then push them to the front
        vertexCache.insert(vertexCache.begin(), triangleCopy.v0_);
        vertexCache.insert(vertexCache.begin(), triangleCopy.v1_);
        vertexCache.insert(vertexCache.begin(), triangleCopy.v2_);

        // Update positions & scores of all vertices in the cache
        // Give position -1 if vertex is going to be erased
        for (unsigned i = 0; i < vertexCache.size(); ++i)
        {
            ModelVertex& vertex = vb->vertices_[vertexCache[i]];
            if (i >= VERTEX_CACHE_SIZE)
                vertex.cachePosition_ = -1;
            else
                vertex.cachePosition_ = i;
            CalculateScore(vertex);
        }

        // Finally erase the extra vertices
        if (vertexCache.size() > VERTEX_CACHE_SIZE)
            vertexCache.resize(VERTEX_CACHE_SIZE);
    }

    // Rewrite the index data now
    unsigned i = subGeom->indexStart_;
    for (unsigned j = 0; j < newTriangles.size(); ++j)
    {
        ib->indices_[i++] = newTriangles[j].v0_;
        ib->indices_[i++] = newTriangles[j].v1_;
        ib->indices_[i++] = newTriangles[j].v2_;
    }
}

void CalculateScore(ModelVertex& vertex)
{
    // Linear-Speed Vertex Cache Optimisation by Tom Forsyth from
    // http://home.comcast.net/~tom_forsyth/papers/fast_vert_cache_opt.html
    const float cacheDecayPower = 1.5f;
    const float lastTriScore = 0.75f;
    const float valenceBoostScale = 2.0f;
    const float valenceBoostPower = 0.5f;

    if (vertex.useCount_ == 0)
    {
        // No tri needs this vertex!
        vertex.score_ = -1.0f;
        return;
    }

    float score = 0.0f;
    int cachePosition = vertex.cachePosition_;
    if (cachePosition < 0)
    {
        // Vertex is not in FIFO cache - no score.
    }
    else
    {
        if (cachePosition < 3)
        {
            // This vertex was used in the last triangle,
            // so it has a fixed score, whichever of the three
            // it's in. Otherwise, you can get very different
            // answers depending on whether you add
            // the triangle 1,2,3 or 3,1,2 - which is silly.
            score = lastTriScore;
        }
        else
        {
            // Points for being high in the cache.
            const float scaler = 1.0f / (VERTEX_CACHE_SIZE - 3);
            score = 1.0f - (cachePosition - 3) * scaler;
            score = powf(score, cacheDecayPower);
        }
    }

    // Bonus points for having a low number of tris still to
    // use the vert, so we get rid of lone verts quickly.
    float valenceBoost = powf((float)vertex.useCount_, -valenceBoostPower);
    score += valenceBoostScale * valenceBoost;
    vertex.score_ = score;
}

ea::string SanitateAssetName(const ea::string& name)
{
    ea::string fixedName = name;
    fixedName.replace("<", "");
    fixedName.replace(">", "");
    fixedName.replace("?", "");
    fixedName.replace("*", "");
    fixedName.replace(":", "");
    fixedName.replace("\"", "");
    fixedName.replace("/", "");
    fixedName.replace("\\", "");
    fixedName.replace("|", "");

    return fixedName;
}
