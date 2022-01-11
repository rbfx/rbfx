//
// Copyright (c) 2021-2022 the rbfx project.
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

#include "RenderQuadInstance.h"

#include "../Graphics/Camera.h"
#include "../Graphics/StaticModel.h"
#include "../Scene/Scene.h"
#include "../Graphics/Geometry.h"
#include "Urho3D/Graphics/Material.h"
#include "Urho3D/Resource/ResourceCache.h"

namespace Urho3D
{

namespace ParticleGraphNodes
{
extern const char* GEOMETRY_CATEGORY;

static const float INV_SQRT_TWO = 1.0f / sqrtf(2.0f);

RenderQuadDrawable::RenderQuadDrawable(Context* context)
    : Drawable(context, DRAWABLE_GEOMETRY)
    , geometry_(context->CreateObject<Geometry>())
    , vertexBuffer_(context_->CreateObject<VertexBuffer>())
    , indexBuffer_(context_->CreateObject<IndexBuffer>())
{
    geometry_->SetVertexBuffer(0, vertexBuffer_);
    geometry_->SetIndexBuffer(indexBuffer_);

    batches_.resize(1);
    batches_[0].geometry_ = geometry_;
    batches_[0].geometryType_ = GEOM_BILLBOARD;
    batches_[0].worldTransform_ = &transforms_[0];
}

RenderQuadDrawable::~RenderQuadDrawable() = default;

void RenderQuadDrawable::UpdateBatches(const FrameInfo& frame)
{
     distance_ = frame.camera_->GetDistance(GetWorldBoundingBox().Center());

    // Calculate scaled distance for animation LOD
    float scale = GetWorldBoundingBox().Size().DotProduct(DOT_SCALE);
    // If there are no billboards, the size becomes zero, and LOD'ed updates no longer happen. Disable LOD in that case
    if (scale > M_EPSILON)
        lodDistance_ = frame.camera_->GetLodDistance(distance_, scale, lodBias_);
    else
        lodDistance_ = 0.0f;

    batches_[0].distance_ = distance_;
    batches_[0].numWorldTransforms_ = 2;
    // Billboard positioning
    transforms_[0] = node_->GetWorldTransform();
}

void RenderQuadDrawable::UpdateGeometry(const FrameInfo& frame)
{
    //if (bufferSizeDirty_ || indexBuffer_->IsDataLost())
    //    UpdateBufferSize();

    //if (bufferDirty_ || sortThisFrame_ || vertexBuffer_->IsDataLost())
    //    UpdateVertexBuffer(frame);
}

UpdateGeometryType RenderQuadDrawable::GetUpdateGeometryType()
{
    return UPDATE_MAIN_THREAD;
}

void RenderQuadDrawable::SetMaterial(Material* material)
{
    batches_[0].material_ = material;
    MarkNetworkUpdate();
}

void RenderQuadDrawable::Commit()
{
    MarkPositionsDirty();
    MarkNetworkUpdate();
}

Material* RenderQuadDrawable::GetMaterial() const { return batches_[0].material_; }

void RenderQuadDrawable::SetMaterialAttr(const ResourceRef& value)
{
    auto* cache = GetSubsystem<ResourceCache>();
    SetMaterial(cache->GetResource<Material>(value.name_));
}

ResourceRef RenderQuadDrawable::GetMaterialAttr() const
{
    return GetResourceRef(batches_[0].material_, Material::GetTypeStatic());
}

void RenderQuadDrawable::OnWorldBoundingBoxUpdate()
{
    //unsigned enabledBillboards = 0;
    //const Matrix3x4& worldTransform = node_->GetWorldTransform();
    //Matrix3x4 billboardTransform = relative_ ? worldTransform : Matrix3x4::IDENTITY;
    //Vector3 billboardScale = scaled_ ? worldTransform.Scale() : Vector3::ONE;
    //BoundingBox worldBox;

    //for (unsigned i = 0; i < billboards_.size(); ++i)
    //{
    //    if (!billboards_[i].enabled_)
    //        continue;

    //    float size =
    //        INV_SQRT_TWO * (billboards_[i].size_.x_ * billboardScale.x_ + billboards_[i].size_.y_ * billboardScale.y_);
    //    if (fixedScreenSize_)
    //        size *= billboards_[i].screenScaleFactor_;

    //    Vector3 center = billboardTransform * billboards_[i].position_;
    //    Vector3 edge = Vector3::ONE * size;
    //    worldBox.Merge(BoundingBox(center - edge, center + edge));

    //    ++enabledBillboards;
    //}

    //// Always merge the node's own position to ensure particle emitter updates continue when the relative mode is
    //// switched
    //worldBox.Merge(node_->GetWorldPosition());

    //worldBoundingBox_ = worldBox;
}

void RenderQuadDrawable::UpdateBufferSize()
{
    //unsigned numBillboards = billboards_.size();

    //if (vertexBuffer_->GetVertexCount() != numBillboards * 4 || geometryTypeUpdate_)
    //{
    //    if (faceCameraMode_ == FC_DIRECTION)
    //    {
    //        vertexBuffer_->SetSize(
    //            numBillboards * 4, MASK_POSITION | MASK_NORMAL | MASK_COLOR | MASK_TEXCOORD1 | MASK_TEXCOORD2, true);
    //        geometry_->SetVertexBuffer(0, vertexBuffer_);
    //    }
    //    else
    //    {
    //        vertexBuffer_->SetSize(
    //            numBillboards * 4, MASK_POSITION | MASK_COLOR | MASK_TEXCOORD1 | MASK_TEXCOORD2, true);
    //        geometry_->SetVertexBuffer(0, vertexBuffer_);
    //    }
    //    geometryTypeUpdate_ = false;
    //}

    //bool largeIndices = (numBillboards * 4) >= 65536;

    //if (indexBuffer_->GetIndexCount() != numBillboards * 6)
    //    indexBuffer_->SetSize(numBillboards * 6, largeIndices);

    //bufferSizeDirty_ = false;
    //bufferDirty_ = true;
    //forceUpdate_ = true;

    //if (!numBillboards)
    //    return;

    //// Indices do not change for a given billboard capacity
    //void* destPtr = indexBuffer_->Lock(0, numBillboards * 6, true);
    //if (!destPtr)
    //    return;

    //if (!largeIndices)
    //{
    //    auto* dest = (unsigned short*)destPtr;
    //    unsigned short vertexIndex = 0;
    //    while (numBillboards--)
    //    {
    //        dest[0] = vertexIndex;
    //        dest[1] = vertexIndex + 1;
    //        dest[2] = vertexIndex + 2;
    //        dest[3] = vertexIndex + 2;
    //        dest[4] = vertexIndex + 3;
    //        dest[5] = vertexIndex;

    //        dest += 6;
    //        vertexIndex += 4;
    //    }
    //}
    //else
    //{
    //    auto* dest = (unsigned*)destPtr;
    //    unsigned vertexIndex = 0;
    //    while (numBillboards--)
    //    {
    //        dest[0] = vertexIndex;
    //        dest[1] = vertexIndex + 1;
    //        dest[2] = vertexIndex + 2;
    //        dest[3] = vertexIndex + 2;
    //        dest[4] = vertexIndex + 3;
    //        dest[5] = vertexIndex;

    //        dest += 6;
    //        vertexIndex += 4;
    //    }
    //}

    //indexBuffer_->Unlock();
    //indexBuffer_->ClearDataLost();
}

void RenderQuadDrawable::UpdateVertexBuffer(const FrameInfo& frame)
{
    //// If using animation LOD, accumulate time and see if it is time to update
    //if (animationLodBias_ > 0.0f && lodDistance_ > 0.0f)
    //{
    //    animationLodTimer_ += animationLodBias_ * frame.timeStep_ * ANIMATION_LOD_BASESCALE;
    //    if (animationLodTimer_ >= lodDistance_)
    //        animationLodTimer_ = fmodf(animationLodTimer_, lodDistance_);
    //    else
    //    {
    //        // No LOD if immediate update forced
    //        if (!forceUpdate_)
    //            return;
    //    }
    //}

    //unsigned numBillboards = billboards_.size();
    //unsigned enabledBillboards = 0;
    //const Matrix3x4& worldTransform = node_->GetWorldTransform();
    //Matrix3x4 billboardTransform = relative_ ? worldTransform : Matrix3x4::IDENTITY;
    //Vector3 billboardScale = scaled_ ? worldTransform.Scale() : Vector3::ONE;

    //// First check number of enabled billboards
    //for (unsigned i = 0; i < numBillboards; ++i)
    //{
    //    if (billboards_[i].enabled_)
    //        ++enabledBillboards;
    //}

    //sortedBillboards_.resize(enabledBillboards);
    //unsigned index = 0;

    //// Then set initial sort order and distances
    //for (unsigned i = 0; i < numBillboards; ++i)
    //{
    //    Billboard& billboard = billboards_[i];
    //    if (billboard.enabled_)
    //    {
    //        sortedBillboards_[index++] = &billboard;
    //        if (sorted_)
    //            billboard.sortDistance_ =
    //                frame.camera_->GetDistanceSquared(billboardTransform * billboards_[i].position_);
    //    }
    //}

    //batches_[0].geometry_->SetDrawRange(TRIANGLE_LIST, 0, enabledBillboards * 6, false);

    //bufferDirty_ = false;
    //forceUpdate_ = false;
    //if (!enabledBillboards)
    //    return;

    //if (sorted_)
    //{
    //    ea::quick_sort(sortedBillboards_.begin(), sortedBillboards_.end(), CompareBillboards);
    //    Vector3 worldPos = node_->GetWorldPosition();
    //    // Store the "last sorted position" now
    //    previousOffset_ = (worldPos - frame.camera_->GetNode()->GetWorldPosition());
    //}

    //auto* dest = (float*)vertexBuffer_->Lock(0, enabledBillboards * 4, true);
    //if (!dest)
    //    return;

    //if (faceCameraMode_ != FC_DIRECTION)
    //{
    //    for (unsigned i = 0; i < enabledBillboards; ++i)
    //    {
    //        Billboard& billboard = *sortedBillboards_[i];

    //        Vector2 size(billboard.size_.x_ * billboardScale.x_, billboard.size_.y_ * billboardScale.y_);
    //        unsigned color = billboard.color_.ToUInt();
    //        if (fixedScreenSize_)
    //            size *= billboard.screenScaleFactor_;

    //        float rotationMatrix[2][2];
    //        SinCos(billboard.rotation_, rotationMatrix[0][1], rotationMatrix[0][0]);
    //        rotationMatrix[1][0] = -rotationMatrix[0][1];
    //        rotationMatrix[1][1] = rotationMatrix[0][0];

    //        dest[0] = billboard.position_.x_;
    //        dest[1] = billboard.position_.y_;
    //        dest[2] = billboard.position_.z_;
    //        ((unsigned&)dest[3]) = color;
    //        dest[4] = billboard.uv_.min_.x_;
    //        dest[5] = billboard.uv_.min_.y_;
    //        dest[6] = -size.x_ * rotationMatrix[0][0] + size.y_ * rotationMatrix[0][1];
    //        dest[7] = -size.x_ * rotationMatrix[1][0] + size.y_ * rotationMatrix[1][1];

    //        dest[8] = billboard.position_.x_;
    //        dest[9] = billboard.position_.y_;
    //        dest[10] = billboard.position_.z_;
    //        ((unsigned&)dest[11]) = color;
    //        dest[12] = billboard.uv_.max_.x_;
    //        dest[13] = billboard.uv_.min_.y_;
    //        dest[14] = size.x_ * rotationMatrix[0][0] + size.y_ * rotationMatrix[0][1];
    //        dest[15] = size.x_ * rotationMatrix[1][0] + size.y_ * rotationMatrix[1][1];

    //        dest[16] = billboard.position_.x_;
    //        dest[17] = billboard.position_.y_;
    //        dest[18] = billboard.position_.z_;
    //        ((unsigned&)dest[19]) = color;
    //        dest[20] = billboard.uv_.max_.x_;
    //        dest[21] = billboard.uv_.max_.y_;
    //        dest[22] = size.x_ * rotationMatrix[0][0] - size.y_ * rotationMatrix[0][1];
    //        dest[23] = size.x_ * rotationMatrix[1][0] - size.y_ * rotationMatrix[1][1];

    //        dest[24] = billboard.position_.x_;
    //        dest[25] = billboard.position_.y_;
    //        dest[26] = billboard.position_.z_;
    //        ((unsigned&)dest[27]) = color;
    //        dest[28] = billboard.uv_.min_.x_;
    //        dest[29] = billboard.uv_.max_.y_;
    //        dest[30] = -size.x_ * rotationMatrix[0][0] - size.y_ * rotationMatrix[0][1];
    //        dest[31] = -size.x_ * rotationMatrix[1][0] - size.y_ * rotationMatrix[1][1];

    //        dest += 32;
    //    }
    //}
    //else
    //{
    //    for (unsigned i = 0; i < enabledBillboards; ++i)
    //    {
    //        Billboard& billboard = *sortedBillboards_[i];

    //        Vector2 size(billboard.size_.x_ * billboardScale.x_, billboard.size_.y_ * billboardScale.y_);
    //        unsigned color = billboard.color_.ToUInt();
    //        if (fixedScreenSize_)
    //            size *= billboard.screenScaleFactor_;

    //        float rot2D[2][2];
    //        SinCos(billboard.rotation_, rot2D[0][1], rot2D[0][0]);
    //        rot2D[1][0] = -rot2D[0][1];
    //        rot2D[1][1] = rot2D[0][0];

    //        dest[0] = billboard.position_.x_;
    //        dest[1] = billboard.position_.y_;
    //        dest[2] = billboard.position_.z_;
    //        dest[3] = billboard.direction_.x_;
    //        dest[4] = billboard.direction_.y_;
    //        dest[5] = billboard.direction_.z_;
    //        ((unsigned&)dest[6]) = color;
    //        dest[7] = billboard.uv_.min_.x_;
    //        dest[8] = billboard.uv_.min_.y_;
    //        dest[9] = -size.x_ * rot2D[0][0] + size.y_ * rot2D[0][1];
    //        dest[10] = -size.x_ * rot2D[1][0] + size.y_ * rot2D[1][1];

    //        dest[11] = billboard.position_.x_;
    //        dest[12] = billboard.position_.y_;
    //        dest[13] = billboard.position_.z_;
    //        dest[14] = billboard.direction_.x_;
    //        dest[15] = billboard.direction_.y_;
    //        dest[16] = billboard.direction_.z_;
    //        ((unsigned&)dest[17]) = color;
    //        dest[18] = billboard.uv_.max_.x_;
    //        dest[19] = billboard.uv_.min_.y_;
    //        dest[20] = size.x_ * rot2D[0][0] + size.y_ * rot2D[0][1];
    //        dest[21] = size.x_ * rot2D[1][0] + size.y_ * rot2D[1][1];

    //        dest[22] = billboard.position_.x_;
    //        dest[23] = billboard.position_.y_;
    //        dest[24] = billboard.position_.z_;
    //        dest[25] = billboard.direction_.x_;
    //        dest[26] = billboard.direction_.y_;
    //        dest[27] = billboard.direction_.z_;
    //        ((unsigned&)dest[28]) = color;
    //        dest[29] = billboard.uv_.max_.x_;
    //        dest[30] = billboard.uv_.max_.y_;
    //        dest[31] = size.x_ * rot2D[0][0] - size.y_ * rot2D[0][1];
    //        dest[32] = size.x_ * rot2D[1][0] - size.y_ * rot2D[1][1];

    //        dest[33] = billboard.position_.x_;
    //        dest[34] = billboard.position_.y_;
    //        dest[35] = billboard.position_.z_;
    //        dest[36] = billboard.direction_.x_;
    //        dest[37] = billboard.direction_.y_;
    //        dest[38] = billboard.direction_.z_;
    //        ((unsigned&)dest[39]) = color;
    //        dest[40] = billboard.uv_.min_.x_;
    //        dest[41] = billboard.uv_.max_.y_;
    //        dest[42] = -size.x_ * rot2D[0][0] - size.y_ * rot2D[0][1];
    //        dest[43] = -size.x_ * rot2D[1][0] - size.y_ * rot2D[1][1];

    //        dest += 44;
    //    }
    //}

    //vertexBuffer_->Unlock();
    //vertexBuffer_->ClearDataLost();
}

void RenderQuadDrawable::MarkPositionsDirty()
{
    Drawable::OnMarkedDirty(node_);
}

void RenderQuadInstance::Init(ParticleGraphNode* node, ParticleGraphLayerInstance* layer)
{
    InstanceBase::Init(node, layer);
}

RenderQuadInstance::~RenderQuadInstance()
{
}

void RenderQuadInstance::Prepare(unsigned numParticles)
{
}

} // namespace ParticleGraphNodes

} // namespace Urho3D
