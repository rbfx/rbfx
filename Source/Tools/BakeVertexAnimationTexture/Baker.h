//
// Copyright (c) 2023-2023 the rbfx project.
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

#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/Animation.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Scene/Node.h>

namespace Urho3D
{

struct Options
{
    bool precise_{false};
    float targetFramerate_{30};
    ea::string inputModel_;
    ea::string inputAnimation_;
    ea::string outputFolder_;
    ea::string diffuse_;
    Vector3 translate_{0.5f, 0.0f, 0.5f};
    float scale_{0.5f};
};

struct VertexBufferReader
{
    VertexBufferReader(VertexBuffer* vertexBuffer);

    Vector3 GetPosition(unsigned index) const;
    Vector3 GetNormal(unsigned index) const;
    Color GetColor(unsigned index) const;
    Vector2 GetUV(unsigned index) const;

    VertexBuffer* vertexBuffer_;
    unsigned positionOffset_;
    VertexElementType positionType_{MAX_VERTEX_ELEMENT_TYPES};
    unsigned normalOffset_;
    unsigned uvOffset_;
    unsigned colorOffset_;
};

#pragma pack(push)
#pragma pack(1)
struct VertexStructure
{
    Vector3 position_;
    Vector3 normal_;
    unsigned color_;
    Vector2 uv0_;
    Vector2 uv1_;
};
#pragma pack(pop)

class Baker
{
public:
    Baker(Context* context, const Options& options);
    void LoadAnimation();
    void BuildVAT();
    void Bake();

    void LoadModel();

private:
    Options options_;
    Context* context_;
    SharedPtr<Model> model_;
    SharedPtr<Animation> animation_;
    unsigned textureWidth_{};
    unsigned textureHeight_{};
    unsigned rowsPerFrame_{};
    unsigned verticesPerRow_{1024};
    unsigned numFrames_{};
    Matrix3x4 positionTransform_;
    Matrix3 normalTransform_;
};


}
