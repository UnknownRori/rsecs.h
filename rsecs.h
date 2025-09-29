/*
rsecs.h - v0.4 UnknownRori <unknownrori@proton.me>

Unprofressional implementation of ECS for C99 with stb style header file
I suggest on using <https://github.com/SanderMertens/flecs> instead of this for production ready stuff.
The current implementation is by using Sparse Set and Bitmask Archetype, and it can be somewhat cache friendly.
But all of the operation should be O(1) [Creating, Updating] for Deleting currently it will loop all of it to deallocate them

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
 - void secs_init_world(secs_world*); - Initialize [`secs_world`] struct
 - void secs_free_world(secs_world*); - Free memory allocated inside [`secs_world`] struct
 - void secs_reset_world(secs_world* world) - Set all the count to 0 effectively mark everything as unused except the registered component

 - secs_entity_id secs_spawn(secs_world*); - Creating new entity
 - void secs_despawn(secs_world*, secs_entity_id); - Despawning entity
 - void secs_insert_comp(secs_world*, secs_entity_id, secs_component_mask, void*); - Attach a component into entity and overwrite if it exist
 - bool secs_has_comp(secs_world*, secs_entity_id, secs_component_mask); - Check if entity has component
 - bool secs_has_not_comp(secs_world*, secs_entity_id, secs_component_mask); - Check if entity doesn't component
 - void secs_remove_comp(secs_world*, secs_entity_id, secs_component_mask); - Remove component from entity

 - void* secs_get_comp(secs_world*, secs_entity_id, secs_component_mask); - Get the component from entity, it will return NULL if it doesnt have any
 - secs_query_iterator secs_query_iter(secs_world*, secs_query); - Create a iterator from query
 - bool secs_query_iter_next(secs_query_iterator*); - Continue the iteration
 - void* secs_field(secs_query_iterator*, secs_component_mask); - Get the component from the iteration

### Macro
 - SECS_INIT_WORLD(WORLD)                   - Initialize [`secs_world`] struct.
 - SECS_REGISTER_COMPONENT(WORLD, TYPES)    - Register component into [`secs_world`] struct and also initialize [`secs_world`] memory chunk
 - CREATE_QUERY(QUERY)                      - Generate query for iteration

## Flag

 - RSECS_IMPLEMENTATION     - Include the implementation detail
 - RSECS_STRIP_PREFIX       - Remove all the `secs_` prefixes by using macro

## Built-in Dependencies

 - https://raw.githubusercontent.com/UnknownRori/rstb/refs/heads/main/rstb_da.h

## Change Log

 - 0.0      - Initial Proof of concept
 - 0.1      - Implement the stb style implementation, change bunch of API
 - 0.2      - Small Optimization, fix some buggy unnecesarily allocation, improve query API, Improve documentation
 - 0.3      - Added reset world function
 - 0.4      - Fix some data size calculation and implement remove in component pool properly

*/


#pragma once

#ifndef RSECS_H
#define RSECS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RSECS_MAJOR_VERSION 0
#define RSECS_MINOR_VERSION 2

#ifndef RSECS_DEF
    #define RSECS_DEF
#endif // RSECS_DEF

#ifndef RSECS_ASSERT
    #include <assert.h>
    #define RSECS_ASSERT assert
#endif // RSECS_ASSERT


/// --------------------------------
/// INFO : RSECS Contract
/// --------------------------------

/// uint64_t provided at least 2^64 in 64-bit machine so roughly 18.446.744.073.709.551.616
typedef uint64_t secs_entity_id;
/// This component mask in allow up to 64 component in the 64-bit machine
typedef uint64_t secs_component_mask;

typedef struct secs_world secs_world;

typedef struct secs_query {
    /// This will make sure that entity that has the mask be included
    secs_component_mask has;
    /// This will make sure that entity has that mask to be excluded
    secs_component_mask exclude;
} secs_query;

typedef struct secs_query_iterator {
    secs_query      query;
    secs_world*     world;
    secs_entity_id  position;
} secs_query_iterator;

#define SECS_INIT_WORLD(WORLD) secs_init_world(WORLD) 
#define SECS_REGISTER_COMPONENT(WORLD, TYPE) secs_register_component((WORLD), sizeof(TYPE))
/// Generate a query by using format 
/// `SECS_CREATE_QUERY(.has = POSITION_ID, .exclude = OUT_OF_BOUND_ID)`;``
#define SECS_CREATE_QUERY(...) (secs_query) {__VA_ARGS__}

#define secs_query_iter_current(IT) (IT)->position
#define secs_query_iter_reset(IT) (IT)->position = -1


