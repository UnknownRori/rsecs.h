/*
rsecs.h - v0.1 UnknownRori <unknownrori@proton.me>

Very unprofessional implementation of the ECS system for C, 
I suggest you using <https://github.com/SanderMertens/flecs> instead for production ready stuff.
This is just for fun learning project.

Table of Contents : 
- Quick Example
- API
- Flag
- Built-in Dependencies
- Change Log

## Quick Example

```c
#include <stdio.h>
#define RSECS_STRIP_PREFIX
#define RSECS_IMPLEMENTATION
#include "../rsecs.h"


typedef struct Player { } Player;
typedef struct Position {
    float x, y;
} Position;
typedef struct Hitpoint {
    float value;
} Hitpoint;

int main()
{
    secs_world world = {0};
    INIT_WORLD(&world);

    const int PLAYER_ID =   REGISTER_COMPONENT(&world, Player);
    const int POSITION_ID = REGISTER_COMPONENT(&world, Position);
    const int HITPOINT_ID = REGISTER_COMPONENT(&world, Hitpoint);

    secs_entity_id player_entity = secs_spawn(&world);
    secs_entity_id enemy_entity = secs_spawn(&world);

    insert_comp(&world, player_entity, PLAYER_ID, &(Player) {});
    insert_comp(&world, player_entity, POSITION_ID, &(Position) {.x = 35.f, .y = 34.f });
    insert_comp(&world, player_entity, HITPOINT_ID, &(Hitpoint) { .value = 100.f });

    insert_comp(&world, enemy_entity, POSITION_ID, &(Position) {.x = 200.f, .y = 100.f });
    insert_comp(&world, enemy_entity, HITPOINT_ID, &(Hitpoint) { .value = 20.f });

    secs_query query = CREATE_QUERY(POSITION_ID | HITPOINT_ID);
    secs_query_iterator it = query_iter(&world, query);

    printf("---------------------\n");
    printf("Do some query stuff\n");
    printf("---------------------\n");
    printf("\n");
    while (query_iter_next(&it)) {
        Position* pos = field(&it, POSITION_ID);
        Hitpoint* hp = field(&it, HITPOINT_ID);

        printf("---------------------\n");
        printf("Entity ID: %zx\n", query_iter_current(&it));
        printf("Pos: (x: %f - y: %f)\n", pos->x, pos->y);
        printf("HP: (%f)\n", hp->value);
    }

    return 0;
}
```

## API

### Struct
 - secs_world               - Lifeblood of ECS system, it store component  like why you ask?
 - secs_entity_id           - Lifeblood of the id system, it just mapped to size_t
 - secs_component_mask      - Lifeblood of the mask system, it just mapped to size_t
 - secs_query               - Query parameter for fetching entity with certain component combination
 - secs_query_iterator      - Ready to use iterator
### Function
 - secs_entity_id secs_spawn(secs_world*); - Creating new entity
 - void secs_despawn(secs_world*, secs_entity_id); - Despawning entity
 - void secs_insert_comp(secs_world*, secs_entity_id, secs_component_mask, void*); - Attach a component into entity and overwrite if it exist
 - bool secs_has_comp(secs_world*, secs_entity_id, secs_component_mask); - Check if entity has component
 - void secs_remove_comp(secs_world*, secs_entity_id, secs_component_mask); - Remove component from entity
 - void* secs_get_comp(secs_world*, secs_entity_id, secs_component_mask); - Get the component from entity, it will return NULL if it doesnt have any
 - secs_query_iterator secs_query_iter(secs_world*, secs_query); - Create a iterator from query
 - bool secs_query_iter_next(secs_query_iterator*); - Continue the iteration
 - void* secs_field(secs_query_iterator*, secs_component_mask); - Get the component from the iteration

### Macro
 - SECS_INIT_WORLD(WORLD)                   - Initialize `secs_world` struct.
 - SECS_REGISTER_COMPONENT(WORLD, TYPES)    - Register component into `secs_world` struct and also initialize `secs_world` memory chunk
 - CREATE_QUERY(QUERY)                      - Generate query for iteration

## Flag

 - RSECS_IMPLEMENTATION     - Include the implementation detail
 - RSECS_STRIP_PREFIX       - Remove all the `secs_` prefixes by using macro

## Built-in Dependencies

 - https://github.com/UnknownRori/rstb/blob/main/rstb_da.h

## Change Log

 - 0.0      - Initial Proof of concept
 - 0.1      - Implement the stb style implementation, change bunch of API

*/


