#pragma once

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

// NOTE(soimn): Entities work like GameObjects in Unity, where an entity is just a set of properties and default 
//              data. Prefabs in this engine are "blueprints" with a name attached, and will be loaded from a file 
//              that describes them. These prefabs can then be used in the world chunk files and instantiated by a 
//              spawners, events, triggers or anything else for that matter. Prefabs can also be modified, like 
//              describing: Great_Lizalfos - burnable_component, will result in the entity consisting of all the 
//              components that belong to a Great Lizalfos, excluding the burnable_component.
struct Entity
{
    // U64 components;
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

// IDEA(soimn): Accessing all components, even those that are not present
// All component access is done through a get/set function that checks if the component is present by comparing 
// the component type with the component flag. If the component is present, the real values present in the 
// component is copied into memory, else the function zeroes memory.
// GetComponentValues(component_flag, component_type, memory)
// SetComponentValues(component_flag, component_type, memory)
// Pro: increase in flexibilty, Con: overhead on component access

/*
enum COMPONENT_TYPE
{

};

inline void
GetComponent(Entity* entity, Enum8(COMPONENT_TYPE) type, void* result)
{
    U64 component_flag = entity->components;
    
    U64 type_bit = (1 << type);
    
    if (component_flag & type)
    {
        // TODO(soimn): Compute the offset to the right component and read the values into _result_
    }
    
    else
    {
        ZeroSize(result, size);
    }
}

inline void
SetComponent(Entity* entity, Enum8(COMPONENT_TYPE) type, void* value)
{
    U64 component_flag = entity->components;
    
    U64 type_bit = (1 << type);
    
    if (component_flag & type)
    {
        // TODO(soimn): Compute the offset to the right component and write _value_ to the component
    }
}
*/