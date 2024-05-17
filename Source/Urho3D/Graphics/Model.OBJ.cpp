// Copyright (c) 2024-2024 the rbfx project.
// Copyright(c) 2016 Robert Smith (https://github.com/Bly7/OBJ-Loader)
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

#include "../Precompiled.h"

#include <Urho3D/Graphics/Model.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/Resource/BinaryFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/IndexBuffer.h>
#include <Urho3D/Graphics/VertexBuffer.h>
#include <Urho3D/Graphics/Geometry.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Resource/XMLFile.h>

#include "../DebugNew.h"

namespace Urho3D
{
namespace 
{
// Namespace: Algorithm
//
// Description: The namespace that holds all of the
// Algorithms needed for OBJL
namespace algorithm
{
// Vector3 Multiplication Opertor Overload
//Vector3 operator*(const float& left, const Vector3& right)
//{
//    return right * left;
//}

// A test to see if P1 is on the same side as P2 of a line segment ab
bool SameSide(Vector3 p1, Vector3 p2, Vector3 a, Vector3 b)
{
    Vector3 cp1 = (b - a).CrossProduct(p1 - a);
    Vector3 cp2 = (b - a).CrossProduct(p2 - a);

    if (cp1.DotProduct(cp2) >= 0)
        return true;
    else
        return false;
}

// Generate a cross produect normal for a triangle
Vector3 GenTriNormal(Vector3 t1, Vector3 t2, Vector3 t3)
{
    Vector3 u = t2 - t1;
    Vector3 v = t3 - t1;

    Vector3 normal = u.CrossProduct(v);

    return normal;
}

// Check to see if a Vector3 Point is within a 3 Vector3 Triangle
bool inTriangle(Vector3 point, Vector3 tri1, Vector3 tri2, Vector3 tri3)
{
    // Test to see if it is within an infinite prism that the triangle outlines.
    bool within_tri_prisim =
        SameSide(point, tri1, tri2, tri3) && SameSide(point, tri2, tri1, tri3) && SameSide(point, tri3, tri1, tri2);

    // If it isn't it will never be on the triangle
    if (!within_tri_prisim)
        return false;

    // Calulate Triangle's Normal
    Vector3 n = GenTriNormal(tri1, tri2, tri3).Normalized();

    // Project the point onto this normal
    Vector3 proj = n * point.DotProduct(n);

    // If the distance from the triangle to the point is 0
    //	it lies on the triangle
    if (Equals(proj.Length(), 0.0f))
        return true;
    else
        return false;
}

// Split a String into a string array at a given token
inline void split(const ea::string& in, ea::vector<ea::string>& out, ea::string token)
{
    out.clear();

    ea::string temp;

    for (int i = 0; i < int(in.size()); i++)
    {
        ea::string test = in.substr(i, token.size());

        if (test == token)
        {
            if (!temp.empty())
            {
                out.push_back(temp);
                temp.clear();
                i += (int)token.size() - 1;
            }
            else
            {
                out.push_back("");
            }
        }
        else if (i + token.size() >= in.size())
        {
            temp += in.substr(i, token.size());
            out.push_back(temp);
            break;
        }
        else
        {
            temp += in[i];
        }
    }
}

// Get tail of string after first token and possibly following spaces
inline ea::string tail(const ea::string& in)
{
    size_t token_start = in.find_first_not_of(" \t");
    size_t space_start = in.find_first_of(" \t", token_start);
    size_t tail_start = in.find_first_not_of(" \t", space_start);
    size_t tail_end = in.find_last_not_of(" \t");
    if (tail_start != ea::string::npos && tail_end != ea::string::npos)
    {
        return in.substr(tail_start, tail_end - tail_start + 1);
    }
    else if (tail_start != ea::string::npos)
    {
        return in.substr(tail_start);
    }
    return "";
}

// Get first token of string
inline ea::string firstToken(const ea::string& in)
{
    if (!in.empty())
    {
        size_t token_start = in.find_first_not_of(" \t");
        size_t token_end = in.find_first_of(" \t", token_start);
        if (token_start != ea::string::npos && token_end != ea::string::npos)
        {
            return in.substr(token_start, token_end - token_start);
        }
        else if (token_start != ea::string::npos)
        {
            return in.substr(token_start);
        }
    }
    return "";
}

// Get element at given index position
template <class T> inline const T& getElement(const ea::vector<T>& elements, ea::string& index)
{
    int idx = ToInt(index);
    if (idx < 0)
        idx = int(elements.size()) + idx;
    else
        idx--;
    return elements[idx];
}

} // namespace algorithm

// Structure: Vertex
//
// Description: Model Vertex object that holds
//	a Position, Normal, and Texture Coordinate
struct Vertex
{
    // Position Vector
    Vector3 Position;

