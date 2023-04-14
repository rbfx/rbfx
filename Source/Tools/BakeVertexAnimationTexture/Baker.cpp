#include "Baker.h"

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/SoftwareModelAnimator.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Resource/Image.h>

void Help(const ea::string& reason);

namespace Urho3D
{
VertexBufferReader::VertexBufferReader(VertexBuffer* vertexBuffer)
    : vertexBuffer_(vertexBuffer)
{
    if (vertexBuffer->HasElement(TYPE_VECTOR3, SEM_POSITION))
    {
        positionOffset_ = vertexBuffer->GetElementOffset(SEM_POSITION);
        positionType_ = TYPE_VECTOR3;
    }
    else if (vertexBuffer->HasElement(TYPE_VECTOR4, SEM_POSITION))
    {
        positionOffset_ = vertexBuffer->GetElementOffset(SEM_POSITION);
        positionType_ = TYPE_VECTOR4;
    }
    else
    {
        Help("Vector3 positions not found in model");
    }

    if (vertexBuffer->HasElement(TYPE_VECTOR3, SEM_NORMAL))
    {
        normalOffset_ = vertexBuffer->GetElementOffset(SEM_NORMAL);
    }
    else
    {
        Help("Vector3 normal not found in model");
    }

    uvOffset_ = vertexBuffer->HasElement(TYPE_VECTOR2, SEM_TEXCOORD) ? vertexBuffer->GetElementOffset(SEM_TEXCOORD)
                                                                     : M_MAX_UNSIGNED;

    colorOffset_ = vertexBuffer->HasElement(TYPE_UBYTE4_NORM, SEM_COLOR) ? vertexBuffer->GetElementOffset(SEM_COLOR)
                                                                     : M_MAX_UNSIGNED;
}

Vector3 VertexBufferReader::GetPosition(unsigned index) const
{
    if (positionOffset_ != M_MAX_UNSIGNED)
    {
        auto data = vertexBuffer_->GetShadowData();
        if (data)
        {
            data += index * vertexBuffer_->GetVertexSize() + positionOffset_;
            switch (positionType_)
            {
            case TYPE_VECTOR3: return *reinterpret_cast<Vector3*>(data);
            case TYPE_VECTOR4: return reinterpret_cast<Vector4*>(data)->ToVector3();
            }
        }
    }
    return Vector3::ZERO;
}

Vector3 VertexBufferReader::GetNormal(unsigned index) const
{
    if (normalOffset_ != M_MAX_UNSIGNED)
    {
        if (auto data = vertexBuffer_->GetShadowData())
        {
            data += index * vertexBuffer_->GetVertexSize() + normalOffset_;
            return *reinterpret_cast<Vector3*>(data);
        }
    }
    return Vector3::UP;
}

Color VertexBufferReader::GetColor(unsigned index) const
{
    if (colorOffset_ != M_MAX_UNSIGNED)
    {
        if (auto data = vertexBuffer_->GetShadowData())
        {
            Color c;
            c.FromUInt(*reinterpret_cast<unsigned*>(data + index * vertexBuffer_->GetVertexSize() + colorOffset_));
            return c;
        }
    }
    return Color::WHITE;
}

Vector2 VertexBufferReader::GetUV(unsigned index) const
{
    if (uvOffset_ != M_MAX_UNSIGNED)
    {
        if (const auto data = vertexBuffer_->GetShadowData())
        {
            return *reinterpret_cast<Vector2*>(data + index * vertexBuffer_->GetVertexSize() + uvOffset_);
        }
    }
    return Vector2::ZERO;
}

Baker::Baker(Context* context, const Options& options)
    : context_(context), options_(options)
{
}



void Baker::Bake()
{
    positionTransform_ = Matrix3x4{options_.translate_, Quaternion::IDENTITY, options_.scale_};
    normalTransform_ = positionTransform_.RotationMatrix();

    LoadModel();
    LoadAnimation();


    SharedPtr<Image> diffuseTexture;
    if (!options_.diffuse_.empty())
    {
        diffuseTexture = MakeShared<Image>(context_);
        auto diffFile = MakeShared<File>(context_, options_.diffuse_);
        if (!diffuseTexture->Load(*diffFile))
        {
            Help("Failed to load diffuse texture: " + options_.diffuse_);
        }
        diffuseTexture = diffuseTexture->GetDecompressedImage();
    }

    if (model_->GetVertexBuffers().size() != 1)
    {
        Help("TODO: Implement support of multiple vertex buffers");
    }

    // Build vertex buffer.
    ea::vector<VertexStructure> vertexData;
    ea::vector<unsigned short> indexData;

    {
        auto sourceVertexBuffer = model_->GetVertexBuffers().front();
        VertexBufferReader vbReader{sourceVertexBuffer};

        vertexData.resize(sourceVertexBuffer->GetVertexCount());
        textureWidth_ = NextPowerOfTwo(vertexData.size());
        Vector2 subPixelOffset = Vector2{0.5f, 0.5f};
        for (unsigned i = 0; i < vertexData.size(); ++i)
        {
            auto& vertex = vertexData[i];
            vertex.position_ = positionTransform_ * vbReader.GetPosition(i);
            vertex.normal_ = normalTransform_* vbReader.GetNormal(i);
            Color color = vbReader.GetColor(i);
            vertex.color_ = color.ToUInt();
            vertex.uv0_ = vbReader.GetUV(i);
            vertex.uv1_ = Vector2((i + subPixelOffset.x_) / static_cast<float>(textureWidth_),
                subPixelOffset.y_ / static_cast<float>(textureHeight_));
        }

        for (auto lods : model_->GetGeometries())
        {
            auto geometry = lods.front();
            auto ib = geometry->GetIndexBuffer();
            auto start = geometry->GetIndexStart();
            auto count = geometry->GetIndexCount();
            if (ib->GetIndexSize() == 2)
            {
                auto data = reinterpret_cast<unsigned short*>(ib->GetShadowData());
                for (unsigned i = start; i < count; ++i)
                {
                    indexData.push_back(data[i]);
                }
            }
            else if (ib->GetIndexSize() == 4)
            {
                auto data = reinterpret_cast<unsigned*>(ib->GetShadowData());
                for (unsigned i = start; i < count; ++i)
                {
                    indexData.push_back(data[i]);
                }
            }
        }
    }

    BuildVAT();

    auto outputModel = MakeShared<Model>(context_);
    outputModel->SetBoundingBox({-Vector3::ONE, Vector3::ONE});
    outputModel->SetNumGeometries(1);
    outputModel->SetNumGeometryLodLevels(0, 1);

    auto outputVertexBuffer = MakeShared<VertexBuffer>(context_);
    auto outputIndexBuffer = MakeShared<IndexBuffer>(context_);

    ea::vector<VertexElement> elements;
    elements.push_back(VertexElement(TYPE_VECTOR3, SEM_POSITION));
    elements.push_back(VertexElement(TYPE_VECTOR3, SEM_NORMAL));
    elements.push_back(VertexElement(TYPE_UBYTE4_NORM, SEM_COLOR));
    elements.push_back(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD));
    elements.push_back(VertexElement(TYPE_VECTOR2, SEM_TEXCOORD,1));
    outputVertexBuffer->SetSize(vertexData.size(), elements);
    outputVertexBuffer->SetData(vertexData.data());