#pragma once

#ifndef RSECS_H
#define RSECS_H

#include <stdbool.h>

/// --------------------------------
/// INFO : RSECS Contract
/// --------------------------------

/// size_t provided at least 2^64 in 64-bit machine so roughly 18.446.744.073.709.551.616
typedef size_t secs_entity_id;
/// This component mask in allow up to 64 component in the 64-bit machine
typedef size_t secs_component_mask;

typedef struct secs_world secs_world;

typedef struct secs_query {
    secs_component_mask mask;
} secs_query;

typedef struct secs_query_iterator {
    secs_world*             world;
    secs_component_mask     mask;
    size_t                  position;
} secs_query_iterator;

/// Spawning an entity and doing some chore to setup the world to accomodate new entity
/// It might be use old entity id
secs_entity_id secs_spawn(secs_world* world);
/// Remove the entity id from active entity
void secs_despawn(secs_world* world, secs_entity_id id);

/// Insert a generic component into component pool by copying by value
/// It will also overwrite if it already exist
/// WARNING : When using this function avoid using `|` on component mask as it will cause undefined behavior
void secs_insert_comp(secs_world* world, secs_entity_id id, secs_component_mask mask, void* component);

/// Check if entity has component mask
bool secs_has_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);

/// Mark component on that entity id as garbage
void secs_remove_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);

/// Get generic component from entity id, if it doesn't find it will fail at assertion
/// You will also need to cast into appropriate type, and it will return NULL if it doesn't have
/// WARNING : When using this function avoid fetching multiple component by doing `|` on 2 or more component mask id (please use 1 only)
void* secs_get_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);


/// Create a query iterator from the query, currently only save the world pointer into iterator struct
secs_query_iterator secs_query_iter(secs_world* world, secs_query query);
/// Advance the iterator
bool secs_query_iter_next(secs_query_iterator* it);
/// Get the component from corresponding iterator
/// WARNING : When using this function avoid fetching multiple component by doing `|` on 2 or more component mask id (please use 1 only)
void* secs_field(secs_query_iterator* it, secs_component_mask mask);

#ifndef RSECS_ASSERT
    #include <assert.h>
    #define RSECS_ASSERT assert
#endif


#ifdef RSECS_IMPLEMENTATION
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

rstb_da_decl(char, secs_comp_chunk);
rstb_da_decl(secs_entity_id, secs_entity_chunk);
rstb_da_decl(secs_component_mask, secs_comp_mask_chunk);

typedef struct secs_comp_list {
    // The size of the component inside the dense array
    size_t size_of_component;

    secs_comp_mask_chunk  dense;
    secs_entity_chunk     sparse;
} secs_comp_list;

rstb_da_decl(secs_comp_list, secs_comp_list_chunk);

struct secs_world {
    // Current mask
    size_t component_mask;
    // The next of entity id
    // TODO : Make sure this id can be recycle
    size_t next_entity_id;

    secs_comp_list_chunk lists;
    secs_comp_mask_chunk mask;
};

#define SECS_INIT_WORLD(WORLD) (WORLD)->component_mask = 1; 
#define SECS_REGISTER_COMPONENT(WORLD, TYPE) (WORLD)->component_mask; \
    do { \
        rstb_da_reserve(&(WORLD)->lists, (WORLD)->component_mask + 1); \
        (WORLD)->lists.items[(WORLD)->component_mask].size_of_component = sizeof(TYPE); \
        (WORLD)->component_mask = (WORLD)->component_mask << 1; \
    } while (0)
