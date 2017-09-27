//
// Copyright (c) 2008-2017 the Urho3D project.
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

#include "../Graphics/StaticModel.h"
#include "../Graphics/VertexBuffer.h"
#include "../Graphics/IndexBuffer.h"
#include "../Graphics/Model.h"
#include "../Scene/Node.h"
#include "../Graphics/Geometry.h"
#include "../IO/Log.h"

#include "ConstructiveSolidGeometry.h"
#include <csgjs/csgjs.hpp>


namespace Urho3D
{

// Returns true if two polygons share at least one vertex.
static bool CSGIsPolygonAdjacent(const csgjs_polygon& a, const csgjs_polygon& b)
{
    for (const auto& v1 : a.vertices)
    {
        for (const auto& v2 : b.vertices)
        {
            if (Abs(length(v1.pos - v2.pos)) <= std::numeric_limits<float>::epsilon())
                return true;
        }
    }
    return false;
}

/// Converts geometry of the node to polygon list required by csgjs. Optionally transform matrix may be applied to
/// geometry so that manipulations take into account node position, rotation and scale.
std::vector<csgjs_polygon> CSGStaticModelToPolygons(StaticModel* staticModel,
    const Matrix3x4& transform = Matrix3x4::IDENTITY)
{
    std::vector<csgjs_polygon> list;

    // Transform will be applied to vertices and normals.
    auto geom = staticModel->GetLodGeometry(0, 0);  // TODO: handle all LODs

    assert(geom->GetNumVertexBuffers() == 1);       // TODO: should we support multiple vertex buffers?

    auto ib = geom->GetIndexBuffer();
    auto vb = geom->GetVertexBuffer(0);

    auto elements = vb->GetElements();
    auto vertexSize = vb->GetVertexSize();
    auto indexSize = ib->GetIndexSize();
    auto vertexData = vb->GetShadowData();

    for (auto i = 0; i < ib->GetIndexCount(); i += 3)
    {
        std::vector<csgjs_vertex> triangle(3);

        for (int j = 0; j < 3; j++)
        {
            unsigned index = 0;
            if (indexSize > sizeof(uint16_t))
                index = *(uint32_t*)(ib->GetShadowData() + (i + j) * indexSize);
            else
                index = *(uint16_t*)(ib->GetShadowData() + (i + j) * indexSize);

            csgjs_vertex& vertex = triangle[j];
            unsigned char* vertexInData = &vertexData[vertexSize * index];
            for (unsigned k = 0; k < elements.Size(); k++)
            {
                const auto& el = elements.At(k);
                switch (el.semantic_)
                {
                case SEM_POSITION:
                {
                    assert(el.type_ == TYPE_VECTOR3);
                    Vector3 pos = transform * Vector4(*reinterpret_cast<Vector3*>(vertexInData + el.offset_), 1);
                    static_assert(sizeof(pos) == sizeof(vertex.pos), "Data size mismatch");
                    memcpy(&vertex.pos, &pos, sizeof(pos));
                    break;
                }
                case SEM_NORMAL:
                {
                    assert(el.type_ == TYPE_VECTOR3);
                    Vector3 normal = transform * Vector4(*reinterpret_cast<Vector3*>(vertexInData + el.offset_), 0);
                    static_assert(sizeof(normal) == sizeof(vertex.normal), "Data size mismatch");
                    memcpy(&vertex.normal, &normal, sizeof(normal));
                    break;
                }
                case SEM_TEXCOORD:
                {
                    assert(el.type_ == TYPE_VECTOR2);
                    static_assert(sizeof(Vector2) <= sizeof(vertex.uv), "Data size mismatch");
                    memcpy(&vertex.uv, reinterpret_cast<Vector2*>(vertexInData + el.offset_), sizeof(Vector2));
                    break;
                }
                case SEM_COLOR:
                {
                    assert(el.type_ == TYPE_UBYTE4_NORM);
                    static_assert(sizeof(vertex.color) == 4, "Data size mismatch");
                    memcpy(&vertex.color, reinterpret_cast<unsigned*>(vertexInData + el.offset_), sizeof(vertex.color));
                    break;
                }
                default:
                    break;
                }
            }
        }
        list.emplace_back(csgjs_polygon(std::move(triangle)));
    }

    return list;
}

/// Converts vertices to triangles.
template<typename T>
inline unsigned char* CSGSetIndices(void* indexData, size_t numVertices, size_t p)
{
    auto indices = reinterpret_cast<T*>(indexData);
    for (unsigned j = 2; j < numVertices; j++)
    {
        indices[0] = (T)p;
        indices[1] = (T)(p + j - 1);
        indices[2] = (T)(p + j);
        indices += 3;
    }
    return (unsigned char*)indices;
}

struct PolygonBucket
{
    unsigned vertexCount_ = 0;
    unsigned indexCount_ = 0;
    Vector<csgjs_polygon> polygons_;