    outputIndexBuffer->SetSize(indexData.size(), false);
    outputIndexBuffer->SetData(indexData.data());

    outputModel->SetVertexBuffers({outputVertexBuffer}, {}, {});
    outputModel->SetIndexBuffers({outputIndexBuffer});

    auto outputGeometry = MakeShared<Geometry>(context_);
    outputGeometry->SetNumVertexBuffers(1);
    outputGeometry->SetVertexBuffer(0, outputVertexBuffer);
    outputGeometry->SetIndexBuffer(outputIndexBuffer);
    outputGeometry->SetDrawRange(TRIANGLE_LIST, 0, indexData.size(), 0, vertexData.size(), false);
    outputModel->SetGeometry(0, 0, outputGeometry);
    {
        auto outputModelFileName = options_.outputFolder_ + GetFileNameAndExtension(options_.inputModel_);
        auto modelFile = MakeShared<File>(context_, outputModelFileName, FILE_WRITE);
        if (!outputModel->Save(*modelFile))
        {
            Help("Failed to save model file: " + outputModelFileName);
        }
    }
}

void Baker::LoadModel()
{
    model_ = MakeShared<Model>(context_);
    {
        auto modelFile = MakeShared<File>(context_, options_.inputModel_);
        if (!model_->Load(*modelFile))
        {
            Help("Failed to parse model file: " + options_.inputModel_);
        }
    }
}

void Baker::LoadAnimation()
{
    animation_ = MakeShared<Animation>(context_);
    auto aniFile = MakeShared<File>(context_, options_.inputAnimation_);
    if (!animation_->Load(*aniFile))
    {
        Help("Failed to parse animation file: " + options_.inputAnimation_);
    }
    textureHeight_ = NextPowerOfTwo(animation_->GetLength() * options_.targetFramerate_);
    textureHeight_ = Max(1, Min(textureHeight_, 2048));
}

Vector2 Encode16bit(float x)
{
    unsigned short u16 = static_cast<unsigned short>(x * 65535.0f);
    auto high = u16 / 256;
    auto low = u16 % 256;
    return Vector2(high / 255.0f, low / 255.0f);
}

inline float Decode16bit(Vector2 enc)
{
    return enc.DotProduct(Vector2(65280.0f / 65535.0f, 255.0f / 65535.0f));
}

