// This code is in the public domain -- castano@gmail.com

#pragma once
#ifndef NV_MESH_EXPORTOBJ_H	
#define NV_MESH_EXPORTOBJ_H

#include "nvmesh/nvmesh.h"

namespace nv
{
    class Stream;
    namespace HalfEdge { class Mesh; }

    bool exportMesh(const HalfEdge::Mesh * mesh, const char * name);

    bool exportMesh_OBJ(const HalfEdge::Mesh * mesh, Stream * stream);	

} // nv namespace

#endif // NV_MESH_EXPORTOBJ_H