    bool Contains(const csgjs_polygon& polygon) const
    {
        for (const auto& container_polygon : polygons_)
        {
            if (CSGIsPolygonAdjacent(container_polygon, polygon))
                return true;
        }
        return false;
    }
};

PODVector<Geometry*> CSGPolygonsToGeometry(const std::vector<csgjs_polygon>& polygons, Context* context,
    const PODVector<VertexElement>& elements, bool disjoint = false)
{
    Vector<PolygonBucket> buckets;

    // Non-disjoint geometries will reside in single bucket.
    if (!disjoint)
        buckets.Resize(1);

    for (const auto& poly : polygons)
    {
        // Count vertices and indices.
        auto vertexCount = poly.vertices.size();
        auto indexCount = (poly.vertices.size() - 2) * 3;

        PolygonBucket* target_bucket = nullptr;
        if (disjoint)
        {
            // Sort all vertex positions of adjacent polygons into separate buckets. Used for splitting objects into
            // separate geometries.
            for (auto& bucket : buckets)
            {
                if (bucket.Contains(poly))
                {
                    target_bucket = &bucket;
                    break;
                }
            }

            if (target_bucket == nullptr)
            {
                buckets.Push(PolygonBucket());
                target_bucket = &buckets.Back();
            }
        }
        else
            target_bucket = &buckets.Front();

        target_bucket->vertexCount_ += vertexCount;
        target_bucket->indexCount_ += indexCount;
        target_bucket->polygons_.Push(poly);
    }

    if (disjoint)
    {
        // We most likely ended up with too many separate buckets due to polygons being not sorted. We iterate through
        // existing buckets and merge them if they share any vertices.
        size_t last_num_buckets = buckets.Size() + 1;
        while (last_num_buckets != buckets.Size())
        {
            last_num_buckets = buckets.Size();

            for (auto it = buckets.Begin(); it != buckets.End(); it++)
            {
                for (auto jt = buckets.Begin(); jt != buckets.End(); jt++)
                {
                    if (it != jt)
                    {
                        for (const auto& polygon : (*jt).polygons_)
                        {
                            if ((*it).Contains(polygon))
                            {
                                (*it).indexCount_ += (*jt).indexCount_;
                                (*it).vertexCount_ += (*jt).vertexCount_;
                                (*it).polygons_.Push((*jt).polygons_);
                                buckets.Erase(jt);
                                // Terminate outter loops too.
                                it = buckets.End() - 1;
                                jt = buckets.End() - 1;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }

    PODVector<Geometry*> result;
    for (const auto& bucket : buckets)
    {
        size_t p = 0;
        SharedPtr<VertexBuffer> vb(new VertexBuffer(context));
        SharedPtr<IndexBuffer> ib(new IndexBuffer(context));

        vb->SetShadowed(true);
        vb->SetSize(bucket.vertexCount_, elements);

        ib->SetShadowed(true);
        ib->SetSize(bucket.indexCount_, bucket.vertexCount_ > std::numeric_limits<uint16_t>::max());

        auto* vertexData = static_cast<unsigned char*>(vb->Lock(0, vb->GetVertexCount()));
        auto* indexData = static_cast<unsigned char*>(ib->Lock(0, ib->GetIndexCount()));
        bool bigIndices = ib->GetIndexSize() > sizeof(uint16_t);

        for (const auto& poly : bucket.polygons_)
        {
            for (const auto& vertex : poly.vertices)
            {
                for (unsigned k = 0; k < elements.Size(); k++)
                {
                    const auto& el = elements.At(k);
                    switch (el.semantic_)
                    {
                    case SEM_POSITION:
                    {
                        assert(el.type_ == TYPE_VECTOR3);
                        memcpy(reinterpret_cast<Vector3*>(vertexData + el.offset_), &vertex.pos, sizeof(vertex.pos));
                        break;
                    }
                    case SEM_NORMAL:
                    {
                        assert(el.type_ == TYPE_VECTOR3);
                        memcpy(reinterpret_cast<Vector3*>(vertexData + el.offset_), &vertex.normal, sizeof(vertex.normal));
                        break;
                    }
                    case SEM_TEXCOORD:
                    {
                        assert(el.type_ == TYPE_VECTOR2);
                        memcpy(reinterpret_cast<Vector2*>(vertexData + el.offset_), &vertex.uv, sizeof(Vector2));
                        break;
                    }
                    case SEM_COLOR:
                    {
                        assert(el.type_ == TYPE_UBYTE4_NORM || el.type_ == TYPE_UBYTE4);
                        *reinterpret_cast<unsigned*>(vertexData + el.offset_) = vertex.color;
                        break;
                    }
                    case SEM_BINORMAL:  // TODO: recalculate
                    case SEM_TANGENT:  // TODO: recalculate
                    case SEM_BLENDWEIGHTS:  // TODO: ???
                    case SEM_BLENDINDICES:  // TODO: ???
                    case SEM_OBJECTINDEX:  // TODO: ???
                    default:
                        break;
                    }
                }
                vertexData += vb->GetVertexSize();
            }

            if (bigIndices)
                indexData = CSGSetIndices<uint32_t>(indexData, poly.vertices.size(), p);
            else
                indexData = CSGSetIndices<uint16_t>(indexData, poly.vertices.size(), p);

            p += poly.vertices.size();
        }

        vb->Unlock();
        ib->Unlock();

        Geometry* geom = new Geometry(context);
        geom->SetVertexBuffer(0, vb);
        geom->SetIndexBuffer(ib);
        geom->SetDrawRange(TRIANGLE_LIST, 0, ib->GetIndexCount());
        result.Push(geom);
    }
    return result;
}

CSGManipulator::CSGManipulator(Node* baseNode) : Object(baseNode->GetContext()), baseNode_(baseNode)
{
    StaticModel* staticModel = baseNode->GetComponent<StaticModel>();
    if (staticModel == nullptr)
    {
        URHO3D_LOGERROR("Node must contain StaticModel component.");
        return;
    }
    nodeA_ = new csgjs_csgnode(CSGStaticModelToPolygons(staticModel));
}

CSGManipulator::~CSGManipulator()
{
    delete nodeA_;
    nodeA_ = nullptr;
}

void CSGManipulator::PerformAction(Node* other, csg_function action)
{
    assert(nodeA_ != nullptr);

    StaticModel* staticModel = other->GetComponent<StaticModel>();
    if (staticModel == nullptr)
    {
        URHO3D_LOGERROR("Node must contain StaticModel component.");
        return;
    }
    // Transformation relative to base node. Gometry of `other` node is moved in such a way that base node appears to be
    // at origin point (0, 0, 0).
    auto transform = other->GetTransform() * baseNode_->GetTransform().Inverse();
    // Create csgjs node from polygons of other node.
    csgjs_csgnode* nodeB = new csgjs_csgnode(CSGStaticModelToPolygons(staticModel, transform));
    // Perform action.
    csgjs_csgnode* nodeC = action(nodeA_, nodeB);
    // Delete old nodes.
    delete nodeA_;
    delete nodeB;
    // New result is a new base node.
    nodeA_ = nodeC;
}

void CSGManipulator::Union(Node* other)
{
    PerformAction(other, &csg_union);
}

void CSGManipulator::Intersection(Node* other)
{
    PerformAction(other, &csg_intersect);
}

void CSGManipulator::Subtract(Node* other)
{
    PerformAction(other, &csg_subtract);
}

PODVector<Geometry*> CSGManipulator::BakeGeometries(bool disjoint)
{
    assert(nodeA_ != nullptr);

    // TODO: lods, vertex buffers.

    // Bake geometries
    std::vector<csgjs_polygon> polygons = nodeA_->allPolygons();
    const PODVector<VertexElement>& elements = baseNode_->GetComponent<StaticModel>()->GetLodGeometry(0, 0)->GetVertexBuffer(0)->GetElements();
    return CSGPolygonsToGeometry(polygons, context_, elements, disjoint);
}

Node* CSGManipulator::BakeSingle()
{
    assert(nodeA_ != nullptr);
    auto geometries = BakeGeometries(false);
    assert(geometries.Size() == 1);
    Model* newModel = CreateModelResource(geometries);
    // Replace geometries in the model component. Geometries are now visible in the scene.
    baseNode_->GetComponent<StaticModel>()->SetModel(newModel);
    return baseNode_;
}

PODVector<Node*> CSGManipulator::BakeSeparate()
{
    assert(nodeA_ != nullptr);
    auto geometries = BakeGeometries(false);
    Model* newModel = CreateModelResource(geometries);
    // Replace geometries in the model component. Geometries are now visible in the scene.
    baseNode_->GetComponent<StaticModel>()->SetModel(newModel);

    // TODO: ...
    PODVector<Node*> nodes;
    nodes.Push(baseNode_.Get());
    return nodes;
}

Model* CSGManipulator::CreateModelResource(PODVector<Geometry*> geometries)
{
    // Create new model resource with new geometries.
    auto newModel = new Model(context_);
    newModel->SetNumGeometries(geometries.Size());
    unsigned index = 0;
    for (auto newGeometry : geometries)
        newModel->SetGeometry(index++, 0, newGeometry);
    return newModel;
}

}
