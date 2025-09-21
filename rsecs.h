/*
rsecs.h - v0.0 UnknownRori <unknownrori@proton.me>

Very unprofessional implementation of the ECS system for C, 
I suggest you using <https://github.com/SanderMertens/flecs> instead for production ready stuff.
This is just for fun learning project.

And I still not yet add the prefixes on this

Table of Contents : 
- Quick Example
- Change Log

## Change Log

 - 0.0      - Initial Proof of concept

*/


#pragma once

#ifndef RSECS_H
#define RSECS_H

/// --------------------------------
/// INFO : RSECS Implementation
/// --------------------------------

#ifndef RSECS_IMPLEMENTATION
/// --------------------------------
/// INFO : I'm lazy okay for creating dynamic array
/// --------------------------------

/* 
rstb_da.h - v1.1 UnknownRori <unknownrori@proton.me>

This is a single-header-file library that provides easy to use
Dynamic Array (da) for C by using macro system.

## Change Log
 - 1.0  - Initial Release
 - 1.1  - Added rstb_da_remove_unordered, rstb_da_last
 
*/

#pragma once

#ifndef RSTB_DA_H
#define RSTB_DA_H

#include <stddef.h>
#include <string.h>

/// Customizable macro definition

#ifndef RSTB_DA_REALLOC
    #include <stdlib.h>
    #define RSTB_DA_REALLOC realloc
#endif

#ifndef RSTB_DA_FREE
    #include <stdlib.h>
    #define RSTB_DA_FREE free
#endif

#ifndef RSTB_DA_ASSERT
    #include <assert.h>
    #define RSTB_DA_ASSERT assert
#endif

#ifndef RSTB_DA_INIT_CAP
    #define RSTB_DA_INIT_CAP 10
#endif

#define rstb_da_decl(TYPE, NAME)  \
    typedef struct {        \
        TYPE    *items;     \
        size_t  capacity;   \
        size_t  count;      \
    } NAME;

#define rstb_da_reserve(DA, EXPECTED) \
    do { \
        if ((EXPECTED) > (DA)->capacity) { \
            int old = (DA)->capacity; \
            if ((DA)->capacity == 0) { \
                (DA)->capacity = RSTB_DA_INIT_CAP; \
            } \
            while ((EXPECTED) > (DA)->capacity) { \
                (DA)->capacity *= 2; \
            } \
            (DA)->items = RSTB_DA_REALLOC((DA)->items, (DA)->capacity * sizeof(*(DA)->items)); \
            RSTB_DA_ASSERT((DA)->items && "Buy more RAM lol"); \
            memset((DA)->items + old, 0, ((DA)->capacity - old) * sizeof(*(DA)->items)); \
        } \
    } while(0)

#define rstb_da_append(DA, VALUE) \
    do { \
        rstb_da_reserve((DA), (DA)->count + 1); \
        (DA)->items[(DA)->count++] = VALUE; \
    } while (0) 

#define rstb_da_append_many(DA, NEW_ITEM, NEW_ITEM_COUNT) \
    do { \
        rstb_da_reserve((DA), (DA)->count + (NEW_ITEM_COUNT)); \
        memcpy((DA)->items + (DA)->count, (NEW_ITEM), (NEW_ITEM_COUNT)*sizeof(*(DA)->items)); \
        (DA)->count += (NEW_ITEM_COUNT); \
    } while(0)

#define rstb_da_remove_unordered(DA, INDEX) \
    do { \
        size_t j = (INDEX); \
        RSTB_DA_ASSERT((DA)->count > INDEX && "Out of bound"); \
        (DA)->items[j] = (DA)->items[--(DA)->count]; \
    } while (0) 

#define rstb_da_last(DA) (DA)->items[(RSTB_DA_ASSERT((DA)->count > 0), (DA)->count - 1)]
#define rstb_da_foreach(TYPE, VAR, DA) for (TYPE *VAR = (DA)->items; VAR < (DA)->items + (DA)->count; VAR++)

#define rstb_da_free(DA) RSTB_DA_FREE((DA)->items) 
#define rstb_da_reset(DA) (DA)->count = 0

