#pragma once

enum ENTITY_TYPE
{
    Entity_NoType = 0,
};

enum ENTITY_FLAGS
{
    
};

// UPDATE ORDER
// 0. Add or remove entities from the entity storage
// 1. Update player independently of everything else, add to buffers
// 2. Update all other entities, add to buffers
// 3. Process collision pairs
// 4. Sort proximity query buffer
// 5. Cull and render the mesh info list

// NOTE(soimn): If an entity is scheduled for removal, it is not updated and not added to any of the buffers, and 
//              it will be removed at the end of the frame. The current_instance counter does not register 
//              entities scheduled for removal, thus the indices in the proximity query buffer are valid after the 
//              entity storage has been adjusted.

// COLLISION PROCESSING
// Collider (bounding sphere, collision volume, type)
// Collision pair (collision info, type_0, type_1, e_0, e_1)
// Collider type (Static, Player, Projectile, Entity, Object) /// What about weapons?

// NOTE(soimn): Parenting is solved by sorting the entities such that all of the children to an entity trails the 
//              parent in memory, this enables the update loop to keep track of the previous parents' 
//              transformations and apply them to all children.

struct Entity_Proximity_Query_Info
{
    U64 id;
    Flag32(ENTITY_FLAGS) flags;
    V4 position_mag;
};

struct Entity
{
    // TODO(soimn): See if there is a way to access all the components at the same time without much hassel, 
    //              instead of accessing them sequentially.
    U64 components;
    Transform transform;
    Flag32(ENTITY_FLAGS) flags;
    U32 child_count;
    
    Camera_Filter filter;
    Asset_ID asset_id;
    Bounding_Sphere bounding_sphere;
    
    // TODO(soimn): Should these be moved to a seperate array?
    U64 persistent_id;
    Asset_Tag tags[ASSET_MAX_PER_ASSET_TAG_COUNT];
};