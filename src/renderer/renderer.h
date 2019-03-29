#pragma once

#include "ant_shared.h"

#include "math/vector.h"
#include "math/matrix.h"

// NOTE(soimn):
// The material and shader catalogues could be severed from the asset system and functions like "AppendMaterial" 
// could be introduced, however severing this connection, and all other connnections, with the asset system may 
// not be benefitial.

// Material catalog
// Shader catalog

// NOTE(soimn): global format or several shader specific formats?
struct Vertex_XNTBUC
{
    V3 position;
    V3 normal;
    V3 tangent;
    V3 bitangent;
    V2 uv;
    V4 color;
};

struct GPU_Buffer
{
    U64 handle;
    U32 offset;
};

typedef GPU_Buffer Vertex_Buffer;
typedef GPU_Buffer Index_Buffer;

struct Shader
{
    // Reflection and other stuff
};

typedef U32 Shader_ID;

struct Material
{
    Shader_ID shader_id;
    // Material props and textures
    // bitfield containing parameter options, will be checked against shader parameters when validating
};

typedef U32 Material_ID;

struct Triangle_List
{
    // Tag?
    
    Material_ID material;
    U32 offset;
    U32 vertex_count;
};

struct Triangle_Mesh
{
    Vertex_Buffer vertex_buffer;
    Index_Buffer index_buffer;
    
    Triangle_List* triangle_lists;
    U32 triangle_list_count;
};

struct Render_Target
{
    // FBO handle, index instead?
    U64 handle;
    
    // Viewport of sorts
    V2 region_position;
    V2 region_dimensions;
};

struct Camera
{
    V3 position;
    V3 heading; // Normalized vector pointing along the view direction
    F32 near_plane;
    F32 far_plane;
    F32 fov;
};

// Push mesh for culling, sorting, batching, uploading and drawing
// PushMesh :: (Transform transform, Triangle_Mesh mesh, Camera camera, Render_Target render_target);

// Draw mesh at position and cull with the specified camera frustum
// DrawMesh :: (Transform transform, Triangle_Mesh mesh, Camera camera, Render_Target render_target);

// DrawQuad :: (V2 position, V2 dimensions, Render_Target render_target);