/// Initialize the [`secs_world`] by allocating necessarily memory to it
RSECS_DEF void secs_init_world(secs_world* world);
/// Register the component size and return a component mask that can be used on inserting, removing, and querying
RSECS_DEF secs_component_mask secs_register_component(secs_world* world, size_t size_component);
/// De-allocate all allocated memory inside the [`secs_world`]
RSECS_DEF void secs_free_world(secs_world* world);
/// Mark everything as empty except registered component
RSECS_DEF void secs_reset_world(secs_world* world);

/// Spawning an entity and doing some chore to setup the world to accomodate new entity
/// It might be use old entity id
RSECS_DEF secs_entity_id secs_spawn(secs_world* world);
/// Remove the entity id from active entity
RSECS_DEF void secs_despawn(secs_world* world, secs_entity_id id);

/// Insert a generic component into component pool by copying by value
/// It will also overwrite if it already exist
/// WARNING : Avoid using `|` (Bit OR) when passing the mask IT WILL CAUSE UNDEFINED BEHAVIOR
RSECS_DEF void secs_insert_comp(secs_world* world, secs_entity_id id, secs_component_mask mask, void* component);

/// Check if entity has component mask
RSECS_DEF bool secs_has_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);
/// Check if entity doesn't component mask
RSECS_DEF bool secs_has_not_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);

/// Mark component on that entity id as garbage
RSECS_DEF void secs_remove_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);

/// Get generic component from entity id, if it doesn't find it will fail at assertion
/// You will also need to cast into appropriate type, and it will return NULL if it doesn't have
/// WARNING : Avoid using `|` (Bit OR) when passing the mask IT WILL CAUSE UNDEFINED BEHAVIOR
RSECS_DEF void* secs_get_comp(secs_world* world, secs_entity_id id, secs_component_mask mask);


/// Create a query iterator from the query, and setup the iteration data based on the [`world`] and [`secs_query`] struct
RSECS_DEF secs_query_iterator secs_query_iter(secs_world* world, secs_query query);
/// Advance the iterator
RSECS_DEF bool secs_query_iter_next(secs_query_iterator* it);
/// Get the component from corresponding iterator
/// WARNING : Avoid using `|` (Bit OR) when passing the mask IT WILL CAUSE UNDEFINED BEHAVIOR
RSECS_DEF void* secs_field(secs_query_iterator* it, secs_component_mask mask);

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
            size_t old = (DA)->capacity; \
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

#define _SECS_GET_OFFSET(BASE, INDEX, SIZE) ((char*)BASE) + ((INDEX) * (SIZE))

rstb_da_decl(char, secs_comp_chunk);
rstb_da_decl(secs_entity_id, secs_entity_chunk);
rstb_da_decl(secs_component_mask, secs_comp_mask_chunk);

// Pre-compute index array based on the component mask
static secs_component_mask _secs_comp_map[64] = {
    (secs_component_mask)0x0,
    (secs_component_mask)0x1,
    (secs_component_mask)0x1 << 1,
    (secs_component_mask)0x1 << 2,
    (secs_component_mask)0x1 << 3,
    (secs_component_mask)0x1 << 4,
    (secs_component_mask)0x1 << 5,
    (secs_component_mask)0x1 << 6,
    (secs_component_mask)0x1 << 7,
    (secs_component_mask)0x1 << 8,
    (secs_component_mask)0x1 << 9,
    (secs_component_mask)0x1 << 10,
    (secs_component_mask)0x1 << 11,
    (secs_component_mask)0x1 << 12,
    (secs_component_mask)0x1 << 13,
    (secs_component_mask)0x1 << 14,
    (secs_component_mask)0x1 << 15,
    (secs_component_mask)0x1 << 16,
    (secs_component_mask)0x1 << 17,
    (secs_component_mask)0x1 << 18,
    (secs_component_mask)0x1 << 19,
    (secs_component_mask)0x1 << 20,
    (secs_component_mask)0x1 << 21,
    (secs_component_mask)0x1 << 22,
    (secs_component_mask)0x1 << 23,
    (secs_component_mask)0x1 << 24,
    (secs_component_mask)0x1 << 25,
    (secs_component_mask)0x1 << 26,
    (secs_component_mask)0x1 << 27,
    (secs_component_mask)0x1 << 28,
    (secs_component_mask)0x1 << 29,
    (secs_component_mask)0x1 << 30,
    (secs_component_mask)0x1 << 31,
    (secs_component_mask)0x1 << 32,
    (secs_component_mask)0x1 << 33,
    (secs_component_mask)0x1 << 34,
    (secs_component_mask)0x1 << 35,
    (secs_component_mask)0x1 << 36,
    (secs_component_mask)0x1 << 37,
    (secs_component_mask)0x1 << 38,
    (secs_component_mask)0x1 << 39,
    (secs_component_mask)0x1 << 40,
    (secs_component_mask)0x1 << 41,
    (secs_component_mask)0x1 << 42,
    (secs_component_mask)0x1 << 43,
    (secs_component_mask)0x1 << 44,
    (secs_component_mask)0x1 << 45,
    (secs_component_mask)0x1 << 46,
    (secs_component_mask)0x1 << 47,
    (secs_component_mask)0x1 << 48,
    (secs_component_mask)0x1 << 49,
    (secs_component_mask)0x1 << 50,
    (secs_component_mask)0x1 << 51,
    (secs_component_mask)0x1 << 52,
    (secs_component_mask)0x1 << 53,
    (secs_component_mask)0x1 << 54,
    (secs_component_mask)0x1 << 55,
    (secs_component_mask)0x1 << 56,
    (secs_component_mask)0x1 << 57,
    (secs_component_mask)0x1 << 58,
    (secs_component_mask)0x1 << 59,
    (secs_component_mask)0x1 << 60,
    (secs_component_mask)0x1 << 61,
    (secs_component_mask)0x1 << 62,
};

