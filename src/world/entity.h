#pragma once

enum ENTITY_TYPE
{
    Entity_NoType = 0,
};

enum ENTITY_FLAGS
{
    
};

//// IS THIS REALLY A GOOD IDEA, OR SHOULD A MORE COMPONENT-BASED APPROACH BE ADOPTED INSTEAD?

// TODO(soimn): Problems
// - When to load and unload entities
// - How to add and remove entities from all subsystems efficiently

// IDEA(soimn): Loading / unloading entities
// Stop giving a shit about small temporary memory leaks and instead waste some memory in order to keep everything 
// nice and contiguous, and cleanup once in a while.

// IDEA(soimn): Solving the parenting issue
// What about a transform diff table storing all the transformations done to an entity by the Update* function, 
// where the table is sorted such that parents trail their children and when the time comes to resolve a child's 
// position, the diff table is iterated and all the appropriate transforms are applied.
// HOWEVER, when should the children be submitted to the collsision system, and how should the data be transmitted 
//          if the submission is to be handled outside of the Update* function?
// Pro: update order irrelevant. Con: collider submission

// UPDATE ORDER
// 0. SLOW! - Add and remove entities from the rendering info, proximity query and specific type buffers
// 1. (optional) Sort proximity query buffer
// 2. Update player independently of everything else, add to collision list and issue events if needed
// 3. Update each entity type by itself, add to collision list and issue events if needed
// 4. Report collisions
// 5. Process collision pairs
// 6. (optional) Sort proximity query buffer
// 7. Cull and render

// PROXIMITY QUERY BUFFER
// - position (read), type, flags, reference (read)

// RENDERER
// - transform (read), asset id (read), bounding sphere (read), camera filter (read)

// SPECIFIC UPDATE FUNCTION
// - transform (read / write), bounding sphere (read / write), flags (read / write)

// COLLISION PROCESSING
// Collider (bounding sphere, collision volume, type)
// Collision pair (collision info, type_0, type_1, e_0, e_1)
// Collider type (Static, Player, Projectile, Entity, Object) /// What about weapons?

// TODO(soimn): Split magnitude into a packed array
struct Entity_Proximity_Query_Info
{
    U64 type_id;
    Enum32(ENTITY_TYPE) type;
    Flag32(ENTITY_FLAGS) flags;
    V4 position_mag;
};

struct Entity
{
    Mesh_Rendering_Info* rendering_info;
    Enum32(ENTITY_TYPE) type;
    Flag32(ENTITY_FLAGS) flags;
    Transform transform;
    
    // TODO(soimn): Should these be moved to a seperate array?
    U64 persistent_id;
    Asset_Tag tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
};


// TODO(soimn): Use this only if limiting everything to contiguous arrays is not flexible enough
/*
struct Entity_Array
{
    U8* first;
    U8* push_ptr;
    Entity_Array* next;
    U32 count;
    U32 size;
};

inline Entity_Array*
PushEntityArray_(Memory_Arena* arena, UMM type_size, U8 type_alignment, U32 count, Entity_Array* previous = 0)
{
    Assert(arena && type_size && type_alignment && count);
    
    Entity_Array* result = 0;
    
    U8 adjustment = AlignOffset((Entity_Array*)0 + 1, 8);
    
    UMM rounded_type_size = (type_size + AlignOffset((void*) type_size, type_alignment)) * count;
    UMM total_size = sizeof(Entity_Array) + adjustment + rounded_type_size;
    
    void* memory = PushSize(arena, total_size, alignof(Entity_Array));
    ZeroSize(memory, total_size);
    
    result = (Entity_Array*) memory;
    result->first    = (U8*) Align(result + 1, type_alignment);
    result->push_ptr = result->first;
    result->size     = count;
    
    if (previous)
    {
        previous->next = result;
    }
    
    return result;
}

#define PushEntityArray(arena, type, count, ...) PushEntityArray_(arena, sizeof(type), alignof(type), count, ##__VA_ARGS__)
*/