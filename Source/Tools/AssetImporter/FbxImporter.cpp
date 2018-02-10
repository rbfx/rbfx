#include <Urho3D/Urho3DAll.h>
#include <CLI11.hpp>
#include <ofbx.h>

using namespace Urho3D;

/// Command line options
String inputPath_;
String outputPath_;
/// Work data
SharedPtr<Context> context_;
SharedPtr<FileSystem> fs_;
ofbx::IScene* fbx_ = nullptr;
SharedPtr<Scene> scene_;

Vector2 ToVector2(const ofbx::Vec2& vec) { return { (float)vec.x, (float)vec.y }; };
Vector3 ToVector3(const ofbx::Vec3& vec) { return { (float)vec.x, (float)vec.y, (float)vec.z }; };
Vector4 ToVector4(const ofbx::Vec4& vec) { return { (float)vec.x, (float)vec.y, (float)vec.z, (float)vec.w }; };
Color ToColor(const ofbx::Vec4& vec) { return { (float)vec.x, (float)vec.y, (float)vec.z, (float)vec.w }; };
Matrix4 ToMatrix4(const ofbx::Matrix& m)
{
    return {
        (float)m.m[0], (float)m.m[1], (float)m.m[2], (float)m.m[3],
        (float)m.m[4], (float)m.m[5], (float)m.m[6], (float)m.m[7],
        (float)m.m[8], (float)m.m[9], (float)m.m[10],(float)m.m[11],
        (float)m.m[12],(float)m.m[13],(float)m.m[14],(float)m.m[15],
    };
}

PODVector<VertexElement> GetVertexDeclaration(const ofbx::Geometry* geom)
{
    PODVector<VertexElement> vertex;

    auto* pos = geom->getVertices();
    auto* norm = geom->getNormals();
    auto* uvs = geom->getUVs();
    auto* colors = geom->getColors();
    auto* tang = geom->getTangents();

    if (pos)
        vertex.Push(VertexElement(TYPE_VECTOR3, SEM_POSITION));

    if (norm)
        vertex.Push(VertexElement(TYPE_VECTOR3, SEM_NORMAL));

    if (uvs)
        vertex.Push(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD));

    if (colors)
        vertex.Push(VertexElement(TYPE_UBYTE4, SEM_COLOR));

    if (tang)
        vertex.Push(VertexElement(TYPE_VECTOR4, SEM_TANGENT));

    if (geom->getSkin() != nullptr)
    {
        vertex.Push(VertexElement(TYPE_VECTOR4, SEM_BLENDWEIGHTS));
        vertex.Push(VertexElement(TYPE_UBYTE4, SEM_BLENDINDICES));
    }

    return vertex;
}

bool UseLargeIndices(const ofbx::Geometry* geom)
{
    return geom->getVertexCount() > 0xFFFF; // TODO: optimize ib
}

template<typename T>
void WriteIndices(void* buffer, unsigned offset, const ofbx::Geometry* geom)
{
    auto* data = (T*)buffer;
    for (uint32_t i = 0; i < geom->getVertexCount(); i++)
        data[offset + i] = i;
}