typedef struct secs_comp_list {
    // The size of the component inside the dense array
    size_t size_of_component;

    secs_comp_chunk     dense;
    secs_entity_chunk   sparse;
} secs_comp_list;

rstb_da_decl(secs_comp_list, secs_comp_list_chunk);

struct secs_world {
    size_t component_mask;
    size_t next_entity_id;

    secs_comp_list_chunk lists;
    secs_comp_mask_chunk mask;
    secs_entity_chunk    dead;
};

static size_t __secs_get_comp_from_bitmask(secs_component_mask mask)
{
    int low = 0;
    int high = (sizeof(_secs_comp_map)/sizeof(_secs_comp_map[0])) - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (_secs_comp_map[mid] == mask) {
            return mid;
        } else if (_secs_comp_map[mid] < mask) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    RSECS_ASSERT(0 && "UNREACHABLE");
    return 0;
}

RSECS_DEF void secs_init_world(secs_world* world)
{
    memset(world, 0, sizeof(secs_world));
    world->component_mask = 1;
}

RSECS_DEF secs_component_mask secs_register_component(secs_world* world, size_t size_component)
{
    secs_component_mask temp = world->component_mask;
    size_t index = __secs_get_comp_from_bitmask(temp);
    rstb_da_reserve(&(world)->lists, index + 1);
    world->lists.items[index].size_of_component = size_component;
    world->component_mask = world->component_mask << 1;
    return temp;
}

RSECS_DEF void secs_free_world(secs_world* world)
{
    rstb_da_free(&world->mask);
    rstb_da_free(&world->dead);
    rstb_da_foreach(secs_comp_list, x, &world->lists) {
        rstb_da_free(&x->dense);
        rstb_da_free(&x->sparse);
    }
    rstb_da_free(&world->lists);
}

RSECS_DEF void secs_reset_world(secs_world* world)
{
    world->mask.count = 0;
    world->dead.count = 0;
    rstb_da_foreach(secs_comp_list, x, &world->lists) {
        x->dense.count = 0;
        x->sparse.count = 0;
    }
}

RSECS_DEF secs_entity_id secs_spawn(secs_world* world)
{
    if (world->dead.count > 0) {
        secs_entity_id id = world->dead.items[0];
        rstb_da_remove_unordered(&world->dead, 0);
        world->mask.items[id] = 0;
        return id;
    }
    secs_entity_id id = world->next_entity_id++;
    rstb_da_reserve(&world->mask, id + 1);
    world->mask.count += 1;
    return id;
}

RSECS_DEF void secs_despawn(secs_world* world, secs_entity_id id)
{
    RSECS_ASSERT(world->mask.count > id && "Entity is not found");
    for (size_t i = 0; i < 64; i++) {
        if (secs_has_comp(world, id, _secs_comp_map[i])) {
            secs_remove_comp(world, id, _secs_comp_map[i]);
        }
    }
    world->mask.items[id] = 0;
    rstb_da_append(&world->dead, id);
}

