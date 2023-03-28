#include "Baker.h"

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
}

Vector3 VertexBufferReader::GetPosition(unsigned index) const
{
    auto data = vertexBuffer_->GetShadowData();
    if (data)
    {
        data += index*vertexBuffer_->GetVertexSize() + positionOffset_;
        switch (positionType_)
        {
        case TYPE_VECTOR3: return *reinterpret_cast<Vector3*>(data);
        case TYPE_VECTOR4: return reinterpret_cast<Vector4*>(data)->ToVector3();
        }
    }
    return Vector3::ZERO;
}

Vector3 VertexBufferReader::GetNormal(unsigned index) const
{
    auto data = vertexBuffer_->GetShadowData();
    if (data)
    {
        data += index * vertexBuffer_->GetVertexSize() + normalOffset_;
        return *reinterpret_cast<Vector3*>(data);
    }
    return Vector3::UP;
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


    SharedPtr<Image> diffuseTexture = MakeShared<Image>(context_);
    if (!options_.diffuse_.empty())
    {
        auto diffFile = MakeShared<File>(context_, options_.diffuse_);
        if (!diffuseTexture->Load(*diffFile))
        {
            Help("Failed to load diffuse texture: " + options_.diffuse_);
        }
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
        unsigned uvOffset = sourceVertexBuffer->HasElement(TYPE_VECTOR2, SEM_TEXCOORD)
            ? sourceVertexBuffer->GetElementOffset(SEM_TEXCOORD)
            : M_MAX_UNSIGNED;
        unsigned colorOffset = sourceVertexBuffer->HasElement(TYPE_UBYTE4_NORM, SEM_COLOR)
            ? sourceVertexBuffer->GetElementOffset(SEM_COLOR)
            : M_MAX_UNSIGNED;

        vertexData.resize(sourceVertexBuffer->GetVertexCount());
        auto data = sourceVertexBuffer->GetShadowData();
        auto vertexSize = sourceVertexBuffer->GetVertexSize();
        textureWidth_ = NextPowerOfTwo(vertexData.size());
        Vector2 subPixelOffset = options_.half_ ? Vector2{0.25f, 0.5f} : Vector2{0.5f, 0.5f};
        for (unsigned i = 0; i < vertexData.size(); ++i)
        {
            auto* vertexPointer = data + i * vertexSize;
            auto& vertex = vertexData[i];
            vertex.position_ = positionTransform_ * vbReader.GetPosition(i);
            vertex.normal_ = normalTransform_* vbReader.GetNormal(i);
            Color color{1, 1, 1, 1};
            if (colorOffset != M_MAX_UNSIGNED)
            {
                color.FromUInt(*reinterpret_cast<unsigned*>(vertexPointer + colorOffset));
            }
            if (uvOffset != M_MAX_UNSIGNED && diffuseTexture)
            {
                auto uv = *reinterpret_cast<Vector2*>(vertexPointer + uvOffset);
                Vector2 texSize = diffuseTexture->GetSize().ToVector2();
                color = color
                    * diffuseTexture->GetPixel(
                        static_cast<int>(uv.x_ * texSize.x_), static_cast<int>(uv.y_ * texSize.y_));
            }
            vertex.color_ = color.ToUInt();
            vertex.uv_ = Vector2((i + subPixelOffset.x_) / static_cast<float>(textureWidth_),
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

inline Vector2 EncodeFloatRG(float v)
{
    Vector2 kEncodeMul = Vector2(1.0f, 255.0f);
    float kEncodeBit = 1.0f / 255.0f;
    Vector2 enc = kEncodeMul * v;

    enc = enc.Fract();
    enc.x_ = enc.x_ - enc.y_ * kEncodeBit;
    return enc;
}
inline float DecodeFloatRG(Vector2 enc)
{
    Vector2 kDecodeDot = Vector2(1.0f, 1.0f / 255.0f);
    return enc.DotProduct(kDecodeDot);
}

void Baker::BuildVAT()
{
    auto posLookUp = MakeShared<Image>(context_);
    posLookUp->SetSize(textureWidth_ * (options_.half_ ? 2 : 1), textureHeight_, 4);

    auto normLookUp = MakeShared<Image>(context_);
    normLookUp->SetSize(textureWidth_, textureHeight_, 4);

    SharedPtr<Node> node = MakeShared<Node>(context_);
    auto animatedModel = new AnimatedModel(context_);
    node->AddComponent(animatedModel, 0);
    auto animationController = new AnimationController(context_);
    node->AddComponent(animationController, 0);
    animatedModel->SetModel(model_);
    AnimationParameters animationParameters{animation_};
    animationController->PlayExistingExclusive(animationParameters);

    auto geometry = animatedModel->GetLodGeometry(0, 0);
    auto vertexBuffer = geometry->GetVertexBuffer(0);
    VertexBufferReader vbReader{vertexBuffer};

    auto dt = animation_->GetLength() / textureHeight_;
    auto normalPitch = 4 * normLookUp->GetWidth();
    auto positionPitch = 4 * posLookUp->GetWidth();
    for (unsigned y=0; y< textureHeight_; ++y)
    {
        auto posData = posLookUp->GetData() + y * positionPitch;
        auto normData = normLookUp->GetData() + y * normalPitch;

        animatedModel->ApplyAnimation();

        for (unsigned i = 0; i < vertexBuffer->GetVertexCount(); ++i)
        {
            auto pos = positionTransform_ * vbReader.GetPosition(i);
            pos = VectorMin(VectorMax(pos, Vector3::ZERO), Vector3::ONE);
            if (options_.half_)
            {
                auto R = EncodeFloatRG(pos.x_);
                auto G = EncodeFloatRG(pos.y_);
                auto B = EncodeFloatRG(pos.z_);
                Color firstColor {R.x_, G.x_, B.x_, 1.0f};
                *reinterpret_cast<unsigned*>(posData) = firstColor.ToUInt();
                posData += 4;
                Color secondColor{R.y_, G.y_, B.y_, 1.0f};
                *reinterpret_cast<unsigned*>(posData) = secondColor.ToUInt();
                posData += 4;
            }
            else
            {
                Color colorPos{pos.x_, pos.y_, pos.z_, 1.0};
                *reinterpret_cast<unsigned*>(posData) = colorPos.ToUInt();
                posData += 4;
            }

            auto norm = normalTransform_ * vbReader.GetNormal(i);
            norm.Normalize();
            norm = (norm + Vector3(1.0f, 1.0f, 1.0f)) * 0.5f;
            Color normColor{norm.x_, norm.y_, norm.z_, 1.0};
            *reinterpret_cast<unsigned*>(normData) = normColor.ToUInt();
            normData += 4;
        }

        animationController->Update(dt);
        FrameInfo frame;
        frame.timeStep_ = dt;
        animatedModel->UpdateGeometry(frame);
    }

    auto fileNameWithoutExt = options_.outputFolder_ + GetFileName(options_.inputAnimation_);
    posLookUp->SaveFile(FileIdentifier("file", fileNameWithoutExt + ".pos.dds"));
    normLookUp->SaveFile(FileIdentifier("file", fileNameWithoutExt + ".norm.dds"));
}

} // namespace Urho3D