Material* SaveMaterial(const ofbx::Material* fbxMaterial)
{
    XMLFile file(context_);
    auto material = file.GetOrCreateRoot("material");
    SharedPtr<Material> result(new Material(context_));

    TextureUnit texTypes[] = {TU_DIFFUSE, TU_NORMAL};  // Matches ofbx::Texture::TextureType
    const char* textureNames[] = {"diffuse", "normal"};
    for (auto i = 0; i < ofbx::Texture::COUNT; i++)
    {
        if (const auto* fbxTex = fbxMaterial->getTexture(static_cast<ofbx::Texture::TextureType>(i)))
        {
            char buf[1024] = { };
            fbxTex->getRelativeFileName().toString(buf);
            String sourcePath = AddTrailingSlash(GetPath(inputPath_)) + buf;
            if (!fs_->FileExists(sourcePath))
            {
                fbxTex->getFileName().toString(buf);
                sourcePath = buf;
                if (!fs_->FileExists(sourcePath))
                {
                    URHO3D_LOGERRORF("Missing %s texture %s", textureNames[i], sourcePath.CString());
                    continue;
                }
            }

            String name = "Textures/" + GetFileNameAndExtension(sourcePath);
            String destPath = outputPath_ + name;

            XMLElement texture = material.CreateChild("texture");
            texture.SetString("unit", textureNames[i]);
            texture.SetString("name", name);

            fs_->CreateDirsRecursive(GetPath(destPath));
            fs_->Copy(sourcePath, destPath);

            SharedPtr<Texture> tex(new Texture(context_));
            tex->LoadFile(destPath);
            tex->SetName(name);
            context_->GetCache()->AddManualResource(tex);
            result->SetTexture(texTypes[i], tex);
        }
    }

    if (material.GetChild("texture").IsNull())
        material.CreateChild("technique").SetAttribute("name", "Techniques/NoTexture.xml");
    else
        material.CreateChild("technique").SetAttribute("name", "Techniques/Diff.xml");

    // Diffuse color
    {
        auto diffColor = material.CreateChild("param");
        diffColor.SetAttribute("name", "MatDiffColor");
        auto color = fbxMaterial->getDiffuseColor();
        diffColor.SetVariantValue(Color(color.r, color.g, color.b));
    }

    String name = ToString("Materials/%s.xml", fbxMaterial->name);
    String outputFile = outputPath_ + name;
    fs_->CreateDirsRecursive(GetPath(outputFile));
    file.SaveFile(outputFile);

    result->LoadFile(outputFile);
    result->SetName(name);
    context_->GetCache()->AddManualResource(result);

    return result;
}

Skeleton GetSkeleton(ofbx::Mesh* pSkin);

Skeleton GetSkeleton(ofbx::Geometry* fbxGeom)
{
    Skeleton skeleton;
    const auto* fbxSkin = fbxGeom->getSkin();
    const auto* fbxScene = &fbxGeom->getScene();
    PODVector<const ofbx::Object*> bones;

    if (fbxSkin != nullptr)
    {
        for (auto i = 0; i < fbxSkin->getClusterCount(); i++)
        {
            const auto* bone = fbxSkin->getCluster(i)->getLink();
            if (!bones.Contains(bone))
                bones.Push(bone);
        }
    }

    for (auto i = 0; i < fbxScene->getAnimationStackCount(); i++)
    {
        const auto* stack = fbxScene->getAnimationStack(i);
        auto j = 0;
        while (const auto* layer = stack->getLayer(j++))
        {
            auto k = 0;
            while (const auto* node = layer->getCurveNode(k++))
            {
                if (const auto* bone = node->getBone())
                {
                    if (!bones.Contains(bone))
                        bones.Push(node->getBone());
                }
            }
        }
    }

    auto& skelBones = skeleton.GetModifiableBones();
    unsigned index = 0;
    for (const auto* bone : bones)
    {
        Bone skelBone;
        if (bone->getParent() == 0)
            skeleton.SetRootBoneIndex(index);
        else
            skelBone.parentIndex_ = bones.IndexOf(bone->getParent());
        skelBone.name_ = bone->name;
        skelBone.collisionMask_ = BONECOLLISION_SPHERE | BONECOLLISION_BOX;

        skelBones.Push(skelBone);
    }

    return skeleton;
}

