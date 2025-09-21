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


    return 0;
}