unsigned char* EncodeRGBA8(unsigned char* data, const Vector3& v)
{
    Color normColor{v.x_, v.y_, v.z_, 1.0};
    *reinterpret_cast<unsigned*>(data) = normColor.ToUInt();
    return data += 4;
}

unsigned char* EncodeRGBA16(unsigned char* data, unsigned offset, const Vector3& v)
{
    auto R = Encode16bit(v.x_);
    auto G = Encode16bit(v.y_);
    auto B = Encode16bit(v.z_);
    assert(Urho3D::Abs(v.x_ - Decode16bit(R)) < 1e-4f);
    assert(Urho3D::Abs(v.y_ - Decode16bit(G)) < 1e-4f);
    assert(Urho3D::Abs(v.z_ - Decode16bit(B)) < 1e-4f);
    EncodeRGBA8(data, Vector3(R.x_, G.x_, B.x_));
    EncodeRGBA8(data + offset, Vector3(R.y_, G.y_, B.y_));
    return data + 4;
}

void Baker::BuildVAT()
{
    unsigned textureWidth = textureWidth_ * (options_.precise_ ? 2 : 1);

    auto posLookUp = MakeShared<Image>(context_);
    posLookUp->SetSize(textureWidth, textureHeight_, 4);

    auto normLookUp = MakeShared<Image>(context_);
    normLookUp->SetSize(textureWidth, textureHeight_, 4);

    auto softwareModelAnimator = MakeShared<SoftwareModelAnimator>(context_);
    softwareModelAnimator->Initialize(model_, true, 4);

    SharedPtr<Scene> scene = MakeShared<Scene>(context_);
    Node* node = scene->CreateChild();
    auto animatedModel = new AnimatedModel(context_);
    node->AddComponent(animatedModel, 0);
    auto animationController = new AnimationController(context_);
    node->AddComponent(animationController, 0);
    animatedModel->SetModel(model_);
    animatedModel->ApplyAnimation();
    assert(0 == animatedModel->GetGeometrySkinMatrices().size());

    auto skeleton = animatedModel->GetSkeleton();
    ea::vector<Matrix3x4> boneMatrices;
    boneMatrices.resize(skeleton.GetNumBones());

    AnimationParameters animationParameters{animation_};
    animationController->Update(0.0f);
    animationController->PlayNew(animationParameters);

    //auto geometry = softwareModelAnimator->GetLodGeometry(0, 0);
    //auto vertexBuffer = geometry->GetVertexBuffer(0);
    auto vertexBuffer = softwareModelAnimator->GetVertexBuffers().front();
    VertexBufferReader vbReader{vertexBuffer};

    auto dt = animation_->GetLength() / textureHeight_;
    auto normalPitch = 4 * normLookUp->GetWidth();
    auto positionPitch = 4 * posLookUp->GetWidth();
    unsigned highPrecisionOffset = positionPitch / 2;

    for (unsigned y=0; y< textureHeight_; ++y)
    {
        animationController->UpdateAnimationTime(animation_, dt*y);
        animationController->Update(0.0f);
        animatedModel->ApplyAnimation();

        for (unsigned boneIndex = 0; boneIndex < boneMatrices.size(); ++boneIndex)
        {
            auto bone = skeleton.GetBone(boneIndex);
            if (bone->node_)
                boneMatrices[boneIndex] = bone->node_->GetWorldTransform() * bone->offsetMatrix_;
            else
                boneMatrices[boneIndex] = Matrix3x4::IDENTITY;
        }
        softwareModelAnimator->ResetAnimation();
        softwareModelAnimator->ApplySkinning(boneMatrices);
        softwareModelAnimator->Commit();

        auto posData = posLookUp->GetData() + y * positionPitch;
        auto normData = normLookUp->GetData() + y * normalPitch;

        for (unsigned i = 0; i < vertexBuffer->GetVertexCount(); ++i)
        {
            auto pos = positionTransform_ * vbReader.GetPosition(i);
            pos = VectorMin(VectorMax(pos, Vector3::ZERO), Vector3::ONE);
            auto norm = ((normalTransform_ * vbReader.GetNormal(i)).Normalized() + Vector3(1.0f, 1.0f, 1.0f)) * 0.5f;
            if (options_.precise_)
            {
                posData = EncodeRGBA16(posData, highPrecisionOffset, pos);
                normData = EncodeRGBA16(normData, highPrecisionOffset, norm);
            }
            else
            {
                posData = EncodeRGBA8(posData, pos);
                normData = EncodeRGBA8(normData, norm);
            }
        }
    }

    auto fileNameWithoutExt = options_.outputFolder_ + GetFileName(options_.inputAnimation_);
    posLookUp->SaveFile(FileIdentifier("file", fileNameWithoutExt + ".pos.dds"));
    normLookUp->SaveFile(FileIdentifier("file", fileNameWithoutExt + ".norm.dds"));
}



} // namespace Urho3D