    // Normal Vector
    Vector3 Normal;

    // Texture Coordinate Vector
    Vector2 TextureCoordinate;
};

// Generate vertices from a list of positions,
//	tcoords, normals and a face line
void GenVerticesFromRawOBJ(ea::vector<Vertex>& oVerts, const ea::vector<Vector3>& iPositions,
    const ea::vector<Vector2>& iTCoords, const ea::vector<Vector3>& iNormals, ea::string icurline)
{
    ea::vector<ea::string> sface, svert;
    Vertex vVert;
    algorithm::split(algorithm::tail(icurline), sface, " ");

    bool noNormal = false;

    // For every given vertex do this
    for (int i = 0; i < int(sface.size()); i++)
    {
        // See What type the vertex is.
        int vtype;

        algorithm::split(sface[i], svert, "/");

        // Check for just position - v1
        if (svert.size() == 1)
        {
            // Only position
            vtype = 1;
        }

        // Check for position & texture - v1/vt1
        if (svert.size() == 2)
        {
            // Position & Texture
            vtype = 2;
        }

        // Check for Position, Texture and Normal - v1/vt1/vn1
        // or if Position and Normal - v1//vn1
        if (svert.size() == 3)
        {
            if (!svert[1].empty())
            {
                // Position, Texture, and Normal
                vtype = 4;
            }
            else
            {
                // Position & Normal
                vtype = 3;
            }
        }

        // Calculate and store the vertex
        switch (vtype)
        {
        case 1: // P
        {
            vVert.Position = algorithm::getElement(iPositions, svert[0]);
            vVert.TextureCoordinate = Vector2(0, 0);
            noNormal = true;
            oVerts.push_back(vVert);
            break;
        }
        case 2: // P/T
        {
            vVert.Position = algorithm::getElement(iPositions, svert[0]);
            vVert.TextureCoordinate = algorithm::getElement(iTCoords, svert[1]);
            noNormal = true;
            oVerts.push_back(vVert);
            break;
        }
        case 3: // P//N
        {
            vVert.Position = algorithm::getElement(iPositions, svert[0]);
            vVert.TextureCoordinate = Vector2(0, 0);
            vVert.Normal = algorithm::getElement(iNormals, svert[2]);
            oVerts.push_back(vVert);
            break;
        }
        case 4: // P/T/N
        {
            vVert.Position = algorithm::getElement(iPositions, svert[0]);
            vVert.TextureCoordinate = algorithm::getElement(iTCoords, svert[1]);
            vVert.Normal = algorithm::getElement(iNormals, svert[2]);
            oVerts.push_back(vVert);
            break;
        }
        default:
        {
            break;
        }
        }
    }

    // take care of missing normals
    // these may not be truly acurate but it is the
    // best they get for not compiling a mesh with normals
    if (noNormal)
    {
        Vector3 A = oVerts[0].Position - oVerts[1].Position;
        Vector3 B = oVerts[2].Position - oVerts[1].Position;

        Vector3 normal = A.CrossProduct(B);

        for (int i = 0; i < int(oVerts.size()); i++)
        {
            oVerts[i].Normal = normal;
        }
    }
}

// Triangulate a list of vertices into a face by printing
//	inducies corresponding with triangles within it
void VertexTriangluation(ea::vector<unsigned int>& oIndices, const ea::vector<Vertex>& iVerts)
{
    // If there are 2 or less verts,
    // no triangle can be created,
    // so exit
    if (iVerts.size() < 3)
    {
        return;
    }
    // If it is a triangle no need to calculate it
    if (iVerts.size() == 3)
    {
        oIndices.push_back(0);
        oIndices.push_back(1);
        oIndices.push_back(2);
        return;
    }

    // Create a list of vertices
    ea::vector<Vertex> tVerts = iVerts;

    while (true)
    {
        // For every vertex
        for (int i = 0; i < int(tVerts.size()); i++)
        {
            // pPrev = the previous vertex in the list
            Vertex pPrev;
            if (i == 0)
            {
                pPrev = tVerts[tVerts.size() - 1];
            }
            else
            {
                pPrev = tVerts[i - 1];
            }

            // pCur = the current vertex;
            Vertex pCur = tVerts[i];

            // pNext = the next vertex in the list
            Vertex pNext;
            if (i == tVerts.size() - 1)
            {
                pNext = tVerts[0];
            }
            else
            {
                pNext = tVerts[i + 1];
            }

            // Check to see if there are only 3 verts left
            // if so this is the last triangle
            if (tVerts.size() == 3)
            {
                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(tVerts.size()); j++)
                {
                    if (iVerts[j].Position == pCur.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == pPrev.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == pNext.Position)
                        oIndices.push_back(j);
                }

                tVerts.clear();
                break;
            }
            if (tVerts.size() == 4)
            {
                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(iVerts.size()); j++)
                {
                    if (iVerts[j].Position == pCur.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == pPrev.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == pNext.Position)
                        oIndices.push_back(j);
                }

                Vector3 tempVec;
                for (int j = 0; j < int(tVerts.size()); j++)
                {
                    if (tVerts[j].Position != pCur.Position && tVerts[j].Position != pPrev.Position
                        && tVerts[j].Position != pNext.Position)
                    {
                        tempVec = tVerts[j].Position;
                        break;
                    }
                }

                // Create a triangle from pCur, pPrev, pNext
                for (int j = 0; j < int(iVerts.size()); j++)
                {
                    if (iVerts[j].Position == pPrev.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == pNext.Position)
                        oIndices.push_back(j);
                    if (iVerts[j].Position == tempVec)
                        oIndices.push_back(j);
                }

                tVerts.clear();
                break;
            }

            // If Vertex is not an interior vertex
            float angle = (pPrev.Position - pCur.Position).Angle(pNext.Position - pCur.Position);
            if (angle <= 0 && angle >= 180)
                continue;

            // If any vertices are within this triangle
            bool inTri = false;
            for (int j = 0; j < int(iVerts.size()); j++)
            {
                if (algorithm::inTriangle(iVerts[j].Position, pPrev.Position, pCur.Position, pNext.Position)
                    && iVerts[j].Position != pPrev.Position && iVerts[j].Position != pCur.Position
                    && iVerts[j].Position != pNext.Position)
                {
                    inTri = true;
                    break;
                }
            }
            if (inTri)
                continue;

            // Create a triangle from pCur, pPrev, pNext
            for (int j = 0; j < int(iVerts.size()); j++)
            {
                if (iVerts[j].Position == pCur.Position)
                    oIndices.push_back(j);
                if (iVerts[j].Position == pPrev.Position)
                    oIndices.push_back(j);
                if (iVerts[j].Position == pNext.Position)
                    oIndices.push_back(j);
            }

            // Delete pCur from the list
            for (int j = 0; j < int(tVerts.size()); j++)
            {
                if (tVerts[j].Position == pCur.Position)
                {
                    tVerts.erase(tVerts.begin() + j);
                    break;
                }
            }

            // reset i to the start
            // -1 since loop will add 1 to it
            i = -1;
        }