RSECS_DEF void secs_insert_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id, void* component)
{
    size_t index = __secs_get_comp_from_bitmask(component_id);
    RSECS_ASSERT(index < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    secs_comp_list* comp = &world->lists.items[index];
    if (secs_has_comp(world, entity_id, component_id)) {
        memcpy(
            _SECS_GET_OFFSET(comp->dense.items, comp->sparse.items[entity_id], comp->size_of_component), 
            component, 
            comp->size_of_component
        );
        return;
    }
    rstb_da_reserve(&(comp->sparse), entity_id + 1);
    comp->sparse.items[entity_id] = comp->dense.count;
    comp->dense.count += 1;
    rstb_da_reserve(&(comp->dense), comp->dense.count * comp->size_of_component);
    memcpy(
        _SECS_GET_OFFSET(comp->dense.items, comp->sparse.items[entity_id], comp->size_of_component), 
        component, 
        comp->size_of_component
    );
    world->mask.items[entity_id] |= component_id;
}

RSECS_DEF bool secs_has_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    RSECS_ASSERT(world->mask.count > entity_id && "Entity is not found");
    return (world->mask.items[entity_id] & component_id) == component_id;
}

RSECS_DEF bool secs_has_not_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    RSECS_ASSERT(world->mask.count > entity_id && "Entity is not found");
    return (world->mask.items[entity_id] & component_id) == 0;
}

RSECS_DEF void secs_remove_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    size_t index = __secs_get_comp_from_bitmask(component_id);
    RSECS_ASSERT(index < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!secs_has_comp(world, entity_id, component_id)) return;
    world->mask.items[entity_id] &= ~component_id;

    // TODO : Make this much more efficient
    secs_comp_list* comp = &world->lists.items[index];
    for (size_t i = 0; i < comp->sparse.capacity; i++) {
        if (comp->sparse.items[i] == comp->dense.count - 1) {
            memmove(
                _SECS_GET_OFFSET(comp->dense.items, comp->sparse.items[entity_id], comp->size_of_component), 
                _SECS_GET_OFFSET(comp->dense.items, comp->dense.count - 1, comp->size_of_component),
                comp->size_of_component
            );
            comp->sparse.items[i] = comp->sparse.items[entity_id];
            comp->sparse.items[entity_id] = 0;
            comp->dense.count -= 1;
            return;
        }
    }
}

RSECS_DEF void* secs_get_comp(secs_world* world, secs_entity_id entity_id, secs_component_mask component_id)
{
    size_t index = __secs_get_comp_from_bitmask(component_id);
    RSECS_ASSERT(index < world->lists.capacity && "Yo, out of bound!, please register it by using `REGISTER_COMPONENT` and use it's id it generated");
    if (!secs_has_comp(world, entity_id, component_id)) return NULL;

    secs_comp_list* comp = &world->lists.items[index];
    if (comp->sparse.capacity > entity_id) {
        size_t index = comp->sparse.items[entity_id];
        return _SECS_GET_OFFSET(comp->dense.items, index, comp->size_of_component);
    }
    return NULL;
}


RSECS_DEF secs_query_iterator secs_query_iter(secs_world* world, secs_query query)
{
     return (secs_query_iterator) {
        .query = query,
        .world = world,
        .position = (uint64_t)-1,
    };
}

RSECS_DEF bool secs_query_iter_next(secs_query_iterator* it)
{
    while (it->world->mask.count > it->position + 1) {
        it->position++;
        if (secs_has_comp(it->world, it->position, it->query.has) && secs_has_not_comp(it->world, it->position, it->query.exclude)) {
            return true;
        }
    }
    return false;
}
RSECS_DEF void* secs_field(secs_query_iterator* it, secs_component_mask mask)
{
    return secs_get_comp(it->world, it->position, mask);
}


#endif //RSECS_IMPLEMENTATION

#ifdef RSECS_STRIP_PREFIX
    #define INIT_WORLD(WORLD) SECS_INIT_WORLD(WORLD)
    #define REGISTER_COMPONENT(WORLD, TYPE) SECS_REGISTER_COMPONENT(WORLD, TYPE)
    #define CREATE_QUERY(...) SECS_CREATE_QUERY(__VA_ARGS__)

    #define insert_comp(WORLD, ID, MASK, ...) secs_insert_comp((WORLD), (ID), (MASK), (__VA_ARGS__))
    #define remove_comp(WORLD, ID, MASK) secs_remove_comp((WORLD), (ID), (MASK))
    #define has_comp(WORLD, ID, MASK) secs_has_comp((WORLD), (ID), (MASK))
    #define has_not_comp(WORLD, ID, MASK) secs_has_not_comp((WORLD), (ID), (MASK))
    #define get_comp(WORLD, ID, MASK) secs_get_comp((WORLD), (ID), (MASK))

    #define query_iter(WORLD, QUERY) secs_query_iter((WORLD), (QUERY))
    #define query_iter_next(IT) secs_query_iter_next((IT))
    #define query_iter_reset(IT) secs_query_iter_reset((IT))
    #define query_iter_current(IT) secs_query_iter_current(IT)
    #define field(IT, MASK) secs_field((IT), (MASK))
#endif // RSECS_STRIP_PREFIX

#endif // RSECS_H