void SaveModel(const ofbx::Object* fbxNode, Urho3D::Node* sceneNode)
{
    SharedPtr<Model> model;
    unsigned geomIndex = 0;
    Vector<SharedPtr<VertexBuffer>> vbVector;
    Vector<SharedPtr<IndexBuffer>> ibVector;
    BoundingBox bb;
    unsigned startVertexOffset = 0;
    unsigned startIndexOffset = 0;
    unsigned numGeometries = 0;
    unsigned totalIndices = 0;
    bool combineBuffers = true;
    PODVector<VertexElement> vertex;
    SharedPtr<VertexBuffer> vb;
    SharedPtr<IndexBuffer> ib;

    int linkIndex = 0;
    while (ofbx::Object* child = fbxNode->resolveObjectLink(linkIndex++))
    {
        if (child->getType() == ofbx::Object::Type::MESH)
        {
            auto* geom = ((ofbx::Mesh*)child)->getGeometry();
            if (numGeometries == 0)
                vertex = GetVertexDeclaration(geom);
            else if (GetVertexDeclaration(geom) != vertex)
                combineBuffers = false;

            totalIndices += geom->getVertexCount(); // TODO: optimize ib

            if (geom->getVertexCount() > 0xFFFF)
                combineBuffers = false;

            numGeometries++;
        }
    }

    if (numGeometries > 0)
    {
        model = new Model(context_);
        model->SetNumGeometries(numGeometries);
        sceneNode->GetOrCreateComponent<StaticModel>();
    }

    linkIndex = 0;
    while (ofbx::Object* child = fbxNode->resolveObjectLink(linkIndex++))
    {
        if (child->getType() == ofbx::Object::Type::MESH)
        {
            BoundingBox bbGeom;
            auto* fbxMesh = (ofbx::Mesh*) child;
            auto* fbxGeom = (ofbx::Geometry*) fbxMesh->getGeometry();
            auto* fbxSkin = fbxGeom->getSkin();
            SharedPtr<Geometry> geom(new Geometry(context_));

            // TODO: Dumb index buffer, optimize later?
            bool largeIndices;
            if (combineBuffers)
                largeIndices = totalIndices > 0xFFFF;
            else
                largeIndices = UseLargeIndices(fbxGeom);

            auto vertexCount = static_cast<unsigned>(fbxGeom->getVertexCount());
            auto indexCount = vertexCount;

            if (!combineBuffers)
                vertex = GetVertexDeclaration(fbxGeom);

            Matrix4 transform = ToMatrix4(fbxMesh->getGlobalTransform()) * ToMatrix4(fbxMesh->getGeometricMatrix());

            // Create new buffers if necessary
            if (!combineBuffers || vbVector.Empty())
            {
                vb = new VertexBuffer(context_);
                ib = new IndexBuffer(context_);

                if (combineBuffers)
                {
                    ib->SetSize(totalIndices, largeIndices);
                    vb->SetSize(totalIndices, vertex);
                }
                else
                {
                    ib->SetSize(indexCount, largeIndices);
                    vb->SetSize(vertexCount, vertex);
                }

                vbVector.Push(vb);
                ibVector.Push(ib);
                startVertexOffset = 0;
                startIndexOffset = 0;
            }

            if (largeIndices)
                WriteIndices<uint32_t>(ib->GetShadowData(), startIndexOffset, fbxGeom);
            else
                WriteIndices<uint16_t>(ib->GetShadowData(), startIndexOffset, fbxGeom);

            auto* pos = fbxGeom->getVertices();
            ofbx::Vec3 poss[30];
            memcpy(poss, pos, sizeof(ofbx::Vec3) * 30);

            auto* norm = fbxGeom->getNormals();
            auto* uvs = fbxGeom->getUVs();
            auto* colors = fbxGeom->getColors();
            auto* tang = fbxGeom->getTangents();

            unsigned vertexSize = vb->GetVertexSize();
            uint8_t* data = vb->GetShadowData();
            for (auto i = 0; i < fbxGeom->getVertexCount(); i++)
            {
                Vector3 position = ToVector3(pos[i]);
                bb.Merge(position);
                bbGeom.Merge(position);

                uint8_t* vertexData = &data[(i + startVertexOffset) * vertexSize];

                *(Vector3*)&vertexData[vb->GetElementOffset(SEM_POSITION)] = transform * position;
                if (norm)
                    *(Vector3*) &vertexData[vb->GetElementOffset(SEM_NORMAL)] = (transform * ToVector3(norm[i])).Normalized();
                if (uvs)
                    *(Vector2*)&vertexData[vb->GetElementOffset(SEM_TEXCOORD)] = ToVector2(uvs[i]);
                if (colors)
                    *(unsigned*)&vertexData[vb->GetElementOffset(SEM_COLOR)] = ToColor(colors[i]).ToUInt();
                if (tang)
                    *(Vector4*) &vertexData[vb->GetElementOffset(SEM_TANGENT)] = Vector4((transform * ToVector3(tang[i])).Normalized(), 1.0f);
            }

            geom->SetIndexBuffer(ib);
            geom->SetVertexBuffer(0, vb);
            geom->SetDrawRange(TRIANGLE_LIST, startIndexOffset, indexCount, true);
            model->SetNumGeometryLodLevels(geomIndex, 1);
            model->SetGeometry(geomIndex, 0, geom);
            model->SetGeometryCenter(geomIndex, bbGeom.Center());

            vbVector.Push(vb);
            ibVector.Push(ib);

            geomIndex++;
            startIndexOffset += indexCount;
            startVertexOffset += vertexCount;

            // Save materials
            for (auto i = 0; i < fbxMesh->getMaterialCount(); i++)
                sceneNode->GetOrCreateComponent<StaticModel>()->SetMaterial(i, SaveMaterial(fbxMesh->getMaterial(i)));

            // Gather bones

            Skeleton skeleton = GetSkeleton(fbxMesh);

//            for (auto i = 0; i < bones.Size(); i++)
//            {
//                for (auto j = i + 1; j < bones.Size(); j++)
//                {
//                    if (bones[i]->getParent() == bones[j])
//                        Swap(bones[i], bones[j]);
//                }
//            }

        }
    }

    if (numGeometries > 0)
    {
        auto* component = sceneNode->GetComponent<StaticModel>();
        PODVector<unsigned> emptyMorphRange;
        model->SetName(fbxNode->name);
        model->SetVertexBuffers(vbVector, emptyMorphRange, emptyMorphRange);
        model->SetIndexBuffers(ibVector);
        model->SetBoundingBox(bb);
        component->SetModel(model);

        String outputFileName = outputPath_ + "Models/" + GetFileName(inputPath_) + ".mdl";
        fs_->CreateDirsRecursive(GetPath(outputFileName));
        File file(context_);
        if (!file.Open(outputFileName, FILE_WRITE))
        {
            PrintLine("Failed writing " + outputFileName, true);
            return;
        }
        component->GetModel()->Save(file);
    }
}