        // if no triangles were created
        if (oIndices.empty())
            break;

        // if no more vertices
        if (tVerts.empty())
            break;
    }
}
}

bool Model::LoadOBJ(Deserializer& source)
{
    ea::vector<Vector3> Positions;
    ea::vector<Vector2> TCoords;
    ea::vector<Vector3> Normals;
    ea::vector<Vertex> Vertices;
    ea::vector<unsigned> Indices;
    bool listening = false;
    ea::vector<Vertex> vVerts;

    ea::vector<unsigned> geometryStart;

    geometryStart.push_back(0);

    unsigned memoryUse = sizeof(Model);
    boundingBox_ = BoundingBox{};

    ea::string curline;

    while (!source.IsEof())
    {
        source.ReadLine(curline);

        // Generate a Mesh Object or Prepare for an object to be created
        if (algorithm::firstToken(curline) == "o" || algorithm::firstToken(curline) == "g" || curline[0] == 'g')
        {
            // Ignore objects as they are useless - there is no information about pivot point and all vertices are in world space
        }
        // Generate a Vertex Position
        if (algorithm::firstToken(curline) == "v")
        {
            Vector3 vpos = ToVector3(algorithm::tail(curline));
            boundingBox_.Merge(vpos);
            Positions.push_back(vpos);
        }
        // Generate a Vertex Texture Coordinate
        if (algorithm::firstToken(curline) == "vt")
        {
            Vector2 vtex = ToVector2(algorithm::tail(curline));
            TCoords.push_back(vtex);
        }
        // Generate a Vertex Normal;
        if (algorithm::firstToken(curline) == "vn")
        {
            Vector3 vnor = ToVector3(algorithm::tail(curline));
            Normals.push_back(vnor);
        }
        // Get Mesh Material Name
        if (algorithm::firstToken(curline) == "usemtl")
        {
            if (geometryStart[geometryStart.size()-1] != Indices.size())
            {
                geometryStart.push_back(Indices.size());
            }
            // New geometry beyond this point
        }
        // Generate a Face (vertices & indices)
        if (algorithm::firstToken(curline) == "f")
        {
            // Generate the vertices
            vVerts.clear();
            GenVerticesFromRawOBJ(vVerts, Positions, TCoords, Normals, curline);

            // Add Vertices
            for (int i = 0; i < int(vVerts.size()); i++)
            {
                Vertices.push_back(vVerts[i]);

                //LoadedVertices.push_back(vVerts[i]);
            }

            ea::vector<unsigned int> iIndices;

            VertexTriangluation(iIndices, vVerts);

            // Add Indices
            for (int i = 0; i < int(iIndices.size()); i++)
            {
                unsigned int indnum = (unsigned int)((Vertices.size()) - vVerts.size()) + iIndices[i];
                Indices.push_back(indnum);

            /*    indnum = (unsigned int)((LoadedVertices.size()) - vVerts.size()) + iIndices[i];
                LoadedIndices.push_back(indnum);*/
            }
        }
        // Load Materials
        if (algorithm::firstToken(curline) == "mtllib")
        {
            // Ignore materials
        }
    }
    bool async = GetAsyncLoadState() == ASYNC_LOADING;
    {
        vertexBuffers_.reserve(1);
        loadVBData_.resize(1);

        VertexBufferDesc& desc{loadVBData_[0]};
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_POSITION, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR3, VertexElementSemantic::SEM_NORMAL, 0));
        desc.vertexElements_.push_back(
            VertexElement(VertexElementType::TYPE_VECTOR2, VertexElementSemantic::SEM_TEXCOORD, 0));

        desc.vertexCount_ = Vertices.size();

        const SharedPtr<VertexBuffer> buffer(MakeShared<VertexBuffer>(context_));
        const unsigned vertexSize = VertexBuffer::GetVertexSize(desc.vertexElements_);
        assert(sizeof(Vertex) == vertexSize);
        desc.dataSize_ = desc.vertexCount_ * vertexSize;
        buffer->SetDebugName(Format("Model '{}' Vertex Buffer #{}", GetName(), 0));

        // Prepare vertex buffer data to be uploaded during EndLoad()
        if (async)
        {
            desc.data_ = new unsigned char[desc.dataSize_];
            if (desc.vertexCount_ > 0)
            {
                memcpy(&desc.data_[0], Vertices.data(), desc.dataSize_);
            }
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            desc.data_.reset(); // Make sure no previous data
            buffer->SetShadowed(true);
            buffer->SetSize(desc.vertexCount_, desc.vertexElements_);
            if (desc.vertexCount_ > 0)
            {
                void* dest = buffer->Map();
                memcpy(dest, Vertices.data(), desc.dataSize_);
                buffer->Unmap();
            }
        }

        memoryUse += sizeof(VertexBuffer) + desc.vertexCount_ * vertexSize;
        vertexBuffers_.push_back(buffer);
    }
    {
        indexBuffers_.reserve(1);
        loadIBData_.resize(1);

        SharedPtr<IndexBuffer> buffer(MakeShared<IndexBuffer>(context_));
        buffer->SetDebugName(Format("Model '{}' Index Buffer #{}", GetName(), 0));

        // Prepare index buffer data to be uploaded during EndLoad()
        if (async)
        {
            IndexBufferDesc& ibData = loadIBData_[0];
            ibData.indexCount_ = Indices.size();
            ibData.indexSize_ = sizeof(unsigned);
            ibData.dataSize_ = ibData.indexCount_ * ibData.indexSize_;
            ibData.data_ = new unsigned char[ibData.dataSize_];
            memcpy(ibData.data_.get(), Indices.data(), ibData.dataSize_);
        }
        else
        {
            // If not async loading, use locking to avoid extra allocation & copy
            buffer->SetShadowed(true);
            buffer->SetSize(Indices.size(), true);
            void* dest = buffer->Map();
            memcpy(dest, Indices.data(), Indices.size() * sizeof(unsigned));
            buffer->Unmap();
        }

        memoryUse += sizeof(IndexBuffer) + Indices.size() * sizeof(unsigned);
        indexBuffers_.push_back(buffer);
    }
    {
        auto numGeometries = geometryStart.size();
        geometries_.reserve(numGeometries);
        geometryBoneMappings_.resize(numGeometries);
        geometryCenters_.resize(numGeometries);
        loadGeometries_.resize(numGeometries);

        for (unsigned geometryIndex = 0; geometryIndex < numGeometries; ++geometryIndex)
        {
            SharedPtr<Geometry> geometry(MakeShared<Geometry>(context_));

            ea::vector<SharedPtr<Geometry>> geometryLodLevels;
            geometryLodLevels.reserve(1);

            loadGeometries_[geometryIndex].resize(1);

            // Prepare geometry to be defined during EndLoad()
            loadGeometries_[geometryIndex][0].type_ = TRIANGLE_LIST;
            loadGeometries_[geometryIndex][0].vbRef_ = 0;
            loadGeometries_[geometryIndex][0].ibRef_ = 0;
            loadGeometries_[geometryIndex][0].indexStart_ = geometryStart[geometryIndex];
            loadGeometries_[geometryIndex][0].indexCount_ =
                ((geometryIndex < numGeometries - 1) ? geometryStart[geometryIndex + 1] : Indices.size())
                - geometryStart[geometryIndex];

            geometryCenters_[geometryIndex] = boundingBox_.Center();

            geometryLodLevels.push_back(geometry);
            memoryUse += sizeof(Geometry);

            geometries_.push_back(geometryLodLevels);
        }
    }

    // Read geometry centers
    SetMemoryUse(memoryUse);
    return true;
}

} // namespace Urho3D