#endif // RSTB_DA_H

/// --------------------------------
/// INFO : RSECS Main Implementation
/// --------------------------------

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <stdbool.h>

typedef size_t EntityId;
typedef size_t ComponentMask;

rstb_da_decl(char, ComponentChunk);
rstb_da_decl(EntityId, EntityChunk);
rstb_da_decl(ComponentMask, ComponentMaskChunk);

typedef struct ComponentList {
    // The size of the component inside the dense array
    size_t size_of_component;

    ComponentChunk  dense;
    EntityChunk     sparse;
} ComponentList;

rstb_da_decl(ComponentList, ComponentListChunk);

typedef struct World {
    // Current mask
    size_t component_mask;
    // The next of entity id
    // TODO : Make sure this id can be recycle
    size_t next_entity_id;

    ComponentListChunk lists;
    ComponentMaskChunk mask;
} World;

typedef struct Query {
    ComponentMask mask;
} Query;

typedef struct QueryIterator {
    World*          world;
    ComponentMask   mask;
    size_t          position;
    bool            first_run;
} QueryIterator;

#define INIT_WORLD(WORLD) (WORLD)->component_mask = 1; 
#define REGISTER_COMPONENT(WORLD, TYPE) (WORLD)->component_mask; \
    do { \
        rstb_da_reserve(&(WORLD)->lists, (WORLD)->component_mask + 1); \
        (WORLD)->lists.items[(WORLD)->component_mask].size_of_component = sizeof(TYPE); \
        (WORLD)->component_mask = (WORLD)->component_mask << 1; \
    } while (0)
#define CREATE_QUERY(BITMASK) (Query) { .mask = (BITMASK) }

EntityId create_entity(World* world)
{
    EntityId id = world->next_entity_id++;
    rstb_da_reserve(&world->mask, id);
    world->mask.count += 1;
    return id;
}

void remove_entity(World* world, EntityId id)
{
    (void)world;
    (void)id;
    assert(0 && "Yo not implemented");
    // TODO : Remove the entity
}

void insert_component(World* world, EntityId entity_id, ComponentMask component_id, void* component)
{
    assert(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    ComponentList* comp = &world->lists.items[component_id];
    rstb_da_append(&(comp->sparse), entity_id);
    rstb_da_reserve(&(comp->dense), comp->sparse.count * comp->size_of_component);
    memcpy(comp->dense.items + (comp->sparse.count - 1) * comp->size_of_component, component,  comp->size_of_component);
    world->mask.items[entity_id] |= component_id;
}

bool has_component(World* world, EntityId entity_id, ComponentMask component_id)
{
    assert(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if ((world->mask.items[entity_id] & component_id) == component_id) return true;
    return false;
}

void remove_component(World* world, EntityId entity_id, ComponentMask component_id)
{
    assert(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!has_component(world, entity_id, component_id)) return;
    world->mask.items[entity_id] &= ~component_id;

    // TODO : Should I remove it properly or just mark it (probably just mark it for faster deletion OwO).
}

void* get_component(World* world, EntityId entity_id, ComponentMask component_id)
{
    assert(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!has_component(world, entity_id, component_id)) return NULL;

    ComponentList* comp = &world->lists.items[component_id];
    if (comp->sparse.count > entity_id) {
        size_t index = comp->sparse.items[entity_id];
        return (void*) (comp->dense.items + index * comp->size_of_component);
    }
    return NULL;
}


QueryIterator query_iter(World* world, Query query)
{
     return (QueryIterator) {
        .world = world,
        .mask = query.mask,
        .position = 0,
        .first_run = false,
    };
}

bool query_iter_next(QueryIterator* it)
{
    if (!it->first_run) {
        it->first_run = true;
    } else {
        it->position++;
    }
    while (it->world->mask.count > it->position) {
        if (has_component(it->world, it->position, it->mask)) return true;
        it->position++;
    }
    return false;
}
void* field(QueryIterator* it, ComponentMask mask)
{
    return get_component(it->world, it->position, mask);
}

#endif //RSECS_IMPLEMENTATION

#endif