void WalkFbxScene(const ofbx::Object* fbxNode, Urho3D::Node* sceneNode)
{
    // Nodes are walked by this function. Other types of objects are walked by their respective functions.
    switch (fbxNode->getType())
    {
    case ofbx::Object::Type::NULL_NODE:
    {
        sceneNode = sceneNode->CreateChild();
        sceneNode->SetPosition(ToVector3(fbxNode->getLocalTranslation()));
        auto rot = fbxNode->getLocalRotation();
        sceneNode->SetRotation(Quaternion((float) rot.x, (float) rot.y, (float) rot.z));
        sceneNode->SetScale(ToVector3(fbxNode->getLocalScaling()));
        // Fall-through
    }
    case ofbx::Object::Type::ROOT:
    {
        sceneNode->SetName(fbxNode->name);

        SaveModel(fbxNode, sceneNode);

        int i = 0;
        while (ofbx::Object* child = fbxNode->resolveObjectLink(i++))
            WalkFbxScene(child, sceneNode);
        break;
    }
    default:
        break;
    }
}

int main(int argc, char** argv)
{
    CLI::App app{"FBX importer"};

    app.add_option("input", inputPath_, "Path to FBX file.")->check(CLI::ExistingFile);
    app.add_option("output", outputPath_, "Path to output directory.")->check(CLI::ExistingDirectory);

    CLI11_PARSE(app, argc, argv);

    // Postprocess input parameters
    outputPath_ = AddTrailingSlash(outputPath_);

    // Set up application state
    context_ = new Context();
    RegisterSceneLibrary(context_);
    RegisterGraphicsLibrary(context_);
#ifdef URHO3D_PHYSICS
    RegisterPhysicsLibrary(context_);
#endif
    context_->RegisterFactory<FileSystem>();
    fs_ = context_->RegisterSubsystem<FileSystem>();
    context_->RegisterSubsystem(new ResourceCache(context_));
    context_->RegisterSubsystem(new WorkQueue(context_));

    // Do the importing!
    SharedPtr<File> file(new File(context_));
    if (!file->Open(inputPath_, FILE_READ))
    {
        PrintLine("Failed to open input file.", true);
        return -1;
    }

    PODVector<uint8_t> buffer(file->GetSize());
    if (file->Read(&buffer.Front(), buffer.Size()) != file->GetSize())
    {
        PrintLine("Failed to read entire input file.", true);
        return -1;
    }

    fbx_ = ofbx::load(&buffer.Front(), buffer.Size());

    scene_ = new Scene(context_);
    WalkFbxScene(fbx_->getRoot(), scene_);
    scene_->Remove();
    scene_ = nullptr;

    return 0;
}