#define SECS_CREATE_QUERY(BITMASK) (secs_query) { .mask = (BITMASK) }
#define secs_query_iter_current(IT) (IT)->position

secs_entity_id secs_spawn(secs_world* world)
{
    secs_entity_id id = world->next_entity_id++;
    rstb_da_reserve(&world->mask, id + 1);
    world->mask.count += 1;
    return id;
}

void secs_despawn(secs_world* world, secs_entity_id id)
{
    RSECS_ASSERT(world->mask.count > id && "Entity is not found");
    world->mask.items[id] = 0;
}

void secs_insert_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id, void* component)
{
    RSECS_ASSERT(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    secs_comp_list* comp = &world->lists.items[component_id];
    if (comp->sparse.count > entity_id) {
        memcpy(comp->dense.items + (comp->sparse.count - 1) * comp->size_of_component, component,  comp->size_of_component);
        return;
    }
    rstb_da_append(&(comp->sparse), entity_id);
    rstb_da_reserve(&(comp->dense), comp->sparse.count * comp->size_of_component);
    memcpy(comp->dense.items + (comp->sparse.count - 1) * comp->size_of_component, component,  comp->size_of_component);
    world->mask.items[entity_id] |= component_id;
}

bool secs_has_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    RSECS_ASSERT(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    RSECS_ASSERT(world->mask.count > entity_id && "Entity is not found");
    if ((world->mask.items[entity_id] & component_id) == component_id) return true;
    return false;
}

void secs_remove_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    RSECS_ASSERT(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!secs_has_comp(world, entity_id, component_id)) return;
    world->mask.items[entity_id] &= ~component_id;
}

void* secs_get_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    RSECS_ASSERT(component_id < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!secs_has_comp(world, entity_id, component_id)) return NULL;

    secs_comp_list* comp = &world->lists.items[component_id];
    if (comp->sparse.count > entity_id) {
        size_t index = comp->sparse.items[entity_id];
        return (void*) (comp->dense.items + index * comp->size_of_component);
    }
    return NULL;
}


secs_query_iterator secs_query_iter(secs_world* world, secs_query query)
{
     return (secs_query_iterator) {
        .world = world,
        .mask = query.mask,
        .position = (size_t)-1,
    };
}

bool secs_query_iter_next(secs_query_iterator* it)
{
    while (it->world->mask.count > it->position + 1) {
        it->position++;
        if (secs_has_comp(it->world, it->position, it->mask)) {
            return true;
        }
    }
    return false;
}
void* secs_field(secs_query_iterator* it, secs_component_mask mask)
{
    return secs_get_comp(it->world, it->position, mask);
}


#ifdef RSECS_STRIP_PREFIX
    #define INIT_WORLD(WORLD) SECS_INIT_WORLD(WORLD)
    #define REGISTER_COMPONENT(WORLD, TYPE) SECS_REGISTER_COMPONENT(WORLD, TYPE)
    #define CREATE_QUERY(QUERY) SECS_CREATE_QUERY(QUERY)

    #define insert_comp(WORLD, ID, MASK, ...) secs_insert_comp((WORLD), (ID), (MASK), (__VA_ARGS__))
    #define remove_comp(WORLD, ID, MASK) secs_remove_comp((WORLD), (ID), (MASK))
    #define has_comp(WORLD, ID, MASK) secs_has_comp((WORLD), (ID), (MASK))
    #define get_comp(WORLD, ID, MASK) secs_get_comp((WORLD), (ID), (MASK))

    #define query_iter(WORLD, QUERY) secs_query_iter((WORLD), (QUERY))
    #define query_iter_next(IT) secs_query_iter_next((IT))
    #define query_iter_current(IT) secs_query_iter_current(IT)
    #define field(IT, MASK) secs_field((IT), (MASK))
#endif // RSECS_STRIP_PREFIX

#endif //RSECS_IMPLEMENTATION

#endif // RSECS_H
